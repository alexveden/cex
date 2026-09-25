#ifndef NDEBUG
#    define NDEBUG
#endif
#include "src/all.c"

test$case(uassert_is_noop)
{
    uassert(false);
    uassert(1 == 2);
    return EOK;
}

test$main();
