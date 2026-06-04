#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"

//test$setup_case() {return EOK;}
//test$teardown_case() {return EOK;}
//test$setup_suite() {return EOK;}
//test$teardown_suite() {return EOK;}

f64 timer_mock(void){
    return 777888.9;
}

test$case(my_test_mocking_capabilities){
    f64 orig_time = os.timer();
     
    os.timer = timer_mock;
    tassert_eq(777888.9, os.timer());

    os.timer = cex_os_timer;
    tassert_ne(777888.9, os.timer());
    tassert_le(os.timer(), orig_time + 1.0);

    return EOK;
}

test$main();
