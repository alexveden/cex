#include <_cexlib/cex.c>
#include <_cexlib/cextest.h>
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
    return EOK;
}


test$NOOPT Exception
foo(int condition)
{
    if (condition == 0) {
        return raise_exc(Error.io, "condition == 0\n");
    }

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
    e$goto(check_with_dollar(-1), fail);
    tassert(false && "unreacheble");
    
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
    test$run(test_null_ptr);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
