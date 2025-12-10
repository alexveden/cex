#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "cexstd/json/json.c"
#include <math.h>
#include <stdint.h>

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

void
print_json_expected(sbuf_c s)
{
    uassert(s);

    io.printf("char* expected = \"");
    for$each (c, s, sbuf.len(&s)) {
        switch (c) {
            case '\\':
                io.printf("\\");
                break;
            case '\n':
                io.printf("\\n\\");
                break;
            case '"':
                io.printf("\\");
                break;
        }
        io.printf("%c", c);
    }

    io.printf("\";");
}

typedef struct Foo
{
    struct
    {
        u32 baz;
        u32 fuzz;
    } foo;
    u32 next;
    u32 baz;
} Foo;

typedef struct Stock
{
    char* ticker;
    u32 id;
} Stock;

typedef struct Order
{
    u32 qty;
    f32 price;
    Stock* stock;
} Order;

void
destroy_stock(Stock* stk, IAllocator allc)
{
    if (stk) {
        uassert(allc);
        if (stk->ticker) { mem$free(allc, stk->ticker); }
    }
}

Exception
deserialize_stock(json_rd_c* jr, Stock* stk, IAllocator allc)
{
    uassert(stk);
    uassert(jr);
    uassert(allc);
    u64 fields_set = 0;
    u64 fields_expected = (1 << 2) - 1;

    json$rd_foreach(k, v, jr)
    {
        if (str$eq(k, "ticker")) {
            fields_set |= (1 << 0);
            stk->ticker = str.slice.clone(v, allc);
        } else if (str$eq(k, "id")) {
            fields_set |= (1 << 1);
            json$rd_egoto(jr, str$convert(v, &stk->id), err);
        }
    }
    if (fields_set != fields_expected) { return Error.not_found; }
    return EOK;
err:
    destroy_stock(stk, allc);
    return jr->error;
}

void
destroy_order(Order* item, IAllocator allc)
{
    if (item) {
        if (item->stock) {
            destroy_stock(item->stock, allc);
            mem$free(allc, item->stock);
        }
    }
}

Exception
deserialize_order(json_rd_c* jr, Order* item, IAllocator allc)
{
    uassert(item);
    uassert(jr);
    uassert(allc);

    json$rd_foreach(k, v, jr)
    {
        if (str$eq(k, "stock")) {
            item->stock = mem$new(allc, Stock);
            if (item->stock == NULL) { json$rd_egoto(jr, Error.memory, err); }
            json$rd_egoto(jr, deserialize_stock(jr, item->stock, allc), err);
        } else if (str$eq(k, "qty")) {
            json$rd_egoto(jr, str$convert(v, &item->qty), err);
        } else if (str$eq(k, "price")) {
            json$rd_egoto(jr, str$convert(v, &item->price), err);
        }
    }
    return EOK;
err:
    destroy_order(item, allc);
    return jr->error;
}

Exception
print_stock(json_wr_c* jw, Stock* stk)
{
    json_wr_c _jw;
    if (!jw) {
        e$ret(json$wr_new(&_jw, stdout, .indent = 4));
        jw = &_jw;
    }

    json$wr_scope(jw, JsonType__obj)
    {
        json$wr_key("ticker");
        json$wr_val(stk->ticker);

        json$wr_key("id");
        json$wr_val(stk->id);
    }

    return EOK;
}

Exception
print_order(json_wr_c* jw, Order* ord)
{
    json_wr_c _jw;
    if (!jw) {
        e$ret(json$wr_new(&_jw, stdout, .indent = 4));
        jw = &_jw;
    }
    json$wr_scope(jw, JsonType__obj)
    {
        json$wr_key("price");
        json$wr_val(ord->price);

        json$wr_key("qty");
        json$wr_val(ord->qty);

        json$wr_key("stock");
        e$ret(print_stock(jw, ord->stock));
    }

    return EOK;
}


test$case(json_reader_macro_proto)
{
    Foo data = { 0 };
    str_s content = str$s(
        "{ \"foo\" : {\"baz\": 3, \"fuzz\": 8, \"oops\": 0}, \"next\": [1, 2, 3], \"baz\": 17 }"
    );

    json_rd_c js;
    e$ret(json$rd_new(&js, content.buf, content.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);

    json$rd_foreach(k, v, &js)
    {
        io.printf("key=%S value=%S\n", k, v);
        if (str$eq(k, "foo")) {
            json$rd_foreach(k, v, &js)
            {
                io.printf("\tkey=%S value=%S\n", k, v);
                if (str$eq(k, "fuzz")) {
                    e$ret(str$convert(v, &data.foo.fuzz));
                } else if (str$eq(k, "baz")) {
                    e$goto(str$convert(v, &data.foo.baz), fail);
                }
            }
        } else if (str$eq(k, "next")) {
            u32 sum = 0;
            json$rd_foreach(v, &js)
            {
                u32 _value = 0;
                e$ret(str$convert(v, &_value));
                sum += _value;
            }
            data.next = sum;
        } else if (str$eq(k, "baz")) {
            e$ret(str$convert(v, &data.baz));
        }
    }

fail:
    tassert_er(js.error, EOK);

    tassert_eq(data.next, 1 + 2 + 3);
    tassert_eq(data.baz, 17);
    tassert_eq(data.foo.baz, 3);
    tassert_eq(data.foo.fuzz, 8);


    return EOK;
}

test$case(json_reader_macro_get_scope)
{
    str_s content = str$s(
        "{\"arr\": [1, 2, 3], \"args\" : {\"baz\": 3, \"fuzz\": 8}, \"req_type\": 17 }"
    );

    json_rd_c js;
    e$ret(json$rd_new(&js, content.buf, content.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);

    str_s arr_scope = { 0 };
    str_s obj_scope = { 0 };
    u32 req_type = 0;
    json$rd_foreach(k, v, &js)
    {
        (void)k;
        (void)v;
        if (str$eq(k, "arr")) {
            arr_scope = json$rd_get_scope_str_s(&js, JsonType__arr);
            tassert_eq(arr_scope, str$s("[1, 2, 3]"));
        } else if (str$eq(k, "args")) {
            obj_scope = json$rd_get_scope_str_s(&js, JsonType__obj);
            tassert_eq(obj_scope, str$s("{\"baz\": 3, \"fuzz\": 8}"));
        } else if (str$eq(k, "req_type")) {
            e$ret(str$convert(v, &req_type));
        }
    }
    tassert_er(js.error, EOK);
    tassert_eq(req_type, 17);

    u32 arr_sum = 0;
    e$ret(json$rd_new(&js, arr_scope.buf, arr_scope.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__arr);
    json$rd_foreach(v, &js)
    {
        u32 res = 0;
        e$ret(str$convert(v, &res));
        tassert(res > 0);
        arr_sum += res;
    }
    tassert_er(js.error, EOK);
    tassert_eq(arr_sum, 1 + 2 + 3);

    e$ret(json$rd_new(&js, obj_scope.buf, obj_scope.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);
    bool has_baz = false;
    bool has_fuzz = false;
    json$rd_foreach(k, v, &js)
    {
        (void)v;
        if (str$eq(k, "baz")) {
            has_baz = true;
        } else if (str$eq(k, "fuzz")) {
            has_fuzz = true;
        } else {
            tassert(false);
        }
    }

    tassert_er(js.error, EOK);
    tassert(has_baz);
    tassert(has_fuzz);

    return EOK;
}

test$case(json_reader_array_of_objects)
{
    struct Item
    {
        i32 qty;
        f32 price;
    };

    arr$(struct Item) items = arr$new(items, mem$);

    str_s content = str$s("{ items : [{qty: 1, price: 123}, {qty: -100, price: 999}]  }");

    json_rd_c js;
    e$ret(json$rd_new(&js, content.buf, content.len, .strict_mode = false));
    json$rd_foreach(k, v, &js)
    {
        (void)v;
        if (str$eq(k, "items")) {
            json$rd_foreach(it, &js)
            {
                (void)it;
                struct Item i = { 0 };
                json$rd_foreach(k, v, &js)
                {
                    io.printf("k=%S, v=%S\n", k, v);
                    if (str$eq(k, "qty")) {
                        e$goto(str$convert(v, &i.qty), end);
                    } else if (str$eq(k, "price")) {
                        e$goto(str$convert(v, &i.price), end);
                    }
                }

                arr$push(items, i);
            }
        }
    }

end:
    tassert_er(js.error, EOK);

    tassert_eq(arr$len(items), 2);
    tassert_eq(items[0].price, 123);
    tassert_eq(items[0].qty, 1);
    tassert_eq(items[1].price, 999);
    tassert_eq(items[1].qty, -100);

    arr$free(items);

    return EOK;
}

test$case(json_reader_strict_mode_keys)
{

    str_s content = str$s("{ \"items\" : 1, \"foo\": 2  }");

    json_rd_c js;
    e$ret(json$rd_new(&js, content.buf, content.len, .strict_mode = true));

    bool has_items = false;
    bool has_foo = false;
    json$rd_foreach(k, v, &js)
    {
        (void)v;
        if (str$eq(k, "items")) {
            has_items = true;
        } else if (str$eq(k, "foo")) {
            has_foo = true;
        }
    }
    tassert_er(js.error, EOK);
    tassert_eq(has_items, true);
    tassert_eq(has_foo, true);

    return EOK;
}

test$case(json_reader_strict_mode_keys_bad_start)
{

    str_s content = str$s("{ items : 1, \"foo\": 2  }");

    json_rd_c js;
    e$ret(json$rd_new(&js, content.buf, content.len, .strict_mode = true));

    bool has_items = false;
    bool has_foo = false;
    json$rd_foreach(k, v, &js)
    {
        (void)v;
        if (str$eq(k, "items")) {
            has_items = true;
        } else if (str$eq(k, "foo")) {
            has_foo = true;
        }
    }
    tassert_er(js.error, "Keys without double quotes (strict mode)");
    tassert_eq(has_items, false);
    tassert_eq(has_foo, false);

    return EOK;
}

test$case(json_reader_strict_mode_keys_bad_following)
{

    str_s content = str$s("{ \"items\" : 1, foo: 2  }");

    json_rd_c js;
    e$ret(json$rd_new(&js, content.buf, content.len, .strict_mode = true));

    bool has_items = false;
    bool has_foo = false;
    json$rd_foreach(k, v, &js)
    {
        (void)v;
        if (str$eq(k, "items")) {
            has_items = true;
        } else if (str$eq(k, "foo")) {
            has_foo = true;
        }
    }
    tassert_er(js.error, "Keys without double quotes (strict mode)");
    tassert_eq(has_items, true);
    tassert_eq(has_foo, false);

    return EOK;
}

test$case(json_reader_json5_single_quote_keys)
{

    str_s content = str$s("{ 'items' : 1, 'foo': 2  }");

    json_rd_c js;
    e$ret(json$rd_new(&js, content.buf, content.len, .strict_mode = false));

    bool has_items = false;
    bool has_foo = false;
    json$rd_foreach(k, v, &js)
    {
        (void)v;
        if (str$eq(k, "items")) {
            has_items = true;
        } else if (str$eq(k, "foo")) {
            has_foo = true;
        }
    }
    tassert_er(js.error, EOK);
    tassert_eq(has_items, true);
    tassert_eq(has_foo, true);

    return EOK;
}


test$case(json_writer_macro_proto_indent4)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4));
        // tassert_er(EOK, json$wr_new(&jb, stdout, .indent = 0));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("foo2");
            json$wr_val("1");

            json$wr_key("foo3");
            json$wr_fmt("%d", 4);

            json$wr_key("bar");
            json$wr_scope(&jb, JsonType__arr)
            {
                json$wr_val("foo");
                json$wr_fmt("%d", 39);

                json$wr_scope(&jb, JsonType__arr)
                {
                    for (u32 i = 0; i < 10; i++) { json$wr_val(i); }
                }
                json$wr_scope(&jb, JsonType__obj) {}
                json$wr_scope(&jb, JsonType__arr) {}
            }
            json$wr_key("far");
            json$wr_scope(&jb, JsonType__obj)
            {
                json$wr_key("zoo");
                json$wr_val(1);
            }
            json$wr_key("arr_empty");
            json$wr_scope(&jb, JsonType__arr) {}

            json$wr_key("obj_empty");
            json$wr_scope(&jb, JsonType__obj) {}
        }

        tassert_er(EOK, jb.error);
        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    \"foo2\": \"1\", \n\
    \"foo3\": 4, \n\
    \"bar\": [\n\
        \"foo\", \n\
        39, \n\
        [\n\
            0, \n\
            1, \n\
            2, \n\
            3, \n\
            4, \n\
            5, \n\
            6, \n\
            7, \n\
            8, \n\
            9\n\
        ], \n\
        {}, \n\
        []\n\
    ], \n\
    \"far\": {\n\
        \"zoo\": 1\n\
    }, \n\
    \"arr_empty\": [], \n\
    \"obj_empty\": {}\n\
}";
        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_macro_proto_no_indent)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 0));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("foo2");
            json$wr_val("1");

            json$wr_key("foo3");
            json$wr_fmt("%d", 4);

            json$wr_key("bar");
            json$wr_scope(&jb, JsonType__arr)
            {
                json$wr_val("foo");
                json$wr_fmt("%d", 39);

                json$wr_scope(&jb, JsonType__arr)
                {
                    for (u32 i = 0; i < 10; i++) { json$wr_val(i); }
                }
                json$wr_scope(&jb, JsonType__obj) {}
                json$wr_scope(&jb, JsonType__arr) {}
            }
            json$wr_key("far");
            json$wr_scope(&jb, JsonType__obj)
            {
                json$wr_key("zoo");
                json$wr_val(1);
            }
            json$wr_key("arr_empty");
            json$wr_scope(&jb, JsonType__arr) {}

            json$wr_key("obj_empty");
            json$wr_scope(&jb, JsonType__obj) {}
        }

        tassert_er(EOK, jb.error);
        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected =
            "{\"foo2\": \"1\", \"foo3\": 4, \"bar\": [\"foo\", 39, [0, 1, 2, 3, 4, 5, 6, 7, 8, 9], {}, []], \"far\": {\"zoo\": 1}, \"arr_empty\": [], \"obj_empty\": {}}";

        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_macro_only_fmt)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_fmt("\"cool\": %d", 4);

            json$wr_key("arr");
            json$wr_scope(&jb, JsonType__arr)
            {
                for (u32 i = 0; i < 10; i++) { json$wr_val(i); }
            }
        }

        tassert_er(EOK, jb.error);
        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    \"cool\": 4, \n\
    \"arr\": [\n\
        0, \n\
        1, \n\
        2, \n\
        3, \n\
        4, \n\
        5, \n\
        6, \n\
        7, \n\
        8, \n\
        9\n\
    ]\n\
}";
        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_multi_func_serde_concept)
{
    Stock stk = {
        .id = 8899,
        .ticker = "UBER",
    };

    Order ord = {
        .price = 100.33,
        .qty = 33,
        .stock = &stk,
    };

    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        e$ret(json$wr_new(&jb, .buf = &buf, .indent = 4));

        e$ret(print_order(&jb, &ord));

        tassert_er(EOK, jb.error);
        e$ret(json$wr_validate(&jb));

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    \"price\": 100.330002, \n\
    \"qty\": 33, \n\
    \"stock\": {\n\
        \"ticker\": \"UBER\", \n\
        \"id\": 8899\n\
    }\n\
}";
        tassert_eq(buf, expected);

        json_rd_c jr;
        e$ret(json$rd_new(&jr, expected, 0, .strict_mode = true));

        Order ord2 = { 0 };
        e$ret(deserialize_order(&jr, &ord2, _));

        tassert_eq((int)ord2.price, 100);
        tassert_eq(ord2.qty, 33);
        tassert(ord2.stock != NULL);
        tassert_eq(ord2.stock->ticker, "UBER");
        tassert_eq(ord2.stock->id, 8899);
    }

    return EOK;
}

test$case(json_writer_multi_func_deser_order_err)
{
    mem$scope(tmem$, _)
    {

        char* expected = "{\n\
    \"price\": 100.330002, \n\
    \"qty\": 33, \n\
    \"stock\": {\n\
        \"ticker\": \"UBER\", \n\
        \"id\": null\n\
    }\n\
}";
        json_rd_c jr;
        e$ret(json$rd_new(&jr, expected, 0, .strict_mode = true));

        Order ord2 = { 0 };
        if (deserialize_order(&jr, &ord2, _)) {
            io.printf(json$rd_err_fmt(&jr));
            tassert_eq(jr.error, Error.argument);
            tassert_eq(jr._impl.lexer.line + 1, 6);
            return EOK;
        }
    }

    return Error.assert;
}

test$case(json_writer_val_types)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4));

        json$wr_scope(&jb, JsonType__arr)
        {
            u8 v1 = UINT8_MAX;
            json$wr_val(v1);
            i8 v2 = INT8_MIN;
            json$wr_val(v2);
            i16 v3 = INT16_MIN;
            json$wr_val(v3);
            u16 v4 = UINT16_MAX;
            json$wr_val(v4);
            i32 v5 = INT32_MIN;
            json$wr_val(v5);
            u32 v6 = UINT32_MAX;
            json$wr_val(v6);
            i64 v7 = INT64_MIN;
            json$wr_val(v7);
            u64 v8 = UINT64_MAX;
            json$wr_val(v8);
            f32 v10 = HUGE_VAL;
            json$wr_val(v10);
            f32 v11 = -HUGE_VAL;
            json$wr_val(v11);
            f32 v12 = NAN;
            json$wr_val(v12);
            f64 v13 = HUGE_VAL;
            json$wr_val(v13);
            f64 v14 = -HUGE_VAL;
            json$wr_val(v14);
            f64 v15 = NAN;
            json$wr_val(v15);
            bool v16 = true;
            json$wr_val(v16);
            bool v17 = false;
            json$wr_val(v17);

            const char* s1 = "const";
            json$wr_val(s1);
            char* s2 = "str";
            json$wr_val(s2);
            str_s s3 = str$s("str_s");
            json$wr_val(s3);
            char* s4 = NULL;
            json$wr_val(s4);
            str_s s5 = { 0 };
            json$wr_val(s5);

            // usize v17 = SIZE_MAX;
            // json$wr_val(v17);
            // isize v18 = PTRDIFF_MIN;
            // json$wr_val(v18);
        }

        tassert_er(EOK, jb.error);
        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "[\n\
    255, \n\
    -128, \n\
    -32768, \n\
    65535, \n\
    -2147483648, \n\
    4294967295, \n\
    -9223372036854775808, \n\
    18446744073709551615, \n\
    inf, \n\
    -inf, \n\
    nan, \n\
    inf, \n\
    -inf, \n\
    nan, \n\
    true, \n\
    false, \n\
    \"const\", \n\
    \"str\", \n\
    \"str_s\", \n\
    null, \n\
    null\n\
]";
        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_reader_error_handling)
{
    str_s content = str$s("{\n \"foo\": \n}");

    json_rd_c js;
    e$ret(json$rd_new(&js, content.buf, content.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);

    u32 val = 0;
    json$rd_foreach(k, v, &js)
    {
        if (str$eq(k, "foo")) { e$ret(str$convert(v, &val)); }
    }
    tassert_er(json$rd_err(&js), "Unexpected token");
    tassert_eq(val, 0);

    // NOTE: json$rd_err_fmt can work with any printf function
    io.printf(json$rd_err_fmt(&js));
    fprintf(stdout, json$rd_err_fmt(&js));
    char* s = str.fmt(mem$, json$rd_err_fmt(&js));
    io.printf(s);
    mem$free(mem$, s);

    return EOK;
}

test$case(json_writer_null_scope)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4));
        // tassert_er(EOK, json$wr_new(&jb, stdout, .indent = 0));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("bar");
            json$wr_val(NULL);

            json$wr_key("far");
            json$wr_scope(&jb, JsonType__obj)
            {
                json$wr_key("zoo");
                json$wr_val(NULL);

                json$wr_key("zoo");
                json$wr_val(NULL);
            }
            json$wr_key("arr_empty");
            json$wr_scope(&jb, JsonType__arr)
            {
                json$wr_val(NULL);
                json$wr_val(NULL);
            }
        }

        tassert_er(EOK, jb.error);
        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    \"bar\": null, \n\
    \"far\": {\n\
        \"zoo\": null, \n\
        \"zoo\": null\n\
    }, \n\
    \"arr_empty\": [\n\
        null, \n\
        null\n\
    ]\n\
}";
        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_null_object)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4));
        // tassert_er(EOK, json$wr_new(&jb, stdout, .indent = 0));

        json$wr_scope(&jb, JsonType__null)
        {
            json$wr_val(NULL);
        }

        tassert_er(EOK, jb.error);
        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "null";
        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_reader_null_field)
{
    str_s content = str$s(
        "{ \"foo\" : null, \"bar\": "
        ", \"baz\": \"null\" }"
    );

    json_rd_c js;
    e$ret(json$rd_new(&js, content.buf, content.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);

    json$rd_foreach(k, v, &js)
    {
        io.printf("key=%S value=%S\n", k, v);
        if (str$eq(k, "foo")) {
            tassert_eq(js.type, JsonType__null);
            tassert_eq(v.buf, NULL);
            tassert_eq(v.len, 1);
        } else if (str$eq(k, "baz")) {
            tassert_eq(v, str$s("null"));
        } else if (str$eq(k, "bar")) {
            tassert_eq(v, str$s(""));
            tassert_eq(v.len, 0);
        }
    }

    return EOK;
}

test$case(json_writer_null_object_value)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4));
        // tassert_er(EOK, json$wr_new(&jb, stdout, .indent = 0));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("foo");
            json$wr_scope(&jb, JsonType__null)
            {
                json$wr_val(NULL);
            }

            json$wr_key("bar");
            json$wr_scope(&jb, JsonType__null)
            {
                json$wr_val(NULL);
            }
        }

        tassert_er(EOK, jb.error);
        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    \"foo\": null, \n\
    \"bar\": null\n\
}";

        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_simplified)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("foo");
            json$wr_scope(&jb, JsonType__null)
            {
                json$wr_val(NULL);
            }

            json$wr_key("bar");
            json$wr_scope(&jb, JsonType__null)
            {
                json$wr_val(NULL);
            }
        }

        tassert_er(EOK, jb.error);
        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    foo: null, \n\
    bar: null\n\
}";

        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_multi_func_serde__missing_fields)
{
    mem$scope(tmem$, _)
    {
        char* expected = "{\n\
    \"price\": 100.330002, \n\
    \"qty\": 33, \n\
    \"stock\": {\n\
        \"ticker\": \"UBER\", \n\
    }\n\
}";

        json_rd_c jr;
        e$ret(json$rd_new(&jr, expected, 0, .strict_mode = true));

        Order ord2 = { 0 };
        tassert_eq(Error.not_found, deserialize_order(&jr, &ord2, _));
    }

    return EOK;
}

test$case(json_reader_is_type_compatible)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4));
        json_rd_c jr;

        jr = (json_rd_c){ .type = JsonType__num };
        u8 v1 = UINT8_MAX;
        tassert(json$rd_is_type_compatible(&jr, &v1));
        i8 v2 = INT8_MIN;
        tassert(json$rd_is_type_compatible(&jr, &v2));
        i16 v3 = INT16_MIN;
        tassert(json$rd_is_type_compatible(&jr, &v3));
        u16 v4 = UINT16_MAX;
        tassert(json$rd_is_type_compatible(&jr, &v4));
        i32 v5 = INT32_MIN;
        tassert(json$rd_is_type_compatible(&jr, &v5));
        u32 v6 = UINT32_MAX;
        tassert(json$rd_is_type_compatible(&jr, &v6));
        i64 v7 = INT64_MIN;
        tassert(json$rd_is_type_compatible(&jr, &v7));
        u64 v8 = UINT64_MAX;
        tassert(json$rd_is_type_compatible(&jr, &v8));
        f32 v10 = HUGE_VAL;
        tassert(json$rd_is_type_compatible(&jr, &v10));
        f64 v13 = HUGE_VAL;
        tassert(json$rd_is_type_compatible(&jr, &v13));

        bool v16 = true;
        tassert(!json$rd_is_type_compatible(&jr, &v16));

        jr = (json_rd_c){ .type = JsonType__null };
        tassert(!json$rd_is_type_compatible(&jr, &v16));
        jr = (json_rd_c){ .type = JsonType__bool };
        tassert(json$rd_is_type_compatible(&jr, &v16));

        jr = (json_rd_c){ .type = JsonType__str };
        const char* s1 = "const";
        tassert(json$rd_is_type_compatible(&jr, &s1));
        char* s2 = "str";
        tassert(json$rd_is_type_compatible(&jr, &s2));
        str_s s3 = str$s("str_s");
        tassert(json$rd_is_type_compatible(&jr, &s3));
        char* s4 = NULL;
        tassert(json$rd_is_type_compatible(&jr, &s4));
        str_s s5 = { 0 };
        tassert(json$rd_is_type_compatible(&jr, &s5));

        jr = (json_rd_c){ .type = JsonType__null };
        tassert(!json$rd_is_type_compatible(&jr, &s5));

        jr = (json_rd_c){ .type = JsonType__null };
        tassert(json$rd_is_type_compatible(&jr, NULL));
    }
    return EOK;
}

test$case(json_writer_unicode_proto)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("1");
            json$wr_val("\"");
            json$wr_key("2");
            json$wr_val("\\");
            json$wr_key("3");
            json$wr_val("\f");
            json$wr_key("4");
            json$wr_val("\n");
            json$wr_key("5");
            json$wr_val("\r");
            json$wr_key("6");
            json$wr_val("\t");
            json$wr_key("7");
            json$wr_val("/");
        }

        tassert_er(EOK, jb.error);

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    1: \"\\\"\", \n\
    2: \"\\\\\", \n\
    3: \"\\f\", \n\
    4: \"\\n\", \n\
    5: \"\\r\", \n\
    6: \"\\t\", \n\
    7: \"\\/\"\n\
}";

        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_unicode_ascii_control)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("1");
            json$wr_val("\x1B");
        }
        tassert_er(EOK, jb.error);

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    1: \"\\u001B\"\n\
}";

        // TODO: this may be forbidden by default without setting flags
        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_unicode_comprehensive)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(4096, _);
        (void)buf;

        // Test with various Unicode scenarios
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        json$wr_scope(&jb, JsonType__obj)
        {
            // 2. ASCII printable characters
            json$wr_key("ascii_printable");
            json$wr_val("Hello World!@#$%^&*()");

            // 3. Latin-1 Supplement (U+0080 to U+00FF)
            json$wr_key("latin1_supplement");
            json$wr_val("©®±µ¼½¾¿ÀÁÂÃÄÅÆÇ");

            // 4. Common symbols and punctuation
            json$wr_key("symbols");
            json$wr_val("€£¥¢§¶†‡•…—–");

            // 5. Common scripts
            json$wr_key("latin_extended");
            json$wr_val("ŠšŽžÀàÁáÂâÃãÄä");

            json$wr_key("greek");
            json$wr_val("ΑαΒβΓγΔδΕεΖζΗηΘθ");

            json$wr_key("cyrillic");
            json$wr_val("АаБбВвГгДдЕеЁёЖж");

            json$wr_key("arabic");
            json$wr_val("اب ت ث ج ح خ د ذ ر ز");

            // 6. Asian scripts
            json$wr_key("chinese");
            json$wr_val("你好世界"); // Hello World

            json$wr_key("japanese");
            json$wr_val("こんにちは世界"); // Hello World

            json$wr_key("korean");
            json$wr_val("안녕하세요 세계"); // Hello World

            // 7. Mathematical symbols
            json$wr_key("math_symbols");
            json$wr_val("∑∏√∞∫≈≠≤≥∈∉∧∨¬⇒⇔");
        }

        tassert_er(EOK, jb.error);

        // Print the generated JSON
        io.printf("\nGenerated JSON:\n%s\n", buf);

        // Verify expected output
        char* expected = "{\n\
    ascii_printable: \"Hello World!@#$%^&*()\", \n\
    latin1_supplement: \"\\u00A9\\u00AE\\u00B1\\u00B5\\u00BC\\u00BD\\u00BE\\u00BF\\u00C0\\u00C1\\u00C2\\u00C3\\u00C4\\u00C5\\u00C6\\u00C7\", \n\
    symbols: \"\\u20AC\\u00A3\\u00A5\\u00A2\\u00A7\\u00B6\\u2020\\u2021\\u2022\\u2026\\u2014\\u2013\", \n\
    latin_extended: \"\\u0160\\u0161\\u017D\\u017E\\u00C0\\u00E0\\u00C1\\u00E1\\u00C2\\u00E2\\u00C3\\u00E3\\u00C4\\u00E4\", \n\
    greek: \"\\u0391\\u03B1\\u0392\\u03B2\\u0393\\u03B3\\u0394\\u03B4\\u0395\\u03B5\\u0396\\u03B6\\u0397\\u03B7\\u0398\\u03B8\", \n\
    cyrillic: \"\\u0410\\u0430\\u0411\\u0431\\u0412\\u0432\\u0413\\u0433\\u0414\\u0434\\u0415\\u0435\\u0401\\u0451\\u0416\\u0436\", \n\
    arabic: \"\\u0627\\u0628 \\u062A \\u062B \\u062C \\u062D \\u062E \\u062F \\u0630 \\u0631 \\u0632\", \n\
    chinese: \"\\u4F60\\u597D\\u4E16\\u754C\", \n\
    japanese: \"\\u3053\\u3093\\u306B\\u3061\\u306F\\u4E16\\u754C\", \n\
    korean: \"\\uC548\\uB155\\uD558\\uC138\\uC694 \\uC138\\uACC4\", \n\
    math_symbols: \"\\u2211\\u220F\\u221A\\u221E\\u222B\\u2248\\u2260\\u2264\\u2265\\u2208\\u2209\\u2227\\u2228\\u00AC\\u21D2\\u21D4\"\n\
}";

        print_json_expected(buf);
        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_unicode_surrogate_pair)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("1");
            json$wr_val("😀"); // Grinning face (U+1F600)
        }
        tassert_er(EOK, jb.error);

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    1: \"\\uD83D\\uDE00\"\n\
}";

        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_wide_ascii)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        sbuf_c val = sbuf.create(1024, _);
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        for (u32 i = 0; i < 100; i++) { e$ret(sbuf.append(&val, "1234567890")); }
        tassert_eq(sbuf.len(&val), 1000);

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("1");
            json$wr_val(val);
        }
        tassert_er(EOK, jb.error);

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = str.fmt(
            _,
            "{\n\
    1: \"%s\"\n\
}",
            val
        );

        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_wide_ascii_unicode)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        sbuf_c val = sbuf.create(1024, _);
        sbuf_c escaped_val = sbuf.create(8196, _);
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        tassert_eq(str.len("😀"), 4); // Grinning face (U+1F600)
        for (u32 i = 0; i < 100; i++) {
            e$ret(sbuf.append(&val, "😀"));
            e$ret(sbuf.append(&escaped_val, "\\uD83D\\uDE00"));
        }
        tassert_eq(sbuf.len(&val), 400);

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("1");
            json$wr_val(val);
        }
        tassert_er(EOK, jb.error);

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = str.fmt(
            _,
            "{\n\
    1: \"%s\"\n\
}",
            escaped_val
        );

        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_wide_ascii_unicode_print)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c val = sbuf.create(1024, _);
        tassert_er(EOK, json$wr_new(&jb, .stream = stdout, .indent = 4, .simplified = true));

        tassert_eq(str.len("😀"), 4); // Grinning face (U+1F600)
        for (u32 i = 0; i < 100; i++) { e$ret(sbuf.append(&val, "😀")); }
        tassert_eq(sbuf.len(&val), 400);

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("1");
            json$wr_val(val);
        }
        tassert_er(EOK, jb.error);
    }
    return EOK;
}

test$case(json_writer_wide_ascii_unicode_print_short)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c val = sbuf.create(1024, _);
        tassert_er(EOK, json$wr_new(&jb, .stream = stdout, .indent = 4, .simplified = true));

        tassert_eq(str.len("😀"), 4); // Grinning face (U+1F600)
        e$ret(sbuf.append(&val, "😀"));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("1");
            json$wr_val(val);
        }
        tassert_er(EOK, jb.error);
    }

    return EOK;
}

test$case(json_writer_unicode_bmp_only)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);

        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 0, .simplified = true));

        // Test all possible Unicode code points in Basic Multilingual Plane
        json$wr_scope(&jb, JsonType__arr)
        {
            // Test boundaries
            json$wr_val("\u00FF"); // Last Latin-1
            json$wr_val("\u0100"); // Latin Extended-A start
            json$wr_val("\u07FF"); // End of some blocks
            json$wr_val("\u0800"); // Start of other blocks
            json$wr_val("\uFFFF"); // Last BMP character (non-character)
        }

        tassert_er(EOK, jb.error);

        // Expected output would be an array with escaped Unicode
        // The exact output depends on your JSON writer implementation

        io.printf("\nBMP boundary test:\n%s\n", buf);
        print_json_expected(buf);
        char* expected = "[\"\\u00FF\", \"\\u0100\", \"\\u07FF\", \"\\u0800\", \"\\uFFFF\"]";
        tassert_eq(expected, buf);
    }
    return EOK;
}

test$case(json_writer_unicode_surrogate_pairs)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(2048, _);

        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 2, .simplified = false));

        json$wr_scope(&jb, JsonType__obj)
        {
            // Supplementary Multilingual Plane (SMP) characters
            // These require UTF-16 surrogate pairs in JSON
            json$wr_key("supplementary_plane");
            json$wr_scope(&jb, JsonType__arr)
            {
                // CJK Unified Ideographs Extension B
                json$wr_val("\U00020000"); // U+20000 (requires two UTF-16 surrogates)

                // Last valid Unicode code point (as of Unicode 13.0)
                json$wr_val("\U0010FFFF"); // U+10FFFF
            }
            // // Test invalid surrogate handling
            // json$wr_key("invalid_surrogates");
            // json$wr_scope(&jb, JsonType__arr)
            // {
            // }
        }

        tassert_er(EOK, jb.error);

        io.printf("\nSupplementary Plane test:\n%s\n", buf);
        print_json_expected(buf);
        char* expected = "{\n\
  \"supplementary_plane\": [\n\
    \"\\uD840\\uDC00\", \n\
    \"\\uDBFF\\uDFFF\"\n\
  ]\n\
}";
        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_unicode_surrogate_invalid_high)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(2048, _);

        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 2, .simplified = false));

        json$wr_scope(&jb, JsonType__obj)
        {
            // Supplementary Multilingual Plane (SMP) characters
            // These require UTF-16 surrogate pairs in JSON
            json$wr_key("supplementary_plane");
            json$wr_scope(&jb, JsonType__arr)
            {
                // Lone high surrogate (invalid)
                json$wr_val("\xD8\x00");
            }
        }

        tassert_er(JsonError.encoding, jb.error);
    }
    return EOK;
}

test$case(json_writer_unicode_surrogate_invalid_low)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(2048, _);

        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 2, .simplified = false));

        json$wr_scope(&jb, JsonType__obj)
        {
            // Supplementary Multilingual Plane (SMP) characters
            // These require UTF-16 surrogate pairs in JSON
            json$wr_key("supplementary_plane");
            json$wr_scope(&jb, JsonType__arr)
            {
                // Lone low surrogate (invalid)
                json$wr_val("\xDC\x00");
            }
        }

        tassert_er(JsonError.encoding, jb.error);
    }
    return EOK;
}

test$case(json_writer_unicode_surrogate_invalid_reversed)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(2048, _);

        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 2, .simplified = false));

        json$wr_scope(&jb, JsonType__obj)
        {
            // Supplementary Multilingual Plane (SMP) characters
            // These require UTF-16 surrogate pairs in JSON
            json$wr_key("supplementary_plane");
            json$wr_scope(&jb, JsonType__arr)
            {
                // Reversed surrogate pair (invalid)
                json$wr_val("\xDC\x00\xD8\x00");
            }
        }

        tassert_er(JsonError.encoding, jb.error);
    }
    return EOK;
}

test$case(json_writer_unicode_surrogate_valid_maxval)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(2048, _);

        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 2, .simplified = false));

        json$wr_scope(&jb, JsonType__obj)
        {
            // Supplementary Multilingual Plane (SMP) characters
            // These require UTF-16 surrogate pairs in JSON
            json$wr_key("supplementary_plane");
            json$wr_scope(&jb, JsonType__arr)
            {
                // Valid surrogate pair for non-character
                json$wr_val("\uFFFF\uFFFF");
            }
        }

        tassert_er(EOK, jb.error);
        io.printf("\nSupplementary Plane test:\n%s\n", buf);
        print_json_expected(buf);

char* expected = "{\n\
  \"supplementary_plane\": [\n\
    \"\\uFFFF\\uFFFF\"\n\
  ]\n\
}";
        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_unicode_key_escaping)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = false));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("😀");
            json$wr_val("😀"); // Grinning face (U+1F600)
        }
        tassert_er(EOK, jb.error);

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    \"\\uD83D\\uDE00\": \"\\uD83D\\uDE00\"\n\
}";

        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_unicode_key_escaping_simplified_keys_error)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("😀");
            json$wr_val("😀"); // Grinning face (U+1F600)
        }
        // NOTE: in simplified=true, escaped chars in keys are not allowed
        tassert_er(JsonError.encoding, jb.error);

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

        char* expected = "{\n\
    \\uD83D\\uDE00: \"\\uD83D\\uDE00\"\n\
}";

        tassert_eq(buf, expected);
    }
    return EOK;
}

test$case(json_writer_unicode_unescape_simple)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        tassert(str.eq("прив", "прив"));

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("foo");
            json$wr_val("прив"); // Grinning face (U+1F600)
        }
        tassert_er(EOK, jb.error);

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

char* expected = "{\n\
    foo: \"\\u043F\\u0440\\u0438\\u0432\"\n\
}";

        tassert_eq(buf, expected);

        json_rd_c jr;
        e$ret(json$rd_new(&jr, expected, 0));

        json$rd_foreach(k, v, &jr) {
            (void)v;
            if(str$eq(k, "foo")) {
                str_s unesc;
                e$ret(json$rd_str_unescape(v, &unesc, _)); 
                io.printf("\nunesc: `%S` len: %d\n", unesc, unesc.len);
                tassert_eq("прив", unesc.buf);
            } else {
                // unreachable();
            }
        }
        tassert_eq(jr.error, EOK);

    }
    return EOK;
}

test$case(json_writer_unicode_unescape_surrogate)
{
    mem$scope(tmem$, _)
    {
        json_wr_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, json$wr_new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        tassert(str.eq("😀", "😀"));
        tassert_eq(str.len("😀"), 4);

        json$wr_scope(&jb, JsonType__obj)
        {
            json$wr_key("foo");
            json$wr_val("😀"); // Grinning face (U+1F600)
        }
        tassert_er(EOK, jb.error);

        io.printf("\nJSON (buf): \n%s\n", buf);
        print_json_expected(buf);

char* expected = "{\n\
    foo: \"\\uD83D\\uDE00\"\n\
}";

        tassert_eq(buf, expected);

        json_rd_c jr;
        e$ret(json$rd_new(&jr, expected, 0));

        json$rd_foreach(k, v, &jr) {
            (void)v;
            if(str$eq(k, "foo")) {
                str_s unesc;
                e$ret(json$rd_str_unescape(v, &unesc, _)); 
                io.printf("\nunesc: `%S` len: %d\n", unesc, unesc.len);
                tassert_eq(unesc.len, 4);
                tassert_eq("😀", unesc.buf);
            } else {
                unreachable();
            }
        }
        tassert_eq(jr.error, EOK);

    }
    return EOK;
}

test$case(json_writer_unicode_unescape_2byte)
{
    mem$scope(tmem$, _)
    {
        str_s unesc;
        e$ret(json$rd_str_unescape(str$s("\u00a9"), &unesc, _)); 
        tassert_eq(str$s("©").len, 2);

        io.printf("\nunesc: `%S` len: %d\n", unesc, unesc.len);
        tassert_eq(unesc.len, 2);
        tassert_eq(unesc, str$s("©"));
    }

    return EOK;
}

test$case(json_writer_unicode_unescape_simple_ascii)
{
    mem$scope(tmem$, _)
    {
        str_s unesc;
        e$ret(json$rd_str_unescape(str$s("foo"), &unesc, _)); 
        io.printf("\nunesc: `%S` len: %d\n", unesc, unesc.len);
        tassert_eq(unesc.len, 3);
        tassert_eq(unesc, str$s("foo"));
    }

    return EOK;
}
test$case(json_writer_unicode_unescape_3byte)
{
    mem$scope(tmem$, _)
    {
        str_s unesc;
        e$ret(json$rd_str_unescape(str$s("\u20aC"), &unesc, _)); 
        tassert_eq(str$s("€").len, 3);

        io.printf("\nunesc: `%S` len: %d\n", unesc, unesc.len);
        tassert_eq(unesc.len, 3);
        tassert_eq(unesc, str$s("€"));
    }

    return EOK;
}

test$case(json_writer_unicode_unescape_bad_hex)
{
    str_s unesc;

    tassert_er(json$rd_str_unescape(str$s("\\u00AH"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\u00AZ"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\u00A"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\uD83D\\uDE0"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\uD83D\\uDE0H"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\uD83D\\uDE0!"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\uD83D\\uDE0Z"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\uD83D\\DE00"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\uD83DuDE00"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\uD83D\\uDBFF"),&unesc, mem$), JsonError.encoding); 
    tassert_er(json$rd_str_unescape(str$s("\\uD83D\\uE000"),&unesc, mem$), JsonError.encoding); 

    return EOK;
}

test$case(json_writer_unicode_unescape_valid_hex_short)
{
    mem$scope(tmem$, _)
    {
        str_s unesc;
        tassert_er(json$rd_str_unescape(str$s("\\u00AA"),&unesc, _), EOK);
        tassert_er(json$rd_str_unescape(str$s("\\uD800\\uDFFF"),&unesc, _),EOK);
    }

    return EOK;
}

test$case(json_writer_unicode_unescape_self_ref)
{
    mem$scope(tmem$, _)
    {
        tassert_eq(str$s("€").len, 3);

        char buf[] = {"\\u20aC\0"};
        str_s slice = str.sstr(buf);
        tassert_eq(slice.len, 6);

        usize cnt = slice.len + 1;
        e$ret(json$rd_str_unescape_inplace(slice, buf, &cnt));

        tassert_eq(cnt, 3);
        tassert_eq(buf, "€");
    }

    return EOK;
}
test$main();
