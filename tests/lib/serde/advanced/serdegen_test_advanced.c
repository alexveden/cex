// WARNING: this is a special test for meta-programming and code generation,
//          it should be compiled and called from another test
#define CEX_BUILD
#define CEX_TEST
#define CEX_IMPLEMENTATION
#include "AdvPosition.h"
#include "AdvStock.h"
#include "cex.h"
#include "lib/json/json.c"
#include "serdegen.c"

void
print_json_expected(sbuf_c s)
{
    uassert(s);

    io.printf("char* expected = \"");
    for$each (c, s, sbuf.len(&s)) {
        switch (c) {
            case '\n':
                io.printf("\\n\\");
                break;
            case '"':
                io.printf("\\");
                break;
        }
        io.printf("%c", c);
    }

    io.printf("\";\n");
}

test$case(serde_basic)
{
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
    e$ret(jw$new(&jw, .indent = 4, .buf = &sb));
    e$ret(serdegen.Stock.serialize(&jw, &s));

    io.printf("\nJSON OUTPUT\n%s\n", sb);
    print_json_expected(sb);
    char* expected = "{\n\
    \"id\": 9988, \n\
    \"ticker\": \"UBER\", \n\
    \"exchange\": \"NYSE\"\n\
}";

    tassert_eq(sb, expected);

    jr_c jr;
    e$ret(jr$new(&jr, sb, 0, .strict_mode = true));

    Stock s2 = { 0 };
    e$ret(serdegen.Stock.deserialize(&jr, &s2, mem$));

    tassert_eq(s2.exchange, "NYSE");
    tassert_eq(s2.ticker, "UBER");
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
    e$ret(jw$new(&jw, .indent = 4, .buf = &sb));
    e$ret(serdegen.Position.serialize(&jw, &p));


    io.printf("\nJSON OUTPUT\n%s\n", sb);
    print_json_expected(sb);
    char* expected = "{\n\
    \"qty\": -10, \n\
    \"fill_price\": 9.123456, \n\
    \"stock\": {\n\
        \"id\": 9988, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"NYSE\"\n\
    }\n\
}";
    tassert_eq(sb, expected);
    jr_c jr;
    e$ret(jr$new(&jr, sb, 0, .strict_mode = true));

    Position p2 = { 0 };
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

test$case(test_NullableItems)
{
    ItemNullable s = { 0 };
    serdegen.ItemNullable.print(&s, NULL);

    sbuf_c sb = sbuf.create(1024, mem$);
    jw_c jw;
    e$ret(jw$new(&jw, .indent = 4, .buf = &sb));
    e$ret(serdegen.ItemNullable.serialize(&jw, &s));

    io.printf("\nJSON OUTPUT\n%s\n", sb);
    print_json_expected(sb);

    char* expected = "{\n\
    \"sbuf_field\": null, \n\
    \"str_s_field\": null, \n\
    \"char_field\": null, \n\
    \"stock_field\": null, \n\
    \"stock_val\": {\n\
        \"id\": 0, \n\
        \"ticker\": null, \n\
        \"exchange\": null\n\
    }\n\
}";
    tassert_eq(sb, expected);

    jr_c jr;
    e$ret(jr$new(&jr, sb, 0, .strict_mode = true));

    ItemNullable s2 = { 0 };
    e$ret(serdegen.ItemNullable.deserialize(&jr, &s2, mem$));

    tassert_eq(s2.char_field, NULL);
    tassert_eq(s2.sbuf_field, NULL);
    tassert_eq(s2.str_s_field.buf, NULL);
    tassert_eq(s2.str_s_field.len, 0);
    tassert(s2.stock_field == NULL);

    sbuf.destroy(&sb);
    serdegen.ItemNullable.destroy(&s2, mem$);

    return EOK;
}

test$case(test_NullableItems_initialized)
{
    sbuf_c sb_item = sbuf.create(1024, mem$);
    e$ret(sbuf.append(&sb_item, "hello_sbuf"));

    ItemNullable s = { .char_field = "hello_char",
                       .sbuf_field = sb_item,
                       .str_s_field = str$s("hello_str_s"),
                       .stock_val = {
                           .exchange = "EXCH",
                           .ticker = "SPY",
                           .id = 9988,
                       } };
    serdegen.ItemNullable.print(&s, NULL);

    sbuf_c sb = sbuf.create(1024, mem$);
    jw_c jw;
    e$ret(jw$new(&jw, .indent = 4, .buf = &sb));
    e$ret(serdegen.ItemNullable.serialize(&jw, &s));

    io.printf("\nJSON OUTPUT\n%s\n", sb);
    print_json_expected(sb);

    char* expected = "{\n\
    \"sbuf_field\": \"hello_sbuf\", \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": null, \n\
    \"stock_val\": {\n\
        \"id\": 9988, \n\
        \"ticker\": \"SPY\", \n\
        \"exchange\": \"EXCH\"\n\
    }\n\
}";
    tassert_eq(sb, expected);

    jr_c jr;
    e$ret(jr$new(&jr, sb, 0, .strict_mode = true));

    ItemNullable s2 = { 0 };
    e$ret(serdegen.ItemNullable.deserialize(&jr, &s2, mem$));

    tassert_eq(s2.char_field, "hello_char");
    tassert_eq(s2.sbuf_field, "hello_sbuf");
    tassert_eq(s2.str_s_field, str$s("hello_str_s"));
    tassert(s2.stock_field == NULL);

    // Make sure new strings are allocated separately
    tassert(s2.char_field != s.char_field);
    tassert(s2.sbuf_field != s.sbuf_field);
    tassert(s2.str_s_field.buf != s.str_s_field.buf);
    tassert_eq(s2.stock_val.exchange, "EXCH");
    tassert_eq(s2.stock_val.ticker, "SPY");
    tassert_eq(s2.stock_val.id, 9988);

    sbuf.destroy(&sb);
    sbuf.destroy(&sb_item);
    serdegen.ItemNullable.destroy(&s2, mem$);

    return EOK;
}

test$case(test_Items_initialized_serialize_null_field)
{
    sbuf_c sb_item = sbuf.create(1024, mem$);
    Stock stk = { .exchange = "FOO", .id = 22, .ticker = "UBER" };


    Item s = {
        .char_field = "hello_char",
        .sbuf_field = "hello_sbuf",
        .str_s_field = str$s("hello_str_s"),
        .stock_field = &stk,
    };
    sbuf.clear(&sb_item);
    tassert_er(
        Error.ok,
        serdegen.Item.print(&s, &(jw_kw){ .buf = &sb_item, .simplified = true, .indent = 4 })
    );
    io.printf("`%s`\n", sb_item);
    print_json_expected(sb_item);
    char* expected = "Item({\n\
    sbuf_field: \"hello_sbuf\", \n\
    str_s_field: \"hello_str_s\", \n\
    char_field: \"hello_char\", \n\
    stock_field: {\n\
        id: 22, \n\
        ticker: \"UBER\", \n\
        exchange: \"FOO\"\n\
    }\n\
})\n\
";
    tassert_eq(expected, sb_item);


    sbuf.clear(&sb_item);
    s = (Item){
        .char_field = NULL,
        .sbuf_field = "hello_sbuf",
        .str_s_field = str$s("hello_str_s"),
        .stock_field = &stk,
    };
    tassert_er(
        JsonError.null_field,
        serdegen.Item.print(&s, &(jw_kw){ .buf = &sb_item, .simplified = true, .indent = 4 })
    );
    io.printf("`%s`\n", sb_item);
    print_json_expected(sb_item);
    expected = "Item({\n\
    sbuf_field: \"hello_sbuf\", \n\
    str_s_field: \"hello_str_s\", \n\
    char_field: null, \n\
    stock_field: {\n\
        id: 22, \n\
        ticker: \"UBER\", \n\
        exchange: \"FOO\"\n\
    }\n\
} [error: NullFieldErrorJSON])\n\
";
    tassert_eq(expected, sb_item);

    sbuf.clear(&sb_item);
    s = (Item){
        .char_field = "hello_char",
        .sbuf_field = NULL,
        .str_s_field = str$s("hello_str_s"),
        .stock_field = &stk,
    };
    tassert_er(
        JsonError.null_field,
        serdegen.Item.print(&s, &(jw_kw){ .buf = &sb_item, .simplified = true, .indent = 4 })
    );
    io.printf("`%s`\n", sb_item);
    print_json_expected(sb_item);
    expected = "Item({\n\
    sbuf_field: null, \n\
    str_s_field: \"hello_str_s\", \n\
    char_field: \"hello_char\", \n\
    stock_field: {\n\
        id: 22, \n\
        ticker: \"UBER\", \n\
        exchange: \"FOO\"\n\
    }\n\
} [error: NullFieldErrorJSON])\n\
";
    tassert_eq(expected, sb_item);


    sbuf.clear(&sb_item);
    s = (Item){
        .char_field = "hello_char",
        .sbuf_field = "hello_sbuf",
        .str_s_field = { 0 },
        .stock_field = &stk,
    };
    tassert_er(
        JsonError.null_field,
        serdegen.Item.print(&s, &(jw_kw){ .buf = &sb_item, .simplified = true, .indent = 4 })
    );
    io.printf("`%s`\n", sb_item);
    print_json_expected(sb_item);
    expected = "Item({\n\
    sbuf_field: \"hello_sbuf\", \n\
    str_s_field: null, \n\
    char_field: \"hello_char\", \n\
    stock_field: {\n\
        id: 22, \n\
        ticker: \"UBER\", \n\
        exchange: \"FOO\"\n\
    }\n\
} [error: NullFieldErrorJSON])\n\
";
    tassert_eq(expected, sb_item);


    sbuf.clear(&sb_item);
    s = (Item){
        .char_field = "hello_char",
        .sbuf_field = "hello_sbuf",
        .str_s_field = str$s("hello_str_s"),
        .stock_field = NULL,
    };
    tassert_er(
        JsonError.null_field,
        serdegen.Item.print(&s, &(jw_kw){ .buf = &sb_item, .simplified = true, .indent = 4 })
    );
    io.printf("`%s`\n", sb_item);
    print_json_expected(sb_item);
    expected = "Item({\n\
    sbuf_field: \"hello_sbuf\", \n\
    str_s_field: \"hello_str_s\", \n\
    char_field: \"hello_char\", \n\
    stock_field: null\n\
} [error: NullFieldErrorJSON])\n\
";
    tassert_eq(expected, sb_item);


    sbuf.destroy(&sb_item);
    return EOK;
}

test$case(test_Items_deserialize_non_nullable)
{
    sbuf_c sb_item = sbuf.create(1024, mem$);
    Stock stk = { .exchange = "FOO", .id = 22, .ticker = "UBER" };


    Item s = {
        .char_field = "hello_char",
        .sbuf_field = "hello_sbuf",
        .str_s_field = str$s("hello_str_s"),
        .stock_field = &stk,
    };
    sbuf.clear(&sb_item);
    tassert_er(
        Error.ok,
        serdegen.Item.print(&s, &(jw_kw){ .buf = &sb_item, .simplified = false, .indent = 4 })
    );
    io.printf("`%s`\n", sb_item);
    print_json_expected(sb_item);
    char* expected = "{\n\
    \"sbuf_field\": \"hello_sbuf\", \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    tassert_eq(expected, sb_item);

    Item s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    e$ret(serdegen.Item.deserialize(&jr, &s2, mem$));
    tassert_eq(s2.char_field, "hello_char");
    tassert_eq(s2.str_s_field, str$s("hello_str_s"));
    tassert_eq(s2.sbuf_field, "hello_sbuf");
    tassert(s2.stock_field != NULL);
    tassert_eq(s2.stock_field->ticker, "UBER");
    tassert_eq(s2.stock_field->exchange, "FOO");
    tassert_eq(s2.stock_field->id, 22);
    serdegen.Item.destroy(&s2, mem$);


    expected = "{\n\
    \"sbuf_field\": null, \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_er(JsonError.null_field, serdegen.Item.deserialize(&jr, &s2, mem$));
    serdegen.Item.destroy(&s2, mem$);


    expected = "{\n\
    \"sbuf_field\": \"hello_str_s\", \n\
    \"str_s_field\":  null, \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_er(JsonError.null_field, serdegen.Item.deserialize(&jr, &s2, mem$));
    serdegen.Item.destroy(&s2, mem$);


    expected = "{\n\
    \"sbuf_field\": \"hello_sbuf\", \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": null, \n\
    \"stock_field\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_er(JsonError.null_field, serdegen.Item.deserialize(&jr, &s2, mem$));
    serdegen.Item.destroy(&s2, mem$);


    expected = "{\n\
    \"sbuf_field\": \"hello_sbuf\", \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": null\n\
}";

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_er(JsonError.null_field, serdegen.Item.deserialize(&jr, &s2, mem$));
    serdegen.Item.destroy(&s2, mem$);


    sbuf.destroy(&sb_item);
    return EOK;
}

test$case(test_Items_deserialize_missing_fields)
{
    char* expected = "{\n\
    \"sbuf_field\": \"hello_sbuf\", \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";

    Item s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    e$ret(serdegen.Item.deserialize(&jr, &s2, mem$));
    serdegen.Item.destroy(&s2, mem$);

    expected = "{\n\
    \"sbuf_field\": \"hello_sbuf\", \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": {\n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_er(JsonError.missing_field, serdegen.Item.deserialize(&jr, &s2, mem$));

expected = "{\n\
    \"sbuf_field\": \"hello_sbuf\", \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
    }\n\
}";
    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_er(JsonError.missing_field, serdegen.Item.deserialize(&jr, &s2, mem$));

    // NOTE: ticker field is optional in null set
    expected = "{\n\
    \"sbuf_field\": \"hello_sbuf\", \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": {\n\
        \"id\": 22, \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    e$ret(serdegen.Item.deserialize(&jr, &s2, mem$));
    tassert_eq(s2.stock_field->ticker, NULL);
    tassert(s2.stock_field_skipped == NULL);

    expected = "{\n\
    \"this_is_unknown_field\": \"hello_sbuf\", \n\
    \"str_s_field\": \"hello_str_s\", \n\
    \"char_field\": \"hello_char\", \n\
    \"stock_field\": {\n\
        \"id\": 22, \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_er(JsonError.unknown_field, serdegen.Item.deserialize(&jr, &s2, mem$));

    serdegen.Item.destroy(&s2, mem$);
    return EOK;
}


test$case(test_Order_type_matching_validation)
{
    sbuf_c sb_item = sbuf.create(1024, mem$);
    Stock stk = { .exchange = "FOO", .id = 22, .ticker = "UBER" };
    Order ord = {.id = 9988, .price = 123.334455, .qty = -10, .exchange = "NICE", .stock = &stk, .is_active = true};

    sbuf.clear(&sb_item);
    tassert_er(
        Error.ok,
        serdegen.Order.print(&ord, &(jw_kw){ .buf = &sb_item, .simplified = false, .indent = 4 })
    );

    io.printf("JSON\n:%s\n", sb_item);
    print_json_expected(sb_item);

char* expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    tassert_eq(sb_item, expected);

    Order s2 = { 0 };
    jr_c jr;

    // This should be valid
    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    e$ret(serdegen.Order.deserialize(&jr, &s2, mem$));
    serdegen.Order.destroy(&s2, mem$);

    io.printf("---------------------------------\n");
expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": 1, \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    e$ret(jr$new(&jr, expected, 0));
    tassert_er(JsonError.wrong_type, serdegen.Order.deserialize(&jr, &s2, mem$));
    io.printf("---------------------------------\n");

    io.printf("---------------------------------\n");
expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"1\", \n\
    \"stock\":  123\n\
}";
    e$ret(jr$new(&jr, expected, 0));
    tassert_er(JsonError.wrong_type, serdegen.Order.deserialize(&jr, &s2, mem$));
    io.printf("---------------------------------\n");

    io.printf("---------------------------------\n");

expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": 1, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    e$ret(jr$new(&jr, expected, 0));
    tassert_er(JsonError.wrong_type, serdegen.Order.deserialize(&jr, &s2, mem$));
    io.printf("---------------------------------\n");
    sbuf.destroy(&sb_item);
    return EOK;
}


test$case(test_Order_deserialize_valid)
{

char* expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    Order s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert(EOK == serdegen.Order.deserialize(&jr, &s2, mem$));
    serdegen.Order.destroy(&s2, mem$);
    return EOK;
}


/* Invalid JSON - missing closing brace */
test$case(test_Order_deserialize_missing_closing_brace)
{
    char* expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n";
    Order s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_ne(EOK, serdegen.Order.deserialize(&jr, &s2, mem$));
    return EOK;
}

/* Invalid JSON - unquoted key */
test$case(test_Order_deserialize_unquoted_key)
{
    char* expected = "{\n\
    id: 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    Order s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = true));
    tassert_ne(EOK, serdegen.Order.deserialize(&jr, &s2, mem$));
    return EOK;
}


/* Invalid JSON - trailing comma */
test$case(test_Order_deserialize_trailing_comma)
{
    char* expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\",\n\
    }\n\
}";
    Order s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = true));
    tassert_ne(EOK, serdegen.Order.deserialize(&jr, &s2, mem$));

    return EOK;
}

/* Invalid JSON - wrong type (string for number) */
test$case(test_Order_deserialize_wrong_type)
{
    char* expected = "{\n\
    \"id\": \"not_a_number\", \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    Order s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_ne(EOK, serdegen.Order.deserialize(&jr, &s2, mem$));
    return EOK;
}

/* Valid JSON with null value */
test$case(test_Order_deserialize_with_null)
{
    char* expected = "{\n\
    \"id\": 9988, \n\
    \"price\": null, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    Order s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_ne(EOK, serdegen.Order.deserialize(&jr, &s2, mem$));
    return EOK;
}


/* Valid JSON with nested array */
test$case(test_Order_deserialize_with_array)
{
    char* expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"tags\": [\"urgent\", \"bulk\"], \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    Order s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_ne(EOK, serdegen.Order.deserialize(&jr, &s2, mem$));
    return EOK;
}

/* FIX: Valid JSON with scientific notation 
test$case(test_Order_deserialize_scientific_notation)
{
    char* expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 1.23334457e2, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    Order s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = true));
    tassert_ne(EOK, serdegen.Order.deserialize(&jr, &s2, mem$));

    return EOK;
}
*/

/* Invalid JSON - duplicate key */
test$case(test_Order_deserialize_duplicate_key)
{
    char* expected = "{\n\
    \"id\": 9988, \n\
    \"id\": 9999, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NICE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UBER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    Order s2 = { 0 };
    jr_c jr;

    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_eq(EOK, serdegen.Order.deserialize(&jr, &s2, mem$));
    tassert_eq(s2.id, 9999);
    serdegen.Order.destroy(&s2, mem$);
    return EOK;
}

/* FIX: Valid JSON with escaped characters 
test$case(test_Order_deserialize_escaped_chars)
{
    char* expected = "{\n\
    \"id\": 9988, \n\
    \"price\": 123.334457, \n\
    \"qty\": -10, \n\
    \"is_active\": true, \n\
    \"exchange\": \"NI\\\"CE\", \n\
    \"stock\": {\n\
        \"id\": 22, \n\
        \"ticker\": \"UB\\nER\", \n\
        \"exchange\": \"FOO\"\n\
    }\n\
}";
    Order s2 = { 0 };
    jr_c jr;

    io.printf("%s\n", expected);
    e$ret(jr$new(&jr, expected, 0, .strict_mode = false));
    tassert_eq(EOK, serdegen.Order.deserialize(&jr, &s2, mem$));
    tassert_eq(s2.exchange, "NI\"CE");
    serdegen.Order.destroy(&s2, mem$);

    return EOK;
}
*/

test$main();

