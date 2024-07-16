#include <cex/cexlib/cex.c>
#include <cex/cexlib/cextest.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <x86intrin.h>

/*
 * SUITE INIT / SHUTDOWN
 */
cextest$teardown(){
    return EOK;
}

cextest$setup(){
    return EOK;
}


cextest$NOOPT Exception
foo(int condition)
{
    if (condition == 0) {
        return raise_exc(Error.io, "condition == 0\n");
    }

    return EOK;
}

cextest$NOOPT int
sys_func(int condition)
{
    if (condition == -1) {
        errno = 999;
    }
    return condition;
}

cextest$NOOPT void*
void_ptr_func(int condition)
{
    if (condition == -1) {
        return NULL;
    }
    return &errno;
}


cextest$case(test_sysfunc)
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
    e$(check(condition));
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

    e$(check(e));

    return EOK;
}

cextest$case(test_e_dollar_macro)
{
    tassert_eqs(EOK, check_with_dollar(true));
    tassert_eqs(Error.memory, check_with_dollar(-1));
    return EOK;
}

cextest$case(test_null_ptr)
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
    cextest$args_parse(argc, argv);
    cextest$print_header();  // >>> all tests below
    
    cextest$run(test_sysfunc);
    cextest$run(test_e_dollar_macro);
    cextest$run(test_null_ptr);
    
    cextest$print_footer();  // ^^^^^ all tests runs are above
    return cextest$exit_code();
}
