#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"


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

test$case(my_test_ns_mock_scope){
    tassert(os.timer == cex_os_timer);

    test$mock_ns(os) {
        os.timer = timer_mock;
        tassert_eq(777888.9, os.timer());
    }
    // restored
    tassert(os.timer == cex_os_timer);

    tassert_ne(777888.9, os.timer());

    return EOK;
}

test$case(my_test_ns_mock_scope_early_return){
    test$mock_ns(os) {
        os.timer = timer_mock;
        tassert_eq(777888.9, os.timer());
        /* early return — cleanup must still fire */
        return EOK;
    }

    /* unreachable, but keeps compiler happy */
    return EOK;
}


test$case(my_test_ns_mock_scope_two_ns){
    test$mock_ns(os, io) {
        os.timer = timer_mock;
        io.printf = NULL;  /* corrupt it */
        tassert_eq(777888.9, os.timer());
    }

    /* both restored */
    tassert_ne(777888.9, os.timer());
    tassert(io.printf != NULL);

    return EOK;
}

test$main();
