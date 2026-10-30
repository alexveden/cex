#include "cex_errors.h"
#include "all.h"
#include <limits.h>

#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
#    if !cex$is_freestanding
_Thread_local
#    endif
_cex_errors_traceback_data_s _cex_errors_traceback_data_array;

#    if CEX_TRACEBACK_VERBOSITY == 2
#        define _cex_errors_traceback_fmt "#%u (%s:%u %s()) [%s] %s\n"
#        define _cex_errors_traceback_args(_idx, _r)                                              \
            (u32)(_idx), (_r)->file, (_r)->line, (_r)->func, (_r)->err, (_r)->msg
#    else
#        define _cex_errors_traceback_fmt "#%u (%s:%u) [%s]\n"
#        define _cex_errors_traceback_args(_idx, _r)                                              \
            (u32)(_idx), (_r)->file, (_r)->line, (_r)->err
#    endif
#endif // CEX_TRACEBACK_VERBOSITY >= 1 && <= 2

#if !defined(cex$enable_minimal) || defined(cex$enable_io)

void
_cex_errors_traceback_print(FILE* stream)
{
#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
    for (u32 i = 0; i < _cex_errors_traceback_data_array.len; i++) {
        const _cex_errors_traceback_s* _r = &_cex_errors_traceback_data_array.items[i];
        (void)io.fprintf(stream, _cex_errors_traceback_fmt, _cex_errors_traceback_args(i, _r));
    }
#else
    (void)stream;
#endif
}

#endif // !defined(cex$enable_minimal) || defined(cex$enable_io)

Exception
_cex_errors_traceback_format(char* buf, usize size)
{
    if (buf == NULL || size == 0) { return Error.argument; }
    if (size > (usize)INT_MAX) { size = (usize)INT_MAX; }
    buf[0] = '\0';

#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
    usize used = 0;
    for (u32 i = 0; i < _cex_errors_traceback_data_array.len; i++) {
        const _cex_errors_traceback_s* r = &_cex_errors_traceback_data_array.items[i];
        int n = cexsp__snprintf(
            buf + used,
            (int)(size - used),
            _cex_errors_traceback_fmt,
            _cex_errors_traceback_args(i, r)
        );
        if (n < 0 || (usize)n >= size - used) {
            buf[size - 1] = '\0';
            return Error.overflow;
        }
        used += (usize)n;
    }
#endif
    return EOK;
}

#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
#    undef _cex_errors_traceback_fmt
#    undef _cex_errors_traceback_args
#endif

#if !defined(NDEBUG) && !defined(__clang_analyzer__) &&                                            \
    CEX_PANIC_VERBOSITY >= 1

#if CEX_PANIC_VERBOSITY == 1
/// Private: emit the panic line (L1 records file:line, plus msg when present)
#    define _cex_errors_report(_stream)                                                            \
        ((msg) ? cexsp__fprintf((_stream), "%s ( %s:%u ) %s\n", prefix, file, line, msg)           \
               : cexsp__fprintf((_stream), "%s ( %s:%u )\n", prefix, file, line))
#else
/// Private: emit the panic line (L2 records file:line, func and message)
#    define _cex_errors_report(_stream)                                                            \
        ((msg) ? __cex__fprintf((_stream), prefix, file, line, func, "%s\n", msg)                  \
               : __cex__fprintf((_stream), prefix, file, line, func, "\n"))
#endif

__attribute__((cold, noinline))
#    ifndef CEX_TEST
__attribute__((noreturn))
#    endif
void
_cex_errors_panic_handler(
    const char* prefix,
    const char* file,
    u32 line,
    const char* func,
    const char* msg
)
{
#    if CEX_PANIC_VERBOSITY == 1
    (void)func;
#    endif

#    ifdef CEX_TEST
    if (!uassert_is_enabled() && strcmp(prefix, _cex_errors_assert_prefix) == 0) {
        _cex_errors_report(stdout);
        return;
    }
#    endif
    // LCOV_EXCL_START -- only reachable on abort(); a forked probe's gcov data is never flushed
    _cex_errors_report(stderr);
    fflush(stdout);
    fflush(stderr);
    sanitizer_stack_trace();
#    ifdef CEX_TEST
    breakpoint();
#    endif
    // LCOV_EXCL_STOP
    abort();
}

#    undef _cex_errors_report

#else // reporting compiled out (NDEBUG / clang analyzer / CEX_PANIC_VERBOSITY == 0)

// LCOV_EXCL_START
__attribute__((cold, noinline, noreturn))
void
_cex_errors_panic_handler(
    const char* prefix,
    const char* file,
    u32 line,
    const char* func,
    const char* msg
)
{
    (void)prefix;
    (void)file;
    (void)line;
    (void)func;
    (void)msg;
    __builtin_trap();
}

// LCOV_EXCL_STOP
#endif
