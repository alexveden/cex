#include <cex/cextest/cextest.h>
#include <cex/cex.h>
#include <cex/cexlib/list.c>
#include <cex/cexlib/list.h>
#include <cex/cexlib/allocators.c>
#include <stdio.h>

/*
 * SUITE INIT / SHUTDOWN
 */
void
my_test_shutdown_func(void)
{
    // instead of printf(....) consider to use alogf() it's more structural and contains line
    // source:lnumber reference atlogf("atest_shutdown()\n");
}

ATEST_SETUP_F(void)
{
    // atlogf("atest_setup()\n");

    // return NULL;   // if no shutdown logic needed
    return &my_test_shutdown_func; // return pointer to void some_shutdown_func(void)
}

/*
 *
 *   TEST SUITE
 *
 */

ATEST_F(testlist_alloc_capacity)
{
    // 4 is minimum
    atassert_eql(4, list__alloc_capacity(1));
    atassert_eql(4, list__alloc_capacity(2));
    atassert_eql(4, list__alloc_capacity(3));
    atassert_eql(4, list__alloc_capacity(4));

    // up to 1024 grow with factor of 2
    atassert_eql(8, list__alloc_capacity(5));
    atassert_eql(16, list__alloc_capacity(12));
    atassert_eql(64, list__alloc_capacity(45));
    atassert_eql(1024, list__alloc_capacity(1023));

    // after 1024 increase by 20%
    atassert_eql(1024 * 1.2, list__alloc_capacity(1024));
    atassert_eql(1000000 * 1.2, list__alloc_capacity(1000000));

    return NULL;
}

ATEST_F(testlist_new)
{
    const Allocator_c* allocator = AllocatorHeap_new();
    list$define(int) a;

    except_traceback(err, list$new(&a, 5, allocator))
    {
        atassert(false && "list$new fail");
    }

    atassert(a.arr != NULL);
    atassert_eqi(a.count, 0);

    list_head_s* head = (list_head_s*)((char*)a.arr - sizeof(list_head_s));
    atassert_eqi(head->header.magic, 0x1eed);
    atassert_eqi(head->header.elalign, alignof(int));
    atassert_eqi(head->header.elsize, sizeof(int));
    atassert_eqi(head->count, 0);
    atassert_eqi(head->capacity, 8);
    atassert(head->allocator == allocator);

    dlist.destroy(&a);
    AllocatorHeap_free();

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(testlist_append)
{
    const Allocator_c* allocator = AllocatorHeap_new();
    list$define(int) a;

    except_traceback(err, list$new(&a, 4, allocator))
    {
        atassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        atassert_eqs(EOK, dlist.append(&a, &i));
    }
    atassert_eqi(a.count, 4);
    list_head_s* head = list__head((list_c*)&a);
    atassert_eqi(head->count, 4);
    atassert_eqi(head->capacity, 4);

    // validate appended values
    for (u32 i = 0; i < a.count; i++) {
        atassert_eqi(a.arr[i], i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        atassert_eqs(EOK, dlist.append(&a, &i));
    }

    // validate appended values
    for (u32 i = 0; i < a.count; i++) {
        atassert_eqi(a.arr[i], i);
    }

    atassert_eqi(a.count, 8);
    head = list__head((list_c*)&a);
    atassert_eqi(head->count, 8);
    atassert_eqi(head->capacity, 8);

    dlist.destroy(&a);
    AllocatorHeap_free();

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(testlist_extend)
{
    const Allocator_c* allocator = AllocatorHeap_new();

    list$define(int) a;
    except_traceback(err, list$new(&a, 4, allocator))
    {
        atassert(false && "list$new fail");
    }

    int arr[4] = { 0, 1, 2, 3 };
    // a.arr = arr;
    // a.count = 1;

    atassert_eqs(EOK, dlist.extend(&a, arr, arr$len(arr)));
    atassert_eqi(a.count, 4);
    list_head_s* head = list__head((list_c*)&a);
    atassert_eqi(head->count, 4);
    atassert_eqi(head->capacity, 4);

    // validate appended values
    for (u32 i = 0; i < a.count; i++) {
        atassert_eqi(a.arr[i], i);
    }

    int arr2[4] = { 4, 5, 6, 7 };
    // triggers resize
    atassert_eqs(EOK, dlist.extend(&a, arr2, arr$len(arr2)));

    // validate appended values
    for (u32 i = 0; i < a.count; i++) {
        atassert_eqi(a.arr[i], i);
    }

    atassert_eqi(a.count, 8);
    head = list__head((list_c*)&a);
    atassert_eqi(head->count, 8);
    atassert_eqi(head->capacity, 8);

    dlist.destroy(&a);
    AllocatorHeap_free();

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(testlist_iterator)
{
    const Allocator_c* allocator = AllocatorHeap_new();

    list$define(int) a;
    except_traceback(err, list$new(&a,  4, allocator))
    {
        atassert(false && "list$new fail");
    }

    u32 nit = 0;
    for$iter(int, it, dlist.iter(&a, &it.iterator))
    {
        atassert(false && "not expected");
        nit++;
    }
    atassert_eqi(nit, 0);

    int arr[4] = { 0, 1, 2, 300 };
    nit = 0;
    for$array(it, arr, arr$len(arr))
    {
        atassert_eqi(it.idx, nit);
        atassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    atassert_eqi(nit, 4);

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
        atassert_eqi(it.idx, nit);
        atassert_eqi(it.val->foo, nit+1);
        nit++;
    }
    atassert_eqi(nit, 2);

    nit = 0;
    for$array(it, tarr, arr$len(tarr))
    {
        atassert_eqi(it.idx, nit);
        atassert_eqi(it.val->foo, nit+1);
        nit++;
    }
    atassert_eqi(nit, 2);
    atassert_eqs(EOK, dlist.extend(&a, arr, arr$len(arr)));
    atassert_eqi(a.count, 4);

    nit = 0;
    for$iter(*a.arr, it, dlist.iter(&a, &it.iterator))
    {
        atassert_eqi(it.idx.i, nit);
        atassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    atassert_eqi(nit, 4);

    nit = 0;
    for$array(it, a.arr, a.count)
    {
        atassert_eqi(it.idx, nit);
        atassert_eqi(*it.val, arr[nit]);
        nit++;
    }
    atassert_eqi(nit, 4);

    dlist.destroy(&a);
    AllocatorHeap_free();

    return NULL;
}


ATEST_F(testlist_align64)
{
    const Allocator_c* allocator = AllocatorHeap_new();

    struct foo64
    {
        alignas(64) size_t foo;
    };
    _Static_assert(sizeof(struct foo64) == 64, "size");
    _Static_assert(alignof(struct foo64) == 64, "align");

    list$define(struct foo64) a;

    except_traceback(err, list$new(&a, 4, allocator))
    {
        atassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = {.foo = i};
        atassert_eqs(EOK, dlist.append(&a, &f));
    }
    atassert_eqi(a.count, 4);
    list_head_s* head = list__head((list_c*)&a);
    atassert_eqi(head->count, 4);
    atassert_eqi(head->capacity, 4);
    atassert_eqi(head->header.elalign, 64);

    // validate appended values
    for (u32 i = 0; i < a.count; i++) {
        atassert_eqi(a.arr[i].foo, i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = {.foo = i};
        atassert_eqs(EOK, dlist.append(&a, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.count; i++) {
        atassert_eqi(a.arr[i].foo, i);
    }

    atassert_eqi(a.count, 8);
    head = list__head((list_c*)&a);
    atassert_eqi(head->count, 8);
    atassert_eqi(head->capacity, 8);

    dlist.destroy(&a);
    AllocatorHeap_free();

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(testlist_align16)
{
    const Allocator_c* allocator = AllocatorHeap_new();

    struct foo64
    {
        alignas(16) size_t foo;
    };
    _Static_assert(sizeof(struct foo64) == 16, "size");
    _Static_assert(alignof(struct foo64) == 16, "align");

    list$define(struct foo64) a;

    except_traceback(err, list$new(&a, 4, allocator))
    {
        atassert(false && "list$new fail");
    }

    // adding new elements into the end
    for (u32 i = 0; i < 4; i++) {
        struct foo64 f = {.foo = i};
        atassert_eqs(EOK, dlist.append(&a, &f));
    }
    atassert_eqi(a.count, 4);
    list_head_s* head = list__head((list_c*)&a);
    atassert_eqi(head->count, 4);
    atassert_eqi(head->capacity, 4);
    atassert_eqi(head->header.elalign, 16);

    // validate appended values
    for (u32 i = 0; i < a.count; i++) {
        atassert_eqi(a.arr[i].foo, i);
    }

    // check if array got resized
    for (u32 i = 4; i < 8; i++) {
        struct foo64 f = {.foo = i};
        atassert_eqs(EOK, dlist.append(&a, &f));
    }

    // validate appended values
    for (u32 i = 0; i < a.count; i++) {
        atassert_eqi(a.arr[i].foo, i);
    }

    atassert_eqi(a.count, 8);
    head = list__head((list_c*)&a);
    atassert_eqi(head->count, 8);
    atassert_eqi(head->capacity, 8);

    dlist.destroy(&a);
    AllocatorHeap_free();

    return NULL; // Every ATEST_F() must return NULL to succeed!
}
/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
__attribute__((optimize("O0"))) int
main(int argc, char* argv[])
{
    ATEST_PARSE_MAINARGS(argc, argv);
    ATEST_PRINT_HEAD();  // >>> all tests below
    
    ATEST_RUN(testlist_alloc_capacity);
    ATEST_RUN(testlist_new);
    ATEST_RUN(testlist_append);
    ATEST_RUN(testlist_extend);
    ATEST_RUN(testlist_iterator);
    ATEST_RUN(testlist_align64);
    ATEST_RUN(testlist_align16);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
