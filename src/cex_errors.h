#pragma once
#include "cex_base.h"

/**
Compile-time knobs for CEX error handling.

- `CEX_TRACEBACK_VERBOSITY` (0..3, default 2) — buffered traceback ring (levels 1, 2) or
  immediate logging (level 3); level 0 disables tracebacks.
- `CEX_PANIC_VERBOSITY` (0..2, default 1) — `uassert()` / `uassert_always()` reporting.

Full reference: `e$` namespace docs (`./cex help e$`, `docs/_include/e.md`).
*/

/* ==== 1. Knobs & validation ==== */

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

/// Assertion label, shared by uassert()/uassert_always() and _cex_errors_panic_handler()'s
/// suppressible check
#define _cex_errors_assert_prefix "[ASSERT] "

#if defined(mem$asan_enabled)
#    if mem$asan_enabled()
// This should be linked when gcc sanitizer enabled
void __sanitizer_print_stack_trace();
#        define sanitizer_stack_trace() __sanitizer_print_stack_trace()
#    else
#        define sanitizer_stack_trace() ((void)(0))
#    endif
#else
#    define sanitizer_stack_trace() ((void)(0))
#endif

/* ==== 2. Panic axis: CEX_PANIC_VERBOSITY ==== */

/* Hard-fail panic (asserts at levels > 0), default cex$platform_panic target */

/// Cold panic: suppressible [ASSERT] prints to stdout when disabled, everything else aborts
__attribute__((cold, noinline))
#ifndef CEX_TEST
__attribute__((noreturn))
#endif
void _cex_errors_panic_handler(
    const char* prefix,
    const char* file,
    u32 line,
    const char* func,
    const char* msg
);

/* stub modes: uassert compiles away (libc assert() under clang analyzer) */
#if defined(__clang_analyzer__) || defined(NDEBUG)
#    if defined(__clang_analyzer__)
#        define uassert(A) assert(A)
#        define uassert_always(A) assert(A)
#    else
#        define uassert(A) ((void)(0))
#        define uassert_always(A)                                                                  \
            ({                                                                                     \
                if (unlikely(!((A)))) { __builtin_trap(); }                                        \
            })
#    endif
#    define uassert_disable() ((void)0)
#    define uassert_enable() ((void)0)
#endif

/* enabled modes: assertion helpers, then uassert by verbosity */
#if !defined(__clang_analyzer__) && !defined(NDEBUG)

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
#        define uassert_always(A)                                                                  \
            ({                                                                                     \
                if (unlikely(!((A)))) { __builtin_trap(); }                                        \
            })
#    elif CEX_PANIC_VERBOSITY == 1
#        define uassert(A)                                                                         \
            ({                                                                                     \
                if (unlikely(!((A)))) {                                                            \
                    cex$platform_panic(                                                            \
                        _cex_errors_assert_prefix,                                                 \
                        __FILE_NAME__,                                                             \
                        __LINE__,                                                                  \
                        NULL,                                                                      \
                        NULL                                                                       \
                    );                                                                             \
                }                                                                                  \
            })
#        define uassert_always(A)                                                                  \
            ({                                                                                     \
                if (unlikely(!((A)))) {                                                            \
                    cex$platform_panic(                                                            \
                        _cex_errors_assert_prefix,                                                 \
                        __FILE_NAME__,                                                             \
                        __LINE__,                                                                  \
                        NULL,                                                                      \
                        NULL                                                                       \
                    );                                                                             \
                    __builtin_trap();                                                              \
                }                                                                                  \
            })
#    else
#        define uassert(A)                                                                         \
            ({                                                                                     \
                if (unlikely(!((A)))) {                                                            \
                    cex$platform_panic(                                                            \
                        _cex_errors_assert_prefix,                                                 \
                        __FILE_NAME__,                                                             \
                        __LINE__,                                                                  \
                        __func__,                                                                  \
                        #A                                                                         \
                    );                                                                             \
                }                                                                                  \
            })
#        define uassert_always(A)                                                                  \
            ({                                                                                     \
                if (unlikely(!((A)))) {                                                            \
                    cex$platform_panic(                                                            \
                        _cex_errors_assert_prefix,                                                 \
                        __FILE_NAME__,                                                             \
                        __LINE__,                                                                  \
                        __func__,                                                                  \
                        #A                                                                         \
                    );                                                                             \
                    __builtin_trap();                                                              \
                }                                                                                  \
            })
#    endif
#endif

/* ==== 3. Traceback axis: CEX_TRACEBACK_VERBOSITY ==== */

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
extern
#    if !cex$is_freestanding
    _Thread_local
#    endif
    _cex_errors_traceback_data_s _cex_errors_traceback_data_array;

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

/// raises an error, code: `return e$raise(Error.integrity, "ooops");`, msg must be a literal
#    define e$raise(return_uerr, error_msg) ((return_uerr))

/// Non disposable assert, returns Error.assert CEX exception when failed
#    define e$assert(A)                                                                            \
        ({                                                                                         \
            if (unlikely(!((A)))) { return Error.assert; }                                         \
        })

/// catches the error of function inside scope + records a traceback frame
#    define e$except(_var_name, _func)                                                             \
        for (Exc _var_name = _func; unlikely(_var_name != EOK); _var_name = EOK)

/// catches the error of system function (if negative value + errno), records a frame
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

/// immediately returns from function with _func error + records a traceback frame
#    define e$ret(_func)                                                                           \
        for (Exc cex$tmpname(__cex_err_traceback_) = _func;                                        \
             unlikely(cex$tmpname(__cex_err_traceback_) != EOK);                                   \
             cex$tmpname(__cex_err_traceback_) = EOK)                                              \
        return cex$tmpname(__cex_err_traceback_)

/// `goto _label` when _func returned error + records a traceback frame
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


#    if CEX_LOG_LVL > 0
#        define __cex__traceback(uerr, fail_func)                                                  \
            (__cex__fprintf(                                                                       \
                stdout,                                                                            \
                "[^STCK]  ",                                                                       \
                __FILE_NAME__,                                                                     \
                __LINE__,                                                                          \
                __func__,                                                                          \
                "^^^^^ [%s] in function call `%s`\n",                                              \
                uerr,                                                                              \
                fail_func                                                                          \
            ))
#    else
#        define __cex__traceback(uerr, fail_func) __cex__fprintf_dummy()
#    endif

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

/* ==== 4. Traceback read-back (buffered levels fill the ring; 0/3 stay empty) ==== */

#if !defined(cex$enable_minimal) || defined(cex$enable_io)

/// Print the recorded traceback to a stream (no allocation)
void _cex_errors_traceback_print(FILE* stream);

/// Print the whole traceback to a FILE*
#define e$traceback_print(_stream) _cex_errors_traceback_print(_stream)

#else

/// Print the whole traceback to a FILE*
#define e$traceback_print(_stream) ((void)0)

#endif // !defined(cex$enable_minimal) || defined(cex$enable_io)

#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
/// Recorded traceback frames array, use with for$each/for$eachp
#    define e$traceback_arr (_cex_errors_traceback_data_array.items)
/// Number of recorded traceback frames
#    define e$traceback_len (_cex_errors_traceback_data_array.len)
/// Drop all recorded traceback frames
#    define e$traceback_reset() (_cex_errors_traceback_data_array.len = 0)
#else
/// Recorded traceback frames array, use with for$each/for$eachp
#    define e$traceback_arr ((_cex_errors_traceback_s*)NULL)
/// Number of recorded traceback frames
#    define e$traceback_len 0
/// Drop all recorded traceback frames
#    define e$traceback_reset() ((void)0)
#endif
