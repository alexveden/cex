// #gcc_args -Wl,--wrap=malloc
#include <alloca.h>
#include <cex/cexlib/cex.c>
#include <cex/cexlib/allocators.c>
#include <cex/cextest/cextest.h>
#include <cex/cextest/fff.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Fake Functions of app_external.c
 */
DEFINE_FFF_GLOBALS
FAKE_VALUE_FUNC(void*, __wrap_malloc, size_t, size_t)
void* __real_malloc(size_t, size_t);

void test_shutdown(void) {
    uassert_disable();
    allocators.heap.destroy();
    allocators.staticarena.destroy();

}
/*
 * SUITE INIT / SHUTDOWN
 */
ATEST_SETUP_F(void)
{
    RESET_FAKE(__wrap_malloc);
    __wrap_malloc_fake.custom_fake = __real_malloc;
    uassert_enable();
    return test_shutdown;
}

/*
 *
 *   TEST SUITE
 *
 */
ATEST_F(test_allocator_heap)
{

    const Allocator_i* allocator = allocators.heap.create();
    char* buf = allocator->malloc(123);
    allocator_heap_s* a = (allocator_heap_s*)allocator;
    atassert_eqi(a->stats.n_allocs, 1);
    atassert(buf != NULL);
    memset(buf, 1, 123); // more than 123, should trigger sanitizer

    buf = allocator->realloc(buf, 1024);
    atassert(buf != NULL);
    memset(buf, 1, 1024); // more than 1024, should trigger sanitizer


    // emits number allocations don't match number of free
    allocator->free(buf);
    atassert_eqi(a->stats.n_allocs, 1);
    atassert_eqi(a->stats.n_free, 1);
    atassert(allocators.heap.destroy() == NULL);
    return NULL;
}

ATEST_F(test_allocator_heap_memory_leak_check)
{

    const Allocator_i* allocator = allocators.heap.create();
    char* buf = allocator->malloc(123);
    allocator_heap_s* a = (allocator_heap_s*)allocator;
    atassert_eqi(a->stats.n_allocs, 1);
    atassert(buf != NULL);
    atassert_eqi(a->stats.n_allocs, 1);
    atassert_eqi(a->stats.n_free, 0);
    atassert(allocators.heap.destroy() == NULL);

    free(buf); // calm down the sanitizer
    return NULL;
}

ATEST_F(test_allocator_heap_fopen_unclosed)
{

    const Allocator_i* allocator = allocators.heap.create();
    FILE* f = allocator->fopen("tests/data/allocator_fopen.txt", "w+");
    atassert(f != NULL);
    atassert_eqi(4, fwrite("test", 1, 4, f));

    allocator_heap_s* a = (allocator_heap_s*)allocator;
    atassert_eqi(a->stats.n_fopen, 1);
    atassert(allocators.heap.destroy() == NULL);
    fclose(f);

    allocator = allocators.heap.create();
    f = allocator->fopen("tests/data/allocator_fopen.txt", "w");
    atassert(f != NULL);
    atassert_eqi(4, fwrite("test", 1, 4, f));
    atassert(allocator->fclose(f) != -1);

    atassert_eqi(a->stats.n_fopen, 1);
    atassert_eqi(a->stats.n_fclose, 1);
    atassert(allocators.heap.destroy() == NULL);

    allocator = allocators.heap.create();
    int fd = allocator->open("tests/data/allocator_fopen.txt", O_RDONLY, 0640);
    atassert(fd != -1);
    atassert_eqi(a->stats.n_open, 1);
    atassert_eqi(a->stats.n_close, 0);

    char buf[16];
    read(fd, buf, 4);
    buf[4] = '\0'; // null term
    atassert_eqs(buf, "test");

    close(fd);

    atassert(allocators.heap.destroy() == NULL);
    allocator = allocators.heap.create();
    fd = allocator->open("tests/data/allocator_fopen.txt", O_RDONLY, 0640);
    atassert(fd != -1);
    atassert_eqi(a->stats.n_open, 1);
    atassert(allocator->close(fd) != -1);
    atassert_eqi(a->stats.n_close, 1);

    atassert(allocators.heap.destroy() == NULL);

    return NULL;
}

ATEST_F(test_allocator_double_creation)
{

    const Allocator_i* allocator = allocators.heap.create();
    uassert_disable();
    const Allocator_i* allocator2 = allocators.heap.create();
    atassert(allocator != NULL);
    atassert(allocator2 != NULL);
    (void)allocator;
    (void)allocator2;
    allocators.heap.destroy();

    return NULL;
}

ATEST_F(test_allocator_alloc_aligned)
{

    const Allocator_i* allocator = allocators.heap.create();

    char* buf2 = allocator->malloc_aligned(1024, 2048);
    atassert(buf2 != NULL);
    atassert_eqi((size_t)buf2 % 1024, 0);

    // alloc some unaligned number of bytes
    char* buf = allocator->malloc(51123);
    atassert_eqi((size_t)buf % alignof(size_t), 0);

    char* buf3 = allocator->realloc_aligned(buf2, 1024, 1024);
    // buf2 = allocator->realloc(buf2, 1024);
    atassert(buf3 != NULL);
    atassert(buf3 != buf2);
    atassert_eqi((size_t)buf3 % 1024, 0);

    allocator->free(buf);
    // allocator->free(buf2);  // double free!
    allocator->free(buf3);
    atassert(allocators.heap.destroy() == NULL);

    return NULL;
}

ATEST_F(test_allocator_heap_calloc)
{

    const Allocator_i* allocator = allocators.heap.create();

    char* buf2 = allocator->calloc(512, 2);
    atassert(buf2 != NULL);
    atassert_eqi((size_t)buf2 % alignof(size_t), 0);

    char buf_zero[1024] = {0};
    atassert_eqi(0, memcmp(buf2, buf_zero, 1024));

    allocator->free(buf2);
    atassert(allocators.heap.destroy() == NULL);

    return NULL;
}

ATEST_F(test_allocator_static_arena_stack)
{
    char buf[1024];
    const Allocator_i* allocator = allocators.staticarena.create(buf, arr$len(buf));
    atassert(allocator != NULL);

    allocator_staticarena_s* a = (allocator_staticarena_s*)allocator;

    atassert(a->magic != 0);
    atassert(a->base.free != NULL);
    atassert(a->base.malloc != NULL);
    atassert(a->base.calloc != NULL);
    atassert(a->base.realloc != NULL);
    atassert(a->base.malloc_aligned != NULL);
    atassert(a->base.realloc_aligned != NULL);
    atassert(a->base.fopen != NULL);
    atassert(a->base.fclose != NULL);
    atassert(a->base.open != NULL);
    atassert(a->base.close != NULL);

    atassert(a->mem != NULL);
    atassert_eqi((u64)a->next % sizeof(size_t), 0);

    // two small variables - aligned to 64
    void* v1 = allocator->malloc(12);
    atassert(v1 != NULL);
    atassert_eqi((u64)v1 % sizeof(size_t), 0);
    atassert_eqi(a->stats.n_allocs, 1);

    void* v2 = allocator->malloc(12);
    atassert(v2 != NULL);
    atassert_eqi((u64)v2 % sizeof(size_t), 0);
    atassert_eqi(a->stats.n_allocs, 2);

    // data composition check
    atassert(v1 != v2);
    atassert_eqi((char*)v2 - (char*)v1, sizeof(size_t) * 2);
    atassert_eqi((char*)a->next - (char*)v2, sizeof(size_t) * 2);

    // capacity overflow
    u32 used = (char*)a->next - (char*)a->mem;
    void* v3 = allocator->malloc(1024 - used + 1);
    atassert(v3 == NULL);


    // exact capacity match
    void* v4 = allocator->malloc(1024 - used);
    atassert(v4 != NULL);
    atassert_eqi(a->stats.n_allocs, 3);

    // buffer is totally full, reject even smallest part
    void* v5 = allocator->malloc(1);
    atassert(v5 == NULL);

    // Re-alloc not supported, and raises the assertion
    uassert_disable();
    void* v6 = allocator->realloc(v1, 30);
    atassert(v6 == NULL);

    // Free is not needed but still acts well
    atassert_eqi(a->stats.n_allocs, 3);

    void* v7 = allocator->malloc(0);
    atassert(v7 == NULL);

    // Arena cleanup
    uassert_enable();
    allocator->free(v1);
    allocator->free(v2);
    allocator->free(v4);

    allocator = allocators.staticarena.destroy();
    atassert(allocator == NULL);

    atassert(a->mem == NULL);
    atassert(a->next == NULL);
    atassert(a->max == NULL);
    atassert(a->magic == 0);
    atassert(a->base.free != NULL);
    atassert(a->base.malloc != NULL);
    atassert(a->base.realloc != NULL);

    return NULL;
}

ATEST_F(test_allocator_static_arena_stack_aligned)
{

    alignas(64) char buf[1024];

    const Allocator_i* allocator = allocators.staticarena.create(buf, arr$len(buf));

    allocator_staticarena_s* a = (allocator_staticarena_s*)allocator;

    atassert(allocator != NULL);
    atassert(a->mem != NULL);
    // atassert_eqi(a->alignment, 64);

    // NOTE: alloca() may not return 64 aligned pointer!!!
    // atassert_eqi((u64)a->mem % 64, 0);  // occasionally this can fail!
    atassert_eqi((u64)a->next % sizeof(size_t), 0);

    // two small variables - aligned to 64
    void* v1 = allocator->malloc(12);
    atassert(v1 != NULL);
    atassert_eqi((u64)v1 % sizeof(size_t), 0);

    atassert(alignof(allocator_heap_s) == 64); // this struct is 64 aligned
    void* v2 = allocator->malloc_aligned(64, 64);
    atassert(v2 != NULL);
    atassert_eqi((u64)v2 % 64, 0);

    // data composition check
    atassert(v1 != v2);
    atassert_eqi((char*)v2 - (char*)v1, 64);
    atassert_eqi((char*)a->next - (char*)v2, 64);

    allocator_heap_s* t = (allocator_heap_s*)v2;
    //  Without alignment we would have unaligned access - undefined behaviour
    //  san!
    memset(t, 0, sizeof(*t));
    t->magic = 123;
    t->stats.n_free++;

    // capacity overflow
    u32 used = (char*)a->next - (char*)a->mem;
    void* v3 = allocator->malloc(1024 - used + 1);
    atassert(v3 == NULL);

    // exact capacity match
    v3 = allocator->malloc(1024 - used);
    atassert(v3 != NULL);
    allocator->free(v1);
    allocator->free(v2);
    allocator->free(v3);

    // buffer is totally full, reject even smallest part
    atassert(allocator->malloc(1) == NULL);

    // Re-alloc not supported, and raises the assertion
    uassert_disable();
    v1 = allocator->realloc(v1, 30);
    atassert(v1 == NULL);

    v1 = allocator->malloc(0);
    atassert(v1 == NULL);

    // Arena cleanup
    allocator = allocators.staticarena.destroy();
    atassert(allocator == NULL);

    atassert(a->mem == NULL);
    atassert(a->magic == 0);
    // interface remain in place!
    atassert(a->base.free != NULL);
    atassert(a->base.malloc != NULL);
    atassert(a->base.realloc != NULL);

    return NULL;
}

ATEST_F(test_allocator_static_arena_memory_leak_check)
{

    alignas(64) char buf[1024];

    const Allocator_i* allocator = allocators.staticarena.create(buf, arr$len(buf));
    allocator_staticarena_s* a = (allocator_staticarena_s*)allocator;

    atassert(allocator != NULL);
    atassert(a->mem != NULL);
    void* v1 = allocator->malloc(12);
    atassert(v1 != NULL);
    atassert_eqi(a->stats.n_allocs, 1);
    atassert_eqi(a->stats.n_free, 0);
    allocator = allocators.staticarena.destroy();
    return NULL;
}

ATEST_F(test_allocator_static_arena_calloc)
{

    alignas(64) char buf[1025];

    // initial buffer is unaligned
    const Allocator_i* allocator = allocators.staticarena.create(buf+1, arr$len(buf)-1);
    allocator_staticarena_s* a = (allocator_staticarena_s*)allocator;

    memset(buf, 'z', 1025);

    atassert(allocator != NULL);
    atassert(a->mem != NULL);
    atassert_eqi((u64)a->next % sizeof(size_t), 0);

    // two small variables - aligned to 64
    void* v1 = allocator->calloc(3, 4);
    atassert_eqi((u64)a->next % sizeof(size_t), 0);

    // aligned!
    atassert_eqi((u64)v1 % sizeof(size_t), 0);
    atassert(v1 != NULL);
    atassert_eqi(a->stats.n_allocs, 1);

    char zero_buf[12] = {0};
    atassert_eqi(0, memcmp(v1, zero_buf, 12));

    void* v2 = allocator->calloc(4, 3);
    atassert_eqi((u64)a->next % sizeof(size_t), 0);
    atassert(v2 != NULL);
    atassert_eqi((u64)v2 % sizeof(size_t), 0);
    atassert_eqi(a->stats.n_allocs, 2);

    // data composition check
    atassert(v1 != v2);
    atassert_eqi((char*)v2 - (char*)v1, sizeof(size_t) * 2);
    atassert_eqi((char*)a->next - (char*)v2, sizeof(size_t) * 2);

    // capacity overflow
    u32 used = (char*)a->next - (char*)a->mem;
    atassert_eqi(used, 39);
    void* v3 = allocator->calloc(1024 - used, 1);
    atassert(v3 != NULL);

    // no more allocations
    void* v4 = allocator->calloc(1, 1);
    atassert(v4 == NULL);


    // Free is not needed but still acts well
    allocator->free(v1);
    allocator->free(v2);
    allocator->free(v3);
    atassert_eqi(a->stats.n_free, 3);

    // Arena cleanup
    allocators.staticarena.destroy();

    return NULL;
}

ATEST_F(test_allocator_staticarena_fopen_unclosed)
{
    char arena[2048];

    const Allocator_i* allocator = allocators.staticarena.create(arena, sizeof(arena));

    FILE* f = allocator->fopen("tests/data/allocator_fopen.txt", "w+");
    atassert(f != NULL);
    atassert_eqi(4, fwrite("test", 1, 4, f));

    allocator_staticarena_s* a = (allocator_staticarena_s*)allocator;
    atassert_eqi(a->stats.n_fopen, 1);
    atassert(allocators.staticarena.destroy() == NULL);
    fclose(f);

    allocator = allocators.staticarena.create(arena, sizeof(arena));
    f = allocator->fopen("tests/data/allocator_fopen.txt", "w");
    atassert(f != NULL);
    atassert_eqi(4, fwrite("test", 1, 4, f));
    atassert(allocator->fclose(f) != -1);

    atassert_eqi(a->stats.n_fopen, 1);
    atassert_eqi(a->stats.n_fclose, 1);
    atassert(allocators.staticarena.destroy() == NULL);

    allocator = allocators.staticarena.create(arena, sizeof(arena));
    int fd = allocator->open("tests/data/allocator_fopen.txt", O_RDONLY, 0640);
    atassert(fd != -1);
    atassert_eqi(a->stats.n_open, 1);
    atassert_eqi(a->stats.n_close, 0);

    char buf[16];
    read(fd, buf, 4);
    buf[4] = '\0'; // null term
    atassert_eqs(buf, "test");

    close(fd);
    atassert(allocators.staticarena.destroy() == NULL);

    allocator = allocators.staticarena.create(arena, sizeof(arena));
    fd = allocator->open("tests/data/allocator_fopen.txt", O_RDONLY, 0640);
    atassert(fd != -1);
    atassert_eqi(a->stats.n_open, 1);
    atassert(allocator->close(fd) != -1);
    atassert_eqi(a->stats.n_close, 1);

    atassert(allocators.staticarena.destroy() == NULL);

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
    ATEST_RUN(test_allocator_heap_memory_leak_check);
    ATEST_RUN(test_allocator_heap_fopen_unclosed);
    ATEST_RUN(test_allocator_double_creation);
    ATEST_RUN(test_allocator_alloc_aligned);
    ATEST_RUN(test_allocator_heap_calloc);
    ATEST_RUN(test_allocator_static_arena_stack);
    ATEST_RUN(test_allocator_static_arena_stack_aligned);
    ATEST_RUN(test_allocator_static_arena_memory_leak_check);
    ATEST_RUN(test_allocator_static_arena_calloc);
    ATEST_RUN(test_allocator_staticarena_fopen_unclosed);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
