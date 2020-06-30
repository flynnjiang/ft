#ifndef __FT_MQ_H__
#define __FT_MQ_H__

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Message priority */
#define FT_MQ_PRI_LOW           0
#define FT_MQ_PRI_MID           1
#define FT_MQ_PRI_HIGH          2

#define FT_MQ_PRI_MIN           FT_MQ_PRI_LOW
#define FT_MQ_PRI_MAX           FT_MQ_PRI_HIGH
#define FT_MQ_PRI_MASK          0x0F


/* MQ protocol code */
#define FT_MQ_PROTOCOL_UNKNOWN  0x00
#define FT_MQ_PROTOCOL_IPC      0x01
#define FT_MQ_PROTOCOL_RPC      0x02




struct ft_mq_msg {
    struct ft_mq_msg *next; /* Inner use */

    int protocol;           /**< Upper protocol */
    int data_len;           /**< Data length */

    char data[1];
};


struct ft_mq {
    unsigned int count;

    pthread_mutex_t mutex;
    pthread_cond_t  cond;

    struct ft_mq_msg *msg_list[FT_MQ_PRI_MAX + 1];
};


extern struct ft_mq_msg *ft_mq_msg_alloc(int size);
extern void ft_mq_msg_free(struct ft_mq_msg *msg);


extern struct ft_mq *ft_mq_create(void);
extern void ft_mq_destroy(struct ft_mq *mq);


extern int ft_mq_put(struct ft_mq *mq, struct ft_mq_msg *msg, unsigned int flags);
extern struct ft_mq_msg *ft_mq_get(struct ft_mq *mq, int ms);

extern void ft_mq_clear(struct ft_mq *mq);



#ifdef __cplusplus
}
#endif

#endif /* __FT_MQ_H__ */
