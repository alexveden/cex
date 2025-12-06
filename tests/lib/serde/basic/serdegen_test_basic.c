// WARNING: this is a special test for meta-programming and code generation, 
//          it should be compiled and called from another test
#define CEX_BUILD
#define CEX_TEST
#define CEX_IMPLEMENTATION
#include "cex.h"
#include "serdegen.c"
#include "lib/json/json.c"
#include "Stock.h"
#include "Position.h"

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

test$case(test_Stock_serialize)
{
    Stock s = { .id = 9988, .exchange = "NYSE", .ticker = "UBER" };
    serdegen.Stock.print(&s, NULL);

    sbuf_c sb = sbuf.create(1024, mem$);
    jw_c jw;
    e$ret(jw$new(&jw, .indent = 4, .buf = sb));
    e$ret(serdegen.Stock.serialize(&jw, &s));

    io.printf("\nsbuf=`%s`\n", sb);


    jr_c jr;
    e$ret(jr$new(&jr, sb, 0, .strict_mode = true));

    Stock s2 = {0};
    e$ret(serdegen.Stock.deserialize(&jr, &s2, mem$));

    tassert_eq(s2.exchange, "NYSE" );
    tassert_eq(s2.ticker, "UBER" );
    tassert_eq(s2.id, 9988);


    sbuf.destroy(&sb);
    serdegen.Stock.destroy(&s2, mem$);

    return EOK;
}

test$case(test_Position_serialize)
{
    Stock s = { .id = 9988, .exchange = "NYSE", .ticker = "UBER" };
    Position p = {
        .qty = -10,
        .fill_price = 9.123456,
        .stock = &s,
    };

    serdegen.Position.print(&p, NULL);
    sbuf_c sb = sbuf.create(1024, mem$);
    jw_c jw;
    e$ret(jw$new(&jw, .indent = 4, .buf = sb));
    e$ret(serdegen.Position.serialize(&jw, &p));

    jr_c jr;
    e$ret(jr$new(&jr, sb, 0, .strict_mode = true));

    Position p2 = {0};
    e$ret(serdegen.Position.deserialize(&jr, &p2, mem$));

    tassert_eq(p2.qty, -10);
    tassert_eq_almost(p2.fill_price, 9.123456f, 0.03);

    tassert(p2.stock);
    tassert_eq(p2.stock->exchange, "NYSE");
    tassert_eq(p2.stock->ticker, "UBER");

    sbuf.destroy(&sb);
    serdegen.Position.destroy(&p2, mem$);

    return EOK;
}

test$main();
