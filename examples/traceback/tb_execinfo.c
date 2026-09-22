// Traceback experiment: POSIX <execinfo.h> backtrace()
//
// Compares the write()-based backtrace_symbols_fd() (async-signal-friendlier)
// against the malloc()-based backtrace_symbols().
#if defined(__linux__) || defined(__APPLE__)

#    include <execinfo.h>
#    include <stdio.h>
#    include <stdlib.h>

static void
tb_print_fd(void)
{
    void* frames[64];
    int n = backtrace(frames, 64);
    fprintf(stderr, "--- backtrace_symbols_fd (n=%d) ---\n", n);
    backtrace_symbols_fd(frames, n, 2);
}

static void
tb_print_malloc(void)
{
    void* frames[64];
    int n = backtrace(frames, 64);
    char** syms = backtrace_symbols(frames, n);
    fprintf(stderr, "--- backtrace_symbols (n=%d) ---\n", n);
    if (syms) {
        for (int i = 0; i < n; i++) { fprintf(stderr, "%s\n", syms[i]); }
        free(syms);
    }
}

static int
tb_level3(void)
{
    tb_print_fd();
    tb_print_malloc();
    return 0;
}

static int
tb_level2(void)
{
    return tb_level3();
}

static int
tb_level1(void)
{
    return tb_level2();
}

int
main(void)
{
    return tb_level1();
}

#else

#    include <stdio.h>

int
main(void)
{
    printf("SKIP: <execinfo.h> not available on this platform\n");
    return 0;
}

#endif
