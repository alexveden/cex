#include "src/all.c"

test$noopt Exception
foo(int condition)
{
    if (condition == 0) { return e$raise(Error.io, "condition == 0"); }
    if (condition == 2) { return Error.memory; }

    return EOK;
}

test$noopt int
sys_func(int condition)
{
    if (condition == -1) { errno = 999; }
    if (condition < -1) { errno = -condition; }
    return condition;
}

test$noopt void*
void_ptr_func(int condition)
{
    if (condition == -1) { return NULL; }
    return &errno;
}


test$case(test_sysfunc)
{

    int ret = 0;

    errno = 777;
    u32 nit = 0;
    tassert_eq(-1, sys_func(-1));
    e$except_errno (ret = sys_func(-1)) {
        log$error("Except: ret=%d errno=%d\n", ret, errno);
        tassert_eq(_tmp_errno, 999);
        tassert_eq(errno, 999);
        tassert_eq(ret, -1);
        nit++;
    }
    tassert_eq(nit, 1);

    errno = 777;
    nit = 0;
    e$except_errno (ret = sys_func(100)) {
        tassert(false && "not expected");
        nit++;
    }
    tassert_eq(nit, 0);
    tassert_eq(ret, 100);

    errno = 777;
    nit = 0;
    e$except_errno (ret = sys_func(0)) {
        tassert(false && "not expected");
        nit++;
    }
    tassert_eq(nit, 0);
    tassert_eq(ret, 0);

    errno = 777;
    nit = 0;
    e$except_errno (ret = sys_func(-2)) {
        nit++;
        tassert_eq(errno, 2);
    }
    tassert_eq(nit, 1);
    tassert_eq(ret, -2);
    return EOK;
}

Exception
check(int condition)
{
    if (condition == -1) { return Error.memory; }
    return EOK;
}

Exception
check_with_dollar(int condition)
{
    e$ret(check(condition));
    return EOK;
}

Exception
check_with_assert(int condition)
{
    e$assert(condition != -1);

    return EOK;
}

Exception
check_optimized(int e)
{

    int ret = 0;

    errno = 777;
    u32 nit = 0;
    e$except_errno (ret = sys_func(-1)) { return Error.io; }
    (void)nit;
    (void)e;

    e$ret(check(e));

    return EOK;
}

test$case(test_except_true)
{
    int ret = 8;
    e$except_true (ret = sys_func(0)) { tassert(false && "unexpected"); }
    tassert_eq(ret, 0);

    u32 nit = 0;
    e$except_true (ret = sys_func(1)) { nit++; }
    tassert_eq(ret, 1);
    tassert_eq(nit, 1);

    // Any non zero value works
    e$except_true (ret = sys_func(100)) { nit++; }
    tassert_eq(ret, 100);
    tassert_eq(nit, 2);

    e$except_true (ret = sys_func(-1)) { nit++; }
    tassert_eq(ret, -1);
    tassert_eq(nit, 3);

    return EOK;
}

test$case(test_e_dollar_macro)
{
    tassert_eq(EOK, check_with_dollar(true));
    tassert_eq(Error.memory, check_with_dollar(-1));
    return EOK;
}

test$case(test_e_dollar_macro_goto)
{
    Exc result = EOK;
    // On fail jumps to 'fail' label + sets the result to returned by check_with_dollar()
    e$goto(check_with_dollar(-1), setresult);
    tassert(false && "unreacheble, the above must fail");

setresult:
    // e$goto() - previous jump didn't change result
    tassert_er(Error.ok, result);

    // we can explicitly set result value to the error of the call
    e$goto(result = check_with_dollar(-1), fail);
    tassert(false && "unreacheble, the above must fail");

fail:
    tassert_er(Error.memory, result);

    return EOK;
}


test$case(test_null_ptr)
{
    void* res = NULL;
    e$except_null (res = void_ptr_func(1)) { tassert(false && "not expected"); }
    tassert(res != NULL);

    e$except_null (res = void_ptr_func(-1)) { tassert(res == NULL); }
    tassert(res == NULL);

    e$except_null (void_ptr_func(-1)) { tassert(res == NULL); }

    return EOK;
}

test$case(test_nested_excepts)
{

    tassert_eq(Error.io, foo(0));

    e$except_silent (err, foo(0)) {
        tassert_er(err, Error.io);

        e$except_silent (err, foo(2)) { tassert_er(err, Error.memory); }

        // err, back after nested handling!
        tassert_er(err, Error.io);
        return EOK;
    }

    tassert(false && "unreachable");
    return EOK;
}


test$case(test_all_errors_set)
{
    const Exc* e = &Error.ok;
    tassert(Error.ok == NULL);
    tassert(EOK == NULL);
    tassert(e[0] == NULL && "EOK!");

    for (u32 i = 1; i < sizeof(Error) / sizeof(Error.ok); i++) {
        tassertf(e[i] != NULL, "Error. %d-th member is NULL", i); // Empty error
        tassertf(strlen(e[i]) > 2, "Exception is empty or too short: [%d]: %s", i, e[i]);
    }

    return EOK;
}
test$case(test_eassert)
{
    Exc result = EOK;

    // On fail jumps to 'fail' label + sets the result to returned by check_with_dollar()
    e$goto(result = check_with_assert(-1), fail);

    tassert(false && "unreacheble, the above must fail");

fail:
    tassert_er(Error.assert, result);

    return EOK;
}

test$case(test_log)
{
    log$error("log error test\n");
    log$warn("log warn test\n");
    log$info("log info test\n");
    log$debug("log debug test\n");

    return EOK;
}


test$case(test_foreachp_zero_offset_to_null_ub)
{
    int* arr = NULL;

    for$eachp(it, arr)
    {
        (void)it;
        tassert(false && "never!");
    }

    for$eachp(it, arr, 0)
    {
        (void)it;
        tassert(false && "never!");
    }
    return EOK;
}

test$main();
