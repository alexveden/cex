#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"
#include "lib/json/json.c"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(json_writer_macro_proto)
{
    mem$scope(tmem$, _)
    {
        json_writer_c jb;
        tassert_er(EOK, json.writer.create(&jb, 1024, 4, _));
        jw$buf(&jb, JsonType__obj)
        {
            jw$fmt("// How about a comment? %d\n", 2);
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
            }
            jw$kobj_scope("far")
            {
                jw$fmt("\"%s_%d\": %d,\n", "mykey", 2, 77);
                jw$kval("zoo", "%d", 1);
            }
        }
        io.printf("JSON: \n`%s`", jb.buf);
        // tassert(false);
        tassert_eq(jb.buf, json.writer.get(&jb));
        tassert_er(EOK, jb.error);
    }
    return EOK;
}


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
    if (js.error) {
        // We can report JSON file, line:col + error message
        return Error.runtime;
    }

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
    struct Items {
        u32 qty;
        f32 price;
    };
    struct Foo
    {
        u32 next;
        u32 baz;
    } data = { 0 };
    (void)data;

    str_s content = str$s(
        "{ items : [{qty: 1, price: 123}, {qty: -100, price: 999}]  }"
    );

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
                io.printf("--type=%d key=%S val=%S\n", js.type,  js.key, js.val);
            }
        }
    }
    tassert_er(js.error, EOK);
    tassert(false);

    return EOK;
}
test$main();
