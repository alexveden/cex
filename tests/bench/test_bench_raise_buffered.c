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
    const char* func;
    const char* msg;
    u32 line;
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
    u32 _pad[3];
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
#define e$raise_alt(return_uerr, error_msg)                                                        \
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
#define e$raise_alt(return_uerr, error_msg) ((return_uerr))
#endif

#if CEX_ERR_LVL >= 1
#define e$ret_alt(_func)                                                                           \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (e$push_err_msg(cex$tmpname(__cex_err_traceback_), __FILE_NAME__, __LINE__,           \
                             __func__, #_func),                                                    \
              1)                                                                                   \
         );                                                                                        \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    return cex$tmpname(__cex_err_traceback_)

#define e$goto_alt(_func, _label)                                                                  \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (e$push_err_msg(cex$tmpname(__cex_err_traceback_), __FILE_NAME__, __LINE__,           \
                             __func__, #_func),                                                    \
              1)                                                                                   \
         );                                                                                        \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    goto _label
#else
#define e$ret_alt(_func)                                                                           \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func;                                            \
         unlikely(cex$tmpname(__cex_err_traceback_) != EOK);                                       \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    return cex$tmpname(__cex_err_traceback_)

#define e$goto_alt(_func, _label)                                                                  \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func;                                            \
         unlikely(cex$tmpname(__cex_err_traceback_) != EOK);                                       \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    goto _label
#endif

__attribute__((used, noinline)) Exc
raise_buffered(int i)
{
    if (i == 1) { return e$raise_alt(Error.io, "raise io"); }
    if (i == 2) { return e$raise_alt(Error.memory, "raise memory"); }
    if (i == 3) { return e$raise_alt(Error.argument, "raise argument"); }
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

__attribute__((used, noinline)) Exc
ret_buffered(int i)
{
    if (i == 1) { e$ret_alt(raise_plain(1)); }
    if (i == 2) { e$ret_alt(raise_plain(2)); }
    if (i == 3) { e$ret_alt(raise_plain(3)); }
    return EOK;
}

__attribute__((used, noinline)) Exc
goto_buffered(int i)
{
    Exc err = EOK;
    if (i == 1) { e$goto_alt(err = raise_plain(1), _done); }
    if (i == 2) { e$goto_alt(err = raise_plain(2), _done); }
    if (i == 3) { e$goto_alt(err = raise_plain(3), _done); }
_done:
    return err;
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

test$bench(ret_buffered)
{
    if (ret_buffered(1) == EOK) { return Error.runtime; }
    if (ret_buffered(2) == EOK) { return Error.runtime; }
    if (ret_buffered(3) == EOK) { return Error.runtime; }
    return EOK;
}

test$bench(goto_buffered)
{
    if (goto_buffered(1) == EOK) { return Error.runtime; }
    if (goto_buffered(2) == EOK) { return Error.runtime; }
    if (goto_buffered(3) == EOK) { return Error.runtime; }
    return EOK;
}

test$main();
