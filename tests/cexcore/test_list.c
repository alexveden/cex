#include <_cexcore/cextest.h>
#include <_cexcore/cex.c>
#include <_cexcore/list.c>
#include <_cexcore/list.h>
#include <_cexcore/allocators.c>
#include <stdio.h>

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

test$case(testlist_alloc_capacity)
{
    // 4 is minimum
    tassert_eql(4, list__alloc_capacity(1));
    tassert_eql(4, list__alloc_capacity(2));
    tassert_eql(4, list__alloc_capacity(3));
    tassert_eql(4, list__alloc_capacity(4));

    // up to 1024 grow with factor of 2
    tassert_eql(8, list__alloc_capacity(5));
    tassert_eql(16, list__alloc_capacity(12));
    tassert_eql(64, list__alloc_capacity(45));
    tassert_eql(1024, list__alloc_capacity(1023));

    // after 1024 increase by 20%
    tassert_eql(1024 * 1.2, list__alloc_capacity(1024));
    u64 expected_cap = 1000000 * 1.2;
    u64 alloc_cap = list__alloc_capacity(1000000);
    tassert(expected_cap-alloc_cap <= 1); // x32 - may have precision rounding

    return EOK;
}

test$case(testlist_new)
{
    list$define(int) a;

    except(err, list$new(&a, 5, allocator))
    {
        tassert(false && "list$new fail");
    }

    tassert(a.arr != NULL);
    tassert_eqi(a.len, 0);

    list_head_s* head = (list_head_s*)((char*)a.arr - _CEX_LIST_BUF);
    tassert_eqi(head->header.magic, 0x1eed);
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 8);
    tassert(head->allocator == allocator);

    list.destroy(&a);
    return EOK;

}

test$case(testlist_append)
{
    list$define(int) a;

    except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        tassert_eqs(EOK, list.append(&a, &i));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        tassert_eqs(EOK, list.append(&a, &i));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a);
    return EOK;

}

test$case(testlist_insert)
{
    list$define(int) a;

    except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    tassert_eqs(EOK, list.append(&a, &(int){1}));
    tassert_eqs(EOK, list.append(&a, &(int){2}));
    tassert_eqs(EOK, list.append(&a, &(int){3}));
    tassert_eqi(a.len, 3);

    tassert_eqs(Error.argument, list.insert(&a, &(int){4}, 4));
    tassert_eqs(Error.ok, list.insert(&a, &(int){4}, 3)); // same as append!
    //
    tassert_eqi(a.len, 4);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);


    list.clear(&a);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list.append(&a, &(int){1}));
    tassert_eqs(EOK, list.append(&a, &(int){2}));
    tassert_eqs(EOK, list.append(&a, &(int){3}));
    tassert_eqs(Error.ok, list.insert(&a, &(int){4}, 0)); // same as append!
    tassert_eqi(a.len, 4);
    tassert_eqi(a.arr[0], 4);
    tassert_eqi(a.arr[1], 1);
    tassert_eqi(a.arr[2], 2);
    tassert_eqi(a.arr[3], 3);

    list.clear(&a);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list.append(&a, &(int){1}));
    tassert_eqs(EOK, list.append(&a, &(int){2}));
    tassert_eqs(EOK, list.append(&a, &(int){3}));
    tassert_eqs(Error.ok, list.insert(&a, &(int){4}, 1)); // same as append!
    tassert_eqi(a.len, 4);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 4);
    tassert_eqi(a.arr[2], 2);
    tassert_eqi(a.arr[3], 3);

    list.destroy(&a);
    return EOK;

}

test$case(testlist_del)
{
    list$define(int) a;

    except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }
    tassert_eqs(Error.argument, list.del(&a, 0));

    // adding new elements into the end
    tassert_eqs(EOK, list.append(&a, &(int){1}));
    tassert_eqs(EOK, list.append(&a, &(int){2}));
    tassert_eqs(EOK, list.append(&a, &(int){3}));
    tassert_eqi(a.len, 3);
    tassert_eqs(Error.argument, list.del(&a, 3));
    tassert_eqs(Error.ok, list.del(&a, 2));

    tassert_eqi(a.len, 2);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);

    list.clear(&a);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list.append(&a, &(int){1}));
    tassert_eqs(EOK, list.append(&a, &(int){2}));
    tassert_eqs(EOK, list.append(&a, &(int){3}));
    tassert_eqs(EOK, list.append(&a, &(int){4})); 

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);

    tassert_eqs(Error.ok, list.del(&a, 2));

    tassert_eqi(a.len, 3);
    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 4);

    list.clear(&a);
    tassert_eqi(a.len, 0);
    tassert_eqs(EOK, list.append(&a, &(int){1}));
    tassert_eqs(EOK, list.append(&a, &(int){2}));
    tassert_eqs(EOK, list.append(&a, &(int){3}));
    tassert_eqs(EOK, list.append(&a, &(int){4})); 

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);

    tassert_eqs(Error.ok, list.del(&a, 0));

    tassert_eqi(a.len, 3);
    tassert_eqi(a.arr[0], 2);
    tassert_eqi(a.arr[1], 3);
    tassert_eqi(a.arr[2], 4);

    list.destroy(&a);
    return EOK;

}

int test_int_cmp(const void* a, const void* b){
    return *(int*)a - *(int*)b; 
}

test$case(testlist_sort)
{
    list$define(int) a;

    except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    tassert_eqs(EOK, list.append(&a, &(int){5}));
    tassert_eqs(EOK, list.append(&a, &(int){1}));
    tassert_eqs(EOK, list.append(&a, &(int){3}));
    tassert_eqs(EOK, list.append(&a, &(int){2}));
    tassert_eqs(EOK, list.append(&a, &(int){4})); 
    tassert_eqi(a.len, 5);


    list.sort(&a, test_int_cmp);
    tassert_eqi(a.len, 5);

    tassert_eqi(a.arr[0], 1);
    tassert_eqi(a.arr[1], 2);
    tassert_eqi(a.arr[2], 3);
    tassert_eqi(a.arr[3], 4);
    tassert_eqi(a.arr[4], 5);


    list.destroy(&a);
    return EOK;

}

test$case(testlist_extend)
{

    list$define(int) a;
    except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    int arr[4] = { 0, 1, 2, 3 };
    // a.arr = arr;
    // a.len = 1;

    tassert_eqs(EOK, list.extend(&a, arr, arr$len(arr)));
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    int arr2[4] = { 4, 5, 6, 7 };
    // triggers resize
    tassert_eqs(EOK, list.extend(&a, arr2, arr$len(arr2)));

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a);
    return EOK;

}

test$case(testlist_iterator)
{

    list$define(int) a;
    except(err, list$new(&a,  4, allocator))
    {
        tassert(false && "list$new fail");
    }

    u32 nit = 0;
    for$iter(int, it, list.iter(&a, &it.iterator))
    {
        tassert(false && "not expected");
        nit++;
    }
    tassert_eqi(nit, 0);

    int arr[4] = { 0, 1, 2, 300 };
    nit = 0;
    for$array(it, arr, arr$len(arr))
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);

    struct tstruct
    {
        u32 foo;
        char* bar;
    } tarr[2] = {
        { .foo = 1 },
        { .foo = 2 },
    };
    nit = 0;
    for$array(it, tarr, arr$len(tarr))
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(it.val->foo, nit+1);
        nit++;
    }
    tassert_eqi(nit, 2);

    nit = 0;
    for$array(it, tarr, arr$len(tarr))
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(it.val->foo, nit+1);
        nit++;
    }
    tassert_eqi(nit, 2);
    tassert_eqs(EOK, list.extend(&a, arr, arr$len(arr)));
    tassert_eqi(a.len, 4);

    nit = 0;
    for$iter(*a.arr, it, list.iter(&a, &it.iterator))
    {
        tassert_eqi(it.idx.i, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);

    nit = 0;
    for$array(it, a.arr, a.len)
    {
        tassert_eqi(it.idx, nit);
        tassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    tassert_eqi(nit, 4);

    list.destroy(&a);

    return EOK;
}

test$case(testlist_align256)
{

    struct foo64
    {
        alignas(256) size_t foo;
    };
    _Static_assert(sizeof(struct foo64) == 256, "size");
    _Static_assert(alignof(struct foo64) == 256, "align");

    list$define(struct foo64) a;

    except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = {.foo = i};
        tassert_eqs(EOK, list.append(&a, &f));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(head->header.elalign, 256);

    // pointer is aligned!
    tassert_eqi(0, (size_t)&a.arr[0] % head->header.elalign);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = {.foo = i};
        tassert_eqs(EOK, list.append(&a, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a);
    return EOK;

}

test$case(testlist_align64)
{

    struct foo64
    {
        alignas(64) size_t foo;
    };
    _Static_assert(sizeof(struct foo64) == 64, "size");
    _Static_assert(alignof(struct foo64) == 64, "align");

    list$define(struct foo64) a;

    except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = {.foo = i};
        tassert_eqs(EOK, list.append(&a, &f));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(head->header.elalign, 64);

    // pointer is aligned!
    tassert_eqi(0, (size_t)&a.arr[0] % head->header.elalign);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = {.foo = i};
        tassert_eqs(EOK, list.append(&a, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a);
    return EOK;

}

test$case(testlist_align16)
{

    struct foo64
    {
        alignas(16) size_t foo;
    };
    _Static_assert(sizeof(struct foo64) == 16, "size");
    _Static_assert(alignof(struct foo64) == 16, "align");

    list$define(struct foo64) a;

    except(err, list$new(&a, 4, allocator))
    {
        tassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = {.foo = i};
        tassert_eqs(EOK, list.append(&a, &f));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);
    tassert_eqi(head->header.elalign, 16);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = {.foo = i};
        tassert_eqs(EOK, list.append(&a, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i);
    }

    tassert_eqi(a.len, 8);
    head = list__head((list_c*)&a);
    tassert_eqi(head->count, 8);
    tassert_eqi(head->capacity, 8);

    list.destroy(&a);
    return EOK;

}

test$case(testlist_append_static)
{
    list$define(int) a;

    alignas(32) char buf[_CEX_LIST_BUF + sizeof(int)*4];

    tassert_eqs(EOK, list$new_static(&a, buf, arr$len(buf)));
    tassert_eqi(list.capacity(&a), 4);

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        tassert_eqs(EOK, list.append(&a, &i));
    }
    tassert_eqi(a.len, 4);
    list_head_s* head = list__head((list_c*)&a);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i], i);
    }

    tassert_eqs(Error.overflow, list.append(&a, &(int){1}));
    tassert_eqs(Error.overflow, list.extend(&a, a.arr, a.len));

    tassert_eqi(a.len, 4);
    tassert_eqi(head->count, 4);
    tassert_eqi(head->capacity, 4);

    list.destroy(&a);
    // header reset to 0
    tassert_eqi(head->header.magic, 0);
    tassert_eqi(head->count, 0);
    tassert_eqi(head->capacity, 0);
    return EOK;

}

test$case(testlist_static_buffer_validation)
{
    list$define(int) a;

    alignas(32) char buf[_CEX_LIST_BUF + sizeof(int)*1];

    tassert_eqs(EOK, list$new_static(&a, buf, arr$len(buf)));
    tassert_eqi(list.capacity(&a), 1);

    // No capacity for 1 element
    tassert_eqs(Error.overflow, list$new_static(&a, buf, arr$len(buf)-1));

    list.destroy(&a);
    return EOK;

}

test$case(testlist_static_with_alignment)
{
    struct foo64
    {
        alignas(64) size_t foo;
    };
    _Static_assert(sizeof(struct foo64) == 64, "size");
    _Static_assert(alignof(struct foo64) == 64, "align");

    list$define(struct foo64) a;

    alignas(64) char buf[sizeof(list_head_s) + 64*2];
    size_t unalign = 1;
    char* unaligned = buf + unalign;

    tassert_eqs(EOK, list$new_static(&a, unaligned, arr$len(buf)-unalign));
    tassert_eqi(list.capacity(&a), 1);

    // adding new elements into the end
    for (u32 i = 0; i < list.capacity(&a); i++) {
        tassert_eqs(EOK, list.append(&a, &(struct foo64){.foo = i+1}));
    }

    // Address is aligned to 64
    tassert_eqi(0, (size_t)(a.arr) % _Alignof(struct foo64));

    tassert_eqi(a.len, 1);
    list_head_s* head = list__head((list_c*)&a);
    tassert_eqi(((char*)a.arr - (char*)head), _CEX_LIST_BUF);
    tassert_eqi(((char*)a.arr - (char*)buf), 64);
    tassert_eqi(((char*)head - (char*)unaligned), 31);

    tassert_eqi(head->count, 1);
    tassert_eqi(head->capacity, 1);

    // validate appended values
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i+1);
    }

    tassert_eqs(Error.overflow, list.append(&a, &(struct foo64){.foo = 1}));
    tassert_eqs(Error.overflow, list.extend(&a, &(struct foo64){.foo = 1}, 1));

    tassert_eqi(a.len, 1);
    tassert_eqi(head->count, 1);
    tassert_eqi(head->capacity, 1);

    // wreck head to make sure we didn't touch the aligned data
    memset(head, 0xff, sizeof(*head));
    for (u32 i = 0; i < a.len; i++) {
        tassert_eqi(a.arr[i].foo, i+1);
    }

    // list.destroy(&a);
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
    
    test$run(testlist_alloc_capacity);
    test$run(testlist_new);
    test$run(testlist_append);
    test$run(testlist_insert);
    test$run(testlist_del);
    test$run(testlist_sort);
    test$run(testlist_extend);
    test$run(testlist_iterator);
    test$run(testlist_align256);
    test$run(testlist_align64);
    test$run(testlist_align16);
    test$run(testlist_append_static);
    test$run(testlist_static_buffer_validation);
    test$run(testlist_static_with_alignment);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
