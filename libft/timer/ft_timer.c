/*
 * ft_timer.h Timer.
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

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ft_timer.h"
#include "memwatch.h"

/* Gloable vars */
static int g_ft_timer_init = 0;
static struct ft_timer_mgt g_ft_timer_mgt = {0};



static int start_timer_nolock(struct ft_timer *timer)
{
    unsigned int totalcnt = 0;
    struct ft_list *pos = NULL;
    struct ft_list *prev = NULL;
    struct ft_list *next = NULL;
    struct ft_timer *entry = NULL;

    if (1 != g_ft_timer_mgt.is_running)
        return -1;

    if (NULL == timer)
        return -1;

    if (FT_TIMER_STATUS_RUNNING == timer->status)
        return 0;
    
    ft_list_del(&timer->node);

    prev = &g_ft_timer_mgt.running;
    next = prev->next;

    /* 找到正确的插入位置 */
    totalcnt = 0;
    timer->cntdown = timer->overtime / FT_TIMER_PRECISION;
    ft_list_for_each(pos, &g_ft_timer_mgt.running) {
        entry = ft_list_entry(pos, struct ft_timer, node);
        totalcnt += entry->cntdown;

        if (timer->cntdown < totalcnt)
            break;

        prev = pos;
        next = pos->next;
    }

    /* 如果前面有其他定时器，需要根据前一个定时器重新计算自己的相对时间 */
    if (prev != &g_ft_timer_mgt.running) {
        entry = ft_list_entry(prev, struct ft_timer, node);
        timer->cntdown -= entry->cntdown;
    }

    /* 如果后面有其他定时器，需要重新计算后一个定时器的剩余相对时间 */
    if (next != &g_ft_timer_mgt.running) {
        entry = ft_list_entry(next, struct ft_timer, node);
        entry->cntdown -= timer->cntdown;
    }


    timer->status = FT_TIMER_STATUS_RUNNING;

    ft_list_insert(&timer->node, prev, next);

#if 0
    printf("[%d]start timer: %p, cntdown = %u, overtime=%u\n",
            getpid(), &timer->node, timer->cntdown, timer->overtime);

    ft_list_for_each(pos, &g_ft_timer_mgt.running)
        printf("[%d]running timers: %p\n", getpid(), pos);
    ft_list_for_each(pos, &g_ft_timer_mgt.stopped)
        printf("[%d]stopped timers: %p\n", getpid(), pos);
    ft_list_for_each(pos, &g_ft_timer_mgt.timedout)
        printf("[%d]timeout timers: %p\n", getpid(), pos);
#endif

    return 0;
}

static int stop_timer_nolock(struct ft_timer *timer)
{
    struct ft_timer *next = NULL;
    
    if (1 != g_ft_timer_mgt.is_running)
        return -1;

    if (NULL == timer)
        return -1;

    if (FT_TIMER_STATUS_STOPPED == timer->status)
        return 0;

    /* Re-calculate next timer's cntdown */
    if (! ft_list_is_last(&timer->node, &g_ft_timer_mgt.running))
    {
        next = ft_list_entry(timer->node.next, struct ft_timer, node);
        next->cntdown += timer->cntdown;
    }

    /* Move to stop_list */
    timer->status = FT_TIMER_STATUS_STOPPED;
    timer->cntdown = 0;

    ft_list_move(&timer->node, &g_ft_timer_mgt.stopped);

    return 0;
}

static void *timer_thread_routine(void *arg)
{
    struct ft_timer *timer = NULL;
    struct timeval tv;
    int ret = -1;

    while (1 == g_ft_timer_mgt.is_running)
    {
        /* Wait timeout */        
        // 用 pthread_cond_timedwait可能更有效 
        tv.tv_sec = 0;
        tv.tv_usec = FT_TIMER_PRECISION * 1000;
        ret = select(0, NULL, NULL, NULL, &tv);
        if (ret < 0)
            continue;

        pthread_mutex_lock(&g_ft_timer_mgt.mutex);

        if (ft_list_empty(&g_ft_timer_mgt.running))
        {
            pthread_mutex_unlock(&g_ft_timer_mgt.mutex);            
            continue;
        }

        /* First timer Countdown */
        timer = ft_list_first_entry(&g_ft_timer_mgt.running, struct ft_timer, node);
        timer->cntdown--;

#if 0
        if (0 == (timer->cntdown) % 100)
            printf("[%d]1st timer: %p, cntdown=%u, overtime=%u\n",
                    getpid(), &timer->node, timer->cntdown, timer->overtime);
#endif

        /* Move timedout timer to timeout_list */
        while (!ft_list_empty(&g_ft_timer_mgt.running))
        {
            timer = ft_list_first_entry(&g_ft_timer_mgt.running, struct ft_timer, node);
            if (0 != timer->cntdown)
                break;

            timer->status = FT_TIMER_STATUS_STOPPED;
            ft_list_move_tail(&timer->node, &g_ft_timer_mgt.timedout);
        }

        /* Do timedout callback, outside the mutex */
        while (!ft_list_empty(&g_ft_timer_mgt.timedout))
        {
            timer = ft_list_first_entry(&g_ft_timer_mgt.timedout, struct ft_timer, node);

            if (timer->cb_func)
                timer->cb_func(timer, timer->cb_data);

            if (FT_TIMER_TYPE_CYCLE == FT_TIMER_TYPE(timer)) {
                start_timer_nolock(timer);
            } else {
                ft_list_move(&timer->node, &g_ft_timer_mgt.stopped);        
            }
        }

        pthread_mutex_unlock(&g_ft_timer_mgt.mutex);
    }

    return NULL;
}




int ft_timer_init(void)
{
    int err = -1;

    memset(&g_ft_timer_mgt, 0, sizeof(g_ft_timer_mgt));

    g_ft_timer_mgt.count = 0;

    ft_list_init(&g_ft_timer_mgt.stopped);
    ft_list_init(&g_ft_timer_mgt.running);
    ft_list_init(&g_ft_timer_mgt.timedout);

    pthread_mutex_init(&g_ft_timer_mgt.mutex, NULL);

    g_ft_timer_mgt.is_running = 1;
    g_ft_timer_mgt.tid = 0;
    err = pthread_create(&g_ft_timer_mgt.tid, NULL, timer_thread_routine, NULL);
    if (err < 0) {
        //LOGE("call pthread_create() failed, err = %d", err);
        return -1;
    }

    g_ft_timer_init = 1;

    return 0;
}


void ft_timer_deinit()
{
    int err = -1;
    struct ft_list *pos = NULL, *tmp = NULL;
    struct ft_timer *timer = NULL;

    if (! g_ft_timer_init)
        return;

    /* Stop thread */
    g_ft_timer_mgt.is_running = 0;
    if (g_ft_timer_mgt.tid > 0)
    {
        err = pthread_join(g_ft_timer_mgt.tid, NULL);
        if (err < 0) {
            //LOGE("call pthread_join() failed, err = %d, err");
        }
        g_ft_timer_mgt.tid = 0;
    }

    /* Destroy all timers */
    ft_list_for_each_safe(pos, tmp, &g_ft_timer_mgt.running) {
        ft_list_del(pos);

        timer = ft_list_entry(pos, struct ft_timer, node);
        free(timer);

        g_ft_timer_mgt.count--;
    }

    ft_list_for_each_safe(pos, tmp, &g_ft_timer_mgt.stopped) {
        ft_list_del(pos);

        timer = ft_list_entry(pos, struct ft_timer, node);
        free(timer);

        g_ft_timer_mgt.count--;
    }

    ft_list_for_each_safe(pos, tmp, &g_ft_timer_mgt.timedout) {
        ft_list_del(pos);

        timer = ft_list_entry(pos, struct ft_timer, node);
        free(timer);

        g_ft_timer_mgt.count--;
    }

    g_ft_timer_init = 0;
}




struct ft_timer *ft_timer_create(unsigned int ms,
                                 ft_timer_callback cb,
                                 void *cb_data,
                                 int flags)
{
    struct ft_timer *timer = NULL;

    if (1 != g_ft_timer_mgt.is_running)
        return NULL;

    timer = (struct ft_timer *)malloc(sizeof(struct ft_timer));
    if (NULL == timer)
        return NULL;

    /* Add to stop list */
    memset(timer, 0, sizeof(struct ft_timer));
    timer->status   = FT_TIMER_STATUS_STOPPED;
    timer->cntdown  = 0;
    timer->overtime = ms;
    timer->cb_func  = cb;
    timer->cb_data  = cb_data;
    timer->flags    = flags;

    pthread_mutex_lock(&g_ft_timer_mgt.mutex);

    ft_list_add(&timer->node, &g_ft_timer_mgt.stopped);
    g_ft_timer_mgt.count++;

    pthread_mutex_unlock(&g_ft_timer_mgt.mutex);
    
    return timer;
}

void ft_timer_destroy(struct ft_timer *timer)
{

    if (NULL == timer)
        return;

    ft_timer_stop(timer);

    pthread_mutex_lock(&g_ft_timer_mgt.mutex);

    ft_list_del(&timer->node);
    g_ft_timer_mgt.count--;

    pthread_mutex_unlock(&g_ft_timer_mgt.mutex);

    free(timer);
}


int ft_timer_start(struct ft_timer *timer)
{
    int err = -1;

    pthread_mutex_lock(&g_ft_timer_mgt.mutex);
    err = start_timer_nolock(timer);
    pthread_mutex_unlock(&g_ft_timer_mgt.mutex);

    return err;
}


int ft_timer_start_ex(struct ft_timer *timer, unsigned int ms)
{
    int err = -1;

    if (NULL == timer)
        return -1;

    pthread_mutex_lock(&g_ft_timer_mgt.mutex);
    timer->overtime = ms;
    err = start_timer_nolock(timer);
    pthread_mutex_unlock(&g_ft_timer_mgt.mutex);

    return err;
}


int ft_timer_stop(struct ft_timer *timer)
{
    int err = -1;

    pthread_mutex_lock(&g_ft_timer_mgt.mutex);
    err = stop_timer_nolock(timer);
    pthread_mutex_unlock(&g_ft_timer_mgt.mutex);

    return err;
}


int ft_timer_restart(struct ft_timer *timer)
{
    int err = -1;

    if (NULL == timer)
        return -1;

    pthread_mutex_lock(&g_ft_timer_mgt.mutex);
    err = stop_timer_nolock(timer);
    pthread_mutex_unlock(&g_ft_timer_mgt.mutex);

    return err;
}


int ft_timer_restart_ex(struct ft_timer *timer, unsigned int ms)
{
    int err = -1;

    if (NULL == timer)
        return -1;

    pthread_mutex_lock(&g_ft_timer_mgt.mutex);

    stop_timer_nolock(timer);
    timer->overtime = ms;
    err = start_timer_nolock(timer);

    pthread_mutex_unlock(&g_ft_timer_mgt.mutex);

    return err;
}


int ft_timer_timeout(struct ft_timer *timer)
{

    /* NOTE: 不建议在timer回调里调用? */

    if (NULL == timer && FT_TIMER_STATUS_RUNNING != timer->status)
        return -1;

    pthread_mutex_lock(&g_ft_timer_mgt.mutex);

    stop_timer_nolock(timer);

    if (timer->cb_func)
        timer->cb_func(timer, timer->cb_data);

    if (FT_TIMER_TYPE_CYCLE == FT_TIMER_TYPE(timer))
        start_timer_nolock(timer);

    pthread_mutex_unlock(&g_ft_timer_mgt.mutex);
    
    return 0;
}

