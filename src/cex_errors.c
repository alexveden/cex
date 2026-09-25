#include "cex_errors.h"
#include "all.h"

#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
_Thread_local _cex_errors_traceback_data_s _cex_errors_traceback_data_array;

/// Private: emit one recorded frame to `_sink` using the printf-like `_write`
#    if CEX_TRACEBACK_VERBOSITY == 2
#        define _cex_traceback_fmt(_sink, _write, _idx, _r)                                        \
            (_write)(                                                                              \
                (_sink),                                                                           \
                "#%u (%s:%u %s()) [%s] %s\n",                                                      \
                (u32)(_idx),                                                                       \
                (_r)->file,                                                                        \
                (_r)->line,                                                                        \
                (_r)->func,                                                                        \
                (_r)->err,                                                                         \
                (_r)->msg                                                                          \
            )
#    else
#        define _cex_traceback_fmt(_sink, _write, _idx, _r)                                        \
            (_write)((_sink), "#%u (%s:%u) [%s]\n", (u32)(_idx), (_r)->file, (_r)->line, (_r)->err)
#    endif
#endif // CEX_TRACEBACK_VERBOSITY >= 1 && <= 2

sbuf_c
_cex_errors_traceback_fmt(IAllocator allc)
{
#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
    sbuf_c b = sbuf.create(256, allc);
    if (b == NULL) { return NULL; }
    for (u32 i = 0; i < _cex_errors_traceback_data_array.len; i++) {
        (void)_cex_traceback_fmt(&b, sbuf.appendf, i, &_cex_errors_traceback_data_array.items[i]);
    }
    return b;
#else
    return sbuf.create(1, allc);
#endif
}

void
_cex_errors_traceback_print(FILE* stream)
{
#if CEX_TRACEBACK_VERBOSITY >= 1 && CEX_TRACEBACK_VERBOSITY <= 2
    for (u32 i = 0; i < _cex_errors_traceback_data_array.len; i++) {
        (void)_cex_traceback_fmt(stream, io.fprintf, i, &_cex_errors_traceback_data_array.items[i]);
    }
#else
    (void)stream;
#endif
}

#undef _cex_traceback_fmt

#if !defined(NDEBUG) && !defined(__clang_analyzer__) &&                                            \
    (CEX_PANIC_VERBOSITY == 1 || CEX_PANIC_VERBOSITY == 2)

#if CEX_PANIC_VERBOSITY == 1
/// Private: emit the panic line (L1 records only file:line)
#    define _cex_errors_report(_stream)                                                            \
        cexsp__fprintf((_stream), "%s ( %s:%u )\n", prefix, file, line)
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
_cex_errors_fail(const char* prefix, const char* file, u32 line, const char* func, const char* msg)
{
#    if CEX_PANIC_VERBOSITY == 1
    (void)func;
    (void)msg;
#    endif

#    ifdef CEX_TEST
    if (!uassert_is_enabled() && strcmp(prefix, _cex_errors_assert_prefix) == 0) {
        _cex_errors_report(stdout);
        return;
    }
#    endif
    _cex_errors_report(stderr);
    fflush(stdout);
    fflush(stderr);
    sanitizer_stack_trace();
#    ifdef CEX_TEST
    breakpoint();
#    endif
    abort();
}

#    undef _cex_errors_report
#endif
