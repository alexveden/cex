#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/json/json.c"
#include "myserde.c"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(test_Stock_serialize)
{
    Stock s = { .id = 9988, .exchange = "NYSE", .ticker = "UBER" };
    myserde.Stock.print(&s, NULL);

    sbuf_c sb = sbuf.create(1024, mem$);
    jw_c jw;
    e$ret(jw$new(&jw, .indent = 4, .buf = sb));
    e$ret(myserde.Stock.serialize(&jw, &s, NULL));

    io.printf("\nsbuf=`%s`\n", sb);


    jr_c jr;
    e$ret(jr$new(&jr, sb, 0, .strict_mode = true));

    Stock s2 = {0};
    e$ret(myserde.Stock.deserialize(&jr, &s2, mem$));

    tassert_eq(s2.exchange, "NYSE" );
    tassert_eq(s2.ticker, "UBER" );
    tassert_eq(s2.id, 9988);


    sbuf.destroy(&sb);
    myserde.Stock.destroy(&s2, mem$);

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

    myserde.Position.print(&p, NULL);
    sbuf_c sb = sbuf.create(1024, mem$);
    jw_c jw;
    e$ret(jw$new(&jw, .indent = 4, .buf = sb));
    e$ret(myserde.Position.serialize(&jw, &p, NULL));

    jr_c jr;
    e$ret(jr$new(&jr, sb, 0, .strict_mode = true));

    Position p2 = {0};
    e$ret(myserde.Position.deserialize(&jr, &p2, mem$));

    tassert_eq(p2.qty, -10);
    tassert_eq(p2.fill_price, 9.12f);

    tassert(p2.stock);
    tassert_eq(p2.stock->exchange, "NYSE");
    tassert_eq(p2.stock->ticker, "UBER");

    sbuf.destroy(&sb);
    myserde.Position.destroy(&p2, mem$);

    return EOK;
}

test$main();
