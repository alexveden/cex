#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/mylib.c"

//test$setup_case() {return EOK;}
//test$teardown_case() {return EOK;}
//test$setup_suite() {return EOK;}
//test$teardown_suite() {return EOK;}

test$case(mylib_test_case){
    tassert_eq(mylib_add(1, 2), 3);
    // Next will be available after calling `cex process lib/mylib.c`
    // tassert_eq(mylib.add(1, 2), 3);
    return EOK;
}

test$bench(mylib_test_bench){
    // This code only run when `./cex test bench <filename>` is called
    // consider calling project specific function, you may use test$setup_case() for making test data
    // Contents of this function intentionally are not optimized by compiler
    mylib_add(1, 2);
    return EOK;
}

test$main();
