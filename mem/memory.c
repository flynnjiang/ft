#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include "ft_mem.h"

#include "mem_block.h"

/* 暂时不实现碎片优化管理，只做内存跟踪 */

static int mem_init = 0;

static struct mem_block_mgr mem_mgr;


void ft_mem_init()
{
    mem_mgr.blk_num = 0;
    mem_mgr.blk_size = 0;

    ft_list_init(&mem_mgr.blks);

    pthread_mutex_init(&mem_mgr.mutex, NULL);
    
    mem_init = 1;
}

void ft_mem_uninit()
{
    struct mem_block *blk = NULL;
    struct mem_block *tmp = NULL;

    mem_init = 0;

    /* 释放泄露的内存 */
    ft_list_for_each_entry_safe(blk, tmp, &mem_mgr.blks, node) {
        ft_list_del(&blk->node);
        fprintf(stderr, "memory not free\n");
        free(blk);
    }

    mem_mgr.blk_num = 0;
    mem_mgr.blk_size = 0;

    pthread_mutex_destroy(&mem_mgr.mutex);
}

void *_ft_mem_alloc(size_t size, const char *file, const char *func, int line)
{
    struct mem_block *blk = NULL;
    size_t actual_size = 0;

    if (size <= 0)
        return NULL;

    if (0 == mem_init)
        return NULL;

    actual_size = sizeof(struct mem_block) + size + sizeof(unsigned int);
    blk = (struct mem_block *)malloc(actual_size);
    if (NULL == blk)
        return NULL;

    memset(blk, 0, actual_size);
    blk->size = size;
    blk->owner = pthread_self();
    blk->alloc_time = time(NULL);
    blk->alloc_file = file;
    blk->alloc_func = func;
    blk->alloc_line = line;

    MEM_MAGIC_HEAD(blk) = MEM_MAGIC_NUM;
    MEM_MAGIC_TAIL(blk) = MEM_MAGIC_NUM;

    pthread_mutex_lock(&mem_mgr.mutex);

    ft_list_add(&blk->node, &mem_mgr.blks);
    mem_mgr.blk_num++;
    mem_mgr.blk_size += size;
    mem_mgr.alloc_cnt++;

    pthread_mutex_unlock(&mem_mgr.mutex);

    return MEM_BLK2PTR(blk);
}


void ft_mem_free(void *ptr)
{
    struct mem_block *blk = NULL;

    if (NULL == ptr)
        return;

    if (0 == mem_init)
        return;   

    blk = MEM_PTR2BLK(ptr);

    /* 前后边界都异常的话，可能为非法内存块，或发生了严重越界 */
    if (MEM_MAGIC_NUM != MEM_MAGIC_HEAD(blk)
                && MEM_MAGIC_NUM != MEM_MAGIC_TAIL(blk))
        return;

    pthread_mutex_lock(&mem_mgr.mutex);

    ft_list_del(&blk->node);
    mem_mgr.blk_num--;
    mem_mgr.blk_size -= blk->size;
    mem_mgr.free_cnt++;

    pthread_mutex_unlock(&mem_mgr.mutex);

    free(blk);
}


void ft_mem_dump()
{
    struct mem_block *blk = NULL;

    printf(
"============================ MEMORY DUMP ===========================\n"
"blocks used num = %d, blocks used size = %d bytes\n"
"total alloc cnt = %d, total free cnt   = %d\n"
"\n"
"addr            bytes     time        owner     file:line:func\n",
        mem_mgr.blk_num, mem_mgr.blk_size, mem_mgr.alloc_cnt, mem_mgr.free_cnt);

    pthread_mutex_lock(&mem_mgr.mutex);

    ft_list_for_each_entry(blk, &mem_mgr.blks, node) {
        printf("%-16p%-10lu%-12lu%-10lu%s:%d:%s\n",
                blk, blk->size, blk->alloc_time,
                blk->owner, blk->alloc_file,
                blk->alloc_line, blk->alloc_func);
    }

    pthread_mutex_unlock(&mem_mgr.mutex);
}
