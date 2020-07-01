/*
 * ft_ipc.h IPC module.
 * Copyright (C) {2020}  {Jiang Feng}
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


#include "ft_ipc.h"
#include "memwatch.h"



static struct ft_ipc_sync_entry *sync_get_entry_nolock(struct ft_ipc_sync *sync, unsigned int sid)
{
    struct ft_ipc_sync_entry *entry = NULL;

    if (NULL == sync)
        return NULL;

    ft_list_for_each_entry(entry, &sync->entries, node) {
        if (entry->sid == sid)
            return entry;
    }

    return NULL;
}


static int sync_del_entry_nolock(struct ft_ipc_sync_entry *entry)
{
    if (NULL == entry)
        return -1;

    ft_list_del(&entry->node);

    pthread_cond_destroy(&entry->cond);

    if (entry->rsp) {
        ft_ipc_msg_free(entry->rsp);
        entry->rsp = NULL;
    }

    free(entry);

    return 0;
}


static void sync_init(struct ft_ipc_sync *sync)
{
    if (sync) {
        ft_list_init(&sync->entries);
        pthread_mutex_init(&sync->mutex, NULL);
    }

}

static void sync_deinit(struct ft_ipc_sync *sync)
{
    struct ft_ipc_sync_entry *entry = NULL;
    struct ft_ipc_sync_entry *next = NULL;

    if (sync) {
        pthread_mutex_destroy(&sync->mutex);

        ft_list_for_each_entry_safe(entry, next, &sync->entries, node) {
            sync_del_entry_nolock(entry);
        }
    }
}

static int sync_add_request(struct ft_ipc_sync *sync, unsigned int sid)
{
    struct ft_ipc_sync_entry *entry = NULL;

    if (NULL == sync)
        return -1;

    pthread_mutex_lock(&sync->mutex);

    /* 是否有重复? */
    entry = sync_get_entry_nolock(sync, sid);
    if (entry) {
        pthread_mutex_unlock(&sync->mutex);
        return -1;
    }

    entry = (struct ft_ipc_sync_entry *)malloc(sizeof(struct ft_ipc_sync_entry));
    if (entry) {
        entry->sid = sid;
        entry->rsp = NULL;
        pthread_cond_init(&entry->cond, NULL);

        ft_list_add(&entry->node, &sync->entries);
    }

    pthread_mutex_unlock(&sync->mutex);

    return 0;
}


static int sync_del_request(struct ft_ipc_sync *sync, unsigned int sid)
{
    struct ft_ipc_sync_entry *entry = NULL;

    if (NULL == sync)
        return -1;

    pthread_mutex_lock(&sync->mutex);

    entry = sync_get_entry_nolock(sync, sid);
    if (entry)
        sync_del_entry_nolock(entry);

    pthread_mutex_unlock(&sync->mutex);

    return 0;
}


static int sync_try_put_response(struct ft_ipc_sync *sync, int sid, struct ft_ipc_msg *msg)
{
    struct ft_ipc_msg *rsp = NULL;
    struct ft_ipc_sync_entry *entry = NULL;

    if (NULL == sync || NULL == msg)
        return -1;

    rsp = ft_ipc_msg_clone(msg);
    if (! rsp)
        return -1;

    pthread_mutex_lock(&sync->mutex);

    entry = sync_get_entry_nolock(sync, sid);
    if (entry) {
        entry->rsp = rsp;
        pthread_cond_signal(&entry->cond);
        pthread_mutex_unlock(&sync->mutex);

        return 0;
    }

    pthread_mutex_unlock(&sync->mutex);

    ft_ipc_msg_free(rsp);

    return -1;
}


static struct ft_ipc_msg *sync_get_response(struct ft_ipc_sync *sync, int sid, int ms)
{
    int err = -1;
    struct timespec ts;
    struct ft_ipc_sync_entry *entry = NULL;
    struct ft_ipc_msg *msg = NULL;

    if (NULL == sync)
        return NULL;

    pthread_mutex_lock(&sync->mutex);

    entry = sync_get_entry_nolock(sync, sid);
    if (! entry) {
        pthread_mutex_unlock(&sync->mutex);
        return NULL;
    }

    if (ms <= 0) { // 无限等待
        err = 0;
        while (! entry->rsp && 0 == err)
            err = pthread_cond_wait(&entry->cond, &sync->mutex);

    } else { // 固定时长等待
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec  += ms / 1000;
        ts.tv_nsec += (ms % 1000) * 1000 * 1000;

        err = 0;
        while (! entry->rsp&& 0 == err)
            err = pthread_cond_timedwait(&entry->cond, &sync->mutex, &ts);
    }

    msg = entry->rsp;
    entry->rsp = NULL;

    sync_del_entry_nolock(entry);

    pthread_mutex_unlock(&sync->mutex);

    return msg;
}




static int get_sockaddr_un(int id, struct sockaddr_un *addr)
{
    if (id < 0 || NULL == addr)
        return -1;

    addr->sun_family = AF_UNIX;
    addr->sun_path[0] = '\0';   /* Hide the file */
    snprintf(addr->sun_path + 1, sizeof(addr->sun_path) - 1, "ft_ipc_addr_%d", id);

    return 0;
}



static void *recv_thread_routine(void *arg)
{
    int err = -1;
    int len = 0;
    struct sockaddr_un addr = {0};
    socklen_t addr_len = 0;
    fd_set rfds;
    struct timeval tv;
    struct ft_ipc *ipc = (struct ft_ipc *)arg;
    char buf[2048];
    struct ft_ipc_msg *msg = NULL;

    msg = (struct ft_ipc_msg *)buf;

    while (ipc->is_running) {

        FD_ZERO(&rfds);
        FD_SET(ipc->sockfd, &rfds);

        tv.tv_sec  = 1;
        tv.tv_usec = 0;


        err = select(ipc->sockfd + 1, &rfds, NULL, NULL, &tv);
        if (err <= 0)
            continue;


        memset(buf, 0, sizeof(buf));
        memset(&addr, 0, sizeof(addr));
        addr_len = 0;
        len = recvfrom(ipc->sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addr_len);
        if (len <= 0)
            continue;

        /* Check message */
        if (msg->hdr.data_len != len - sizeof(struct ft_ipc_msg_hdr))
            continue;

        ft_ipc_msg_dump(msg);

        /* FIXME: need check addr == msg.hdr.src */

        /* Handle response */
        if (FT_IPC_MSG_TYPE_RSP == msg->hdr.type) {
            if (0 == sync_try_put_response(&ipc->sync, msg->hdr.sid, msg))
                continue;
        }

        /* Call user callback */
        if (ipc->user_cb)
            ipc->user_cb(ipc, FT_IPC_EVENT_MSG_INCOMING, msg, len, ipc->user_context);

    }

    return NULL;
}


/* return message seq number */
static int send_msg(struct ft_ipc *ipc, struct ft_ipc_msg *msg)
{
    int size = 0;
    struct sockaddr_un addr = {0};

    if (NULL == ipc || NULL == msg)
        return -1;

    msg->hdr.src = ipc->self_id;
    msg->hdr.seq = ipc->msg_seq++;

    get_sockaddr_un(msg->hdr.dest, &addr);
    size = sendto(ipc->sockfd, msg, FT_IPC_MSG_LEN(msg), 0,
                (struct sockaddr *)&addr, sizeof(addr));
    if (size != FT_IPC_MSG_LEN(msg))
        return -1;
    
    return msg->hdr.seq;
}


struct ft_ipc_msg *ft_ipc_msg_alloc(int data_len)
{
    struct ft_ipc_msg *msg = NULL;

    if (data_len < 0)
        return NULL;

    msg = (struct ft_ipc_msg *)malloc(sizeof(struct ft_ipc_msg_hdr) + data_len);
    if (msg) {
        memset(msg, 0, sizeof(struct ft_ipc_msg_hdr) + data_len);
        msg->hdr.data_len = data_len;
    }

    return msg;
}


struct ft_ipc_msg *ft_ipc_msg_clone(const struct ft_ipc_msg *msg)
{
    struct ft_ipc_msg *new = NULL;

    if (! msg)
        return NULL;

    new = ft_ipc_msg_alloc(msg->hdr.data_len);
    if (new)
    {
        memcpy(new, msg, sizeof(struct ft_ipc_msg_hdr) + msg->hdr.data_len);
        return new;
    }

    return NULL;
}


void ft_ipc_msg_free(struct ft_ipc_msg *msg)
{
    if (msg)
        free(msg);
}

void ft_ipc_msg_dump(const struct ft_ipc_msg *msg)
{
    if (! msg)
        return;

    printf("IPC[%d->%d][%d][%d:%d]ID=%d,DataLen=%d\n",
            msg->hdr.src, msg->hdr.dest, msg->hdr.type,
            msg->hdr.seq, msg->hdr.sid, msg->hdr.id, msg->hdr.data_len);
}



struct ft_ipc *ft_ipc_create(int id, ft_ipc_event_callback cb, void *cb_context)
{
    int err = -1;
    struct ft_ipc *ipc = NULL;
    struct sockaddr_un addr = {0};

    ipc = (struct ft_ipc *)malloc(sizeof(struct ft_ipc));
    if (NULL == ipc)
        return NULL;

    ipc->user_cb = cb;
    ipc->user_context = cb_context;

    ipc->is_running = 0;
    ipc->recv_tid = 0;

    ipc->self_id = id;
    ipc->msg_seq = 0;
    ipc->msg_sid = 0;

    do {
        sync_init(&ipc->sync);

        /* Create unix domain socket */
        ipc->sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (ipc->sockfd < 0)
            break;

        memset(&addr, 0, sizeof(addr));
        get_sockaddr_un(id, &addr);
        err = bind(ipc->sockfd, (struct sockaddr *)&addr, sizeof(addr));
        if (err < 0)
            break;


        /* Start recv thread */
        ipc->is_running = 1;
        err = pthread_create(&ipc->recv_tid, NULL, recv_thread_routine, ipc);
        if (0 != err)
            break;


        return ipc;

    } while(0);


    /* Error handle */
    if (ipc->sockfd > 0)
        close(ipc->sockfd);

    free(ipc);


    return NULL;
}


void ft_ipc_destroy(struct ft_ipc *ipc)
{
    int err = -1;
    void *res = NULL;

    if (ipc) {

        ipc->is_running = 0;
        if (ipc->recv_tid > 0) {
            err = pthread_join(ipc->recv_tid, &res);
            if (0 == err)
                ipc->recv_tid = 0;
        }

        if (ipc->sockfd > 0) {
            close(ipc->sockfd);
            ipc->sockfd = -1;
        }


        sync_deinit(&ipc->sync);

        free(ipc);
    }
}


/*
 * ms < 0 : nerver timeout
 * ms = 0 : async call
 * ms > 0 : timeout after ms
 */
int ft_ipc_request(struct ft_ipc *ipc, int dest, int id,
        const void *data, int data_len, struct ft_ipc_msg **rsp, int ms)
{
    int err = -1;
    int rc  = -1;
    struct ft_ipc_msg *req = NULL;
    struct ft_ipc_msg *msg = NULL;

    if (NULL == ipc || data_len < 0)
        return -1;

    req = ft_ipc_msg_alloc(data_len);
    if (NULL == req)
        return -1;

    req->hdr.type = FT_IPC_MSG_TYPE_REQ;
    req->hdr.dest = dest;
    req->hdr.id   = id;
    req->hdr.sid  = ipc->msg_sid++;
    if (data && data_len > 0)
        memcpy(req->data, data, data_len);

    if (0 != ms) { /* sync request */
        sync_add_request(&ipc->sync, req->hdr.sid);

        err = send_msg(ipc, req);
        if (err >= 0) {
            msg = sync_get_response(&ipc->sync, req->hdr.sid, ms);
            if (msg) {
                rc = 0;

                if (rsp)
                    *rsp = msg;
                else
                    ft_ipc_msg_free(msg);
            }
        } else {
            sync_del_request(&ipc->sync, req->hdr.sid);
        }

    } else { /* asyn request */

        err = send_msg(ipc, req);
        rc = (err < 0 ? -1 : 0);
    }


    ft_ipc_msg_free(req);

    return rc;
}



int ft_ipc_response(struct ft_ipc *ipc, const struct ft_ipc_msg *req, const void *data, int data_len)
{
    int err = -1;
    struct ft_ipc_msg *rsp = NULL;

    if (NULL == ipc || NULL == req || data_len < 0)
        return -1;

    rsp = ft_ipc_msg_alloc(data_len);
    if (! rsp)
        return -1;

    rsp->hdr.type = FT_IPC_MSG_TYPE_RSP;
    rsp->hdr.dest = req->hdr.src;
    rsp->hdr.id   = req->hdr.id;
    rsp->hdr.sid  = req->hdr.sid;

    if (data && data_len > 0)
        memcpy(rsp->data, data, data_len);

    err = send_msg(ipc, rsp);

    ft_ipc_msg_free(rsp);

    return (err < 0 ? -1 : 0);
}



int ft_ipc_inform(struct ft_ipc *ipc, int dest, int id, const void *data, int data_len)
{
    int err = -1;
    struct ft_ipc_msg *msg = NULL;

    if (NULL == ipc || data_len < 0)
        return -1;

    msg = ft_ipc_msg_alloc(data_len);
    if (NULL == msg)
        return -1;

    msg->hdr.type = FT_IPC_MSG_TYPE_INF;
    msg->hdr.dest = dest;
    msg->hdr.id   = id;

    if (data && data_len > 0)
        memcpy(msg->data, data, data_len);

    err = send_msg(ipc, msg);

    ft_ipc_msg_free(msg);

    return (err < 0 ? -1 : 0);
}




