#define CEX_TRACEBACK_LVL 1
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

test$setup_case()
{
    e$traceback_reset();
    return EOK;
}

test$case(lvl1_single_frame)
{
    tassert_eq(err_raise(1), Error.io);
    tassert_eq(e$traceback_len, 1);
    tassert_eq(e$traceback_arr[0].err, Error.io);
    tassert(e$traceback_arr[0].file != NULL);

    u32 n = 0;
    for$each(it, e$traceback_arr, e$traceback_len)
    {
        (void)it;
        n++;
    }
    tassert_eq(n, 1);

    char* s = e$traceback_fmt(mem$);
    tassert(str.find(s, "#0") != NULL);
    tassert(str.find(s, "raise io") == NULL);
    sbuf.destroy(&s);
    return EOK;
}

test$case(lvl1_ret_appends)
{
    tassert_eq(err_ret(1), Error.io);
    tassert_eq(e$traceback_len, 2);

    u32 n = 0;
    for$each(it, e$traceback_arr, e$traceback_len)
    {
        (void)it;
        n++;
    }
    tassert_eq(n, 2);
    return EOK;
}

test$case(lvl1_raise_resets)
{
    tassert_eq(err_raise(1), Error.io);
    tassert_eq(err_raise(1), Error.io);
    tassert_eq(e$traceback_len, 1);

    u32 n = 0;
    for$each(it, e$traceback_arr, e$traceback_len)
    {
        (void)it;
        n++;
    }
    tassert_eq(n, 1);
    return EOK;
}

test$main();
