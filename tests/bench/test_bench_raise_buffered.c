#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"

#define BENCH_RAISE_ERRS_CAP 512

#ifndef CEX_ERR_LVL
#define CEX_ERR_LVL 2
#endif
static_assert(CEX_ERR_LVL >= 0 && CEX_ERR_LVL <= 2, "CEX_ERR_LVL must be 0, 1, or 2");

#if CEX_ERR_LVL >= 2
typedef struct {
    Exc err;
    const char* file;
    u32 line;
    const char* func;
    const char* msg;
} _bench_err_s;
#else
typedef struct {
    Exc err;
    const char* file;
    u32 line;
} _bench_err_s;
#endif

typedef struct {
    u32 len;
    _bench_err_s items[BENCH_RAISE_ERRS_CAP];
} _bench_errs_s;

_Thread_local static _bench_errs_s g_buffered;

static volatile u64 g_sink;

#if CEX_ERR_LVL >= 2
#define e$push_err_msg(_err, _file, _line, _func, _msg)                                            \
    ({                                                                                             \
        if (g_buffered.len < BENCH_RAISE_ERRS_CAP) {                                               \
            g_buffered.items[g_buffered.len++] = (_bench_err_s){ .err = (_err),                    \
                                                                 .file = (_file),                  \
                                                                 .line = (_line),                  \
                                                                 .func = (_func),                  \
                                                                 .msg = (_msg) };                  \
        }                                                                                          \
    })
#elif CEX_ERR_LVL == 1
#define e$push_err_msg(_err, _file, _line, _func, _msg)                                            \
    ({                                                                                             \
        if (g_buffered.len < BENCH_RAISE_ERRS_CAP) {                                               \
            g_buffered.items[g_buffered.len].err = (_err);                                         \
            g_buffered.items[g_buffered.len].file = (_file);                                       \
            g_buffered.items[g_buffered.len].line = (_line);                                       \
            g_buffered.len++;                                                                      \
        }                                                                                          \
    })
#else
#define e$push_err_msg(_err, _file, _line, _func, _msg) ((void)0)
#endif

#if CEX_ERR_LVL >= 1
#define e$raise_alt_msg(return_uerr, error_msg)                                                    \
    ({                                                                                             \
        static_assert(                                                                             \
            !__builtin_types_compatible_p(typeof(error_msg), typeof(&(error_msg)[0])),             \
            "error_msg must be a string literal, not a pointer"                                    \
        );                                                                                         \
        /* "" error_msg compiles only for string literals (adjacent-literal concatenation) */      \
        g_buffered.len = 0;                                                                        \
        e$push_err_msg((return_uerr), __FILE_NAME__, __LINE__, __func__, ("" error_msg));          \
        (return_uerr);                                                                             \
    })
#else
#define e$raise_alt_msg(return_uerr, error_msg) ((return_uerr))
#endif

__attribute__((used, noinline)) Exc
raise_buffered(int i)
{
    if (i == 1) { return e$raise_alt_msg(Error.io, "raise io"); }
    if (i == 2) { return e$raise_alt_msg(Error.memory, "raise memory"); }
    if (i == 3) { return e$raise_alt_msg(Error.argument, "raise argument"); }
    return EOK;
}

__attribute__((used, noinline)) Exc
raise_plain(int i)
{
    if (i == 1) { return Error.io; }
    if (i == 2) { return Error.memory; }
    if (i == 3) { return Error.argument; }
    return EOK;
}

test$setup_case()
{
    g_buffered.len = 0;
    return EOK;
}

test$teardown_case()
{
    u64 sum = 0;
    for (u32 i = 0; i < g_buffered.len; i++) {
        sum += (u64)g_buffered.items[i].line;
        sum += (u64)(usize)g_buffered.items[i].err;
        sum += (u64)(usize)g_buffered.items[i].file;
#if CEX_ERR_LVL >= 2
        sum += (u64)(usize)g_buffered.items[i].func;
        sum += (u64)(usize)g_buffered.items[i].msg;
#endif
    }
    g_sink = sum;
    return EOK;
}

test$bench(raise_buffered)
{
    if (raise_buffered(1) == EOK) { return Error.runtime; }
    if (raise_buffered(2) == EOK) { return Error.runtime; }
    if (raise_buffered(3) == EOK) { return Error.runtime; }
    return EOK;
}

test$bench(raise_plain)
{
    if (raise_plain(1) == EOK) { return Error.runtime; }
    if (raise_plain(2) == EOK) { return Error.runtime; }
    if (raise_plain(3) == EOK) { return Error.runtime; }
    return EOK;
}

test$main();
