#ifndef __FT_MEM_H__
#define __FT_MEM_H__

#include <stddef.h>

/* 普通缓冲区。每次读取缓冲区中的全部数据 */
#define FT_BUFF_TYPE_NORMAL    0

/* 行缓冲区。每次从缓冲区中读取一行字符 */
#define FT_BUFF_TYPE_LINE      1

/* 块缓冲区。每次从缓冲区中读取固定大小的数据块 */
#define FT_BUFF_TYPE_BLOCK     2

/* 自定义缓冲区。每次从缓冲区中读取符合自定义规则的数据块 */
#define FT_BUFF_TYPE_CUSTOM    3


typedef int (*ft_buff_valid_func)(const void *data, size_t data_len);


typedef struct _ft_buff_st {
    int type;

    size_t blk_size;        /* 块缓冲大小，仅块缓冲区使用 */

    ft_buff_valid_func buff_valid;  /* 缓冲区数据是否有效的判断函数 */

    unsigned char *data;
    unsigned char *readp;
    unsigned char *writep;
    unsigned char *endp;
} ft_buff_t;



extern void ft_mem_init();

extern void ft_mem_uninit();

extern void ft_mem_dump();


#define ft_mem_alloc(_size) \
        _ft_mem_alloc(_size, __FILE__, __func__, __LINE__)

extern void *_ft_mem_alloc(size_t size,
            const char *file, const char *func, int line);

extern void ft_mem_free(void *ptr);

void ft_dump_stack();





#if 0

extern ft_buff_t *ft_buff_create(int type, size_t size, ...);
extern void ft_buff_destory(ft_buff_t *b);

extern int ft_buff_in(ft_buff_t *b, void *data, size_t data_len);
extern void *ft_buff_out(ft_buff_t *b, size_t *out_len);


size_t ft_buff_total_size(ft_buff_t *b);
size_t ft_buff_free_size(ft_buff_t *b);
size_t ft_buff_data_size(ft_buff_t *b);


int buff_clean(ft_buff_t *b);
int buff_flush(ft_buff_t *b);

int buff_empty(ft_buff_t *b);
int buff_full(ft_buff_t *b);

int buff_dump(ft_buff_t *b);
#endif


#endif /* __FT_MEM_H__ */
