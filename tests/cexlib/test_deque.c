#include <cex/cexlib/cextest.h>
#include <cex/cexlib/cex.c>
#include <cex/cexlib/allocators.c>
#include <cex/cexlib/deque.c>
#include <cex/cexlib/deque.h>
#include <stdalign.h>
#include <stdio.h>

const Allocator_i* allocator;
/*
* SUITE INIT / SHUTDOWN
*/
cextest$teardown(){
    allocator = allocators.heap.destroy();
    return EOK;
}

cextest$setup()
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

cextest$case(testlist_alloc_capacity)
{
    // 4 is minimum
    tassert_eql(16, deque__alloc_capacity(0));
    tassert_eql(16, deque__alloc_capacity(1));
    tassert_eql(16, deque__alloc_capacity(15));
    tassert_eql(16, deque__alloc_capacity(16));
    tassert_eql(32, deque__alloc_capacity(17));
    tassert_eql(32, deque__alloc_capacity(32));
    tassert_eql(128, deque__alloc_capacity(100));


    return NULL;
}

cextest$case(test_deque_new)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        tassert(false && "deque$new fail");
    }

    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = deque__head(a);
    // deque_head_s head = (*a)._head;
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->header.eloffset, sizeof(deque_head_s));
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 0);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 0);
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert(head->allocator == allocator);

    deque.destroy(&a);
    return EOK;

}

cextest$case(test_element_alignment_16)
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
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elalign, 16);
    tassert_eqi(head->header.elsize, 16);
    tassert_eqi(head->header.eloffset, 64);
    tassert_eqi(head->capacity, 16);
    tassert(head->allocator == allocator);
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        struct foo16 f = { .foo = i };
        tassert_eqs(EOK, deque.append(&a, &f));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 0; i < 16; i++) {
        struct foo16* f = deque.dequeue(&a);
        tassertf(f != NULL, "%d\n: NULL", i);
        tassertf(f->foo == i, "%ld: i=%d\n", f->foo, i);
        nit++;
    }
    tassert_eqi(nit, 16);
    tassert_eqi(deque.len(&a), 0);

    deque.destroy(&a);
    return EOK;

}

cextest$case(test_element_alignment_64)
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
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elalign, 64);
    tassert_eqi(head->header.elsize, 64);
    tassert_eqi(head->header.eloffset, 64);
    tassert_eqi(head->capacity, 16);
    tassert(head->allocator == allocator);
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        struct foo64 f = { .foo = i };
        tassert_eqs(EOK, deque.append(&a, &f));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 0; i < 16; i++) {
        struct foo64* f = deque.dequeue(&a);
        tassertf(f != NULL, "%d\n: NULL", i);
        tassertf(f->foo == i, "%ld: i=%d\n", f->foo, i);
        nit++;
    }
    tassert_eqi(nit, 16);

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);

    uassert_disable();
    tassert_eqs(Error.argument, deque$new(&a, struct foo128, 0, false, allocator));
    return EOK;

}

cextest$case(test_deque_new_append_pop)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.append(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 16; i > 0; i--) {
        u32* p = deque.pop(&a);
        tassertf(p != NULL, "%d\n: NULL", i);
        tassertf(*p == (i - 1), "%d: i=%d\n", *p, i);
        nit++;
    }
    tassert_eqi(nit, 16);
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.enqueue(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    for (u32 i = 0; i < 16; i++) {
        u32* p = deque.dequeue(&a);
        tassertf(p != NULL, "%d\n: NULL", i);
        tassertf(*p == i, "%d: i=%d\n", *p, i);
    }
    tassert_eqi(deque.len(&a), 0);
    tassert_eqi(head->idx_head, 16);
    tassert_eqi(head->idx_tail, 16);

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return EOK;

}


cextest$case(test_deque_new_append_roll_over)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    u32 nit = 0;
    for (u32 i = 0; i < 16; i++) {
        u32* p = deque.dequeue(&a);
        tassertf(p != NULL, "%d\n: NULL", i);
        nit++;
    }
    tassert_eqi(deque.len(&a), 0);
    tassert_eqi(head->idx_head, 16);
    tassert_eqi(head->idx_tail, 16);

    // Que is emty, next push/append/enque - resets all to zero
    tassert_eqs(EOK, deque.append(&a, &nit));
    tassert_eqs(EOK, deque.append(&a, &nit));
    tassert_eqi(deque.len(&a), 2);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 2);

    deque.dequeue(&a);
    deque.dequeue(&a);
    tassert_eqi(deque.len(&a), 0);
    tassert_eqi(head->idx_head, 2);
    tassert_eqi(head->idx_tail, 2);

    // Que is emty, next push/append/enque - resets all to zero
    tassert_eqs(EOK, deque.append(&a, &nit));
    tassert_eqi(deque.len(&a), 1);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 1);

    deque.clear(&a);
    tassert_eqi(deque.len(&a), 0);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 0);

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return EOK;

}

cextest$case(test_deque_new_append_grow)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // dequeue a couple elements, in this case we expect
    // realloc + growing the deque
    deque.dequeue(&a);
    deque.dequeue(&a);
    tassert_eqi(deque.len(&a), 14);

    u32 nit = 123;
    tassert_eqs(EOK, deque.push(&a, &nit));
    head = &a->_head; // head may change after resize!
    tassert_eqi(deque.len(&a), 15);
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->header.eloffset, 64);
    tassert_eqi(head->header.elalign, 4);
    tassert_eqi(head->max_capacity, 0);
    tassert_eqi(head->capacity, 32);
    tassert_eqi(head->idx_head, 2);
    tassert_eqi(head->idx_tail, 17);

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return EOK;

}

cextest$case(test_deque_new_append_max_cap_wrap)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, false, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    u32 nit = 123;
    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Max capacity set, and no more room for extra
    tassert_eqs(Error.overflow, deque.push(&a, &nit));

    // dequeue a couple elements, in this case we expect
    // realloc + growing the deque
    deque.dequeue(&a);
    deque.dequeue(&a);
    tassert_eqi(deque.len(&a), 14);

    // These two pushes will wrap the ring buffer and add to the beginning
    nit = 123;
    tassert_eqs(EOK, deque.push(&a, &nit));
    nit = 124;
    tassert_eqs(EOK, deque.push(&a, &nit));
    tassert_eqi(deque.len(&a), 16);
    tassert_eqs(Error.overflow, deque.push(&a, &nit));

    head = &a->_head; // head may change after resize!
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->idx_head, 2);
    tassert_eqi(head->idx_tail, 18);

    // Wrapped pop first in reversed order (i.e. as stack)
    tassert_eqi(124, *((int*)deque.pop(&a)));
    tassert_eqi(123, *((int*)deque.pop(&a)));
    // Wraps back to the end of que array and gets last on
    tassert_eqi(15, *((int*)deque.pop(&a)));

    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->idx_head, 2);
    tassert_eqi(head->idx_tail, 15);

    // dequeue returns from the head of the queue
    tassert_eqi(2, *((int*)deque.dequeue(&a)));
    tassert_eqi(3, *((int*)deque.dequeue(&a)));

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return EOK;

}

cextest$case(test_deque_new_append_max_cap__rewrite_overflow)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    u32 nit = 123;
    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Max capacity set, and no more room for extra
    tassert_eqs(EOK, deque.push(&a, &nit));
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 1);
    tassert_eqi(head->idx_tail, 17);

    tassert_eqi(123, *((int*)deque.pop(&a)));
    tassert_eqi(15, *((int*)deque.pop(&a)));

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return EOK;

}


cextest$case(test_deque_new_append_max_cap__rewrite_overflow_with_rollover)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    u32 nit = 123;
    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    tassert_eqi(0, *((int*)deque.dequeue(&a)));
    tassert_eqi(1, *((int*)deque.dequeue(&a)));

    // Max capacity set, and no more room for extra

    nit = 123;
    tassert_eqs(EOK, deque.push(&a, &nit));
    nit = 124;
    tassert_eqs(EOK, deque.push(&a, &nit));
    nit = 125;
    tassert_eqs(EOK, deque.push(&a, &nit));

    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 3);
    tassert_eqi(head->idx_tail, 19);

    // at the beginning
    tassert_eqi(3, *((int*)deque.get(&a, 0)));
    tassert_eqi(125, *((int*)deque.get(&a, deque.len(&a) - 1)));
    tassert(NULL == deque.get(&a, deque.len(&a)));

    tassert_eqi(125, *((int*)deque.pop(&a)));
    tassert_eqi(124, *((int*)deque.pop(&a)));
    tassert_eqi(123, *((int*)deque.pop(&a)));
    // wraps to the end
    tassert_eqi(15, *((int*)deque.pop(&a)));

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return EOK;

}

cextest$case(test_deque_new_append_max_cap__rewrite_overflow__multiloop)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 256; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }

    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 16);

    for (u32 i = 256 - 16; i < 256; i++) {
        tassert_eqi(i, *((int*)deque.dequeue(&a)));
    }

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return EOK;

}

cextest$case(test_deque_new_append_multi_resize)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 0, false, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 0);
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 256; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }

    head = &a->_head; // refresh dangling head pointer
    tassert_eqi(head->capacity, 256);
    tassert_eqi(head->max_capacity, 0);
    tassert_eqi(deque.len(&a), 256);
    for (u32 i = 0; i < 256; i++) {
        tassert_eqi(i, *((int*)deque.get(&a, i)));
    }

    for (u32 i = 0; i < 256; i++) {
        tassert_eqi(i, *((int*)deque.get(&a, 0)));
        tassert_eqi(i, *((int*)deque.dequeue(&a)));
    }

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return EOK;

}

cextest$case(test_deque_iter_get)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    u32 nit = 123;

    nit = 0;
    for$iter(int, it, deque.iter_get(&a, 1, &it.iterator))
    {
        tassert(false && "should never happen!");
        nit++;
    }
    tassert_eqi(nit, 0);

    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Getting que item (keeping element in que)
    nit = 0;
    for$iter(int, it, deque.iter_get(&a, 1, &it.iterator))
    {
        tassert_eqi(it.idx.i, nit);
        tassert_eqi(*it.val, nit);
        nit++;
    }

    // Getting que in reverse order
    nit = 0;
    for$iter(int, it, deque.iter_get(&a, -1, &it.iterator))
    {
        tassert_eqi(it.idx.i, 15 - nit);
        tassert_eqi(*it.val, 15 - nit);
        nit++;
    }

    tassert_eqs(EOK, deque.validate(&a));

    deque.destroy(&a);
    return EOK;

}

cextest$case(test_deque_iter_fetch)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    u32 nit = 123;

    nit = 0;
    for$iter(int, it, deque.iter_fetch(&a, 1, &it.iterator))
    {
        tassert(false && "should never happen!");
        nit++;
    }
    tassert_eqi(nit, 0);

    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Fetching que item (iter with removing from que)
    nit = 0;
    for$iter(int, it, deque.iter_fetch(&a, 1, &it.iterator))
    {
        tassert_eqi(it.idx.i, 0);
        tassert_eqi(*it.val, nit);
        nit++;
    }

    tassert_eqi(deque.len(&a), 0);
    tassert_eqi(head->idx_head, 16);
    tassert_eqi(head->idx_tail, 16);

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    return EOK;

}

cextest$case(test_deque_iter_fetch_reversed)
{
    deque_c a;
    except_traceback(err, deque$new(&a, int, 16, true, allocator))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 0);

    u32 nit = 123;

    nit = 0;
    for$iter(int, it, deque.iter_fetch(&a, -1, &it.iterator))
    {
        tassert(false && "should never happen!");
        nit++;
    }
    tassert_eqi(nit, 0);

    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(deque.len(&a), 16);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    // Fetching que item (iter with removing from que)
    nit = 0;
    for$iter(int, it, deque.iter_fetch(&a, -1, &it.iterator))
    {
        tassert_eqi(*it.val, 15 - nit);
        tassert_eqi(it.idx.i, head->capacity);
        nit++;
    }

    tassert_eqi(deque.len(&a), 0);
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 0);

    tassert_eqs(EOK, deque.validate(&a));
    deque.destroy(&a);
    tassert(a == NULL);
    return EOK;

}

cextest$case(test_deque_static)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    except_traceback(err, deque$new_static(&a, int, buf, arr$len(buf), true))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.rewrite_overflowed, true);
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(head->idx_tail, 0);
    tassert_eqi(head->idx_head, 0);
    tassert(head->allocator == NULL);
    tassert_eqi(deque.len(&a), 0);
    return NULL;
}

cextest$case(test_deque_static_append_grow)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    except_traceback(err, deque$new_static(&a, int, buf, arr$len(buf), false))
    {
        tassert(false && "deque$new fail");
    }

    deque_head_s* head = &a->_head;
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(head->header.magic, 0xdef0);
    tassert_eqi(head->header.elsize, sizeof(int));
    tassert_eqi(head->header.elalign, alignof(int));
    tassert_eqi(head->header.rewrite_overflowed, false);
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 16);


    i32 nit = 1;
    tassert_eqs(Error.overflow, deque.push(&a, &nit));

    tassert_eqs(EOK, deque.validate(&a));
    tassert(deque.destroy(&a) == NULL);
    return EOK;

}

cextest$case(test_deque_static_append_grow_overwrite)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    except_traceback(err, deque$new_static(&a, int, buf, arr$len(buf), true))
    {
        tassert(false && "deque$new fail");
    }
    tassert_eqs(EOK, deque.validate(&a));

    deque_head_s* head = &a->_head;
    tassert_eqi(deque.len(&a), 0);

    for (u32 i = 0; i < 16; i++) {
        tassert_eqs(EOK, deque.push(&a, &i));
    }
    tassert_eqi(head->header.rewrite_overflowed, true);
    tassert_eqi(head->capacity, 16);
    tassert_eqi(head->max_capacity, 16);
    tassert_eqi(deque.len(&a), 16);


    i32 nit = 123;
    tassert_eqi(head->idx_head, 0);
    tassert_eqi(head->idx_tail, 16);

    tassert_eqs(EOK, deque.push(&a, &nit));
    tassert_eqi(123, *(int*)deque.get(&a, deque.len(&a) - 1));
    tassert_eqi(head->idx_head, 1);
    tassert_eqi(head->idx_tail, 17);

    tassert_eqs(EOK, deque.validate(&a));
    a = deque.destroy(&a);
    tassert(a == NULL);
    return EOK;

}

cextest$case(test_deque_validate)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    tassert_eqs(EOK, deque$new_static(&a, int, buf, arr$len(buf), true));
    tassert((void*)a == (void*)buf);
    tassert_eqs(Error.argument, deque.validate(NULL));
    tassert_eqs(EOK, deque.validate(&a));
    return EOK;

}

cextest$case(test_deque_validate__head_gt_tail)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    tassert_eqs(EOK, deque$new_static(&a, int, buf, arr$len(buf), true));
    deque_head_s* head = &a->_head;
    head->idx_head = 1;
    tassert_eqs(Error.integrity, deque.validate(&a));
    return EOK;

}

cextest$case(test_deque_validate__eloffset_weird)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    tassert_eqs(EOK, deque$new_static(&a, int, buf, arr$len(buf), true));
    deque_head_s* head = &a->_head;
    head->header.eloffset = 1;
    tassert_eqs(Error.integrity, deque.validate(&a));
    return EOK;

}

cextest$case(test_deque_validate__eloffset_elalign)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    tassert_eqs(EOK, deque$new_static(&a, int, buf, arr$len(buf), true));
    deque_head_s* head = &a->_head;
    head->header.elalign = 0;
    tassert_eqs(Error.integrity, deque.validate(&a));
    head->header.elalign = 65;
    tassert_eqs(Error.integrity, deque.validate(&a));
    return EOK;

}

cextest$case(test_deque_validate__capacity_gt_max_capacity)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    tassert_eqs(EOK, deque$new_static(&a, int, buf, arr$len(buf), true));
    deque_head_s* head = &a->_head;
    head->max_capacity = 16;
    head->capacity = 17;
    tassert_eqs(Error.integrity, deque.validate(&a));
    return EOK;

}


cextest$case(test_deque_validate__zero_capacity)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    tassert_eqs(EOK, deque$new_static(&a, int, buf, arr$len(buf), true));
    deque_head_s* head = &a->_head;
    head->capacity = 0;
    tassert_eqs(Error.integrity, deque.validate(&a));
    return EOK;

}

cextest$case(test_deque_validate__bad_magic)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    tassert_eqs(EOK, deque$new_static(&a, int, buf, arr$len(buf), true));
    deque_head_s* head = &a->_head;
    head->header.magic = 0xdef1;
    tassert_eqs(Error.integrity, deque.validate(&a));
    return EOK;

}

cextest$case(test_deque_validate__bad_pointer_alignment)
{
    alignas(64) char buf[sizeof(deque_head_s) + sizeof(int) * 16];

    deque_c a;
    tassert_eqs(EOK, deque$new_static(&a, int, buf, arr$len(buf), true));
    a = (void*)(buf + 1);
    tassert_eqs(Error.memory, deque.validate(&a));
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
    cextest$args_parse(argc, argv);
    cextest$print_header();  // >>> all tests below
    
    cextest$run(testlist_alloc_capacity);
    cextest$run(test_deque_new);
    cextest$run(test_element_alignment_16);
    cextest$run(test_element_alignment_64);
    cextest$run(test_deque_new_append_pop);
    cextest$run(test_deque_new_append_roll_over);
    cextest$run(test_deque_new_append_grow);
    cextest$run(test_deque_new_append_max_cap_wrap);
    cextest$run(test_deque_new_append_max_cap__rewrite_overflow);
    cextest$run(test_deque_new_append_max_cap__rewrite_overflow_with_rollover);
    cextest$run(test_deque_new_append_max_cap__rewrite_overflow__multiloop);
    cextest$run(test_deque_new_append_multi_resize);
    cextest$run(test_deque_iter_get);
    cextest$run(test_deque_iter_fetch);
    cextest$run(test_deque_iter_fetch_reversed);
    cextest$run(test_deque_static);
    cextest$run(test_deque_static_append_grow);
    cextest$run(test_deque_static_append_grow_overwrite);
    cextest$run(test_deque_validate);
    cextest$run(test_deque_validate__head_gt_tail);
    cextest$run(test_deque_validate__eloffset_weird);
    cextest$run(test_deque_validate__eloffset_elalign);
    cextest$run(test_deque_validate__capacity_gt_max_capacity);
    cextest$run(test_deque_validate__zero_capacity);
    cextest$run(test_deque_validate__bad_magic);
    cextest$run(test_deque_validate__bad_pointer_alignment);
    
    cextest$print_footer();  // ^^^^^ all tests runs are above
    return cextest$exit_code();
}
