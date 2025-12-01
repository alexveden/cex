#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/json/json.c"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}




test$case(json_reader_macro_proto)
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



test$case(json_writer_macro_proto)
{
    mem$scope(tmem$, _)
    {
        json_writer_c jb;
        sbuf_c buf = sbuf.create(1024, _);
        (void)buf;
        tassert_er(EOK, jw$new(&jb, buf, .indent = 4));
        // tassert_er(EOK, jw$new(&jb, stdout, .indent = 0));

        jw$buf(&jb, JsonType__obj)
        {
            // jw$fmt("// How about a comment? %d\n", 2);
            jw$kstr("foo2", "%d", 1);
            jw$kval("foo3", "%d", 4);
            jw$karr_scope("bar")
            {
                jw$str("%s", "foo");
                jw$val("%d", 39);
                jw$arr_scope()
                {
                    for (u32 i = 0; i < 10; i++) { jw$val("%d", i); }
                }
                jw$obj_scope() {}
                jw$arr_scope() {}
            }
            jw$kobj_scope("far")
            {
                jw$kval("zoo", "%d", 1);
            }
            jw$karr_scope("arr_empty"){}
            jw$kobj_scope("obj_empty"){}
        }

        io.printf("\nJSON (buf): \n`%s`", buf);
        tassert_er(EOK, jb.error);
        tassert(false);
        // tassert_eq(jb.buf, json.writer.get(&jb));
    }
    return EOK;
}
test$main();
