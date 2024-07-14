#include <cex/cex.c>
#include <cex/cex.h>
#include <cex/cextest/cextest.h>
#include <errno.h>
#include <stdio.h>
#include <x86intrin.h>

/*
 * SUITE INIT / SHUTDOWN
 */
void
my_test_shutdown_func(void)
{
    // instead of printf(....) consider to use alogf() it's more structural and contains
    // line source:lnumber reference

    // atlogf("atest_shutdown()\n");
}

ATEST_SETUP_F(void)
{
    // atlogf("atest_setup()\n");

    // return NULL;   // if no shutdown logic needed
    return &my_test_shutdown_func; // return pointer to void some_shutdown_func(void)
}

ATEST_NOOPT Exception
foo(int condition)
{
    if (condition == 0) {
        return raise_exc(Error.io, "condition == 0\n");
    }

    return EOK;
}

ATEST_NOOPT int
sys_func(int condition)
{
    if (condition == -1) {
        errno = 999;
    }

    // e$(foo(condition));
    return condition;
}

ATEST_NOOPT void*
void_ptr_func(int condition)
{
    if (condition == -1) {
        return NULL;
    }
    // e$(foo(condition));
    return &errno;
}


ATEST_F(test_sysfunc)
{

    int ret = 0;

    errno = 777;
    u32 nit = 0;
    except_errno(ret = sys_func(-1))
    {
        printf("Except: ret=%d errno=%d\n", ret, errno);
        atassert_eqi(errno, 999);
        atassert_eqi(ret, -1);
        nit++;
    }
    atassert_eqi(nit, 1);

    errno = 777;
    nit = 0;
    except_errno(ret = sys_func(100))
    {
        atassert(false && "not expected");
        nit++;
    }
    atassert_eqi(nit, 0);
    atassert_eqi(errno, 0); // errno is reset before except_errno() starts!
    atassert_eqi(ret, 100);
    return NULL;
}

Exception
check(int condition)
{
    if (condition == -1){
        return Error.memory;
    }
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

    // except(err, check(e))
    // {
    //     return err;
    // }
    // e$(check(e));
    // except_traceback(err, check(e))
    // {
    //     return err;
    // }
    //
    // Exc err = NULL;
    // if((err = check(e))){
    //     return err;
    // } 

    return EOK;
}

ATEST_F(test_e_dollar_macro)
{

    if (check_optimized(-1) == Error.io){
        return "foo";
    }
    // atassert_eqs(EOK, check(true));
    // atassert_eqs(Error.io, check(false));
    return NULL;
}

ATEST_F(test_null_ptr)
{
    void* res = NULL;
    except_null(res = void_ptr_func(1))
    {
        atassert(false && "not expected");
    }
    atassert(res != NULL);

    except_null(res = void_ptr_func(-1))
    {
        atassert(res == NULL);
    }
    atassert(res == NULL);

    except_null(void_ptr_func(-1))
    {
        atassert(res == NULL);
    }
    return NULL;
}

// ATEST_F(test_error_change)
// {
//     char ** err = (char**)(&Error.integrity);
//     printf("err=%p\n", *err);
//     *err = "foo";
//     atassert_eqs(Error.integrity, "foo");
//
//     atassert_eqs(Error.integrity, "foo");
//     return NULL;
// }
/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    ATEST_PARSE_MAINARGS(argc, argv);
    ATEST_PRINT_HEAD(); // >>> all tests below

    ATEST_RUN(test_sysfunc);
    ATEST_RUN(test_e_dollar_macro);
    ATEST_RUN(test_null_ptr);

    ATEST_PRINT_FOOTER(); // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
