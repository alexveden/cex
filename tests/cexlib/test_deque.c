#include <cex/cextest/cextest.h>
#include <cex/cex.h>
#include <cex/cexlib/allocators.c>
#include <cex/cexlib/deque.c>
#include <cex/cexlib/deque.h>
#include <stdalign.h>
#include <stdio.h>

const Allocator_c* allocator;
/*
 * SUITE INIT / SHUTDOWN
 */
void
my_test_shutdown_func(void)
{
    AllocatorHeap_free();
}

ATEST_SETUP_F(void)
{
    allocator = AllocatorHeap_new();
    uassert_enable();

    // return NULL;   // if no shutdown logic needed
    return &my_test_shutdown_func; // return pointer to void some_shutdown_func(void)
}

/*
 *
 *   TEST SUITE
 *
 */

ATEST_F(test_dlist_alloc_capacity)
{
    // 4 is minimum
    atassert_eql(16, _deque__alloc_capacity(0));
    atassert_eql(16, _deque__alloc_capacity(1));
    atassert_eql(16, _deque__alloc_capacity(15));
    atassert_eql(16, _deque__alloc_capacity(16));
    atassert_eql(32, _deque__alloc_capacity(17));
    atassert_eql(32, _deque__alloc_capacity(32));
    atassert_eql(128, _deque__alloc_capacity(100));


    return NULL;
}

ATEST_F(test_deque_new)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        atassert(false && "deque$new fail");
    }

    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = _deque__head(a);
    // deque_head_s head = (*a)._head;
    atassert_eqi(head->header.magic, 0xdef0);
    atassert_eqi(head->header.elalign, alignof(int));
    atassert_eqi(head->header.elsize, sizeof(int));
    atassert_eqi(head->header.eloffset, sizeof(deque_head_s));
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 0);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 0);
    atassert_eqi(head->header.elsize, sizeof(int));
    atassert(head->allocator == allocator);

    deque.destroy(&a);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_element_alignment_16)
{

    deque_c a;

    struct foo16
    {
        alignas(16) size_t foo;
    };
    _Static_assert(sizeof(struct foo16) == 16, "size");
    _Static_assert(alignof(struct foo16) == 16, "align");

    except_traceback(err, deque$new(&a, struct foo16, 0, false, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->header.magic, 0xdef0);
    atassert_eqi(head->header.elalign, 16);
    atassert_eqi(head->header.elsize, 16);
    atassert_eqi(head->header.eloffset, 64);
    atassert_eqi(head->capacity, 16);
    atassert(head->allocator == allocator);
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        struct foo16 f = { .foo = i };
        atassert_eqs(EOK, deque.append(&a, &f));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 0; i < 16; i++) {
        struct foo16* f = deque.dequeue(&a);
        atassertf(f != NULL, "%d\n: NULL", i);
        atassertf(f->foo == i, "%ld: i=%d\n", f->foo, i);
        nit++;
    }
    atassert_eqi(nit, 16);
    atassert_eqi(deque.count(&a), 0);

    deque.destroy(&a);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_element_alignment_64)
{

    deque_c a;

    struct foo64
    {
        alignas(64) size_t foo;
    };
    _Static_assert(sizeof(struct foo64) == 64, "size");
    _Static_assert(alignof(struct foo64) == 64, "align");

    struct foo128
    {
        alignas(128) size_t foo;
    };

    except_traceback(err, deque$new(&a, struct foo64, 0, false, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->header.magic, 0xdef0);
    atassert_eqi(head->header.elalign, 64);
    atassert_eqi(head->header.elsize, 64);
    atassert_eqi(head->header.eloffset, 64);
    atassert_eqi(head->capacity, 16);
    atassert(head->allocator == allocator);
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        struct foo64 f = { .foo = i };
        atassert_eqs(EOK, deque.append(&a, &f));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 0; i < 16; i++) {
        struct foo64* f = deque.dequeue(&a);
        atassertf(f != NULL, "%d\n: NULL", i);
        atassertf(f->foo == i, "%ld: i=%d\n", f->foo, i);
        nit++;
    }
    atassert_eqi(nit, 16);

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);

    uassert_disable();
    atassert_eqs(Error.argument, deque$new(&a, struct foo128, 0, false, allocator));

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_new_append_pop)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.append(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 16; i > 0; i--) {
        u32* p = deque.pop(&a);
        atassertf(p != NULL, "%d\n: NULL", i);
        atassertf(*p == (i - 1), "%d: i=%d\n", *p, i);
        nit++;
    }
    atassert_eqi(nit, 16);
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.enqueue(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    for (u32 i = 0; i < 16; i++) {
        u32* p = deque.dequeue(&a);
        atassertf(p != NULL, "%d\n: NULL", i);
        atassertf(*p == i, "%d: i=%d\n", *p, i);
    }
    atassert_eqi(deque.count(&a), 0);
    atassert_eqi(head->idx_head, 16);
    atassert_eqi(head->idx_tail, 16);

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}


ATEST_F(test_deque_new_append_roll_over)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 0; i < 16; i++) {
        u32* p = deque.dequeue(&a);
        atassertf(p != NULL, "%d\n: NULL", i);
        nit++;
    }
    atassert_eqi(deque.count(&a), 0);
    atassert_eqi(head->idx_head, 16);
    atassert_eqi(head->idx_tail, 16);

    // Que is emty, next push/append/enque - resets all to zero
    atassert_eqs(EOK, deque.append(&a, &nit));
    atassert_eqs(EOK, deque.append(&a, &nit));
    atassert_eqi(deque.count(&a), 2);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 2);

    deque.dequeue(&a);
    deque.dequeue(&a);
    atassert_eqi(deque.count(&a), 0);
    atassert_eqi(head->idx_head, 2);
    atassert_eqi(head->idx_tail, 2);

    // Que is emty, next push/append/enque - resets all to zero
    atassert_eqs(EOK, deque.append(&a, &nit));
    atassert_eqi(deque.count(&a), 1);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 1);

    deque.clear(&a);
    atassert_eqi(deque.count(&a), 0);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 0);

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_new_append_grow)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    // dequeue a couple elements, in this case we expect
    // realloc + growing the deque
    deque.dequeue(&a);
    deque.dequeue(&a);
    atassert_eqi(deque.count(&a), 14);

    u32 nit = 123;
    atassert_eqs(EOK, deque.push(&a, &nit));
    head = &a->_head; // head may change after resize!
    atassert_eqi(deque.count(&a), 15);
    atassert_eqi(head->header.magic, 0xdef0);
    atassert_eqi(head->header.elsize, sizeof(int));
    atassert_eqi(head->header.eloffset, 64);
    atassert_eqi(head->header.elalign, 4);
    atassert_eqi(head->max_capacity, 0);
    atassert_eqi(head->capacity, 32);
    atassert_eqi(head->idx_head, 2);
    atassert_eqi(head->idx_tail, 17);

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_new_append_max_cap_wrap)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, false, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    u32 nit = 123;
    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    // Max capacity set, and no more room for extra
    atassert_eqs(Error.overflow, deque.push(&a, &nit));

    // dequeue a couple elements, in this case we expect
    // realloc + growing the deque
    deque.dequeue(&a);
    deque.dequeue(&a);
    atassert_eqi(deque.count(&a), 14);

    // These two pushes will wrap the ring buffer and add to the beginning
    nit = 123;
    atassert_eqs(EOK, deque.push(&a, &nit));
    nit = 124;
    atassert_eqs(EOK, deque.push(&a, &nit));
    atassert_eqi(deque.count(&a), 16);
    atassert_eqs(Error.overflow, deque.push(&a, &nit));

    head = &a->_head; // head may change after resize!
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->idx_head, 2);
    atassert_eqi(head->idx_tail, 18);

    // Wrapped pop first in reversed order (i.e. as stack)
    atassert_eqi(124, *((int*)deque.pop(&a)));
    atassert_eqi(123, *((int*)deque.pop(&a)));
    // Wraps back to the end of que array and gets last on
    atassert_eqi(15, *((int*)deque.pop(&a)));

    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->idx_head, 2);
    atassert_eqi(head->idx_tail, 15);

    // dequeue returns from the head of the queue
    atassert_eqi(2, *((int*)deque.dequeue(&a)));
    atassert_eqi(3, *((int*)deque.dequeue(&a)));

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_new_append_max_cap__rewrite_overflow)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    u32 nit = 123;
    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    // Max capacity set, and no more room for extra
    atassert_eqs(EOK, deque.push(&a, &nit));
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 1);
    atassert_eqi(head->idx_tail, 17);

    atassert_eqi(123, *((int*)deque.pop(&a)));
    atassert_eqi(15, *((int*)deque.pop(&a)));

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}


ATEST_F(test_deque_new_append_max_cap__rewrite_overflow_with_rollover)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    u32 nit = 123;
    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    atassert_eqi(0, *((int*)deque.dequeue(&a)));
    atassert_eqi(1, *((int*)deque.dequeue(&a)));

    // Max capacity set, and no more room for extra

    nit = 123;
    atassert_eqs(EOK, deque.push(&a, &nit));
    nit = 124;
    atassert_eqs(EOK, deque.push(&a, &nit));
    nit = 125;
    atassert_eqs(EOK, deque.push(&a, &nit));

    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 3);
    atassert_eqi(head->idx_tail, 19);

    // at the beginning
    atassert_eqi(3, *((int*)deque.get(&a, 0)));
    atassert_eqi(125, *((int*)deque.get(&a, deque.count(&a) - 1)));
    atassert(NULL == deque.get(&a, deque.count(&a)));

    atassert_eqi(125, *((int*)deque.pop(&a)));
    atassert_eqi(124, *((int*)deque.pop(&a)));
    atassert_eqi(123, *((int*)deque.pop(&a)));
    // wraps to the end
    atassert_eqi(15, *((int*)deque.pop(&a)));

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_new_append_max_cap__rewrite_overflow__multiloop)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 256; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }

    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 16);

    for (u32 i = 256 - 16; i < 256; i++) {
        atassert_eqi(i, *((int*)deque.dequeue(&a)));
    }

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_new_append_multi_resize)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 0);
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 256; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }

    head = &a->_head; // refresh dangling head pointer
    atassert_eqi(head->capacity, 256);
    atassert_eqi(head->max_capacity, 0);
    atassert_eqi(deque.count(&a), 256);
    for (u32 i = 0; i < 256; i++) {
        atassert_eqi(i, *((int*)deque.get(&a, i)));
    }

    for (u32 i = 0; i < 256; i++) {
        atassert_eqi(i, *((int*)deque.get(&a, 0)));
        atassert_eqi(i, *((int*)deque.dequeue(&a)));
    }

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_iter_get)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    u32 nit = 123;

    nit = 0;
    for$iter(int, it, deque.iter_get(&a, 1, &it.iterator))
    {
        atassert(false && "should never happen!");
        nit++;
    }
    atassert_eqi(nit, 0);

    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    // Getting que item (keeping element in que)
    nit = 0;
    for$iter(int, it, deque.iter_get(&a, 1, &it.iterator))
    {
        atassert_eqi(it.idx.i, nit);
        atassert_eqi(*it.val, nit);
        nit++;
    }

    // Getting que in reverse order
    nit = 0;
    for$iter(int, it, deque.iter_get(&a, -1, &it.iterator))
    {
        atassert_eqi(it.idx.i, 15 - nit);
        atassert_eqi(*it.val, 15 - nit);
        nit++;
    }

    atassert_eqs(EOK, deque.validate(&a));

    deque.destroy(&a);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_iter_fetch)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    u32 nit = 123;

    nit = 0;
    for$iter(int, it, deque.iter_fetch(&a, 1, &it.iterator))
    {
        atassert(false && "should never happen!");
        nit++;
    }
    atassert_eqi(nit, 0);

    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    // Fetching que item (iter with removing from que)
    nit = 0;
    for$iter(int, it, deque.iter_fetch(&a, 1, &it.iterator))
    {
        atassert_eqi(it.idx.i, 0);
        atassert_eqi(*it.val, nit);
        nit++;
    }

    atassert_eqi(deque.count(&a), 0);
    atassert_eqi(head->idx_head, 16);
    atassert_eqi(head->idx_tail, 16);

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_iter_fetch_reversed)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 0);

    u32 nit = 123;

    nit = 0;
    for$iter(int, it, deque.iter_fetch(&a, -1, &it.iterator))
    {
        atassert(false && "should never happen!");
        nit++;
    }
    atassert_eqi(nit, 0);

    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(deque.count(&a), 16);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    // Fetching que item (iter with removing from que)
    nit = 0;
    for$iter(int, it, deque.iter_fetch(&a, -1, &it.iterator))
    {
        atassert_eqi(*it.val, 15 - nit);
        atassert_eqi(it.idx.i, head->capacity);
        nit++;
    }

    atassert_eqi(deque.count(&a), 0);
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 0);

    atassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    atassert(a == NULL);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_static)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    except_traceback(err, deque$new_static(&a, int, buf, len(buf), true))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(head->header.magic, 0xdef0);
    atassert_eqi(head->header.elsize, sizeof(int));
    atassert_eqi(head->header.elalign, alignof(int));
    atassert_eqi(head->header.rewrite_overflowed, true);
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(head->idx_tail, 0);
    atassert_eqi(head->idx_head, 0);
    atassert(head->allocator == NULL);
    atassert_eqi(deque.count(&a), 0);
    return NULL;
}

ATEST_F(test_deque_static_append_grow)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    except_traceback(err, deque$new_static(&a, int, buf, len(buf), false))
    {
        atassert(false && "deque$new fail");
    }

    deque_head_s* head = &a->_head;
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(head->header.magic, 0xdef0);
    atassert_eqi(head->header.elsize, sizeof(int));
    atassert_eqi(head->header.elalign, alignof(int));
    atassert_eqi(head->header.rewrite_overflowed, false);
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 16);


    i32 nit = 1;
    atassert_eqs(Error.overflow, deque.push(&a, &nit));

    atassert_eqs(EOK, deque.validate(&a));
    atassert(deque.destroy(&a) == NULL);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_static_append_grow_overwrite)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    except_traceback(err, deque$new_static(&a, int, buf, len(buf), true))
    {
        atassert(false && "deque$new fail");
    }
    atassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    atassert_eqi(deque.count(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        atassert_eqs(EOK, deque.push(&a, &i));
    }
    atassert_eqi(head->header.rewrite_overflowed, true);
    atassert_eqi(head->capacity, 16);
    atassert_eqi(head->max_capacity, 16);
    atassert_eqi(deque.count(&a), 16);


    i32 nit = 123;
    atassert_eqi(head->idx_head, 0);
    atassert_eqi(head->idx_tail, 16);

    atassert_eqs(EOK, deque.push(&a, &nit));
    atassert_eqi(123, *(int*)deque.get(&a, deque.count(&a) - 1));
    atassert_eqi(head->idx_head, 1);
    atassert_eqi(head->idx_tail, 17);

    atassert_eqs(EOK, deque.validate(&a));
    a = deque.destroy(&a);
    atassert(a == NULL);


    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_validate)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    atassert_eqs(EOK, deque$new_static(&a, int, buf, len(buf), true));
    atassert((void*)a == (void*)buf);
    atassert_eqs(Error.argument, deque.validate(NULL));
    atassert_eqs(EOK, deque.validate(&a));
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_validate__head_gt_tail)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    atassert_eqs(EOK, deque$new_static(&a, int, buf, len(buf), true));
    deque_head_s* head = &a->_head;
    head->idx_head = 1;
    atassert_eqs(Error.integrity, deque.validate(&a));
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_validate__eloffset_weird)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    atassert_eqs(EOK, deque$new_static(&a, int, buf, len(buf), true));
    deque_head_s* head = &a->_head;
    head->header.eloffset = 1;
    atassert_eqs(Error.integrity, deque.validate(&a));
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_validate__eloffset_elalign)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    atassert_eqs(EOK, deque$new_static(&a, int, buf, len(buf), true));
    deque_head_s* head = &a->_head;
    head->header.elalign = 0;
    atassert_eqs(Error.integrity, deque.validate(&a));
    head->header.elalign = 65;
    atassert_eqs(Error.integrity, deque.validate(&a));
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_validate__capacity_gt_max_capacity)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    atassert_eqs(EOK, deque$new_static(&a, int, buf, len(buf), true));
    deque_head_s* head = &a->_head;
    head->max_capacity = 16;
    head->capacity = 17;
    atassert_eqs(Error.integrity, deque.validate(&a));
    return NULL; // Every ATEST_F() must return NULL to succeed!
}


ATEST_F(test_deque_validate__zero_capacity)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    atassert_eqs(EOK, deque$new_static(&a, int, buf, len(buf), true));
    deque_head_s* head = &a->_head;
    head->capacity = 0;
    atassert_eqs(Error.integrity, deque.validate(&a));
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_validate__bad_magic)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    atassert_eqs(EOK, deque$new_static(&a, int, buf, len(buf), true));
    deque_head_s* head = &a->_head;
    head->header.magic = 0xdef1;
    atassert_eqs(Error.integrity, deque.validate(&a));
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_deque_validate__bad_pointer_alignment)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    atassert_eqs(EOK, deque$new_static(&a, int, buf, len(buf), true));
    a = (void*)(buf + 1);
    atassert_eqs(Error.memory, deque.validate(&a));

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
    
    ATEST_RUN(test_dlist_alloc_capacity);
    ATEST_RUN(test_deque_new);
    ATEST_RUN(test_element_alignment_16);
    ATEST_RUN(test_element_alignment_64);
    ATEST_RUN(test_deque_new_append_pop);
    ATEST_RUN(test_deque_new_append_roll_over);
    ATEST_RUN(test_deque_new_append_grow);
    ATEST_RUN(test_deque_new_append_max_cap_wrap);
    ATEST_RUN(test_deque_new_append_max_cap__rewrite_overflow);
    ATEST_RUN(test_deque_new_append_max_cap__rewrite_overflow_with_rollover);
    ATEST_RUN(test_deque_new_append_max_cap__rewrite_overflow__multiloop);
    ATEST_RUN(test_deque_new_append_multi_resize);
    ATEST_RUN(test_deque_iter_get);
    ATEST_RUN(test_deque_iter_fetch);
    ATEST_RUN(test_deque_iter_fetch_reversed);
    ATEST_RUN(test_deque_static);
    ATEST_RUN(test_deque_static_append_grow);
    ATEST_RUN(test_deque_static_append_grow_overwrite);
    ATEST_RUN(test_deque_validate);
    ATEST_RUN(test_deque_validate__head_gt_tail);
    ATEST_RUN(test_deque_validate__eloffset_weird);
    ATEST_RUN(test_deque_validate__eloffset_elalign);
    ATEST_RUN(test_deque_validate__capacity_gt_max_capacity);
    ATEST_RUN(test_deque_validate__zero_capacity);
    ATEST_RUN(test_deque_validate__bad_magic);
    ATEST_RUN(test_deque_validate__bad_pointer_alignment);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
