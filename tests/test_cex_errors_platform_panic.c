#define CEX_PANIC_VERBOSITY 2

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#    define cex$platform_panic(prefix, file, line, func, msg) _exit(42)
#endif

#include "src/all.c"
#include "cex_errors_fork.h"

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
static Exception
assert_custom_panic_exit(void (*action)(void))
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    int code = get_child_exit_code(action);
    const char* valgrind = getenv("CEX_VALGRIND");
    if (valgrind && valgrind[0] == '1') {
        tassert(code >= 0);
        return EOK;
    }
    tassert_eq(code, 42);
    return EOK;
}

test$case(uassert_uses_custom_panic)
{
    return assert_custom_panic_exit(_run_uassert_panic);
}

test$case(uassert_always_uses_custom_panic)
{
    return assert_custom_panic_exit(_run_uassert_always_panic);
}
#endif

test$main();
