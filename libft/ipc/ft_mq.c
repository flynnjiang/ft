/*
 * ft_mq.h Message queue.
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

#include <stdlib.h>

#include "ft_mq.h"
#include "memwatch.h"


struct ft_mq_msg *ft_mq_msg_alloc(int size)
{
    struct ft_mq_msg *msg = NULL;

    if (size < 0)
        return NULL;

    msg = (struct ft_mq_msg *)malloc(sizeof(struct ft_mq_msg) + size);

    if (msg) {

        msg->next = NULL;
        msg->protocol = FT_MQ_PROTOCOL_UNKNOWN;
        msg->data_len = size;
    }

    return msg;
}


void ft_mq_msg_free(struct ft_mq_msg *msg)
{
    if (msg) {
        free(msg);
    }
}



struct ft_mq *ft_mq_create()
{
    int i = 0;
    struct ft_mq *mq = NULL;

    mq = (struct ft_mq *)malloc(sizeof(struct ft_mq));

    if (mq) {

        mq->count = 0;

        for (i = FT_MQ_PRI_MIN; i <= FT_MQ_PRI_MAX; i++)
            mq->msg_list[i] = NULL;

        pthread_mutex_init(&mq->mutex, NULL);
        pthread_cond_init(&mq->cond, NULL);
    }

    return mq;
}


void ft_mq_destroy(struct ft_mq *mq)
{
    if (NULL == mq)
        return;

    ft_mq_clear(mq);

    pthread_mutex_destroy(&mq->mutex);
    pthread_cond_destroy(&mq->cond);

    free(mq);
}


int ft_mq_put(struct ft_mq *mq, struct ft_mq_msg *msg, unsigned int flags)
{
    int pri = FT_MQ_PRI_LOW;
    struct ft_mq_msg *node = NULL;

    if (NULL == mq || NULL == msg)
        return -1;

    pri = flags & FT_MQ_PRI_MASK;

    if (pri < FT_MQ_PRI_MIN || pri > FT_MQ_PRI_MAX)
        return -1;

    pthread_mutex_lock(&mq->mutex);
#if 0
    while (0 != pthread_mutex_trylock(&mq->mutex))
        usleep(1000);
#endif

    msg->next = NULL;

    if (mq->msg_list[pri]) {

        node = mq->msg_list[pri];
        while (node->next)
            node = node->next;

        node->next = msg;

    } else {

        mq->msg_list[pri] = msg;

    }

    mq->count++;

    pthread_cond_signal(&mq->cond);

    pthread_mutex_unlock(&mq->mutex);

    return 0;
}


struct ft_mq_msg *ft_mq_get(struct ft_mq *mq, int ms)
{
    int err = 0;
    int i = 0;
    struct ft_mq_msg *msg = NULL;
    struct timespec ts = {0};

    if (NULL == mq)
        return NULL;

    if (ms < 0) { // nerver timeout
        pthread_mutex_lock(&mq->mutex);
        while (mq->count <= 0 && 0 == err)
            err = pthread_cond_wait(&mq->cond, &mq->mutex);

    } else {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += ms / 1000;
        ts.tv_nsec += (ms % 1000) * 1000;

        pthread_mutex_lock(&mq->mutex);
        while (mq->count <= 0 && 0 == err)
            err = pthread_cond_timedwait(&mq->cond, &mq->mutex, &ts);
    }


    if (mq->count > 0) {
        for (i = FT_MQ_PRI_HIGH; i >= FT_MQ_PRI_MIN; i--) {
            if (NULL != mq->msg_list[i]) {
                msg = mq->msg_list[i];
                mq->msg_list[i] = msg->next;
                msg->next = NULL;
                mq->count--;
                break;
            }
        }
    }

    pthread_mutex_unlock(&mq->mutex);

    return msg;
}

void ft_mq_clear(struct ft_mq *mq)
{
    int i = 0;
    struct ft_mq_msg *msg = NULL;

    if (NULL == mq)
        return;

    pthread_mutex_lock(&mq->mutex);

    for (i = FT_MQ_PRI_MIN; i <= FT_MQ_PRI_MAX; i++) {
        while (NULL != (msg = mq->msg_list[i])) {
            mq->msg_list[i] = msg->next;
            free(msg);
        }
    }

    pthread_mutex_unlock(&mq->mutex);

}
