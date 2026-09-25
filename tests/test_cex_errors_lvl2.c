#define CEX_TRACEBACK_LVL 2
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

test$noopt Exception
raw_err(void)
{
    return Error.io;
}

test$noopt Exception
append_frame(void)
{
    e$ret(raw_err());
    return EOK;
}

test$noopt int
sys_fail(void)
{
    errno = ENOENT;
    return -1;
}

test$noopt void*
null_ret(void)
{
    return NULL;
}

test$noopt int
true_ret(void)
{
    return 1;
}

test$setup_case()
{
    e$traceback_reset();
    return EOK;
}

test$case(lvl2_record_has_func_and_msg)
{
    tassert_eq(err_raise(1), Error.io);
    tassert_eq(e$traceback_len, 1);
    tassert_eq(e$traceback_arr[0].err, Error.io);
    tassert_eq((char*)e$traceback_arr[0].msg, "raise io");
    tassert(e$traceback_arr[0].func != NULL);

    char* s = e$traceback_fmt(mem$);
    tassert(str.find(s, "#0") != NULL);
    tassert(str.find(s, "raise io") != NULL);
    sbuf.destroy(&s);
    return EOK;
}

test$case(lvl2_ret_chain)
{
    tassert_eq(err_ret(1), Error.io);
    tassert_eq(e$traceback_len, 2);
    tassert_eq((char*)e$traceback_arr[0].msg, "raise io");
    tassert(str.find((char*)e$traceback_arr[1].msg, "err_raise") != NULL);
    return EOK;
}

test$case(lvl2_errno_origin)
{
    e$except_errno(sys_fail()) {}

    tassert_eq(e$traceback_len, 1);
    tassert_eq(e$traceback_arr[0].err, strerror(ENOENT));
    tassert_eq((char*)e$traceback_arr[0].msg, "sys_fail()");
    return EOK;
}

test$case(lvl2_null_origin)
{
    e$except_null(null_ret()) {}

    tassert_eq(e$traceback_len, 1);
    tassert_eq(e$traceback_arr[0].err, Error.null_or_empty);
    tassert_eq((char*)e$traceback_arr[0].msg, "null_ret()");
    return EOK;
}

test$case(lvl2_true_origin)
{
    e$except_true(true_ret()) {}

    tassert_eq(e$traceback_len, 1);
    tassert_eq(e$traceback_arr[0].err, Error.runtime);
    tassert_eq((char*)e$traceback_arr[0].msg, "true_ret()");
    return EOK;
}

test$case(lvl2_ring_clamps)
{
    for (int i = 0; i < CEX_ERR_CAP + 8; i++) { tassert_eq(append_frame(), Error.io); }
    tassert_eq(e$traceback_len, CEX_ERR_CAP);

    u32 n = 0;
    for$each(it, e$traceback_arr, e$traceback_len)
    {
        (void)it;
        n++;
    }
    tassert_eq(n, CEX_ERR_CAP);
    return EOK;
}

test$case(lvl2_print)
{
    tassert_eq(err_raise(1), Error.io);

    FILE* f = tmpfile();
    tassert(f != NULL);

    e$traceback_print(f);

    usize pos = 0;
    tassert_eq(io.ftell(f, &pos), EOK);
    tassert(pos > 0);

    io.fclose(&f);
    return EOK;
}

test$main();
