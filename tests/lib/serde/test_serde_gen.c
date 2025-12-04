#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/json/SerdeGen.c"

//test$setup_case() {return EOK;}
//test$teardown_case() {return EOK;}
//test$setup_suite() {return EOK;}
//test$teardown_suite() {return EOK;}

test$case(my_test_case) {
    mem$scope(tmem$, _) {
        char* code = io.file.load("tests/lib/serde/myserde.h", _);

        SerdeGen_c sg;
        e$ret(SerdeGen.create(&sg, _));
        tassert(code && "Load filed");
        e$ret(SerdeGen.process_code(&sg, code, 0));
    }
    tassert_eq(1, 0);
    return EOK;
}

test$main();
