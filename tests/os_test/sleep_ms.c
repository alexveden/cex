#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int
main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stdout, "Usage: %s <sleep_time_milisec>\n", argv[0]);
        return 1;
    }

    int sleep_time = atoi(argv[1]);
    if (sleep_time <= 0) {
        fprintf(stdout, "sleep_time must be a positive integer.\n");
        return 1;
    }

    printf("Sleeping for %d ms\n", sleep_time);

#ifdef _WIN32
    Sleep(sleep_time);
#else
    usleep(sleep_time * 1000); 
#endif

    int ret_code = sleep_time % 100;
    printf("Done sleep for %d ms, ret_code (sleep_time %% 100): %d\n", sleep_time, ret_code);

    return  ret_code;
}
