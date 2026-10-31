#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}
#else
#    include <errno.h>
#    include <fcntl.h>

int
main(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++) {
        char* end = NULL;
        long fd = strtol(argv[i], &end, 10);
        if (end == argv[i] || *end != '\0' || fd < 0) { return 2; }
        errno = 0;
        if (fcntl((int)fd, F_GETFD) != -1 || errno != EBADF) { return 1; }
    }
    return 0;
}
#endif
