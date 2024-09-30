#include "_cexcore/cex.h"
#include <_cexcore/cex.c>
#include <_cexcore/cextest.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <x86intrin.h>

/*
 * SUITE INIT / SHUTDOWN
 */
test$teardown(){
    return EOK;
}

test$setup(){
    uassert_enable();
    return EOK;
}


test$NOOPT Exception
foo(int condition)
{
    if (condition == 0) {
        return raise_exc(Error.io, "condition == 0\n");
    }
    if (condition == 2)
        return Error.memory;

    return EOK;
}

test$NOOPT int
sys_func(int condition)
{
    if (condition == -1) {
        errno = 999;
    }
    return condition;
}

test$NOOPT void*
void_ptr_func(int condition)
{
    if (condition == -1) {
        return NULL;
    }
    return &errno;
}


test$case(test_sysfunc)
{

    int ret = 0;

    errno = 777;
    u32 nit = 0;
    except_errno(ret = sys_func(-1))
    {
        printf("Except: ret=%d errno=%d\n", ret, errno);
        tassert_eqi(errno, 999);
        tassert_eqi(ret, -1);
        nit++;
    }
    tassert_eqi(nit, 1);

    errno = 777;
    nit = 0;
    except_errno(ret = sys_func(100))
    {
        tassert(false && "not expected");
        nit++;
    }
    tassert_eqi(nit, 0);
    tassert_eqi(errno, 0); // errno is reset before except_errno() starts!
    tassert_eqi(ret, 100);
    return EOK;
}

Exception
check(int condition)
{
    if (condition == -1){
        return Error.memory;
    }
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

Exception check_optimized(int e){

    int ret = 0;

    errno = 777;
    u32 nit = 0;
    except_errno(ret = sys_func(-1))
    {
        return Error.io;
    }
    (void)nit;
    (void)e;

    e$ret(check(e));

    return EOK;
}

test$case(test_e_dollar_macro)
{
    tassert_eqs(EOK, check_with_dollar(true));
    tassert_eqs(Error.memory, check_with_dollar(-1));
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
    tassert_eqe(Error.ok, result);

    // we can explicitly set result value to the error of the call
    e$goto(result = check_with_dollar(-1), fail);
    tassert(false && "unreacheble, the above must fail");
    
fail:
    tassert_eqe(Error.memory, result);

    return EOK;
}

test$case(test_e_dollar_macro_assert)
{
    Exc result = EOK;
    tassert_eqi(e$raise_is_ok(), 1);
    e$raise_enable();
    tassert_eqi(e$raise_is_ok(), 0);
    e$raise_disable();
    tassert_eqi(e$raise_is_ok(), 1);

    // On fail jumps to 'fail' label + sets the result to returned by check_with_dollar()
    e$goto(check_with_dollar(-1), setresult);

setresult:
    // e$goto() - previous jump didn't change result
    tassert_eqe(Error.ok, result);

    // we can explicitly set result value to the error of the call
    e$goto(result = check_with_dollar(-1), fail);
    tassert(false && "unreacheble, the above must fail");
    
fail:
    tassert_eqe(Error.memory, result);

    return EOK;
}


test$case(test_null_ptr)
{
    void* res = NULL;
    except_null(res = void_ptr_func(1))
    {
        tassert(false && "not expected");
    }
    tassert(res != NULL);

    except_null(res = void_ptr_func(-1))
    {
        tassert(res == NULL);
    }
    tassert(res == NULL);

    except_null(void_ptr_func(-1))
    {
        tassert(res == NULL);
    }

    return EOK;
}

test$case(test_nested_excepts)
{

    except_silent(err, foo(0)){
        tassert_eqe(err, Error.io);

        except_silent(err, foo(2)){
            tassert_eqe(err, Error.memory);
        }

        // err, back after nested handling!
        tassert_eqe(err, Error.io);
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

    for(u32 i = 1; i< sizeof(Error)/sizeof(Error.ok); i++){
        tassertf(e[i] != NULL, "Error. %d-th member is NULL", i); // Empty error
        tassertf(strlen(e[i]) > 2, "Exception is empty or too short: [%d]: %s", i, e[i] );
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
    tassert_eqe(Error.assert, result);

    return EOK;
}
/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_sysfunc);
    test$run(test_e_dollar_macro);
    test$run(test_e_dollar_macro_goto);
    test$run(test_e_dollar_macro_assert);
    test$run(test_null_ptr);
    test$run(test_nested_excepts);
    test$run(test_all_errors_set);
    test$run(test_eassert);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
