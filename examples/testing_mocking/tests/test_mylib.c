#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/mylib.c"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

i32
fake_add(mylib_c* self, i32 a, i32 b)
{
    (void)self;
    return a + b + 1;
}

typedef struct mylib_tester_c
{
    mylib_c base;
    u32 magic;
    i32 add_called;
} mylib_tester_c;

mylib_tester_c*
mylib_tester_cast(mylib_c* self) {
    mylib_tester_c* t = (mylib_tester_c*)self;
    uassert(t->magic == 0xf00feed);
    return t;
}

i32
mylib_tester_add(mylib_c* self, i32 a, i32 b)
{
    mylib_tester_c* t = mylib_tester_cast(self);

    log$debug("hi from: mylib_tester!\n");
    // WARNING: using raw function name to avoid infinite recursion
    t->add_called++;

    return mylib_add(self, a, b);
}

mylib_c*
mylib_tester_create(void)
{
    mylib.add = mylib_tester_add;
    mylib_tester_c* tester = mem$malloc(mem$, sizeof(mylib_tester_c));
    *tester = (mylib_tester_c){
        .base = {0},
        .add_called = 0,
        .magic = 0xf00feed,
    };
    return &tester->base;
}

void
mylib_tester_destroy(mylib_c* self)
{
    mylib.add = mylib_add;
    mem$free(mem$, self);
}

test$case(mylib_init)
{
    mylib_c m;
    tassert_eq(mylib.create(&m, 7), EOK);
    tassert_eq(m.base_num, 7);
    tassert_eq(mylib.add(&m, 1, 2), 1 + 2 + 7);
    return EOK;
}

test$case(mylib_test_case_mock)
{
    mylib.add = fake_add;
    mylib_c m;
    tassert_eq(mylib.create(&m, 7), EOK);

    tassert_eq(mylib.add(&m, 1, 2), 4);
    // Next will be available after calling `cex process lib/mylib.c`
    // tassert_eq(mylib.add(1, 2), 3);
    return EOK;
}

test$case(mylib_test_case_fake)
{
    tassert(mylib.add == fake_add);
    return EOK;
}

test$case(mylib_test_tester)
{
    mylib_c* m = mylib_tester_create();
    tassert(m);

    auto t = mylib_tester_cast(m);

    tassert(mylib.add == mylib_tester_add);
    tassert_eq(mylib.add(m, 1, 2), 1 + 2);
    tassert(mylib.add == mylib_tester_add);
    tassert_eq(t->add_called, 1);

    mylib_tester_destroy(m);
    tassert(mylib.add == mylib_add);

    return EOK;
}


test$main();
