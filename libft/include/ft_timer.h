#ifndef __FT_TIMER_H__
#define __FT_TIMER_H__

#include <pthread.h>

#include "ft_list.h"



#ifdef __cplusplus
extern "C" {
#endif


/* Timer's percision, in ms */
#define FT_TIMER_PRECISION          10


#define FT_TIMER_STATUS_STOPPED     0
#define FT_TIMER_STATUS_RUNNING     1
#define FT_TIMER_STATUS_TIMEDOUT    2


#define FT_TIMER_TYPE_ONCE      0x00    /**< 单次定时器 */
#define FT_TIMER_TYPE_CYCLE     0x01    /**< 循环定时器 */

#define FT_TIMER_TYPE_MASK      0x0F
#define FT_TIMER_TYPE(t)       (((struct ft_timer *)(t))->flags & FT_TIMER_TYPE_MASK)


struct ft_timer;


typedef void (*ft_timer_callback)(struct ft_timer *timer, void *cb_data);


struct ft_timer {

    struct ft_list  node;       /**< 内部使用 */

    int status;                 /**< 定时器的状态 */

    unsigned int cntdown;       /**< 与前一个定时器的相对时间 */
    unsigned int overtime;      /**< 超时时长，单位毫秒 */

    unsigned int flags;         /**< 定时器的其他属性 */

    ft_timer_callback cb_func;  /**< 超时回调函数 */
    void             *cb_data;  /**< 超时回调的自定义数据 */

};


struct ft_timer_mgt {

    int is_running;
    pthread_t tid;

    unsigned int count;         /**< 定时器数量计数 */

    struct ft_list  stopped;    /**< 已停止运行的定时器列表 */
    struct ft_list  running;    /**< 正在运行的定时器列表 */
    struct ft_list  timedout;   /**< 已超时（但还未执行回调）的定时器列表 */

    pthread_mutex_t mutex;
};


/**************************************
 * Timer APIs
 **************************************/
extern int  ft_timer_init(void);
extern void ft_timer_deinit(void);

extern struct ft_timer *ft_timer_create(unsigned int ms,
                    ft_timer_callback cb, void *cb_data, int flags);
extern void ft_timer_destroy(struct ft_timer *timer);

extern int ft_timer_start(struct ft_timer *timer);
extern int ft_timer_start_ex(struct ft_timer *timer, unsigned int ms);
extern int ft_timer_stop(struct ft_timer *timer);
extern int ft_timer_restart(struct ft_timer *timer);
extern int ft_timer_restart_ex(struct ft_timer *timer, unsigned int ms);

extern int ft_timer_timeout(struct ft_timer *timer);
//int ft_timer_pause(struct ft_timer *timer);


#ifdef __cplusplus
}
#endif


#endif /* __ft_TIMER_H__ */
