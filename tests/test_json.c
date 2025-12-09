#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/json/json.c"
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
deserialize_stock(jr_c* jr, Stock* stk, IAllocator allc)
{
    uassert(stk);
    uassert(jr);
    uassert(allc);
    u64 fields_set = 0;
    u64 fields_expected = (1 << 2) - 1;

    jr$foreach(k, v, jr)
    {
        if (str$eq(k, "ticker")) {
            fields_set |= (1 << 0);
            stk->ticker = str.slice.clone(v, allc);
        } else if (str$eq(k, "id")) {
            fields_set |= (1 << 1);
            jr$egoto(jr, str$convert(v, &stk->id), err);
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
deserialize_order(jr_c* jr, Order* item, IAllocator allc)
{
    uassert(item);
    uassert(jr);
    uassert(allc);

    jr$foreach(k, v, jr)
    {
        if (str$eq(k, "stock")) {
            item->stock = mem$new(allc, Stock);
            if (item->stock == NULL) { jr$egoto(jr, Error.memory, err); }
            jr$egoto(jr, deserialize_stock(jr, item->stock, allc), err);
        } else if (str$eq(k, "qty")) {
            jr$egoto(jr, str$convert(v, &item->qty), err);
        } else if (str$eq(k, "price")) {
            jr$egoto(jr, str$convert(v, &item->price), err);
        }
    }
    return EOK;
err:
    destroy_order(item, allc);
    return jr->error;
}

Exception
print_stock(jw_c* jw, Stock* stk)
{
    jw_c _jw;
    if (!jw) {
        e$ret(jw$new(&_jw, stdout, .indent = 4));
        jw = &_jw;
    }

    jw$scope(jw, JsonType__obj)
    {
        jw$key("ticker");
        jw$val(stk->ticker);

        jw$key("id");
        jw$val(stk->id);
    }

    return EOK;
}

Exception
print_order(jw_c* jw, Order* ord)
{
    jw_c _jw;
    if (!jw) {
        e$ret(jw$new(&_jw, stdout, .indent = 4));
        jw = &_jw;
    }
    jw$scope(jw, JsonType__obj)
    {
        jw$key("price");
        jw$val(ord->price);

        jw$key("qty");
        jw$val(ord->qty);

        jw$key("stock");
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

    jr_c js;
    e$ret(jr$new(&js, content.buf, content.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);

    jr$foreach(k, v, &js)
    {
        io.printf("key=%S value=%S\n", k, v);
        if (str$eq(k, "foo")) {
            jr$foreach(k, v, &js)
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
            jr$foreach(v, &js)
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

    jr_c js;
    e$ret(jr$new(&js, content.buf, content.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);

    str_s arr_scope = { 0 };
    str_s obj_scope = { 0 };
    u32 req_type = 0;
    jr$foreach(k, v, &js)
    {
        (void)k;
        (void)v;
        if (str$eq(k, "arr")) {
            arr_scope = jr$get_scope_str_s(&js, JsonType__arr);
            tassert_eq(arr_scope, str$s("[1, 2, 3]"));
        } else if (str$eq(k, "args")) {
            obj_scope = jr$get_scope_str_s(&js, JsonType__obj);
            tassert_eq(obj_scope, str$s("{\"baz\": 3, \"fuzz\": 8}"));
        } else if (str$eq(k, "req_type")) {
            e$ret(str$convert(v, &req_type));
        }
    }
    tassert_er(js.error, EOK);
    tassert_eq(req_type, 17);

    u32 arr_sum = 0;
    e$ret(jr$new(&js, arr_scope.buf, arr_scope.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__arr);
    jr$foreach(v, &js)
    {
        u32 res = 0;
        e$ret(str$convert(v, &res));
        tassert(res > 0);
        arr_sum += res;
    }
    tassert_er(js.error, EOK);
    tassert_eq(arr_sum, 1 + 2 + 3);

    e$ret(jr$new(&js, obj_scope.buf, obj_scope.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);
    bool has_baz = false;
    bool has_fuzz = false;
    jr$foreach(k, v, &js)
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

    jr_c js;
    e$ret(jr$new(&js, content.buf, content.len, .strict_mode = false));
    jr$foreach(k, v, &js)
    {
        (void)v;
        if (str$eq(k, "items")) {
            jr$foreach(it, &js)
            {
                (void)it;
                struct Item i = { 0 };
                jr$foreach(k, v, &js)
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

    jr_c js;
    e$ret(jr$new(&js, content.buf, content.len, .strict_mode = true));

    bool has_items = false;
    bool has_foo = false;
    jr$foreach(k, v, &js)
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

    jr_c js;
    e$ret(jr$new(&js, content.buf, content.len, .strict_mode = true));

    bool has_items = false;
    bool has_foo = false;
    jr$foreach(k, v, &js)
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

    jr_c js;
    e$ret(jr$new(&js, content.buf, content.len, .strict_mode = true));

    bool has_items = false;
    bool has_foo = false;
    jr$foreach(k, v, &js)
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

    jr_c js;
    e$ret(jr$new(&js, content.buf, content.len, .strict_mode = false));

    bool has_items = false;
    bool has_foo = false;
    jr$foreach(k, v, &js)
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
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4));
        // tassert_er(EOK, jw$new(&jb, stdout, .indent = 0));

        jw$scope(&jb, JsonType__obj)
        {
            jw$key("foo2");
            jw$val("1");

            jw$key("foo3");
            jw$fmt("%d", 4);

            jw$key("bar");
            jw$scope(&jb, JsonType__arr)
            {
                jw$val("foo");
                jw$fmt("%d", 39);

                jw$scope(&jb, JsonType__arr)
                {
                    for (u32 i = 0; i < 10; i++) { jw$val(i); }
                }
                jw$scope(&jb, JsonType__obj) {}
                jw$scope(&jb, JsonType__arr) {}
            }
            jw$key("far");
            jw$scope(&jb, JsonType__obj)
            {
                jw$key("zoo");
                jw$val(1);
            }
            jw$key("arr_empty");
            jw$scope(&jb, JsonType__arr) {}

            jw$key("obj_empty");
            jw$scope(&jb, JsonType__obj) {}
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
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 0));

        jw$scope(&jb, JsonType__obj)
        {
            jw$key("foo2");
            jw$val("1");

            jw$key("foo3");
            jw$fmt("%d", 4);

            jw$key("bar");
            jw$scope(&jb, JsonType__arr)
            {
                jw$val("foo");
                jw$fmt("%d", 39);

                jw$scope(&jb, JsonType__arr)
                {
                    for (u32 i = 0; i < 10; i++) { jw$val(i); }
                }
                jw$scope(&jb, JsonType__obj) {}
                jw$scope(&jb, JsonType__arr) {}
            }
            jw$key("far");
            jw$scope(&jb, JsonType__obj)
            {
                jw$key("zoo");
                jw$val(1);
            }
            jw$key("arr_empty");
            jw$scope(&jb, JsonType__arr) {}

            jw$key("obj_empty");
            jw$scope(&jb, JsonType__obj) {}
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
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4));

        jw$scope(&jb, JsonType__obj)
        {
            jw$fmt("\"cool\": %d", 4);

            jw$key("arr");
            jw$scope(&jb, JsonType__arr)
            {
                for (u32 i = 0; i < 10; i++) { jw$val(i); }
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
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        e$ret(jw$new(&jb, .buf = &buf, .indent = 4));

        e$ret(print_order(&jb, &ord));

        tassert_er(EOK, jb.error);
        e$ret(jw$validate(&jb));

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

        jr_c jr;
        e$ret(jr$new(&jr, expected, 0, .strict_mode = true));

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
        jr_c jr;
        e$ret(jr$new(&jr, expected, 0, .strict_mode = true));

        Order ord2 = { 0 };
        if (deserialize_order(&jr, &ord2, _)) {
            io.printf(jr$err_fmt(&jr));
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
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4));

        jw$scope(&jb, JsonType__arr)
        {
            u8 v1 = UINT8_MAX;
            jw$val(v1);
            i8 v2 = INT8_MIN;
            jw$val(v2);
            i16 v3 = INT16_MIN;
            jw$val(v3);
            u16 v4 = UINT16_MAX;
            jw$val(v4);
            i32 v5 = INT32_MIN;
            jw$val(v5);
            u32 v6 = UINT32_MAX;
            jw$val(v6);
            i64 v7 = INT64_MIN;
            jw$val(v7);
            u64 v8 = UINT64_MAX;
            jw$val(v8);
            f32 v10 = HUGE_VAL;
            jw$val(v10);
            f32 v11 = -HUGE_VAL;
            jw$val(v11);
            f32 v12 = NAN;
            jw$val(v12);
            f64 v13 = HUGE_VAL;
            jw$val(v13);
            f64 v14 = -HUGE_VAL;
            jw$val(v14);
            f64 v15 = NAN;
            jw$val(v15);
            bool v16 = true;
            jw$val(v16);
            bool v17 = false;
            jw$val(v17);

            const char* s1 = "const";
            jw$val(s1);
            char* s2 = "str";
            jw$val(s2);
            str_s s3 = str$s("str_s");
            jw$val(s3);
            char* s4 = NULL;
            jw$val(s4);
            str_s s5 = { 0 };
            jw$val(s5);

            // usize v17 = SIZE_MAX;
            // jw$val(v17);
            // isize v18 = PTRDIFF_MIN;
            // jw$val(v18);
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

    jr_c js;
    e$ret(jr$new(&js, content.buf, content.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);

    u32 val = 0;
    jr$foreach(k, v, &js)
    {
        if (str$eq(k, "foo")) { e$ret(str$convert(v, &val)); }
    }
    tassert_er(jr$err(&js), "Unexpected token");
    tassert_eq(val, 0);

    // NOTE: jr$err_fmt can work with any printf function
    io.printf(jr$err_fmt(&js));
    fprintf(stdout, jr$err_fmt(&js));
    char* s = str.fmt(mem$, jr$err_fmt(&js));
    io.printf(s);
    mem$free(mem$, s);

    return EOK;
}

test$case(json_writer_null_scope)
{
    mem$scope(tmem$, _)
    {
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4));
        // tassert_er(EOK, jw$new(&jb, stdout, .indent = 0));

        jw$scope(&jb, JsonType__obj)
        {
            jw$key("bar");
            jw$val(NULL);

            jw$key("far");
            jw$scope(&jb, JsonType__obj)
            {
                jw$key("zoo");
                jw$val(NULL);

                jw$key("zoo");
                jw$val(NULL);
            }
            jw$key("arr_empty");
            jw$scope(&jb, JsonType__arr)
            {
                jw$val(NULL);
                jw$val(NULL);
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
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4));
        // tassert_er(EOK, jw$new(&jb, stdout, .indent = 0));

        jw$scope(&jb, JsonType__null)
        {
            jw$val(NULL);
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

    jr_c js;
    e$ret(jr$new(&js, content.buf, content.len, .strict_mode = true));
    tassert_eq(js.type, JsonType__obj);

    jr$foreach(k, v, &js)
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
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4));
        // tassert_er(EOK, jw$new(&jb, stdout, .indent = 0));

        jw$scope(&jb, JsonType__obj)
        {
            jw$key("foo");
            jw$scope(&jb, JsonType__null)
            {
                jw$val(NULL);
            }

            jw$key("bar");
            jw$scope(&jb, JsonType__null)
            {
                jw$val(NULL);
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
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        jw$scope(&jb, JsonType__obj)
        {
            jw$key("foo");
            jw$scope(&jb, JsonType__null)
            {
                jw$val(NULL);
            }

            jw$key("bar");
            jw$scope(&jb, JsonType__null)
            {
                jw$val(NULL);
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

        jr_c jr;
        e$ret(jr$new(&jr, expected, 0, .strict_mode = true));

        Order ord2 = { 0 };
        tassert_eq(Error.not_found, deserialize_order(&jr, &ord2, _));
    }

    return EOK;
}

test$case(json_reader_is_type_compatible)
{
    mem$scope(tmem$, _)
    {
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4));
        jr_c jr;

        jr = (jr_c){ .type = JsonType__num };
        u8 v1 = UINT8_MAX;
        tassert(jr$is_type_compatible(&jr, &v1));
        i8 v2 = INT8_MIN;
        tassert(jr$is_type_compatible(&jr, &v2));
        i16 v3 = INT16_MIN;
        tassert(jr$is_type_compatible(&jr, &v3));
        u16 v4 = UINT16_MAX;
        tassert(jr$is_type_compatible(&jr, &v4));
        i32 v5 = INT32_MIN;
        tassert(jr$is_type_compatible(&jr, &v5));
        u32 v6 = UINT32_MAX;
        tassert(jr$is_type_compatible(&jr, &v6));
        i64 v7 = INT64_MIN;
        tassert(jr$is_type_compatible(&jr, &v7));
        u64 v8 = UINT64_MAX;
        tassert(jr$is_type_compatible(&jr, &v8));
        f32 v10 = HUGE_VAL;
        tassert(jr$is_type_compatible(&jr, &v10));
        f64 v13 = HUGE_VAL;
        tassert(jr$is_type_compatible(&jr, &v13));

        bool v16 = true;
        tassert(!jr$is_type_compatible(&jr, &v16));

        jr = (jr_c){ .type = JsonType__null };
        tassert(!jr$is_type_compatible(&jr, &v16));
        jr = (jr_c){ .type = JsonType__bool };
        tassert(jr$is_type_compatible(&jr, &v16));

        jr = (jr_c){ .type = JsonType__str };
        const char* s1 = "const";
        tassert(jr$is_type_compatible(&jr, &s1));
        char* s2 = "str";
        tassert(jr$is_type_compatible(&jr, &s2));
        str_s s3 = str$s("str_s");
        tassert(jr$is_type_compatible(&jr, &s3));
        char* s4 = NULL;
        tassert(jr$is_type_compatible(&jr, &s4));
        str_s s5 = { 0 };
        tassert(jr$is_type_compatible(&jr, &s5));

        jr = (jr_c){ .type = JsonType__null };
        tassert(!jr$is_type_compatible(&jr, &s5));

        jr = (jr_c){ .type = JsonType__null };
        tassert(jr$is_type_compatible(&jr, NULL));
    }
    return EOK;
}

test$case(json_writer_unicode_proto)
{
    mem$scope(tmem$, _)
    {
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        jw$scope(&jb, JsonType__obj)
        {
            jw$key("1");
            jw$val("\"");
            jw$key("2");
            jw$val("\\");
            jw$key("3");
            jw$val("\f");
            jw$key("4");
            jw$val("\n");
            jw$key("5");
            jw$val("\r");
            jw$key("6");
            jw$val("\t");
            jw$key("7");
            jw$val("/");
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
        jw_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        jw$scope(&jb, JsonType__obj)
        {
            jw$key("1");
            jw$val("\x1B");
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
        jw_c jb;
        sbuf_c buf = sbuf.create(4096, _);
        (void)buf;

        // Test with various Unicode scenarios
        tassert_er(EOK, jw$new(&jb, .buf = &buf, .indent = 4, .simplified = true));

        jw$scope(&jb, JsonType__obj)
        {
            // 2. ASCII printable characters
            jw$key("ascii_printable");
            jw$val("Hello World!@#$%^&*()");

            // 3. Latin-1 Supplement (U+0080 to U+00FF)
            jw$key("latin1_supplement");
            jw$val("©®±µ¼½¾¿ÀÁÂÃÄÅÆÇ");

            // 4. Common symbols and punctuation
            jw$key("symbols");
            jw$val("€£¥¢§¶†‡•…—–");

            // 5. Common scripts
            jw$key("latin_extended");
            jw$val("ŠšŽžÀàÁáÂâÃãÄä");

            jw$key("greek");
            jw$val("ΑαΒβΓγΔδΕεΖζΗηΘθ");

            jw$key("cyrillic");
            jw$val("АаБбВвГгДдЕеЁёЖж");

            jw$key("arabic");
            jw$val("اب ت ث ج ح خ د ذ ر ز");

            // 6. Asian scripts
            jw$key("chinese");
            jw$val("你好世界"); // Hello World

            jw$key("japanese");
            jw$val("こんにちは世界"); // Hello World

            jw$key("korean");
            jw$val("안녕하세요 세계"); // Hello World

            // 7. Mathematical symbols
            jw$key("math_symbols");
            jw$val("∑∏√∞∫≈≠≤≥∈∉∧∨¬⇒⇔");
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

test$main();
