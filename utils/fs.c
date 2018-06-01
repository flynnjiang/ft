#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int ft_is_dir(const char *path)
{
    struct stat st = {0};
    int err = 0;

    if (NULL == path)
        return 0;

    err = stat(path, &st);
    if (0 == err && S_ISDIR(st.st_mode))
        return 1;

    return 0;
}
