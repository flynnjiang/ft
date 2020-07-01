/*
 * ft_ipc.h IPC defines.
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

#ifndef __FT_IPC_H__
#define __FT_IPC_H__

#include <pthread.h>

#include "ft_list.h"


#ifdef __cplusplus
extern "C" {
#endif


/*=====================================================
 * IPC Message
 *=====================================================*/

/* Message Types */
#define FT_IPC_MSG_TYPE_INF     0       /**< Inform */
#define FT_IPC_MSG_TYPE_REQ     1       /**< Request */
#define FT_IPC_MSG_TYPE_RSP     2       /**< Response */


/** Message Header */
struct ft_ipc_msg_hdr {
    int dest;           /**< Destination */
    int src;            /**< Source */
    int type;           /**< Message type */
    unsigned int seq;            /**< Message sequeue number */
    unsigned int sid;            /**< Session ID, for request & response */

    int id;             /**< Message ID */
    int data_len;       /**< Message date length */
};


/** Message */
struct ft_ipc_msg {
    struct ft_ipc_msg_hdr hdr;
    char data[1];       /**< Message data */
};



#define FT_IPC_MSG_LEN(msg)     \
    (sizeof(struct ft_ipc_msg_hdr) + ((struct ft_ipc_msg *)(msg))->hdr.data_len)




/*=====================================================
 * IPC Management
 *=====================================================*/

struct ft_ipc;

#define FT_IPC_EVENT_MSG_INCOMING       0x01    /* Data: struct ft_ipc_msg */
#define FT_IPC_EVENT_SEND_FAILED        0x02    /* Data: None */

typedef void (*ft_ipc_event_callback)(struct ft_ipc *ipc,
                int event, void *data, int data_len, void *context);

/* TODO: Sync Request */
struct ft_ipc_sync_entry {

    struct ft_list node;

    unsigned int sid;
    struct ft_ipc_msg *rsp;

    pthread_cond_t cond;
};

struct ft_ipc_sync
{
    pthread_mutex_t mutex;

    struct ft_list entries;
};


/* TODO: Subscribe Manangement */

struct ft_ipc_sub_item {
    int msg_id;
    int apps_id[32];

    struct ft_ipc_sub_item *next;
};


struct ft_ipc_sub {
    int count;
    struct ft_ipc_sub_item *items;
    pthread_mutex_t mutex;
};



/* IPC Management */

struct ft_ipc {

    int is_running;
    pthread_t recv_tid;         /**< Recv thread ID */

    int sockfd;                 /**< Socket */

    int self_id;                /**< Self ID */
    unsigned int msg_seq;       /**< Message sequeue number */
    unsigned int msg_sid;

    struct ft_ipc_sync sync;   /**< Sync reqeust management */
    //struct ft_ipc_sub sub;      /**< Subscribe managemant */

    ft_ipc_event_callback user_cb;
    void *user_context;
};



extern struct ft_ipc_msg *ft_ipc_msg_alloc(int data_len);
extern struct ft_ipc_msg *ft_ipc_msg_clone(const struct ft_ipc_msg *msg);
extern void ft_ipc_msg_free(struct ft_ipc_msg *msg);
extern void ft_ipc_msg_dump(const struct ft_ipc_msg *msg);

extern struct ft_ipc *ft_ipc_create(int id, ft_ipc_event_callback cb, void *cb_context);
extern void ft_ipc_destroy(struct ft_ipc *ipc);

extern int ft_ipc_request(struct ft_ipc *ipc, int dest, int id,
                          const void *data, int data_len, struct ft_ipc_msg **rsp, int ms);
extern int ft_ipc_response(struct ft_ipc *ipc, const struct ft_ipc_msg *req, const void *data, int data_len);

extern int ft_ipc_inform(struct ft_ipc *ipc, int dest, int id, const void *data, int data_len);


#ifdef __cplusplus
}
#endif

#endif /* __FT_IPC_H__ */
