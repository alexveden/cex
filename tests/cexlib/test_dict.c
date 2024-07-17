#include <_cexlib/cex.c>
#include <_cexlib/allocators.c>
#include <_cexlib/dict.c>
#include <_cexlib/dict.h>
#include <_cexlib/list.c>
#include <_cexlib/list.h>
#include <_cexlib/cextest.h>
#include <fff.h>
#include <stdalign.h>
#include <stdio.h>

DEFINE_FFF_GLOBALS

FAKE_VALUE_FUNC(void*, my_alloc, size_t)

const Allocator_i* allocator;

/*
* SUITE INIT / SHUTDOWN
*/
test$teardown(){
    allocator = allocators.heap.destroy();
    return EOK;
}

test$setup()
{
    uassert_enable();
    allocator = allocators.heap.create();
    return EOK;
}

/*
 *
 *   TEST SUITE
 *
 */


test$case(test_dict_int64)
{
    struct s
    {
        u64 key;
        char val;
    } rec;

    dict_c hm;
    tassert_eqs(EOK, dict$new(&hm, typeof(rec), key, allocator));

    tassert_eqs(dict.set(&hm, &(struct s){ .key = 123, .val = 'z' }), EOK);

    except(err, dict.set(&hm, &(struct s){ .key = 123, .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    except(err, dict.set(&hm, &(struct s){ .key = 133, .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    u64 key = 9890080;

    key = 123;
    const struct s* res = dict.get(&hm, &key);
    tassert(res != NULL);
    tassert(res->key == 123);

    res = dict.geti(&hm, 133);
    tassert(res != NULL);
    tassert(res->key == 133);

    const struct s* res2 = dict.get(&hm, &(struct s){ .key = 222 });
    tassert(res2 == NULL);

    var res3 = (struct s*)dict.get(&hm, &(struct s){ .key = 133 });
    tassert(res3 != NULL);
    tassert_eqi(res3->key, 133);

    tassert_eqi(dict.len(&hm), 2);

    tassert(dict.deli(&hm, 133) != NULL);
    tassert(dict.geti(&hm, 133) == NULL);

    tassert(dict.del(&hm, &(struct s){ .key = 123 }) != NULL);
    tassert(dict.geti(&hm, 123) == NULL);

    tassert(dict.deli(&hm, 12029381038) == NULL);

    tassert_eqi(dict.len(&hm), 0);

    dict.destroy(&hm);

    return EOK;
}

test$case(test_dict_string)
{
    struct s
    {
        char key[30];
        char val;
    };

    dict_c hm;
    tassert_eqs(EOK, dict$new(&hm, struct s, key, allocator));

    tassert_eqs(dict.set(&hm, &(struct s){ .key = "abcd", .val = 'z' }), EOK);

    except(err, dict.set(&hm, &(struct s){ .key = "abcd", .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    except(err, dict.set(&hm, &(struct s){ .key = "xyz", .val = 'z' }))
    {
        tassert(false && "unexpected");
    }

    const struct s* res = dict.get(&hm, "abcd");
    tassert(res != NULL);
    tassert_eqs(res->key, "abcd");

    res = dict.get(&hm, "xyz");
    tassert(res != NULL);
    tassert_eqs(res->key, "xyz");

    const struct s* res2 = dict.get(&hm, &(struct s){ .key = "ffff" });
    tassert(res2 == NULL);


    var res3 = (struct s*)dict.get(&hm, &(struct s){ .key = "abcd" });
    tassert(res3 != NULL);
    tassert_eqs(res3->key, "abcd");

    tassert_eqi(dict.len(&hm), 2);

    tassert(dict.del(&hm, "xyznotexisting") == NULL);

    tassert(dict.del(&hm, &(struct s){ .key = "abcd" }) != NULL);
    tassert(dict.get(&hm, "abcd") == NULL);
    tassert(dict.del(&hm, "xyz") != NULL);
    tassert(dict.get(&hm, "xyz") == NULL);
    tassert_eqi(dict.len(&hm), 0);

    dict.destroy(&hm);

    return EOK;
}


test$case(test_dict_create_generic)
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
    tassert_eqs(Error.integrity, dict$new(&hm, typeof(rec), another_key, allocator));

    tassert_eqs(EOK, dict$new(&hm, typeof(rec), struct_first_key, allocator));

    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "abcd", .val = 'a' }), EOK);
    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "xyz", .val = 'z' }), EOK);

    tassert_eqi(dict.len(&hm), 2);

    const struct s* result = dict.get(&hm, "xyz");
    tassert(result != NULL);
    tassert_eqs(result->struct_first_key, "xyz");
    tassert_eqi(result->val, 'z');

    struct s* res = (struct s*)result;

    // NOTE: it's possible to edit data in dict, as long as you don't touch key!
    res->val = 'f';

    result = dict.get(&hm, "xyz");
    tassert(result != NULL);
    tassert_eqs(result->struct_first_key, "xyz");
    tassert_eqi(result->val, 'f');

    // WARNING: If you try to edit the key it will be lost
    strcpy(res->struct_first_key, "foo");

    result = dict.get(&hm, "foo");
    tassert(result == NULL);
    // WARNING: old key is also lost!
    result = dict.get(&hm, "xyz");
    tassert(result == NULL);

    tassert_eqi(dict.len(&hm), 2);

    dict.destroy(&hm);
    tassert(hm.hashmap == NULL);
    return EOK;
}


test$case(test_dict_generic_auto_cmp_hash)
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

    tassert(_dict$hashfunc(typeof(rec), key_u64) == hm_int_hash);
    tassert(_dict$hashfunc(typeof(rec), key) == hm_str_static_hash);

    // NOTE: these are intentionally unsupported (because we store copy of data, and passing
    // but passing pointers may leave them dangling, or use-after-free)
    // tassert(_dict$hashfunc(typeof(rec), cexstr) == NULL);
    // tassert(_dict$hashfunc(typeof(rec), key_ptr) == NULL);
    return EOK;
}

test$case(test_dict_iter)
{
    struct s
    {
        char struct_first_key[30];
        u64 another_key;
        char val;
    } rec;

    dict_c hm;
    tassert_eqs(EOK, dict$new(&hm, typeof(rec), struct_first_key, allocator));

    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "foo", .val = 'a' }), EOK);
    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "abcd", .val = 'b' }), EOK);
    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "xyz", .val = 'c' }), EOK);
    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "bar", .val = 'd' }), EOK);
    tassert_eqi(dict.len(&hm), 4);

    u32 nit = 0;
    for$iter(typeof(rec), it, dict.iter(&hm, &it.iterator))
    {
        struct s* r = dict.get(&hm, it.val->struct_first_key);
        tassert(r != NULL);
        tassert_eqi(it.idx.i, nit);
        nit++;
    }
    tassert_eqi(nit, 4);

    nit = 0;
    uassert_disable();
    for$iter(typeof(rec), it, dict.iter(&hm, &it.iterator))
    {
        // WARNING: changing of dict during iterator is not allowed, you'll get assert
        dict.clear(&hm);
        nit++;
    }
    tassert_eqi(nit, 1);

    dict.destroy(&hm);
    return EOK;
}

test$case(test_dict_tolist)
{
    struct s
    {
        char struct_first_key[30];
        u64 another_key;
        char val;
    };

    dict_c hm;
    tassert_eqs(EOK, dict$new(&hm, struct s, struct_first_key, allocator));

    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "foo", .val = 'a' }), EOK);
    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "abcd", .val = 'b' }), EOK);
    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "xyz", .val = 'c' }), EOK);
    tassert_eqs(dict.set(&hm, &(struct s){ .struct_first_key = "bar", .val = 'd' }), EOK);
    tassert_eqi(dict.len(&hm), 4);

    list$define(struct s) a;
    tassert_eqs(EOK, dict.tolist(&hm, &a, allocator));
    tassert_eqi(list.len(&a), 4);

    tassert_eqs(Error.argument, dict.tolist(NULL, &a, allocator));
    tassert_eqs(Error.argument, dict.tolist(&hm, NULL, allocator));
    tassert_eqs(Error.argument, dict.tolist(&hm, &a, NULL));

    for$array(it, a.arr, a.len) {
        tassert(dict.get(&hm, it.val) != NULL);
        tassert(dict.get(&hm, it.val->struct_first_key) != NULL);
        // same elements buf different pointer - means copy!
        tassert(dict.get(&hm, it.val->struct_first_key) != it.val);
    }

    dict.destroy(&hm);
    tassert_eqs(Error.integrity, dict.tolist(&hm, &a, allocator));

    list.destroy(&a);

    return EOK;
}
/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_dict_int64);
    test$run(test_dict_string);
    test$run(test_dict_create_generic);
    test$run(test_dict_generic_auto_cmp_hash);
    test$run(test_dict_iter);
    test$run(test_dict_tolist);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
