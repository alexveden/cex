// WARNING: this is a special test for meta-programming and code generation, 
//          it should be called from another test
#define CEX_BUILD
#define CEX_TEST
#define CEX_IMPLEMENTATION
#include "cex.h"
#include "serdegen.c"
#include "lib/json/json.c"
#include "myserde.h"

test$case(serde_basic) {
    Stock s = { .id = 9988, .exchange = "NYSE", .ticker = "UBER" };
    Position p = {
        .qty = -10,
        .fill_price = 9.123456,
        .stock = &s,
    };
    serdegen.Position.print(&p, NULL);
    io.printf("\n");

    // tassert_eq(1, 0);
    // uassert(false);

    return EOK;
}

test$main();
