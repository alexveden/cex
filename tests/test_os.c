#include "src/all.c"
#include <math.h>

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(test_timer)
{
    f64 t = os.timer();
    tassert(t > 0);
    tassert(!isnan(t));
    tassert(t < INFINITY);
    tassert(t > -INFINITY);

    f64 t2 = os.timer();
    tassert(t2 >= t);

    os.sleep(100);
    t2 = os.timer();
    f64 tdiff = t2 - t;
    // NOTE: CI timings may be very slow, we estimate order of magnitude
    tassertf(tdiff > 0.1 && tdiff < 0.35, "%g", tdiff);

    t = t2;
    os.sleep(1100);
    t2 = os.timer();
    tdiff = t2 - t;
    tassertf(tdiff > 1.1 && tdiff < 1.35, "%g", tdiff);

    return EOK;
}

test$case(test_cpu_count)
{

    tassert_ge(os.cpu_count(), 1);
    tassert_le(os.cpu_count(), 128);

    return EOK;
}
test$main();
