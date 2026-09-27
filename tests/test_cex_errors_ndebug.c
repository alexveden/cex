#ifndef NDEBUG
#    define NDEBUG
#endif
#include "src/all.c"
#include "cex_errors_fork.h"

test$case(uassert_is_noop)
{
    uassert(false);
    uassert(1 == 2);
    return EOK;
}

test$case(uassert_always_true_is_noop)
{
    uassert_always(true);
    return EOK;
}

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
test$case(uassert_always_false_traps_in_child)
{
    tassert(is_fatal_in_child(_run_uassert_always_panic));
    return EOK;
}
#endif

test$main();
