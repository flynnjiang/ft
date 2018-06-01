#include <unistd.h>
#include "ft_mem.h"

int  main()
{
    char *p = NULL;

    ft_mem_init();

    p = ft_mem_alloc(8);

    sleep(3);
    ft_mem_dump();

    if (p)
        ft_mem_free(p);

    ft_mem_uninit();

    return 0;
}
