// Traceback experiment: traceback from a crash signal handler
//
// Installs handlers for SIGSEGV / SIGABRT / SIGFPE / SIGILL and prints a
// traceback from inside the handler using the write()-based
// backtrace_symbols_fd(). backtrace() is warmed up before installing handlers
// so it does not allocate inside the signal handler.
//
// argv[1] selects the fault: 0=SIGSEGV 1=SIGABRT 2=SIGFPE 3=SIGILL
#if defined(__linux__) || defined(__APPLE__)

#    include <execinfo.h>
#    include <signal.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <unistd.h>

static void
tb_handler(int sig)
{
    void* frames[64];
    int n = backtrace(frames, 64);
    char msg[96];
    int len = snprintf(msg, sizeof(msg), "\n--- signal %d traceback (n=%d) ---\n", sig, n);
    if (len > 0) { (void)!write(2, msg, (size_t)len); }
    backtrace_symbols_fd(frames, n, 2);
    _exit(128 + sig);
}

static void
tb_install_handlers(void)
{
    void* warm[4];
    (void)backtrace(warm, 4); // pre-allocate backtrace state before handler can fire

    struct sigaction sa;
    sa.sa_handler = tb_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
}

static int
tb_level3(int mode)
{
    volatile int zero = 0;
    if (mode == 0) {
        *(volatile int*)0 = 1; // SIGSEGV
    } else if (mode == 1) {
        abort(); // SIGABRT
    } else if (mode == 2) {
        return 1 / zero; // SIGFPE
    } else {
        __builtin_trap(); // SIGILL
    }
    return 0;
}

static int
tb_level2(int mode)
{
    return tb_level3(mode);
}

static int
tb_level1(int mode)
{
    return tb_level2(mode);
}

int
main(int argc, char** argv)
{
    int mode = argc > 1 ? atoi(argv[1]) : 0;
    tb_install_handlers();
    return tb_level1(mode);
}

#else

#    include <stdio.h>

int
main(void)
{
    printf("SKIP: signal traceback experiment is POSIX-only\n");
    return 0;
}

#endif
