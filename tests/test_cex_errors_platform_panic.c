#define CEX_PANIC_VERBOSITY 2

static int g_panic_calls;

#define cex$platform_panic(prefix, file, line, func, msg) (g_panic_calls++)

#include "src/all.c"

test$case(uassert_uses_custom_panic)
{
    uassert(false);
    tassert_eq(g_panic_calls, 1);
    return EOK;
}

test$case(unreachable_uses_custom_panic)
{
    g_panic_calls = 0;
    unreachable();
    tassert_eq(g_panic_calls, 1);
    return EOK;
}

test$main();
