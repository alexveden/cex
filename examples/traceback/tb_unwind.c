// Traceback experiment: portable <unwind.h> _Unwind_Backtrace()
//
// Works with GCC/Clang runtimes on Linux, macOS and MinGW. Symbol names are
// resolved with dladdr() where available, otherwise raw IPs are printed.
#if defined(__linux__) && !defined(_GNU_SOURCE)
#    define _GNU_SOURCE // Dl_info / dladdr
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(__MINGW32__) || defined(_WIN32)

#    include <stdint.h>
#    include <stdio.h>
#    include <unwind.h>

#    if defined(__linux__) || defined(__APPLE__)
#        include <dlfcn.h>
#        define TB_HAS_DLADDR 1
#    else
#        define TB_HAS_DLADDR 0
#    endif

struct tb_ctx {
    void* frames[64];
    int n;
    int max;
};

static _Unwind_Reason_Code
tb_cb(struct _Unwind_Context* ctx, void* arg)
{
    struct tb_ctx* c = arg;
    if (c->n >= c->max) { return _URC_END_OF_STACK; }
    uintptr_t ip = _Unwind_GetIP(ctx);
    if (ip) { c->frames[c->n++] = (void*)(ip - 1); }
    return _URC_NO_REASON;
}

static int
tb_level3(void)
{
    struct tb_ctx c = { .n = 0, .max = 64 };
    _Unwind_Backtrace(tb_cb, &c);
    fprintf(stderr, "--- _Unwind_Backtrace (n=%d) ---\n", c.n);
    for (int i = 0; i < c.n; i++) {
#    if TB_HAS_DLADDR
        Dl_info info;
        if (dladdr(c.frames[i], &info) && info.dli_sname) {
            fprintf(stderr, "%2d: %s  (%s)\n", i, info.dli_sname, info.dli_fname);
            continue;
        }
#    endif
        fprintf(stderr, "%2d: %p\n", i, c.frames[i]);
    }
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
    printf("SKIP: <unwind.h> not available on this platform\n");
    return 0;
}

#endif
