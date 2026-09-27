#define CEX_PANIC_VERBOSITY 2

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#    define cex$platform_panic(prefix, file, line, func, msg) _exit(42)
#endif

#include "src/all.c"
#include "cex_errors_fork.h"

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
test$case(uassert_uses_custom_panic)
{
    tassert_eq(get_child_exit_code(_run_uassert_panic), 42);
    return EOK;
}

test$case(uassert_always_uses_custom_panic)
{
    tassert_eq(get_child_exit_code(_run_uassert_always_panic), 42);
    return EOK;
}
#endif

test$main();
