#include "src/all.c"
#include "src/all.h"
#include "src/ds.h"
#include "src/test.h"


static void
add_to_arr(arr$(int) * arr)
{
    // arrput(*arr, 2);
    // arrput(*arr, 3);
    // arrput(*arr, 5);
    int z = 200;
    arr$pushm(*arr, z, 3, 5);
}

static void
add_to_str(arr$(const char*) * arr)
{
    static char buf[10] = { "buf" };

    arr$push(*arr, "foooo");
    arr$push(*arr, "barrr");
    arr$push(*arr, "bazzz");
    arr$pushm(*arr, "one", "two", "three", buf);
}

test$case(test_array)
{
    arr$(const char*) array2 = arr$new(array2, mem$);
    arr$(int) array = arr$new(array, mem$);
    tassert(array != NULL);
    tassert_eq(arr$cap(array), 16);

    int z = 200;
    int* zp = &z;
    size_t s = { z };
    size_t s2 = { (*zp) };
    tassert_eq(s, 200);
    tassert_eq(s2, 200);

    add_to_arr(&array);
    add_to_str(&array2);
    tassert_eq(arr$len(array2), 7);

    for (usize i = 0; i < arr$len(array); ++i) { io.printf("%d \n", array[i]); }

    for (usize i = 0; i < arr$len(array2); ++i) { io.printf("%s \n", array2[i]); }
    arr$free(array);
    arr$free(array2);

    return EOK;
}

test$case(test_array_len)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    // Works with CEX dynamic arrays
    tassert_eq(arr$len(array), 3);


    // NULL is supported, and emits 0 length
    arr$free(array);
    tassert(array == NULL); 
    tassert_eq(arr$len(array), 0); // NOTE: NULL array - len = 0

    // Works with static arrays
    char buf[] = {"hello"}; 
    tassert_eq(arr$len(buf), 6); // NOTE: include null term

    // Works with arrays of given capacity
    char buf2[10] = {0};
    tassert_eq(arr$len(buf2), 10);

    // Type doesn't matter
    i32 a[7] = {0};
    tassert_eq(arr$len(a), 7);

    // Works with string literals
    tassert_eq(arr$len("CEX"), 4); // NOTE: include null term


    // Works with CEX hashmap
    hm$(int, int) intmap = hm$new(intmap, mem$);
    hm$set(intmap, 1, 3);
    tassert_eq(arr$len(intmap), 1);

    hm$free(intmap);

    return EOK;
}

test$case(test_array_access)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    // Dynamic array access is natural C index
    tassert_eq(array[2], 3);
    // tassert_eq(arr$at(array, 3), 3); // NOTE: this is bounds checking access, with assertion 
    arr$free(array);

    // Works with static arrays
    char buf[] = {"hello"}; 
    tassert_eq(buf[1], 'e'); 

    // Works with CEX hashmap
    hm$(int, int) intmap = hm$new(intmap, mem$);
    hm$set(intmap, 1, 3);
    hm$set(intmap, 2, 5);
    tassert_eq(arr$len(intmap), 2);

    // Accessing hashmap as array
    // NOTE: hashmap elements are ordered until first deletion
    tassert_eq(intmap[0].key, 1);
    tassert_eq(intmap[0].value, 3);

    tassert_eq(intmap[1].key, 2);
    tassert_eq(intmap[1].value, 5);

    hm$free(intmap);

    return EOK;
}

test$case(test_array_iteration)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    i32 nit = 0; // it's only for testing
    for$each(it, array) {
        tassert_eq(it, ++nit);
        io.printf("el=%d\n", it);
    }
    // Prints: 
    // el=1
    // el=2
    // el=3

    nit = 0;
    // NOTE: prefer this when you work with bigger structs to avoid extra memory copying
    for$eachp(it, array) {
        // TIP: making array index out of `it`
        usize i = it - array;
        tassert_eq(i, nit);

        // NOTE: it now is a pointer
        tassert_eq(*it, ++nit);
        io.printf("el[%zu]=%d\n", i, *it);
    }
    // Prints: 
    // el[0]=1
    // el[1]=2
    // el[2]=3

    // Static arrays work as well (arr$len inferred)
    i32 arr_int[] = {1, 2, 3, 4, 5};
    for$each(it, arr_int) {
        io.printf("static=%d\n", it);
    }
    // Prints:
    // static=1
    // static=2
    // static=3
    // static=4
    // static=5


    // Simple pointer+length also works (let's do a slice)
    i32* slice = &arr_int[2];
    for$each(it, slice, 2) {
        io.printf("slice=%d\n", it);
    }
    // Prints:
    // slice=3
    // slice=4

    arr$free(array);
    return EOK;
}

test$case(test_array_char_ptr)
{
    arr$(char*) array = arr$new(array, mem$);
    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$push(array, "baz");
    for (usize i = 0; i < arr$len(array); ++i) { io.printf("%s \n", array[i]); }
    arr$free(array);

    return EOK;
}


typedef struct
{
    int key;
    float my_val;
    char* my_string;
    int value;
} my_struct;

test$case(test_array_struct)
{
    arr$(my_struct) array = arr$new(array, mem$, .capacity = 128);
    tassert_eq(arr$cap(array), 128);

    my_struct s;
    s = (my_struct){ 20, 5.0, "hello ", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 2.5, "failure", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 1.1, "world!", 0 };
    arr$push(array, s);

    for (usize i = 0; i < arr$len(array); ++i) {
        io.printf("key: %d str: %s\n", array[i].key, array[i].my_string);
    }
    arr$free(array);

    return EOK;
}

test$case(test_array_struct_initialilzer)
{
    arr$(my_struct) array = arr$new(array, mem$, .capacity = 128);
    tassert_eq(arr$cap(array), 128);

    arr$push(array, (my_struct){ 20, 5.0, "hello", 0 });

    tassert_eq(array[0].my_string, "hello");
    tassert_eq(array[0].key, 20);

    arr$free(array);

    return EOK;
}


test$case(test_array_push_returns_a_pointer)
{
    arr$(my_struct) array = arr$new(array, mem$, .capacity = 128);
    tassert_eq(arr$cap(array), 128);

    my_struct* s = arr$push(array, (my_struct){ 20, 5.0, "hello", 0 });

    tassert_eq(array[0].my_string, "hello");
    tassert_eq(array[0].key, 20);

    tassert_eq((usize)s, (usize)&array[0]);
    tassert_eq(s->my_string, "hello");
    tassert_eq(s->key, 20);


    arr$free(array);

    return EOK;
}

typedef struct
{
    int key, b, c, d;
} stbds_struct;
typedef struct
{
    int key[2], b, c, d;
} stbds_struct2;

static char buffer[256];
char*
strkey(int n)
{
    str.sprintf(buffer, sizeof(buffer), "test_%d", n);
    return buffer;
}

test$case(test_orig_arr)
{
    int i, j;

    for (i = 0; i < 20000; i += 50) {
        arr$(int) arr = arr$new(arr, mem$);

        for (j = 0; j < i; ++j) { arr$push(arr, j); }
        arr$free(arr);
        tassert(arr == NULL);
    }

    for (i = 0; i < 4; ++i) {
        arr$(int) arr = arr$new(arr, mem$);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$del(arr, i);
        arr$free(arr);
        tassert(arr == NULL);

        arr = arr$new(arr, mem$);
        tassert(arr != NULL);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$delswap(arr, i);
        arr$free(arr);
        tassert(arr == NULL);
    }

    for (i = 0; i < 5; ++i) {
        arr$(int) arr = arr$new(arr, mem$);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$ins(arr, i, 5);
        tassert(arr[i] == 5);
        tassert_eq(arr$len(arr), 5);
        if (i < 4) { tassert_eq(arr[4], 4); }
        arr$free(arr);
        tassert(arr == NULL);
    }

    return EOK;
}

test$case(test_orig_hashmap)
{
    int i = 1;
    const int testsize = 100000;
    hm$(int, int) intmap = hm$new(intmap, mem$);
    hm$(stbds_struct, int) map = hm$new(map, mem$);
    tassert(intmap != NULL);

    tassert(hm$get(intmap, i) == 0);
    for (i = 0; i < testsize; i += 2) { hm$set(intmap, i, i * 5); }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i, -2) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 5);
        }
    }
    for (i = 0; i < testsize; i += 2) { hm$set(intmap, i, i * 3); }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i, -2) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }
    for (i = 2; i < testsize; i += 4) {
        (void)hm$del(intmap, i); // delete half the entries
    }
    for (i = 0; i < testsize; i += 1) {
        if (i & 3) {
            tassert(hm$get(intmap, i) == 0);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }
    for (i = 0; i < testsize; i += 1) {
        (void)hm$del(intmap, i); // delete the rest of the entries
    }
    for (i = 0; i < testsize; i += 1) { tassert(hm$get(intmap, i) == 0); }
    hm$free(intmap);
    tassert(intmap == NULL);

    intmap = hm$new(intmap, mem$);

    for (i = 0; i < testsize; i += 2) { hm$set(intmap, i, i * 3); }
    hm$free(intmap);

    intmap = hm$new(intmap, mem$);
    hm$set(intmap, 15, 7);
    hm$set(intmap, 11, 3);
    hm$set(intmap, 9, 5);
    tassert(hm$get(intmap, 9) == 5);
    tassert(hm$get(intmap, 11) == 3);
    tassert(hm$get(intmap, 15) == 7);
    hm$free(intmap);

    intmap = hm$new(intmap, mem$);
    for (i = 0; i < testsize; i += 2) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        hm$set(map, s, i * 5);
        tassert(map != NULL);
    }
    hm$free(intmap);

    for (i = 0; i < testsize; i += 1) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        if (i & 1) {
            tassert(hm$get(map, s) == 0);
        } else {
            tassert(hm$get(map, s) == i * 5);
        }
    }

    stbds_struct* map2 = hm$new(map2, mem$);
    for (i = 0; i < testsize; i += 2) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        hm$sets(map2, s);
    }
    hm$free(map2);
    hm$free(map);

    return EOK;
}

test$case(test_array_len_unified)
{
    char buf[10];
    u32 iarr[20];
    char buf_zero[0];
    arr$(char*) array = arr$new(array, mem$);
    tassert_eq(0, arr$len(array));
    tassert(mem$aligned_pointer(array, alignof(size_t)) == array);

    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$push(array, "baz");

    tassert_eq(10, arr$len(buf));
    tassert_eq(20, arr$len(iarr));
    tassert_eq(3, arr$len(array));
    tassert_eq(0, arr$len(buf_zero));
    u32* p = NULL;
    (void)p;
    tassert_eq(0, arr$len(NULL));
    tassert_eq(0, arr$len(p));

    arr$free(array);
    return EOK;
}

test$case(test_for_arr)
{
    char buf[] = { 'a', 'b', 'c' };
    char buf_zero[] = {};
    for$each (v, buf_zero) {
        (void)v;
        tassert(false && "must not happen!");
    }
    arr$(char*) array = arr$new(array, mem$);
    for$each (v, array) {
        (void)v;
        tassert(false && "must not happen!");
    }
    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$push(array, "baz");

    tassert_eq(3, arr$len(buf));
    tassert_eq(3, arr$len(array));

    for$each (v, array) { io.printf("v: %s\n", v); }

    typeof(buf[0])* pbuf = &buf[0];
    typeof(array[0])* pbuf2 = &array[0];

    tassert_eq(pbuf[0], 'a');
    tassert_eq(pbuf2[0], "foo");

    for$each (c, buf) { io.printf("c: %c\n", c); }

    u32 n = 0;
    for$eachp(v, array)
    {
        isize i = v - array;
        tassert_eq(n, i);
        io.printf("v: %s, i: %zd\n", *v, i);
        n++;
    }

    n = 0;
    for$eachp(v, buf)
    {
        isize i = v - buf;
        tassert_eq(n, i);
        io.printf("v: %c, i: %zd\n", *v, i);
        n++;
    }

    // TODO: check if possible for$each(*v, &arr)??
    // for$each(c, &buf) {
    //     io.printf("c: %c\n", *c);
    // }
    typeof((&buf)[0])* pbuf_ptr = &((&buf)[0]);
    // typeof((&buf)[0]) p = pbuf_ptr[0];
    auto p = pbuf_ptr[0];
    tassert_eq(*p, 'a');

    typeof((array)[0])* array_ptr = &((array)[0]);
    // typeof((&array)[0]) p_array = array_ptr[0];
    auto p_array = array_ptr;
    // tassert_eq(*p_array, "foo");
    io.printf("p_array: %p\n", p_array);

    arr$free(array);
    return EOK;
}

test$case(test_for_each_null)
{
    arr$(char*) array = NULL;
    for$each (v, array) {
        (void)v;
        tassert(false && "must not happen!");
    }
    return EOK;
}

test$case(test_for_arr_custom_size)
{

    arr$(char*) array = arr$new(array, mem$);
    for$each (v, array) {
        (void)v;
        tassert(false && "must not happen!");
    }
    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$push(array, "baz");

    u32 n = 0;
    tassert_eq(3, arr$len(array));
    for$each (v, array, arr$len(array) - 1) {
        io.printf("v: %s\n", v);
        n++;
    }
    tassert_eq(n, 2);

    char buf[] = { 'a', 'b', 'c' };
    n = 0;
    tassert_eq(3, arr$len(buf));
    for$each (c, buf, 2) {
        io.printf("c: %c\n", c);
        n++;
    }
    tassert_eq(n, 2);

    arr$free(array);
    return EOK;
}

test$case(test_for_arr_for_struct)
{
    arr$(my_struct) array = arr$new(array, mem$);
    tassert_eq(arr$cap(array), 16);
    for$each (v, array) {
        (void)v;
        tassert(false && "must not happen!");
    }
    my_struct f = { .my_string = "far", .key = 200 };
    arr$push(array, (my_struct){ .my_string = "foo", .key = 200, .my_val = 31.2 });
    arr$push(array, f);
    tassert_eq(2, arr$len(array));

    u32 n = 0;
    // v - is a copy of the array element
    for$each (v, array) {
        io.printf("v: %s\n", v.my_string);
        n++;
        v.key = 77;
    }
    tassert_eq(n, 2);
    // NOT changed
    tassert_eq(array[0].key, 200);
    tassert_eq(array[1].key, 200);

    n = 0;
    for$eachp(v, array, arr$len(array))
    {
        isize i = v - array;
        io.printf("v: %s\n", v->my_string);
        tassert_eq(n, i);
        n++;
        v->key = 77;
    }
    tassert_eq(n, 2);
    // array has been changed by pointer
    tassert_eq(array[0].key, 77);
    tassert_eq(array[1].key, 77);
    arr$free(array);
    return EOK;
}

test$case(test_arr_insert_pop)
{
    arr$(int) arr = arr$new(arr, mem$);
    arr$ins(arr, 0, 5);
    arr$ins(arr, 0, 4);
    arr$ins(arr, 0, 3);
    arr$ins(arr, 0, 2);
    arr$ins(arr, 0, 1);

    tassert_eq(arr$len(arr), 5);
    tassert_eq(arr$at(arr, 0), 1);
    tassert_eq(arr$at(arr, 1), 2);
    tassert_eq(arr$at(arr, 2), 3);
    tassert_eq(arr$at(arr, 3), 4);
    tassert_eq(arr$at(arr, 4), 5);

    tassert_eq(arr$pop(arr), 5);
    tassert_eq(arr$pop(arr), 4);
    tassert_eq(arr$pop(arr), 3);
    tassert_eq(arr$pop(arr), 2);
    tassert_eq(arr$pop(arr), 1);

    tassert_eq(arr$len(arr), 0);

    arr$free(arr);
    return EOK;
}

test$case(test_arr_pushm)
{
    arr$(int) arr = arr$new(arr, mem$);
    arr$(int) arr2 = arr$new(arr2, mem$);
    tassert(mem$aligned_pointer(arr, alignof(size_t)) == arr);

    arr$pushm(arr2, 10, 6, 7, 8, 9, 0);
    tassert_eq(arr$len(arr2), 6);

    int sarr[] = { 4, 5 };

    for (u32 i = 0; i < 100; i++) {
        arr$pushm(arr, 1, 2, 3);
        arr$pusha(arr, sarr);
        // NOTE: overriding arr2 size + using it as a raw pointer
        // arr$pusha(arr, &arr2[1]); // THIS IS WRONG, and fail sanitize/arr$validate - needs len!

        // We may use any pointer as an array, but length become mandatory
        arr$pusha(arr, &arr2[1], 5);
    }
    tassert_eq(1000, arr$len(arr));

    for$each (v, arr) { tassert(v < 10); }

    arr$free(arr);
    arr$free(arr2);
    return EOK;
}

test$case(test_overaligned_struct)
{
    struct test32_s
    {
        alignas(32) usize s;
    };

    arr$(struct test32_s) arr = arr$new(arr, mem$);
    struct test32_s f = { .s = 100 };
    tassert(mem$aligned_pointer(arr, 32) == arr);

    for (u32 i = 0; i < 1000; i++) {
        f.s = i;
        arr$push(arr, f);

        tassert_eq(arr$len(arr), i + 1);
    }
    tassert_eq(arr$len(arr), 1000);

    for (u32 i = 0; i < 1000; i++) {
        tassert_eq(arr[i].s, i);
        tassert(mem$aligned_pointer(&arr[i], 32) == &arr[i]);
    }

    arr$free(arr);
    return EOK;
}

test$case(test_overaligned_struct64)
{
    struct test64_s
    {
        alignas(64) usize s;
    };

    arr$(struct test64_s) arr = arr$new(arr, mem$);
    struct test64_s f = { .s = 100 };
    tassert(mem$aligned_pointer(arr, 64) == arr);

    for (u32 i = 0; i < 1000; i++) {
        f.s = i;
        arr$push(arr, f);
    }
    tassert_eq(arr$len(arr), 1000);

    for (u32 i = 0; i < 1000; i++) {
        tassert_eq(arr[i].s, i);
        tassert(mem$aligned_pointer(&arr[i], 64) == &arr[i]);
    }

    arr$free(arr);
    return EOK;
}


test$case(test_smallest_alignment)
{
    arr$(char) arr;
    tassert(arr$new(arr, mem$) != NULL);
    tassert(mem$aligned_pointer(arr, alignof(size_t)) == arr);

    for (u32 i = 0; i < 1000; i++) { arr$push(arr, i % 10); }
    tassert_eq(arr$len(arr), 1000);

    for (u32 i = 0; i < 1000; i++) {
        tassert_eq(arr[i], i % 10);
        tassert(mem$aligned_pointer(&arr[i], 1) == &arr[i]);
    }

    arr$free(arr);
    return EOK;
}

test$case(test_hashmap_basic)
{
    hm$(int, int) intmap;
    hm$new(intmap, mem$);

    hm$(i64, int) intmap64 = hm$new(intmap64, mem$);
    tassert(intmap != NULL);
    tassert(intmap64 != NULL);

    tassert_eq(hm$len(intmap), 0);
    // tassert_eq(arr$len(intmap), 0);
    int _v = { 1 };

    tassert(hm$set(intmap, 1, 3));
    tassert(hm$set(intmap, 100, _v));
    tassert(hm$set(intmap, 1, 3));

    tassert_eq(hm$len(intmap), 2);
    tassert_eq(hm$get(intmap, 1), 3);
    tassert_eq(*hm$getp(intmap, 1), 3);


    // returns default (all zeros)
    tassert_eq(hm$get(intmap, -1), 0);
    tassert_eq(hm$get(intmap64, -1, 999), 999);
    tassert(hm$getp(intmap, -1) == NULL);

    tassert_eq(hm$len(intmap), 2);
    hm$del(intmap, 1);
    hm$del(intmap, 100);
    tassert_eq(hm$len(intmap), 0);

    auto h = _cexds__header(intmap);
    tassert(h->_hash_table->seed != 0);

    hm$free(intmap);
    hm$free(intmap64);
    tassert(intmap == NULL);
    return EOK;
}

test$case(test_hashmap_basic_struct)
{
    struct test64_s
    {
        usize foo;
        usize bar;
    };

    hm$(int, struct test64_s) intmap = hm$new(intmap, mem$);
    tassert(intmap != NULL);

    tassert_eq(hm$len(intmap), 0);

    struct test64_s s2 = (struct test64_s){ .foo = 2 };
    tassert(hm$set(intmap, 1, (struct test64_s){ .foo = 1 }));
    tassert(hm$set(intmap, 2, s2));
    // tassert(hm$set(intmap, 3, (struct test64_s){3, 4}));

    auto s3 = hm$setp(intmap, 3);
    tassert(s3 != NULL);
    s3->foo = 3;
    s3->bar = 4;

    tassert_eq(hm$len(intmap), 3);
    tassert_eq(arr$len(intmap), 3);

    tassert(hm$getp(intmap, -1) == NULL);
    tassert(hm$getp(intmap, 1) != NULL);


    tassert_eq(hm$get(intmap, 1).foo, 1);
    tassert_eq(hm$get(intmap, 2).foo, 2);
    tassert_eq(hm$get(intmap, 3).foo, 3);

    tassert_eq(hm$getp(intmap, 1)[0].foo, 1);
    tassert_eq(hm$getp(intmap, 2)[0].foo, 2);
    tassert_eq(hm$getp(intmap, 3)[0].foo, 3);

    u32 n = 0;
    for$each (v, intmap) {
        tassert_eq(intmap[n].key, v.key);
        // hasmap also supports bounds checked arr$at
        tassert_eq(arr$at(intmap, n).value.foo, v.value.foo);
        tassert_eq(v.key, n + 1);
        n++;
    }
    tassert_eq(n, arr$len(intmap));

    n = 0;
    for$eachp(v, intmap)
    {
        tassert_eq(intmap[n].key, v->key);
        tassert_eq(intmap[n].value.foo, v->value.foo);
        tassert_eq(v->key, n + 1);
        n++;
    }
    tassert_eq(n, arr$len(intmap));

    // ZII struct if not found
    tassert_eq(hm$get(intmap, -1).foo, 0);
    tassert_eq(hm$get(intmap, -1).bar, 0);
    tassert_eq(hm$get(intmap, -1, s2).foo, 2);

    hm$free(intmap);
    return EOK;
}

test$case(test_hashmap_struct_full_setget)
{
    struct test64_s
    {
        usize fooa; // WARNING: key will be second to test key field offset
        usize key;
    };

    hm$s(struct test64_s) smap = hm$new(smap, mem$);
    tassert(smap != NULL);

    static_assert(offsetof(struct test64_s, key) == sizeof(usize), "unexp");

    tassert_eq(hm$len(smap), 0);

    tassert(hm$sets(smap, (struct test64_s){ .key = 1, .fooa = 10 }));
    tassert_eq(hm$len(smap), 1);
    tassert_eq(smap[0].key, 1);
    tassert_eq(smap[0].fooa, 10);

    struct test64_s* r = hm$gets(smap, 1);
    tassert(r != NULL);
    tassert(r == &smap[0]);
    tassert_eq(r->key, 1);
    tassert_eq(r->fooa, 10);

    struct test64_s s2 = { .key = 2, .fooa = 200 };
    tassert(hm$sets(smap, s2));
    tassert_eq(hm$len(smap), 2);

    r = hm$gets(smap, 1);
    tassert(r != NULL);
    tassert_eq(r->key, 1);
    tassert_eq(r->fooa, 10);

    r = hm$gets(smap, 2);
    tassert(r != NULL);
    tassert_eq(r->key, 2);
    tassert_eq(r->fooa, 200);

    tassert(hm$gets(smap, -1) == NULL);

    tassert(hm$del(smap, 2));
    tassert(hm$del(smap, 1));
    tassert_eq(hm$len(smap), 0);
    tassert(hm$gets(smap, 1) == NULL);
    tassert(hm$gets(smap, 2) == NULL);

    hm$free(smap);
    return EOK;
}

test$case(test_hashmap_keytype)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);
    hm$(i64, int) intmap64 = hm$new(intmap64, mem$);
    hm$(char, int) map1 = hm$new(map1, mem$);
    hm$(char*, int) map2 = hm$new(map2, mem$);
    hm$(const char*, int) map3 = hm$new(map3, mem$);

    struct
    {
        char key[10];
        int value;
    }* map4 = hm$new(map4, mem$);

    hm$(str_s, int) map5 = hm$new(map5, mem$);

    tassert_eq(_cexds__header(intmap)->_hash_table->key_type, _CexDsKeyType__generic);
    tassert_eq(_cexds__header(intmap64)->_hash_table->key_type, _CexDsKeyType__generic);
    tassert_eq(_cexds__header(map1)->_hash_table->key_type, _CexDsKeyType__generic);
    tassert_eq(_cexds__header(map2)->_hash_table->key_type, _CexDsKeyType__charptr);
    tassert_eq(_cexds__header(map3)->_hash_table->key_type, _CexDsKeyType__charptr);
    tassert_eq(_cexds__header(map4)->_hash_table->key_type, _CexDsKeyType__charbuf);
    tassert_eq(_cexds__header(map5)->_hash_table->key_type, _CexDsKeyType__cexstr);

    hm$free(intmap);
    hm$free(intmap64);
    hm$free(map1);
    hm$free(map2);
    hm$free(map3);
    hm$free(map4);
    hm$free(map5);

    return EOK;
}

test$case(test_hashmap_hash)
{
    // We created the same string, but stored in different places
    const char* key = "foobar";
    char key_buf[10] = "foobar";
    char key_buf2[7] = "foobar";
    str_s key_str = str.sbuf(key_buf2, sizeof(key_buf2));

    // Make sure pointers are different
    tassert(str.slice.eq(key_str, str$s("foobar")));
    tassert_eq(sizeof(key_buf2), 7);
    tassert(key_str.buf != key);
    tassert(key_buf != key);
    tassert(key_buf != key_str.buf);

    // mimic how hm$get macro produces initial pointers
    const char* keyvar[1] = { key };

    size_t seed = 27361;
    size_t hash_key = _cexds__hash(_CexDsKeyType__charptr, keyvar, 10000, seed);
    tassert(hash_key > 0);

    tassert_eq(_cexds__hash(_CexDsKeyType__charbuf, key_buf, sizeof(key_buf), seed), hash_key);
    tassert_eq(_cexds__hash(_CexDsKeyType__cexstr, &key_str, sizeof(key_str), seed), hash_key);
    // NOTE: This case is non \0 terminated buffer!
    tassert_eq(_cexds__hash(_CexDsKeyType__charbuf, key_buf2, sizeof(key_buf2), seed), hash_key);

    return EOK;
}

test$case(test_hashmap_string)
{
    char key_buf[10] = "foobar";

    hm$(const char*, int) smap = hm$new(smap, mem$);

    char* k = "foo";
    char* k2 = "baz";

    char key_buf2[10] = "foo";
    char* k3 = key_buf2;
    hm$set(smap, "foo", 3);
    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, k), 3);
    tassert_eq(hm$get(smap, key_buf2), 3);
    tassert_eq(hm$get(smap, k3), 3);

    tassert_eq(hm$get(smap, "bar"), 0);
    tassert_eq(hm$get(smap, k2), 0);
    tassert_eq(hm$get(smap, key_buf), 0);

    tassert_eq(hm$del(smap, key_buf2), 1);
    tassert_eq(hm$len(smap), 0);

    hm$free(smap);
    return EOK;
}

test$case(test_hashmap_bufkey)
{
    struct
    {
        char key[10];
        int value;
    }* smap = hm$new(smap, mem$);

    hm$set(smap, "foo", 3);
    // hm$set(smap, "foobarbazfoo", 4);

    // Doesn't work
    // char* f = "soo";
    // tassert(hm$set(smap, f, 9));

    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    // tassert_eq(hm$get(smap, "barbazfooo0"), 0);

    tassert_eq(hm$del(smap, "foo"), 1);
    tassert_eq(hm$len(smap), 0);
    hm$free(smap);
    return EOK;
}


test$case(test_hashmap_cex_string)
{
    hm$(str_s, int) smap = hm$new(smap, mem$);

    char key_buf[10] = "bar";
    char key_buf2[10] = "baz";

    hm$set(smap, str$s("foo"), 1);
    hm$set(smap, str.sstr(key_buf), 2);
    hm$set(smap, str.sstr(key_buf2), 3);

    tassert_eq(hm$len(smap), 3);
    tassert_eq(hm$get(smap, str$s("foo")), 1);
    tassert_eq(hm$get(smap, str.sstr("bar")), 2);
    tassert_eq(hm$get(smap, str$s("baz")), 3);

    tassert_eq(hm$del(smap, str.sstr("foo")), 1);
    tassert_eq(hm$del(smap, str$s("bar")), 1);
    tassert_eq(hm$del(smap, str$s("baz")), 1);
    tassert_eq(hm$len(smap), 0);

    hm$free(smap);
    return EOK;
}

test$case(test_hashmap_basic_delete)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);
    tassert(intmap != NULL);
    tassert_eq(hm$del(intmap, -1), 0);

    tassert_eq(hm$len(intmap), 0);

    tassert(hm$set(intmap, 1, 10));
    tassert(hm$set(intmap, 2, 20));
    tassert(hm$set(intmap, 3, 30));

    tassert_eq(hm$len(intmap), 3);

    // check order
    tassert_eq(intmap[0].key, 1);
    tassert_eq(intmap[1].key, 2);
    tassert_eq(intmap[2].key, 3);

    // not deleted/found
    tassert_eq(hm$del(intmap, -1), 0);

    tassert_eq(hm$del(intmap, 3), 1);
    tassert_eq(2, hm$len(intmap));
    tassert_eq(intmap[0].key, 1);
    tassert_eq(intmap[1].key, 2);

    tassert_eq(hm$del(intmap, 2), 1);
    tassert_eq(1, hm$len(intmap));
    tassert_eq(intmap[0].key, 1);

    tassert_eq(hm$del(intmap, 1), 1);
    tassert_eq(0, hm$len(intmap));
    // NOTE: this key is not nullified typically out-of-bounds access
    tassert_eq(intmap[0].key, 1);

    tassert_eq(hm$del(intmap, 1), 0);


    tassert(hm$set(intmap, 1, 10));
    tassert(hm$set(intmap, 2, 20));
    tassert(hm$set(intmap, 3, 30));
    tassert_eq(hm$del(intmap, 1), 1);

    // NOTE: swap delete
    tassert_eq(hm$len(intmap), 2);
    tassert_eq(intmap[0].key, 3);
    tassert_eq(intmap[1].key, 2);

    hm$free(intmap);
    return EOK;
}

test$case(test_hashmap_basic_clear)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);
    tassert(intmap != NULL);

    tassert(hm$set(intmap, 1, 10));
    tassert(hm$set(intmap, 2, 20));
    tassert(hm$set(intmap, 3, 30));

    tassert_eq(hm$len(intmap), 3);
    tassert(hm$getp(intmap, 1) != NULL);
    tassert(hm$getp(intmap, 2) != NULL);
    tassert(hm$getp(intmap, 3) != NULL);

    tassert(hm$clear(intmap));
    tassert_eq(hm$len(intmap), 0);
    tassert(hm$getp(intmap, 1) == NULL);
    tassert(hm$getp(intmap, 2) == NULL);
    tassert(hm$getp(intmap, 3) == NULL);

    hm$free(intmap);
    return EOK;
}

test$case(test_hashmap_basic_iteration)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);
    tassert(intmap != NULL);

    tassert(hm$set(intmap, 1, 10));
    tassert(hm$set(intmap, 2, 20));
    tassert(hm$set(intmap, 3, 30));

    tassert_eq(hm$len(intmap), 3);
    tassert_eq(arr$len(intmap), 3);

    u32 nit = 1;
    for$each (it, intmap) {
        tassert_eq(it.key, nit);
        tassert_eq(it.value, nit * 10);
        nit++;
    }
    tassert_eq(nit - 1, arr$len(intmap));

    nit = 1;
    for$eachp(it, intmap)
    {
        tassert_eq(it->key, nit);
        tassert_eq(it->value, nit * 10);
        nit++;
    }
    tassert_eq(nit - 1, arr$len(intmap));

    hm$free(intmap);
    tassert_eq(0, arr$len(intmap));
    tassert_eq(0, hm$len(intmap));
    tassert(intmap == NULL);

    for$eachp(it, intmap)
    {
        (void)it;
        tassert(false && "should never happen");
    }

    return EOK;
}

test$case(test_orig_del_add_clear)
{
    int i = 1;
    const int testsize = 100000;
    hm$(int, int) intmap = hm$new(intmap, mem$);
    tassert(intmap != NULL);

    tassert(hm$get(intmap, i) == 0);
    for (i = 0; i < testsize; i += 2) { hm$set(intmap, i, i * 5); }
    hm$clear(intmap);

    for (i = 0; i < testsize; i += 2) { hm$set(intmap, i, i * 3); }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i, -2) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }
    for (i = 2; i < testsize; i += 4) {
        (void)hm$del(intmap, i); // delete half the entries
    }
    hm$clear(intmap);

    for (i = 0; i < testsize; i += 2) { hm$set(intmap, i, i * 3); }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i, -2) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }

    hm$free(intmap);
    return EOK;
}

test$case(test_mem_scope_lifetime_test)
{
    // NOTE: no mem leak! Temp allocator auto cleanup!
    mem$scope(tmem$, ta)
    {
        arr$(u32) arr = arr$new(arr, ta);
        void* old_arr = arr;
        tassert(old_arr != NULL);
        tassert_eq(arr$cap(arr), 16);
        for (u32 i = 0; i < 16; i++) { arr$push(arr, i); }
        tassert_eq(arr$len(arr), 16);
        tassert_eq(arr$cap(arr), 16);
        mem$scope(tmem$, ta)
        {
            (void)ta;
            uassert_disable();
            arr$pushm(arr, 170, 180, 190);
            // tassert(old_arr = arr);

            tassert_eq(arr$len(arr), 19);
            tassert_eq(arr$cap(arr), 32);

            tassert_eq(arr[16], 170);
            tassert_eq(arr[17], 180);
            tassert_eq(arr[18], 190);
        }
        // WARNING: mem$scope exited => new allocation for arr$pushm marked as poisoned
        //          you'll trigger weird use-after-poison ASAN failure
        tassert(mem$asan_poison_check(&arr[18], sizeof(u32) * 3));
    }
    return EOK;
}

test$case(test_hashmap_string_copy)
{
    hm$(char*, int) smap = hm$new(smap, mem$, .copy_keys = true);

    char key2[10] = "foo";

    hm$set(smap, key2, 3);
    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, key2), 3);
    tassert_eq(smap[0].key, "foo");

    memset(key2, 0, sizeof(key2));
    tassert_eq(smap[0].key, "foo");
    tassert_eq(hm$get(smap, "foo"), 3);

    hm$free(smap);
    return EOK;
}

test$case(test_hashmap_string_copy_custom_struct)
{
    struct
    {
        int value;
        char* key;
    }* smap = hm$new(smap, mem$, .copy_keys = true);

    char key2[10] = "foo";

    hm$set(smap, key2, 3);
    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, key2), 3);
    tassert_eq(smap[0].key, "foo");

    memset(key2, 0, sizeof(key2));
    tassert_eq(smap[0].key, "foo");
    tassert_eq(hm$get(smap, "foo"), 3);

    hm$free(smap);
    return EOK;
}

test$case(test_hashmap_string_copy_del_cleanup)
{
    hm$(char*, int) smap = hm$new(smap, mem$, .copy_keys = true);

    char key2[10] = "foo";

    hm$set(smap, key2, 3);
    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, key2), 3);
    tassert_eq(smap[0].key, "foo");

    memset(key2, 0, sizeof(key2));
    tassert_eq(smap[0].key, "foo");
    tassert_eq(hm$del(smap, "foo"), 1);
    tassert_eq(hm$get(smap, "foo"), 0);

    // Fool hm$free() make it not to cleanup the allocated copy_keys
    auto h = _cexds__header(smap);
    h->_hash_table->copy_keys = false;

    hm$free(smap);
    return EOK;
}

typedef hm$(char* , int) MyHashmap;

struct my_hm_struct {
    MyHashmap hm;
};

test$case(test_hashmap_string_copy_clear_cleanup)
{
    struct my_hm_struct hs = {0};
    hm$new(hs.hm, mem$, .copy_keys = true);

    char key2[10] = "foo";

    hm$set(hs.hm, key2, 3);
    tassert_eq(hm$len(hs.hm), 1);
    tassert_eq(hm$get(hs.hm, "foo"), 3);
    tassert_eq(hm$get(hs.hm, key2), 3);
    tassert_eq(hs.hm[0].key, "foo");

    memset(key2, 0, sizeof(key2));
    tassert_eq(hs.hm[0].key, "foo");
    tassert_eq(hm$clear(hs.hm), 1);
    tassert_eq(hm$get(hs.hm, "foo"), 0);

    // Fool hm$free() make it not to cleanup the allocated copy_keys
    auto h = _cexds__header(hs.hm);
    h->_hash_table->copy_keys = false;

    hm$free(hs.hm);
    return EOK;
}

test$case(test_hashmap_string_copy_arena)
{
    hm$(char*, int) smap = hm$new(smap, mem$, .copy_keys = true, .copy_keys_arena_pgsize = 1024);

    char key2[10] = "foo";

    hm$set(smap, key2, 3);
    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, key2), 3);
    tassert_eq(smap[0].key, "foo");

    memset(key2, 0, sizeof(key2));
    tassert_eq(smap[0].key, "foo");
    tassert_eq(hm$get(smap, "foo"), 3);

    hm$free(smap);
    return EOK;
}


test$case(test_hashmap_string_copy_clear_cleanup_arena)
{
    MyHashmap smap = hm$new(smap, mem$, .copy_keys = true, .copy_keys_arena_pgsize = 1024);
    auto h = _cexds__header(smap);
    AllocatorArena_c* arena = (AllocatorArena_c*)h->_hash_table->key_arena;
    tassert_eq(arena->used, 0);

    char key2[10] = "foo";

    hm$set(smap, key2, 3);
    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, key2), 3);
    tassert_eq(smap[0].key, "foo");
    tassert(h->_hash_table->key_arena != NULL);
    tassert_gt(arena->used, 0);

    memset(key2, 0, sizeof(key2));
    tassert_eq(smap[0].key, "foo");
    tassert_eq(hm$clear(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 0);
    tassert_eq(arena->used, 0);
    tassert(h->_hash_table->key_arena != NULL);

    char key3[10] = "bar";
    hm$set(smap, key3, 2);
    tassert_eq(hm$get(smap, "bar"), 2);
    tassert_eq(hm$get(smap, key3), 2);
    tassert_eq(hm$get(smap, "foo"), 0);
    tassert(h->_hash_table->key_arena != NULL);
    tassert_gt(arena->used, 0);

    // Fool hm$free() make it not to cleanup the allocated copy_keys
    h->_hash_table->copy_keys = false;

    hm$free(smap);
    return EOK;
}
test$main();
