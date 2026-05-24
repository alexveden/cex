#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"

//test$setup_case() {return EOK;}
//test$teardown_case() {return EOK;}
//test$setup_suite() {return EOK;}
//test$teardown_suite() {return EOK;}

test$bench(my_bench){
    //os.sleep(100);
    printf("os.timer() %0.7f\n", os.timer());
    //os.sleep(100);
    printf("os.timer() %0.7f\n", os.timer());

    return EOK;
}

test$bench(my_bench2){
    os.sleep(1);

    return EOK;
}


test$main();
