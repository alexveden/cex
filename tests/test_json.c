#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/json/json.c"

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


Exception
print_stock(json_writer_c* jw, Stock* stk)
{
    json_writer_c _jw;
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
print_order(json_writer_c* jw, Order* ord)
{
    json_writer_c _jw;
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

    json_reader_c js;
    jr$new(&js, content.buf, content.len, .strict_mode = true);
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

test$case(json_reader_low_level)
{
    struct Foo
    {
        struct
        {
            u32 baz;
            u32 fuzz;
        } foo;
        u32 next;
        u32 baz;
    } data = { 0 };
    (void)data;
    str_s content = str$s(
        "{ \"foo\" : {\"baz\": 3, \"fuzz\": 8, \"oops\": 0}, \"next\": [1, 2, 3], \"baz\": 17 }"
    );

    json_reader_c js;
    e$ret(json.reader.create(&js, content.buf, content.len, NULL));
    e$ret(json.reader.step_in(&js, JsonType__obj));

    u32 n_keys = 0;
    while (json.reader.next(&js)) {
        io.printf("key=%S value=%S\n", js.key, js.val);
        n_keys++;
    }
    tassert_er(js.error, EOK);
    tassert_eq(n_keys, 3);

    return EOK;
}

test$case(json_reader_low_level_next_finetune)
{
    str_s content = str$s("null");

    json_reader_c js;
    e$ret(json.reader.create(&js, content.buf, content.len, NULL));
    tassert_eq(js.type, JsonType__null);
    tassert_er(js.error, EOK);

    return EOK;
}

test$case(json_reader_low_level_next_empty_obj)
{
    str_s content = str$s("{}");

    json_reader_c js;
    e$ret(json.reader.create(&js, content.buf, content.len, NULL));
    tassert_eq(js.type, JsonType__obj);
    tassert_er(js.error, EOK);

    return EOK;
}

test$case(json_reader_low_level_next_empty_arr)
{
    str_s content = str$s("[]");

    json_reader_c js;
    e$ret(json.reader.create(&js, content.buf, content.len, NULL));
    tassert_eq(js.type, JsonType__arr);
    tassert_er(js.error, EOK);

    return EOK;
}

test$case(json_reader_low_level_next_empty_number)
{
    str_s content = str$s("123");

    json_reader_c js;
    e$ret(json.reader.create(&js, content.buf, content.len, NULL));
    tassert_eq(js.type, JsonType__num);
    tassert_er(js.error, EOK);

    return EOK;
}

test$case(json_reader_low_level_next_empty_str)
{
    str_s content = str$s("\"123\"");

    json_reader_c js;
    e$ret(json.reader.create(&js, content.buf, content.len, NULL));
    tassert_eq(js.type, JsonType__str);
    tassert_er(js.error, EOK);

    return EOK;
}


test$case(json_reader_low_level_json5_key_names)
{
    struct Items
    {
        u32 qty;
        f32 price;
    };
    struct Foo
    {
        u32 next;
        u32 baz;
    } data = { 0 };
    (void)data;

    str_s content = str$s("{ items : [{qty: 1, price: 123}, {qty: -100, price: 999}]  }");

    json_reader_c js;
    e$ret(json.reader.create(&js, content.buf, content.len, NULL));
    e$assert(js.type == JsonType__obj);
    e$ret(json.reader.step_in(&js, JsonType__obj));

    io.printf("%S\n\n", content);
    while (json.reader.next(&js)) {
        if (str$eq(js.key, "items")) {
            tassert_eq(js.type, JsonType__arr);
            e$ret(json.reader.step_in(&js, JsonType__arr));
            while (json.reader.next(&js)) {
                io.printf("--type=%d key=%S val=%S\n", js.type, js.key, js.val);
            }
        }
    }
    tassert_er(js.error, EOK);

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

    json_reader_c js;
    jr$new(&js, content.buf, content.len, .strict_mode = false);
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

    json_reader_c js;
    jr$new(&js, content.buf, content.len, .strict_mode = true);

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

    json_reader_c js;
    jr$new(&js, content.buf, content.len, .strict_mode = true);

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

    json_reader_c js;
    jr$new(&js, content.buf, content.len, .strict_mode = true);

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

    json_reader_c js;
    jr$new(&js, content.buf, content.len, .strict_mode = false);

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
        json_writer_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, buf, .indent = 4));
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
        json_writer_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, buf, .indent = 0));

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
        json_writer_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, buf, .indent = 4));
        // tassert_er(EOK, jw$new(&jb, stdout, .indent = 0));

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

test$case(json_writer_multi_func_concept)
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
        json_writer_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        tassert_er(EOK, jw$new(&jb, buf, .indent = 4));

        e$ret(print_order(&jb, &ord));

        tassert_er(EOK, jb.error);
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
    }

    return EOK;
}
test$main();
