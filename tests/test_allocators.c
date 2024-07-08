
#include <alloca.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tests/atest.h>

// IMPORTANT: including .c allows testing static functions!
//            also no need to include all headers/sources except testlib2.c in
//            Makefile
// #include "../include/test_lib.c"
#include <include/cex/allocators.c>
#include <include/cex.h>

#include <tests/fff.h>

/*
 * Fake Functions of app_external.c
 */
DEFINE_FFF_GLOBALS
// #gcc_args -Wl,--wrap=malloc
//
//
FAKE_VALUE_FUNC(void*, __wrap_malloc, size_t, size_t)
void* __real_malloc(size_t, size_t);

/*
 * SUITE INIT / SHUTDOWN
 */
ATEST_SETUP_F(void)
{
    RESET_FAKE(__wrap_malloc);
    __wrap_malloc_fake.custom_fake = __real_malloc;
    uassert_enable();
    return NULL; // if no shutdown logic needed
}

/*
 *
 *   TEST SUITE
 *
 */
ATEST_F(test_allocator_heap)
{

    const Allocator_c* allocator = AllocatorHeap_new();
    char* buf = allocator->alloc(123);
    atassert(buf != NULL);
    memset(buf, 1, 123); // more than 123, should trigger sanitizer

    buf = allocator->realloc(buf, 1024);
    atassert(buf != NULL);
    memset(buf, 1, 1024); // more than 1024, should trigger sanitizer

    // emits number allocations don't match number of free
    atassert(AllocatorHeap_free() == NULL);

    // nullified, after AllocatorHeap_free!
    atassert(allocator->alloc == NULL);

    free(buf);

    return NULL;
}

ATEST_F(test_allocator_alloc_aligned)
{

    const Allocator_c* allocator = AllocatorHeap_new();

    char* buf2 = allocator->alloc_aligned(1024, 2048);
    atassert(buf2 != NULL);
    atassert_eqi((size_t)buf2 % 1024, 0);

    // alloc some unaligned number of bytes
    char* buf = allocator->alloc(51123);
    atassert_eqi((size_t)buf % alignof(size_t), 0);

    char* buf3 = allocator->realloc_aligned(buf2, 1024, 1024);
    // buf2 = allocator->realloc(buf2, 1024);
    atassert(buf3 != NULL);
    atassert(buf3 != buf2);
    atassert_eqi((size_t)buf3 % 1024, 0);

    allocator->free(buf);
    // allocator->free(buf2);  // double free!
    allocator->free(buf3);
    atassert(AllocatorHeap_free() == NULL);

    return NULL;
}


ATEST_F(test_allocator_static_arena_stack)
{

    // const Allocator_c* allocator = AllocatorStaticArena_new(8*1024*1024, 64, false);

    char buf[1024];
    const Allocator_c* allocator = AllocatorStaticArena_new(buf, len(buf));

    AllocatorStaticArena_c* a = (AllocatorStaticArena_c*)allocator;
    atassert(a->base.magic != 0);
    atassert(a->base.free != NULL);
    atassert(a->base.alloc != NULL);
    atassert(a->base.realloc != NULL);
    atassert(a->base.alloc_aligned != NULL);
    atassert(a->base.realloc_aligned != NULL);

    atassert(allocator != NULL);
    atassert(a->mem != NULL);

    // NOTE: alloca() may not return 64 aligned pointer!!!
    // atassert_eqi((u64)a->mem % 64, 0);  // occasionally this can fail!
    atassert_eqi((u64)a->next % sizeof(size_t), 0);

    // two small variables - aligned to 64
    void* v1 = allocator->alloc(12);
    atassert(v1 != NULL);
    atassert_eqi((u64)v1 % sizeof(size_t), 0);

    void* v2 = allocator->alloc(12);
    atassert(v2 != NULL);
    atassert_eqi((u64)v2 % sizeof(size_t), 0);

    // data composition check
    atassert(v1 != v2);
    atassert_eqi((char*)v2 - (char*)v1, sizeof(size_t)*2);
    atassert_eqi((char*)a->next - (char*)v2, sizeof(size_t)*2);

    // capacity overflow
    u32 used = (char*)a->next - (char*)a->mem;
    void* v3 = allocator->alloc(1024 - used + 1);
    atassert(v3 == NULL);

    // exact capacity match
    v3 = allocator->alloc(1024 - used);
    atassert(v3 != NULL);

    // buffer is totally full, reject even smallest part
    v2 = allocator->alloc(1);
    atassert(v2 == NULL);


    // Re-alloc not supported, and raises the assertion
    uassert_disable();
    v1 = allocator->realloc(v1, 30);
    atassert(v1 == NULL);

    // Free is not needed but still acts well
    v2 = allocator->free(v2);
    atassert(v2 == NULL);

    v1 = allocator->alloc(0);
    atassert(v1 == NULL);

    // Arena cleanup
    AllocatorStaticArena_free();
    atassert(a->mem == NULL);
    atassert(a->base.magic == 0);
    atassert(a->base.free == NULL);
    atassert(a->base.alloc == NULL);
    atassert(a->base.realloc == NULL);

    return NULL;
}

ATEST_F(test_allocator_static_arena_stack_aligned)
{

    alignas(64) char buf[1024];
    const Allocator_c* allocator = AllocatorStaticArena_new(buf, len(buf));

    AllocatorStaticArena_c* a = (AllocatorStaticArena_c*)allocator;

    atassert(allocator != NULL);
    atassert(a->mem != NULL);
    // atassert_eqi(a->alignment, 64);

    // NOTE: alloca() may not return 64 aligned pointer!!!
    // atassert_eqi((u64)a->mem % 64, 0);  // occasionally this can fail!
    atassert_eqi((u64)a->next % sizeof(size_t), 0);

    // two small variables - aligned to 64
    void* v1 = allocator->alloc(12);
    atassert(v1 != NULL);
    atassert_eqi((u64)v1 % sizeof(size_t), 0);

    atassert(alignof(AllocatorDynamic_c) == 64); // this struct is 64 aligned
    //
    void* v2 = allocator->alloc_aligned(64, 64);
    atassert(v2 != NULL);
    atassert_eqi((u64)v2 % 64, 0);

    // data composition check
    atassert(v1 != v2);
    atassert_eqi((char*)v2 - (char*)v1, 64);
    atassert_eqi((char*)a->next - (char*)v2, 64);

    AllocatorDynamic_c* t = (AllocatorDynamic_c*)v2;
    //  Without alignment we would have unaligned access - undefined behaviour san!
    memset(t, 0, sizeof(*t));
    t->base.magic = 123;
    t->n_free++;

    // capacity overflow
    u32 used = (char*)a->next - (char*)a->mem;
    void* v3 = allocator->alloc(1024 - used + 1);
    atassert(v3 == NULL);

    // exact capacity match
    v3 = allocator->alloc(1024 - used);
    atassert(v3 != NULL);

    // buffer is totally full, reject even smallest part
    v2 = allocator->alloc(1);
    atassert(v2 == NULL);


    // Re-alloc not supported, and raises the assertion
    uassert_disable();
    v1 = allocator->realloc(v1, 30);
    atassert(v1 == NULL);

    // Free is not needed but still acts well
    v2 = allocator->free(v2);
    atassert(v2 == NULL);

    v1 = allocator->alloc(0);
    atassert(v1 == NULL);

    // Arena cleanup
    AllocatorStaticArena_free();
    atassert(a->mem == NULL);
    atassert(a->base.magic == 0);
    atassert(a->base.free == NULL);
    atassert(a->base.alloc == NULL);
    atassert(a->base.realloc == NULL);

    return NULL;
}

ATEST_F(test_allocator_static_arena_heap)
{

    char buf[1024];
    const Allocator_c* allocator = AllocatorStaticArena_new(buf, len(buf));
    AllocatorStaticArena_c* a = (AllocatorStaticArena_c*)allocator;

    atassert(allocator != NULL);
    atassert(a->mem != NULL);
    // atassert_eqi(a->alignment, 64);
    atassert_eqi((u64)a->mem % sizeof(size_t), 0);

    // two small variables - aligned to 64
    void* v1 = allocator->alloc(12);
    atassert(v1 != NULL);

    void* v2 = allocator->alloc(12);
    atassert(v2 != NULL);

    // data composition check
    atassert(v1 != v2);
    atassert_eqi((char*)v2 - (char*)v1, sizeof(size_t)*2);
    atassert_eqi((char*)a->next - (char*)v2, sizeof(size_t)*2);

    // capacity overflow
    atassert_eqi((char*)a->next - (char*)a->mem, 32);
    void* v3 = allocator->alloc(1024 - 32 + 1);
    atassert(v3 == NULL);

    // exact capacity match
    v3 = allocator->alloc(1024 - 32);
    atassert(v3 != NULL);

    // buffer is totally full, reject even smallest part
    v2 = allocator->alloc(1);
    atassert(v2 == NULL);


    // Re-alloc not supported, and raises the assertion
    uassert_disable();
    v1 = allocator->realloc(v1, 30);
    atassert(v1 == NULL);

    // Free is not needed but still acts well
    v2 = allocator->free(v2);
    atassert(v2 == NULL);


    // Arena cleanup
    AllocatorStaticArena_free();
    atassert(a->mem == NULL);
    atassert(a->base.magic == 0);
    atassert(a->base.free == NULL);
    atassert(a->base.alloc == NULL);
    atassert(a->base.realloc == NULL);
    atassert(a->base.realloc_aligned == NULL);
    atassert(a->base.alloc_aligned == NULL);

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
    
    ATEST_RUN(test_allocator_heap);
    ATEST_RUN(test_allocator_alloc_aligned);
    ATEST_RUN(test_allocator_static_arena_stack);
    ATEST_RUN(test_allocator_static_arena_stack_aligned);
    ATEST_RUN(test_allocator_static_arena_heap);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
