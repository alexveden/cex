#define CEX_TRACEBACK_LVL 3
#include "src/all.c"
#include "src/cex_errors.h"
#include "src/cex_errors.c"

test$noopt Exception
err_raise(int i)
{
    if (i == 1) { return e$raise(Error.io, "raise io"); }
    return EOK;
}

test$noopt Exception
err_ret(int i)
{
    e$ret(err_raise(i));
    return EOK;
}

test$case(lvl3_raise_propagates)
{
    tassert_eq(err_raise(1), Error.io);
    return EOK;
}

test$case(lvl3_ret_propagates)
{
    tassert_eq(err_ret(1), Error.io);
    return EOK;
}

test$case(lvl3_traceback_empty)
{
    tassert_eq(err_ret(1), Error.io);
    tassert_eq(e$traceback_len, 0);

    u32 n = 0;
    for$each(it, e$traceback_arr, e$traceback_len)
    {
        (void)it;
        n++;
    }
    tassert_eq(n, 0);

    char* s = e$traceback_fmt(mem$);
    tassert_eq(s, "");
    sbuf.destroy(&s);
    return EOK;
}

test$main();
