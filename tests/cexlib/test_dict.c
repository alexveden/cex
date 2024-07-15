#include <cex/cexlib/cex.c>
#include <cex/cexlib/allocators.c>
#include <cex/cexlib/dict.c>
#include <cex/cexlib/dict.h>
#include <cex/cexlib/list.c>
#include <cex/cexlib/list.h>
#include <cex/cextest/cextest.h>
#include <cex/cextest/fff.h>
#include <stdalign.h>
#include <stdio.h>

DEFINE_FFF_GLOBALS

FAKE_VALUE_FUNC(void*, my_alloc, size_t)

const Allocator_i* allocator;
/*
 * SUITE INIT / SHUTDOWN
 */
void
my_test_shutdown_func(void)
{
    allocator = allocators.heap.destroy();
}

ATEST_SETUP_F(void)
{
    allocator = allocators.heap.create();

    uassert_enable();
    return &my_test_shutdown_func; // return pointer to void some_shutdown_func(void)
}
/*
 *
 *   TEST SUITE
 *
 */


ATEST_F(test_dict_int64)
{
    struct s
    {
        u64 key;
        char val;
    } rec;

    dict_c hm;
    atassert_eqs(EOK, dict$new(&hm, typeof(rec), key, allocator));

    atassert_eqs(dict.set(&hm, &(struct s){ .key = 123, .val = 'z' }), EOK);

    except(err, dict.set(&hm, &(struct s){ .key = 123, .val = 'z' }))
    {
        atassert(false && "unexpected");
    }

    except(err, dict.set(&hm, &(struct s){ .key = 133, .val = 'z' }))
    {
        atassert(false && "unexpected");
    }

    u64 key = 9890080;

    key = 123;
    const struct s* res = dict.get(&hm, &key);
    atassert(res != NULL);
    atassert(res->key == 123);

    res = dict.geti(&hm, 133);
    atassert(res != NULL);
    atassert(res->key == 133);

    const struct s* res2 = dict.get(&hm, &(struct s){ .key = 222 });
    atassert(res2 == NULL);

    var res3 = (struct s*)dict.get(&hm, &(struct s){ .key = 133 });
    atassert(res3 != NULL);
    atassert_eqi(res3->key, 133);

    atassert_eqi(dict.len(&hm), 2);

    atassert(dict.deli(&hm, 133) != NULL);
    atassert(dict.geti(&hm, 133) == NULL);

    atassert(dict.del(&hm, &(struct s){ .key = 123 }) != NULL);
    atassert(dict.geti(&hm, 123) == NULL);

    atassert(dict.deli(&hm, 12029381038) == NULL);

    atassert_eqi(dict.len(&hm), 0);

    dict.destroy(&hm);

    return NULL;
}

ATEST_F(test_dict_string)
{
    struct s
    {
        char key[30];
        char val;
    };

    dict_c hm;
    atassert_eqs(EOK, dict$new(&hm, struct s, key, allocator));

    atassert_eqs(dict.set(&hm, &(struct s){ .key = "abcd", .val = 'z' }), EOK);

    except(err, dict.set(&hm, &(struct s){ .key = "abcd", .val = 'z' }))
    {
        atassert(false && "unexpected");
    }

    except(err, dict.set(&hm, &(struct s){ .key = "xyz", .val = 'z' }))
    {
        atassert(false && "unexpected");
    }

    const struct s* res = dict.get(&hm, "abcd");
    atassert(res != NULL);
    atassert_eqs(res->key, "abcd");

    res = dict.get(&hm, "xyz");
    atassert(res != NULL);
    atassert_eqs(res->key, "xyz");

    const struct s* res2 = dict.get(&hm, &(struct s){ .key = "ffff" });
    atassert(res2 == NULL);


    var res3 = (struct s*)dict.get(&hm, &(struct s){ .key = "abcd" });
    atassert(res3 != NULL);
    atassert_eqs(res3->key, "abcd");

    atassert_eqi(dict.len(&hm), 2);

    atassert(dict.del(&hm, "xyznotexisting") == NULL);

    atassert(dict.del(&hm, &(struct s){ .key = "abcd" }) != NULL);
    atassert(dict.get(&hm, "abcd") == NULL);
    atassert(dict.del(&hm, "xyz") != NULL);
    atassert(dict.get(&hm, "xyz") == NULL);
    atassert_eqi(dict.len(&hm), 0);

    dict.destroy(&hm);

    return NULL;
}


ATEST_F(test_dict_create_generic)
{
    struct s
    {
        char struct_first_key[30];
        u64 another_key;
        char val;
    } rec;

    dict_c hm;
    // WARNING: default dict_c forces keys to be at the beginning of the struct,
    //  if such key passed -> Error.integrity
    atassert_eqs(Error.integrity, dict$new(&hm, typeof(rec), another_key, allocator));

    atassert_eqs(EOK, dict$new(&hm, typeof(rec), struct_first_key, allocator));

    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "abcd", .val = 'a' }), EOK);
    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "xyz", .val = 'z' }), EOK);

    atassert_eqi(dict.len(&hm), 2);

    const struct s* result = dict.get(&hm, "xyz");
    atassert(result != NULL);
    atassert_eqs(result->struct_first_key, "xyz");
    atassert_eqi(result->val, 'z');

    struct s* res = (struct s*)result;

    // NOTE: it's possible to edit data in dict, as long as you don't touch key!
    res->val = 'f';

    result = dict.get(&hm, "xyz");
    atassert(result != NULL);
    atassert_eqs(result->struct_first_key, "xyz");
    atassert_eqi(result->val, 'f');

    // WARNING: If you try to edit the key it will be lost
    strcpy(res->struct_first_key, "foo");

    result = dict.get(&hm, "foo");
    atassert(result == NULL);
    // WARNING: old key is also lost!
    result = dict.get(&hm, "xyz");
    atassert(result == NULL);

    atassert_eqi(dict.len(&hm), 2);

    dict.destroy(&hm);
    atassert(hm.hashmap == NULL);
    return NULL;
}


ATEST_F(test_dict_generic_auto_cmp_hash)
{

    struct s
    {
        char key[30];
        u64 key_u64;
        char* key_ptr;
        char val;
        str_c cexstr;
    } rec;
    (void)rec;

    atassert(_dict$hashfunc(typeof(rec), key_u64) == hm_int_hash);
    atassert(_dict$hashfunc(typeof(rec), key) == hm_str_static_hash);

    // NOTE: these are intentionally unsupported (because we store copy of data, and passing
    // but passing pointers may leave them dangling, or use-after-free)
    // atassert(_dict$hashfunc(typeof(rec), cexstr) == NULL);
    // atassert(_dict$hashfunc(typeof(rec), key_ptr) == NULL);
    return NULL;
}

ATEST_F(test_dict_iter)
{
    struct s
    {
        char struct_first_key[30];
        u64 another_key;
        char val;
    } rec;

    dict_c hm;
    atassert_eqs(EOK, dict$new(&hm, typeof(rec), struct_first_key, allocator));

    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "foo", .val = 'a' }), EOK);
    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "abcd", .val = 'b' }), EOK);
    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "xyz", .val = 'c' }), EOK);
    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "bar", .val = 'd' }), EOK);
    atassert_eqi(dict.len(&hm), 4);

    u32 nit = 0;
    for$iter(typeof(rec), it, dict.iter(&hm, &it.iterator))
    {
        struct s* r = dict.get(&hm, it.val->struct_first_key);
        atassert(r != NULL);
        atassert_eqi(it.idx.i, nit);
        nit++;
    }
    atassert_eqi(nit, 4);

    nit = 0;
    uassert_disable();
    for$iter(typeof(rec), it, dict.iter(&hm, &it.iterator))
    {
        // WARNING: changing of dict during iterator is not allowed, you'll get assert
        dict.clear(&hm);
        nit++;
    }
    atassert_eqi(nit, 1);

    dict.destroy(&hm);
    return NULL;
}

ATEST_F(test_dict_tolist)
{
    struct s
    {
        char struct_first_key[30];
        u64 another_key;
        char val;
    };

    dict_c hm;
    atassert_eqs(EOK, dict$new(&hm, struct s, struct_first_key, allocator));

    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "foo", .val = 'a' }), EOK);
    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "abcd", .val = 'b' }), EOK);
    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "xyz", .val = 'c' }), EOK);
    atassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "bar", .val = 'd' }), EOK);
    atassert_eqi(dict.len(&hm), 4);

    list$define(struct s) a;
    atassert_eqs(EOK, dict.tolist(&hm, &a, allocator));
    atassert_eqi(list.len(&a), 4);

    atassert_eqs(Error.argument, dict.tolist(NULL, &a, allocator));
    atassert_eqs(Error.argument, dict.tolist(&hm, NULL, allocator));
    atassert_eqs(Error.argument, dict.tolist(&hm, &a, NULL));

    for$array(it, a.arr, a.len) {
        atassert(dict.get(&hm, it.val) != NULL);
        atassert(dict.get(&hm, it.val->struct_first_key) != NULL);
        // same elements buf different pointer - means copy!
        atassert(dict.get(&hm, it.val->struct_first_key) != it.val);
    }

    dict.destroy(&hm);
    atassert_eqs(Error.integrity, dict.tolist(&hm, &a, allocator));

    list.destroy(&a);

    return NULL;
}
/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    ATEST_PARSE_MAINARGS(argc, argv);
    ATEST_PRINT_HEAD();  // >>> all tests below
    
    ATEST_RUN(test_dict_int64);
    ATEST_RUN(test_dict_string);
    ATEST_RUN(test_dict_create_generic);
    ATEST_RUN(test_dict_generic_auto_cmp_hash);
    ATEST_RUN(test_dict_iter);
    ATEST_RUN(test_dict_tolist);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
