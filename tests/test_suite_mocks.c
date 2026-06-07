#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"


f64 timer_mock(void){
    return 777888.9;
}

f64 timer_mock2(void){
    return 999999.9;
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

    test$mock_scope(os) {
        os.timer = timer_mock;
        tassert_eq(777888.9, os.timer());
    }
    // restored
    tassert(os.timer == cex_os_timer);

    tassert_ne(777888.9, os.timer());

    return EOK;
}

test$case(my_test_ns_mock_scope_early_return){
    test$mock_scope(os) {
        os.timer = timer_mock;
        tassert_eq(777888.9, os.timer());
        /* early return — cleanup must still fire */
        return EOK;
    }

    /* unreachable, but keeps compiler happy */
    return EOK;
}


test$case(my_test_ns_mock_scope_two_ns){
    test$mock_scope(os, io) {
        os.timer = timer_mock;
        io.printf = NULL;  /* corrupt it */
        tassert_eq(777888.9, os.timer());
    }

    /* both restored */
    tassert_ne(777888.9, os.timer());
    tassert(io.printf != NULL);

    return EOK;
}

test$case(test_ns_mock_nested){
    tassert(os.timer == cex_os_timer);

    test$mock_scope(os) {
        os.timer = timer_mock;
        tassert_eq(777888.9, os.timer());

        test$mock_scope(os) {
            tassert_eq(777888.9, os.timer());
            os.timer = timer_mock2;
            tassert_eq(999999.9, os.timer());
        }

        /* inner restored — outer's mock preserved */
        tassert_eq(777888.9, os.timer());
        tassert(os.timer == timer_mock);
    }

    /* outer restored — original preserved */
    tassert(os.timer == cex_os_timer);
    tassert_ne(777888.9, os.timer());

    return EOK;
}

test$case(test_ns_mock_with_oom){
    tassert(os.timer == cex_os_timer);

    test$alloc_set_oom_probability(1.0);
    // mock must not be affected
    test$mock_scope(os) {
        os.timer = timer_mock;
        tassert_eq(777888.9, os.timer());
    }

    /* outer restored — original preserved */
    tassert(os.timer == cex_os_timer);
    tassert_ne(777888.9, os.timer());

    return EOK;
}

test$main();
