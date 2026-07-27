#define CEX_LOG_LVL 0
#include "src/all.c"


test$noopt Exception
foo(int condition)
{
    if (condition == 0) { return e$raise(Error.io, "condition == 0\n"); }
    if (condition == 2) { return Error.memory; }

    return EOK;
}

test$noopt int
sys_func(int condition)
{
    if (condition == -1) { errno = 999; }
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
    e$except_errno (ret = sys_func(-1)) {
        if (ret != -1 || errno != 999){
            printf("Except: ret=%d errno=%d\n", ret, errno);
        }
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

#define test_err(a)                                                                                \
    ({                                                                                             \
        Exc err = NULL;                                                                            \
        (a) ? a : err;                                                                             \
    })

#define test_a(a)                                                                                  \
    ({                                                                                             \
        int _a = a;                                                                                \
        _a + 1;                                                                                    \
    })
test$case(test_nested_compound_expr)
{
    Exc err = Error.memory;
    Exc a = Error.io;
    tassert(err == Error.memory);
    tassert(a == Error.io);
    tassert(test_err(Error.memory) == Error.memory);
    tassert(test_err(a) == Error.io);
    // tassert(test_err(err) == Error.memory);
    tassert(err == Error.memory);
    tassert(a == Error.io);
    int _a = 0;
    tassert_eq(test_a(2), 3);
    tassert_eq(_a, 0);

    return EOK;
}

test$case(test_nested_excepts)
{

    tassert_er(Error.io, foo(0));
    e$except_silent (err, foo(0)) {
        printf("Loop: %s\n", err);
        tassert(err == Error.io);
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

test$main();
