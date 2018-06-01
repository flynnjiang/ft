#include <execinfo.h>

void ft_dump_stack()
{
    void *traces[32] = {0};
    int size = 0;

    size = backtrace(traces, 32);
    backtrace_symbols_fd(traces, size, 2);
}
