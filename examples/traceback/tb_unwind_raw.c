// Traceback experiment: _Unwind_Backtrace() raw IPs for offline symbolization
//
// Prints only raw instruction pointers, one per line. The runner pipes them to
// addr2line (Linux) or atos (macOS). Build with -no-pie on Linux so runtime
// addresses equal file addresses.
#if defined(__linux__) || defined(__APPLE__) || defined(__MINGW32__) || defined(_WIN32)

#    include <stdint.h>
#    include <stdio.h>
#    include <unwind.h>

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
    for (int i = 0; i < c.n; i++) { printf("0x%lx\n", (unsigned long)(uintptr_t)c.frames[i]); }
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
