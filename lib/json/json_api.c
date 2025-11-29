#include "cex.h"

typedef struct settings_s
{
    u32 foo;
    char* bar;
    arr$(u32) array;
} settings_s;

typedef struct json_c
{
    u32 _foo;
    char* key;
    char* val;

    // Error of the processing
    Exc err;
} json_c;

typedef enum JsonType_e
{
    JsonType__na, // 0
    JsonType__obj,
    JsonType__str,
    JsonType__arr,
    JsonType__cnt, // last
} JsonType_e;

#define jr$scope(...)
#define jr$writer_scope(...)
#define jr$switch(...)
#define jr$case(...)
#define jr$default(...)
#define jr$foreach(...)

/*
{
    "foo": 1,
    "bar": "baz",
    "arr": [1, 2, 3, 4],
}

// Alt json
[4, "foo", {}]

// Alt
"foo"

// Alt
null

*/
Exception
read_json(char* contents)
{
    (void)contents;

    mem$scope(tmem$, _)
    {
        settings_s result = { 0 };
        result.array = arr$new(result.array, _);

        json_c j = { 0 };

        // initialize the memory buffer of (j)
        // TODO:
        // 1. Should check if contents are valid
        // 2. Should check if next item is really JsonType__obj
        // 3. Should set private variable for json file instance
        jr$scope(&j, contents, 0, JsonType__obj)
        {
            // TODO: this should be backed by for loop ??
            jr$switch(j.key)
            {
                jr$case_invalid() {}
                // TODO: This should use only literals with internal check of len+content + JsonType
                jr$case("foo")
                {
                    e$ret(str$convert(j.val, &result.foo));
                }
                // TODO: this should be if else
                jr$case("bar")
                {
                    result.bar = str.clone(j.val, _);
                }
                jr$case("arr")
                {
                    // This checks if j.val is JsonType__arr
                    str_s it = { 0 }; // Mock!

                    //
                    // "arr": [1, 2, 3, 4],
                    // it = "1" // string slice of len=1!
                    // it = "2"
                    // it = "3"
                    jr$foreach(it, j.val)
                    {
                        u32 v = 0;
                        e$ret(str$convert(it, &v));
                        arr$push(result.array, v);
                    }
                }
                jr$default()
                {
                    return e$raise(Error.runtime, "Unknown json field");
                }
            }
        }

        if (j.err) {
            return e$raise(j.err, "Error processing json file.");
            // return j.err;
        }
    }

    return EOK;
}

Exception
read_json_array(char* contents)
{
    (void)contents;

    mem$scope(tmem$, _)
    {
        settings_s result = { 0 };
        result.array = arr$new(result.array, _);

        json_c j = { 0 };

        // input json like this
        // [ {"foo": 1}, {"foo": 2} ]
        jr$scope(&j, contents, 0, JsonType__arr)
        {
            // it - str_s
            jr$foreach(it, j.val, JsonType__obj)
            {
                jr$switch(j.key)
                {
                    jr$case("foo")
                    {
                        e$ret(str$convert(j.val, &result.foo));
                    }
                    jr$case("bar")
                    {
                        result.bar = str.clone(j.val, _);
                    }
                    jr$default()
                    {
                        return e$raise(Error.runtime, "Unknown json field");
                    }
                }
            }
        }

        if (j.err) { return e$raise(j.err, "Error processing json file."); }
    }

    return EOK;
}

#define jw$scope(...)
#define jw$obj_scope(...)
#define jw$arr_scope(...)

#define jw$kval(...)
#define jw$kstr(...)
#define jw$kobj_scope(...)
#define jw$karr_scope(...)

#define jw$val(...)
#define jw$str(...)

Exception
read_json_array_writer(char* contents)
{
    (void)contents;

    mem$scope(tmem$, _)
    {
        settings_s result = { 0 };
        json_c j = { 0 };
        /*
        {
            "foo": {
                "bar": 1,
                "baz": "2",
            },
            "sec": [1, 2, 3],
        }
        */

        jw$scope(&j, JsonType__obj)
        {
            jw$kobj_scope("foo")
            {
                jw$kval("bar", "%d", 1);
                jw$kstr("baz", "2")
            };
            jw$karr_scope("sec")
            {
                for (u32 i = 0; i < 4; i++) { jw$val("%d", i); }
            }
        }


        if (j.err) { return e$raise(j.err, "Error processing json file."); }
    }

    return EOK;
}
