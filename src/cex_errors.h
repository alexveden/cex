#pragma once

/**
Compile-time verbosity knobs for CEX error handling.

- `CEX_TRACEBACK_VERBOSITY` (0..3, default 2) — controls `e$raise`/`e$assert`/`e$except`/
  `e$ret`/`e$goto` and the recorded traceback ring:

    * 0 - no tracebacks, ring disabled (`e$traceback_len == 0`)
    * 1 - ring records `{err, file, line}`
    * 2 - ring records `{err, file, func, msg}`
    * 3 - stock immediate logging, no ring

- `CEX_PANIC_VERBOSITY` (0..2, default 1) — controls `uassert()` and `unreachable()`:

    * 0 - `__builtin_trap()`
    * 1 - `_cex_errors_panic_handler()` prints `file:line`
    * 2 - `_cex_errors_panic_handler()` prints `file:line:func` + the failed expression

*/

#ifndef CEX_TRACEBACK_VERBOSITY
#    define CEX_TRACEBACK_VERBOSITY 2
#endif

#ifndef CEX_PANIC_VERBOSITY
#    define CEX_PANIC_VERBOSITY 1
#endif

static_assert(
    CEX_TRACEBACK_VERBOSITY >= 0 && CEX_TRACEBACK_VERBOSITY <= 3,
    "CEX_TRACEBACK_VERBOSITY must be 0, 1, 2, or 3"
);

static_assert(
    CEX_PANIC_VERBOSITY >= 0 && CEX_PANIC_VERBOSITY <= 2,
    "CEX_PANIC_VERBOSITY must be 0, 1, or 2"
);

/// Max recorded traceback frames (buffered levels)
#ifndef CEX_TRACEBACK_CAP
#    define CEX_TRACEBACK_CAP 32
#endif

/// Assertion label, shared by uassert() and _cex_errors_panic_handler()'s suppressible check
#define _cex_errors_assert_prefix "[ASSERT] "

#if CEX_LOG_LVL > 0
#    define __cex__traceback(uerr, fail_func)                                                      \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[^STCK]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            "^^^^^ [%s] in function call `%s`\n",                                                  \
            uerr,                                                                                  \
            fail_func                                                                              \
        ))
#else
#    define __cex__traceback(uerr, fail_func) __cex__fprintf_dummy()
#endif

/// Hard assertion with ASAN stack trace on failure. Aborts via `_cex_errors_panic_handler()`.
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#    undef unreachable
#endif

#if defined(__clang_analyzer__)
#    define uassert(A) assert(A)
#    define uassert_disable() ((void)0)
#    define uassert_enable() ((void)0)
#    define unreachable() __builtin_unreachable()
#elif defined(NDEBUG)
#    define uassert(A) ((void)(0))
#    define uassert_disable() ((void)0)
#    define uassert_enable() ((void)0)
#    define unreachable() __builtin_unreachable()
#else

#    ifdef CEX_TEST
// this prevents spamming on stderr (i.e. cextest.h output stream in silent mode)
int __cex_test_uassert_enabled = 1;
#        define uassert_disable() __cex_test_uassert_enabled = 0
#        define uassert_enable() __cex_test_uassert_enabled = 1
#        define uassert_is_enabled() (__cex_test_uassert_enabled)
#    else
#        define uassert_disable()                                                                  \
            static_assert(false, "uassert_disable() allowed only when compiled with -DCEX_TEST")
#        define uassert_enable() (void)0
#        define uassert_is_enabled() true
#    endif // #ifdef CEX_TEST

#    if CEX_PANIC_VERBOSITY == 0
#        define uassert(A)                                                                         \
            ({                                                                                     \
                if (unlikely(!((A)))) { __builtin_trap(); }                                        \
            })
#        define unreachable() __builtin_trap()
#    elif CEX_PANIC_VERBOSITY == 1
#        define uassert(A)                                                                         \
            ({                                                                                     \
                if (unlikely(!((A)))) {                                                            \
                    _cex_errors_panic_handler(                                                     \
                        _cex_errors_assert_prefix,                                                 \
                        __FILE_NAME__,                                                             \
                        __LINE__,                                                                  \
                        NULL,                                                                      \
                        NULL                                                                       \
                    );                                                                             \
                }                                                                                  \
            })
#        define unreachable()                                                                      \
            _cex_errors_panic_handler("[UNREACHABLE] ", __FILE_NAME__, __LINE__, NULL, NULL)
#    else
#        define uassert(A)                                                                         \
            ({                                                                                     \
                if (unlikely(!((A)))) {                                                            \
                    _cex_errors_panic_handler(                                                     \
                        _cex_errors_assert_prefix,                                                 \
                        __FILE_NAME__,                                                             \
                        __LINE__,                                                                  \
                        __func__,                                                                  \
                        #A                                                                         \
                    );                                                                             \
                }                                                                                  \
            })
#        define unreachable()                                                                      \
            _cex_errors_panic_handler("[UNREACHABLE] ", __FILE_NAME__, __LINE__, __func__, NULL)
#    endif
#endif

#if CEX_TRACEBACK_VERBOSITY == 1
typedef struct
{
    Exc err;
    const char* file;
    u32 line;
} _cex_errors_traceback_s;
#else
typedef struct
{
    Exc err;
    const char* file;
    const char* func;
    const char* msg;
    u32 line;
} _cex_errors_traceback_s;
#endif

typedef struct
{
    u32 len;
    u32 _pad[3];
    _cex_errors_traceback_s items[CEX_TRACEBACK_CAP];
} _cex_errors_traceback_data_s;

#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
/// Shared traceback ring (defined in cex_errors.c)
extern _Thread_local _cex_errors_traceback_data_s _cex_errors_traceback_data_array;

#    if CEX_TRACEBACK_VERBOSITY == 2
/// Private: append one frame to the ring (clamped at CEX_TRACEBACK_CAP)
#        define _e$push_frame(_err, _file, _line, _func, _msg)                                      \
            ({                                                                                      \
                if (_cex_errors_traceback_data_array.len < CEX_TRACEBACK_CAP) {                     \
                    _cex_errors_traceback_data_array                                                \
                        .items[_cex_errors_traceback_data_array.len++] = (_cex_errors_traceback_s){ \
                        .err = (_err),                                                              \
                        .file = (_file),                                                            \
                        .func = (_func),                                                            \
                        .msg = (_msg),                                                              \
                        .line = (_line)                                                             \
                    };                                                                              \
                }                                                                                   \
            })
#    else
/// Private: append one frame to the ring (clamped at CEX_TRACEBACK_CAP)
#        define _e$push_frame(_err, _file, _line, _func, _msg)                                      \
            ({                                                                                      \
                if (_cex_errors_traceback_data_array.len < CEX_TRACEBACK_CAP) {                     \
                    _cex_errors_traceback_data_array                                                \
                        .items[_cex_errors_traceback_data_array.len++] = (_cex_errors_traceback_s){ \
                        .err = (_err),                                                              \
                        .file = (_file),                                                            \
                        .line = (_line)                                                             \
                    };                                                                              \
                }                                                                                   \
            })
#    endif

/// Private: start a new error chain (reset the ring), then append a frame
#    define _e$push_origin(_err, _file, _line, _func, _msg)                                        \
        ({                                                                                         \
            _cex_errors_traceback_data_array.len = 0;                                              \
            _e$push_frame(_err, _file, _line, _func, _msg);                                        \
        })
#endif // CEX_TRACEBACK_VERBOSITY >= 1 && <= 2

#if CEX_TRACEBACK_VERBOSITY == 0

/// raises an error, code: `return e$raise(Error.integrity, "ooops");`
#    define e$raise(return_uerr, error_msg) ((return_uerr))

/// Non disposable assert, returns Error.assert CEX exception when failed
#    define e$assert(A)                                                                            \
        ({                                                                                         \
            if (unlikely(!((A)))) { return Error.assert; }                                         \
        })

/// catches the error of function inside scope + prints traceback
#    define e$except(_var_name, _func)                                                             \
        for (Exc _var_name = _func; unlikely(_var_name != EOK); _var_name = EOK)

/// catches the error of system function (if negative value + errno), prints errno error
#    define e$except_errno(_expression)                                                            \
        for (int _tmp_errno = 0; unlikely(                                                         \
                 ((_tmp_errno == 0) && ((_expression) < 0) && ((_tmp_errno = errno), 1) &&         \
                  (errno = _tmp_errno, 1))                                                         \
             );                                                                                    \
             _tmp_errno = 1)

/// catches the error is expression returned null
#    define e$except_null(_expression) if (unlikely((_expression) == NULL))

/// catches the error is expression returned true
#    define e$except_true(_expression) if (unlikely(_expression))

/// immediately returns from function with _func error + prints traceback
#    define e$ret(_func)                                                                           \
        for (Exc cex$tmpname(__cex_err_traceback_) = _func;                                        \
             unlikely(cex$tmpname(__cex_err_traceback_) != EOK);                                   \
             cex$tmpname(__cex_err_traceback_) = EOK)                                              \
        return cex$tmpname(__cex_err_traceback_)

/// `goto _label` when _func returned error + prints traceback
#    define e$goto(_func, _label)                                                                  \
        for (Exc cex$tmpname(__cex_err_traceback_) = _func;                                        \
             unlikely(cex$tmpname(__cex_err_traceback_) != EOK);                                   \
             cex$tmpname(__cex_err_traceback_) = EOK)                                              \
        goto _label

#elif CEX_TRACEBACK_VERBOSITY <= 2

#    define e$raise(return_uerr, error_msg)                                                        \
        ({                                                                                         \
            _e$push_origin((return_uerr), __FILE_NAME__, __LINE__, __func__, ("" error_msg));      \
            (return_uerr);                                                                         \
        })

#    define e$assert(A)                                                                            \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                _e$push_origin(Error.assert, __FILE_NAME__, __LINE__, __func__, #A);               \
                return Error.assert;                                                               \
            }                                                                                      \
        })

#    define e$except(_var_name, _func)                                                             \
        for (Exc _var_name = _func; unlikely(                                                      \
                 (_var_name != EOK) &&                                                             \
                 (_e$push_frame(_var_name, __FILE_NAME__, __LINE__, __func__, #_func), 1)          \
             );                                                                                    \
             _var_name = EOK)

#    define e$except_errno(_expression)                                                            \
        for (int _tmp_errno = 0; unlikely(                                                         \
                 ((_tmp_errno == 0) && ((_expression) < 0) && ((_tmp_errno = errno), 1) &&         \
                  (_e$push_origin(                                                                 \
                       strerror(_tmp_errno),                                                       \
                       __FILE_NAME__,                                                              \
                       __LINE__,                                                                   \
                       __func__,                                                                   \
                       #_expression                                                                \
                   ),                                                                              \
                   1) &&                                                                           \
                  (errno = _tmp_errno, 1))                                                         \
             );                                                                                    \
             _tmp_errno = 1)

#    define e$except_null(_expression)                                                             \
        if (unlikely(                                                                              \
                ((_expression) == NULL) && (_e$push_origin(                                        \
                                                Error.null_or_empty,                               \
                                                __FILE_NAME__,                                     \
                                                __LINE__,                                          \
                                                __func__,                                          \
                                                #_expression                                       \
                                            ),                                                     \
                                            1)                                                     \
            ))

#    define e$except_true(_expression)                                                             \
        if (unlikely(                                                                              \
                ((_expression)) &&                                                                 \
                (_e$push_origin(Error.runtime, __FILE_NAME__, __LINE__, __func__, #_expression),   \
                 1)                                                                                \
            ))

#    define e$ret(_func)                                                                           \
        for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                              \
                 (cex$tmpname(__cex_err_traceback_) != EOK) &&                                     \
                 (_e$push_frame(                                                                   \
                      cex$tmpname(__cex_err_traceback_),                                           \
                      __FILE_NAME__,                                                               \
                      __LINE__,                                                                    \
                      __func__,                                                                    \
                      #_func                                                                       \
                  ),                                                                               \
                  1)                                                                               \
             );                                                                                    \
             cex$tmpname(__cex_err_traceback_) = EOK)                                              \
        return cex$tmpname(__cex_err_traceback_)

#    define e$goto(_func, _label)                                                                  \
        for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                              \
                 (cex$tmpname(__cex_err_traceback_) != EOK) &&                                     \
                 (_e$push_frame(                                                                   \
                      cex$tmpname(__cex_err_traceback_),                                           \
                      __FILE_NAME__,                                                               \
                      __LINE__,                                                                    \
                      __func__,                                                                    \
                      #_func                                                                       \
                  ),                                                                               \
                  1)                                                                               \
             );                                                                                    \
             cex$tmpname(__cex_err_traceback_) = EOK)                                              \
        goto _label

#else // CEX_TRACEBACK_VERBOSITY == 3

#    define e$raise(return_uerr, error_msg)                                                        \
        (log$error("[%s] " error_msg "\n", return_uerr), (return_uerr))

#    if CEX_LOG_LVL > 0
#        define e$assert(A)                                                                        \
            ({                                                                                     \
                if (unlikely(!((A)))) {                                                            \
                    __cex__fprintf(                                                                \
                        stdout,                                                                    \
                        "[ASSERT] ",                                                               \
                        __FILE_NAME__,                                                             \
                        __LINE__,                                                                  \
                        __func__,                                                                  \
                        "%s\n",                                                                    \
                        #A                                                                         \
                    );                                                                             \
                    return Error.assert;                                                           \
                }                                                                                  \
            })
#    else
#        define e$assert(A)                                                                        \
            ({                                                                                     \
                if (unlikely(!((A)))) { return Error.assert; }                                     \
            })
#    endif

#    define e$except(_var_name, _func)                                                             \
        for (Exc _var_name = _func;                                                                \
             unlikely((_var_name != EOK) && (__cex__traceback(_var_name, #_func), 1));             \
             _var_name = EOK)

#    define e$except_errno(_expression)                                                            \
        for (int _tmp_errno = 0; unlikely(                                                         \
                 ((_tmp_errno == 0) && ((_expression) < 0) && ((_tmp_errno = errno), 1) &&         \
                  (log$error(                                                                      \
                       "`%s` failed errno: %d, msg: %s\n",                                         \
                       #_expression,                                                               \
                       _tmp_errno,                                                                 \
                       strerror(_tmp_errno)                                                        \
                   ),                                                                              \
                   1) &&                                                                           \
                  (errno = _tmp_errno, 1))                                                         \
             );                                                                                    \
             _tmp_errno = 1)

#    define e$except_null(_expression)                                                             \
        if (unlikely(                                                                              \
                ((_expression) == NULL) && (log$error("`%s` returned NULL\n", #_expression), 1)    \
            ))

#    define e$except_true(_expression)                                                             \
        if (unlikely(((_expression)) && (log$error("`%s` returned non zero\n", #_expression), 1)))

#    define e$ret(_func)                                                                           \
        for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                              \
                 (cex$tmpname(__cex_err_traceback_) != EOK) &&                                     \
                 (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func), 1)                  \
             );                                                                                    \
             cex$tmpname(__cex_err_traceback_) = EOK)                                              \
        return cex$tmpname(__cex_err_traceback_)

#    define e$goto(_func, _label)                                                                  \
        for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                              \
                 (cex$tmpname(__cex_err_traceback_) != EOK) &&                                     \
                 (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func), 1)                  \
             );                                                                                    \
             cex$tmpname(__cex_err_traceback_) = EOK)                                              \
        goto _label

#endif // CEX_TRACEBACK_VERBOSITY

/* Hard-fail panic (asserts + unreachable), prototype-scoped replacement for __cex__panic */

/// Cold panic: suppressible [ASSERT] prints to stdout when disabled, everything else aborts
__attribute__((cold, noinline))
#ifndef CEX_TEST
__attribute__((noreturn))
#endif
void
_cex_errors_panic_handler(
    const char* prefix,
    const char* file,
    u32 line,
    const char* func,
    const char* msg
);

/* Traceback read-back (buffered levels fill the ring; 0/3 stay empty) */

/// Format the recorded traceback into an owned `sbuf_c` (free with sbuf.destroy(&s))
sbuf_c _cex_errors_traceback_fmt(IAllocator allc);

/// Print the recorded traceback to a stream (no allocation)
void _cex_errors_traceback_print(FILE* stream);

/// Format the whole traceback into an owned `sbuf_c`
#define e$traceback_fmt(_allc) _cex_errors_traceback_fmt(_allc)

/// Print the whole traceback to a FILE*
#define e$traceback_print(_stream) _cex_errors_traceback_print(_stream)

#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
/// Recorded frames array, use with for$each/for$eachp
#    define e$traceback_arr (_cex_errors_traceback_data_array.items)
/// Number of recorded frames
#    define e$traceback_len (_cex_errors_traceback_data_array.len)
/// Drop all recorded frames
#    define e$traceback_reset() (_cex_errors_traceback_data_array.len = 0)
#else
/// Recorded frames array (always empty when buffering is disabled)
#    define e$traceback_arr ((_cex_errors_traceback_s*)NULL)
/// Number of recorded frames (always 0 when buffering is disabled)
#    define e$traceback_len 0
/// Drop all recorded frames (no-op when buffering is disabled)
#    define e$traceback_reset() ((void)0)
#endif
