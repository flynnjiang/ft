#ifndef __MEM_BLOCK_H__
#define __MEM_BLOCK_H__

#include <time.h>
#include <pthread.h>

#include "ft_list.h"

#define MEM_MAGIC_NUM 0xAABBAABB

#define MEM_PTR2BLK(_ptr) \
    ((struct mem_block *)((char *)(_ptr) - sizeof(struct mem_block)))

#define MEM_BLK2PTR(_blk) \
    ((void *)((char *)(_blk) + sizeof(struct mem_block)))

#define MEM_MAGIC_HEAD(_blk) ((_blk)->magic_head)

#define MEM_MAGIC_TAIL(_blk) (*((unsigned int *)((char *)(_blk) + sizeof(struct mem_block) + (_blk)->size + sizeof(unsigned int))))


struct mem_block_mgr {
    pthread_mutex_t mutex;
    struct ft_list blks;
    unsigned int blk_num;
    unsigned int blk_size;
    unsigned int alloc_cnt;
    unsigned int free_cnt;
};

/*
 * +-------------------------+
 * |   struct mem_block      |
 * +-------------------------+
 * |                         |
 * |      User Data          |
 * |                         |
 * +-------------------------+
 * |      Magic Number       |
 * +-------------------------+
 */
struct mem_block {
    struct ft_list node;        /* 放在最前，尽量避免被内存越界损坏 */

    size_t size;

    time_t alloc_time;
    const char *alloc_file;
    const char *alloc_func;
    int alloc_line;
    pthread_t owner;

    unsigned int magic_head;    /* 放在最后，作为内存的前边界 */
};


#endif /* __MEM_BLOCK_H__ */
