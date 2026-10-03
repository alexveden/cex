#include <setjmp.h>
#include <stdint.h>

#ifndef NDEBUG
#    define cex$platform_panic _heap_scope_panic
static void _heap_scope_panic(const char*, const char*, uint32_t, const char*, const char*);
#endif

// opt out of allocation-failure panics: this file asserts the NULL-return contract
#define cex$platform_mem_panic(...) (void)0

#include "src/all.c"

#ifndef NDEBUG
static jmp_buf _heap_scope_panic_jmp;
static bool _heap_scope_panic_armed = false;

static void
_heap_scope_panic(const char* prefix, const char* file, u32 line, const char* func, const char* msg)
{
    if (_heap_scope_panic_armed) { longjmp(_heap_scope_panic_jmp, 1); }
    _cex_errors_panic_handler(prefix, file, line, func, msg);
}
#endif

test$case(test_allocator_api)
{
    // mem$->scope_enter = NULL; // GOOD: compiler error (const vtable)
    // NOTE: mem$ is an assignable global in CEX_TEST builds, so `mem$ = NULL;` is allowed there
    u8* p = mem$malloc(mem$, 100);
    tassert(_cex__default_global__allocator_heap.stats.n_allocs > 0);
    tassert_eq(mem$->scope_depth(mem$), 1);


    tassert(p != NULL);
    p = mem$->free(mem$, p);
    tassert(p == NULL);
    // double free is checked if NULL
    tassert(mem$->free(mem$, p) == NULL);

    p = mem$malloc(mem$, 100);
    tassert(p != NULL);
    // mem$free always nullifies pointer
    mem$free(mem$, p);
    tassert(p == NULL);

    p = mem$malloc(mem$, 100);
    u8** pp = &p;
    tassert(p != NULL);
    // mem$free always nullifies pointer
    mem$free(mem$, *pp);
    tassert(p == NULL);


    return EOK;
}
test$case(test_allocator_temp)
{

    mem$scope(tmem$, tal)
    {
        void* p = mem$malloc(tmem$, 100);
        tassert(_cex__default_global__allocator_temp.used >= 100);

        tassert(p != NULL);
        p = mem$free(tmem$, p);

        tassert(mem$ != tmem$);
        tassert(tal == tmem$);
    }

    return EOK;
}

test$case(test_allocator_new)
{

    AllocatorHeap_c* a = mem$malloc(mem$, sizeof(AllocatorHeap_c), alignof(AllocatorHeap_c));
    tassert(a != NULL);
    tassert(mem$aligned_pointer(a, alignof(AllocatorHeap_c)) == a);

    mem$free(mem$, a);
    return EOK;
}

test$case(test_allocator_heap_default_alignment)
{

    AllocatorHeap_c* allc = (AllocatorHeap_c*)mem$;
    allc->stats.n_free = 0;
    allc->stats.n_allocs = 0;

    for (u32 i = 1; i < 1000; i++) {
        usize len = i * 8 + 1;
        u8* a = mem$malloc(mem$, len);
        tassert(a != NULL);
        tassert(mem$aligned_pointer(a, 8) == a);
        // filled with 'poisoned' pattern
        for$each (v, a, len) { tassert(v == 0xf7); }
        mem$free(mem$, a);
    }
    tassert_eq(allc->stats.n_allocs, 999);
    tassert_eq(allc->stats.n_allocs, allc->stats.n_free);

    return EOK;
}

test$case(test_allocator_heap_alloc_header)
{

    tassert_eq(sizeof(u64), 8);
    if (sizeof(usize) == 8) {
        // 64bit (some sanity check assumptions about our arch)
        tassert_eq(alignof(u64), 8);
        tassert_eq(alignof(usize), 8);
        tassert_eq(alignof(void*), 8);
        u64 hdr = _cex_allocator_heap__hdr_set(0x123456789ABC, 0xCD, 0xEF);
        tassert_eq(hdr, 0xefcd123456789abc);
        tassert_eq(0x123456789ABC, _cex_allocator_heap__hdr_get_size(hdr));
        tassert_eq(0xCD, _cex_allocator_heap__hdr_get_offset(hdr));
        tassert_eq(0xEF, _cex_allocator_heap__hdr_get_alignment(hdr));
    } else {
        // 32bit (some sanity check assumptions about our arch)
        // NOTE: armv7 might have alignment 8
        tassert(alignof(u64) == 4 || alignof(u64) == 8);
        tassert(alignof(usize) == 4 || alignof(usize) == 8);
        tassert(alignof(void*) == 4 || alignof(void*) == 8);
        u64 hdr = _cex_allocator_heap__hdr_set(0x123456, 0xCD, 0xEF);
        io.printf("%lx\n", hdr);
        tassert_eq(hdr, 0xefcd000000123456);
        tassert_eq(0x123456, _cex_allocator_heap__hdr_get_size(hdr));
        tassert_eq(0xCD, _cex_allocator_heap__hdr_get_offset(hdr));
        tassert_eq(0xEF, _cex_allocator_heap__hdr_get_alignment(hdr));
    }

    return EOK;
}

test$case(test_allocator_heap_malloc_random_align)
{

    usize align_arr[] = { 0, 1, 2, 4, 8, 16, 32, 64 };
    os.random.seed(20250317);

    for (u32 i = 0; i < 10000; i++) {
        usize al = align_arr[i % arr$len(align_arr)];
        usize size = os.random.range(2, 16) * al;

        if (al <= 1) { size = os.random.range(1, 1001); }

        u8* a = mem$->malloc(mem$, size, al);
        tassert(a != NULL);

        if (al < 8) {
            tassert((usize)a % 8 == 0 && "expected aligned to 8");
        } else {
            tassert((usize)a % al == 0 && "expected aligned to al");
        }

        for$each (v, a, size) { tassert(v == 0xf7); }

        memset(a, 0xaf, size);

        mem$free(mem$, a);
    }

    return EOK;
}

test$case(test_allocator_heap_calloc_random_align)
{

    usize align_arr[] = { 0, 1, 2, 4, 8, 16, 32, 64 };
    os.random.seed(20250317);

    for (u32 i = 0; i < 10000; i++) {
        usize al = align_arr[i % arr$len(align_arr)];
        usize size = os.random.range(2, 16) * al;

        if (al <= 1) { size = os.random.range(1, 1001); }

        usize nmemb = i % 5 + 1;

        u8* a = mem$->calloc(mem$, nmemb, size, al);
        u8* b = mem$->malloc(mem$, 128, 0);
        tassert(a != NULL);
        tassert(b != NULL);

        if (al < 8) {
            tassert((usize)a % 8 == 0 && "expected aligned to 8");
        } else {
            tassert((usize)a % al == 0 && "expected aligned to al");
        }

        for$each (v, a, size * nmemb) { tassert(v == 0); }

        memset(a, 'Z', size * nmemb);

        // printf("size: %ld align: %ld p: %p\n", size, al, a);
        mem$free(mem$, a);
        mem$free(mem$, b);
    }

    return EOK;
}

test$case(test_allocator_heap_realloc)
{

    AllocatorHeap_c* allc = (AllocatorHeap_c*)mem$;
    allc->stats.n_free = 0;
    allc->stats.n_allocs = 0;
    allc->stats.n_reallocs = 0;

    for (u32 i = 1; i < 1000; i++) {
        usize len = i * 8 + 1;
        u8* a = mem$malloc(mem$, len);
        u8* b = mem$->malloc(mem$, 1024, 0);
        tassert(a != NULL);
        tassert(b != NULL);
        tassert(mem$aligned_pointer(a, 8) == a);
        // filled with 'poisoned' pattern
        for$each (v, a, len) { tassert(v == 0xf7); }

        u8* old_a = a;
        a = mem$realloc(mem$, a, len * 2);
        if (old_a != a) {
            // realloced by new pointer

        } else {
            // extended existing
        }


        mem$free(mem$, a);
        mem$free(mem$, b);
    }
    tassert_eq(allc->stats.n_allocs, 1998);
    tassert_eq(allc->stats.n_reallocs, 999);
    tassert_eq(allc->stats.n_allocs, allc->stats.n_free);

    return EOK;
}

test$case(test_allocator_heap_realloc_aligned)
{

    AllocatorHeap_c* allc = (AllocatorHeap_c*)mem$;
    allc->stats.n_free = 0;
    allc->stats.n_allocs = 0;

    usize len = 32 * 10;
    u8* a = mem$malloc(mem$, len, 32);
    u8* b = mem$->malloc(mem$, 1024, 0);
    tassert(a != NULL);
    tassert(b != NULL);
    tassert(mem$aligned_pointer(a, 8) == a);

    u8* new_a = mem$realloc(mem$, a, len * 2, 32);
    tassert(new_a != a);

    mem$free(mem$, new_a);
    mem$free(mem$, b);

    return EOK;
}

test$case(test_allocator_heap_realloc_random_align)
{
    usize align_arr[] = { 0, 1, 2, 4, 8, 16, 32, 64 };
    os.random.seed(20250317);

    for (u32 i = 0; i < 10000; i++) {
        usize al = align_arr[i % arr$len(align_arr)];
        usize size = os.random.range(2, 16) * al;

        if (al <= 1) { size = os.random.range(11, 1011); }

        u8* a = mem$->malloc(mem$, size, al);
        u8* b = mem$->malloc(mem$, 1028, 0);
        tassert(a != NULL);
        tassert(b != NULL);

        memset(a, 'Z', size);
        for$each (v, a, size) { tassert(v == 'Z'); }
        a[0] = 0xCD;
        a[size - 1] = 0xAB;

        if (al < 8) {
            tassert((usize)a % 8 == 0 && "expected aligned to 8");
        } else {
            tassert((usize)a % al == 0 && "expected aligned to al");
        }

        // fprintf(stderr, "i: %d size: %ld align: %ld p: %p\n", i, size, al, a);
        // u8* old_a = a;
        usize new_size = (i % 2 == 0) ? size * 2 : size / 2;
        if (al > 1 && i % 2 != 0) {
            new_size = ((size / al) / 2) * 2 * al;
            tassert(new_size <= size);
            tassert(new_size % al == 0);
        }


        a = mem$realloc(mem$, a, new_size, al);
        // tassert_eq(a[-1], 0xf7);  // ASAN poison check
        tassert(a != NULL);
        tassert(a[0] == 0xCD);
        if (new_size > size) {
            for$each (v, a + 1, size - 2) { tassert(v == 'Z'); }
            tassert(a[size - 1] == 0xAB);
            for (u32 j = size; j < new_size; j++) { tassert_eq(a[j], 0xf7); }
        } else {
            for$each (v, a + 1, new_size - 2) { tassert(v == 'Z'); }
            if (new_size == size) { tassert(a[size - 1] == 0xAB); }
        }

        u64 hdr = *(u64*)(a - sizeof(u64) * 2);
        tassert_eq(new_size, _cex_allocator_heap__hdr_get_size(hdr));

        if (al < 8) {
            tassert((usize)a % 8 == 0 && "expected aligned to 8");
            tassert_eq(8, _cex_allocator_heap__hdr_get_alignment(hdr));
        } else {
            tassert((usize)a % al == 0 && "expected aligned to al");
            tassert_eq(al, _cex_allocator_heap__hdr_get_alignment(hdr));
        }

        memset(a, 'Z', new_size);

        mem$free(mem$, a);
        mem$free(mem$, b);
    }

    return EOK;
}

test$case(test_allocator_heap_invalid_args_return_null)
{
    uassert_disable();

    tassert(_cex_allocator_heap__hdr_make(0, 8) == 0);
#if mem$platform() == 64
    tassert(_cex_allocator_heap__hdr_make(0x1000000000000ULL, 8) == 0);
#endif
    tassert(_cex_allocator_heap__hdr_make(24, 16) == 0);

    tassert(mem$->calloc(mem$, 0, 8, 0) == NULL);
    tassert(mem$->calloc(mem$, 1, 0, 0) == NULL);

    // operands individually valid, product wraps
    usize big = (usize)1 << (mem$platform() / 2);
    tassert(mem$->calloc(mem$, big, big, 0) == NULL);

    tassert(mem$->realloc(mem$, NULL, 16, 0) == NULL);

    u8* p = mem$malloc(mem$, 32, 0);
    tassert(p != NULL);
    tassert(mem$->realloc(mem$, p, 64, 16) == NULL);

    uassert_enable();
    return EOK;
}

#ifndef NDEBUG
test$case(test_allocator_heap_scope_enter_panics)
{
    _heap_scope_panic_armed = true;
    if (setjmp(_heap_scope_panic_jmp) == 0) {
        (void)mem$->scope_enter(mem$);
        tassert(false && "heap allocator scope_enter must panic");
    }
    _heap_scope_panic_armed = false;
    return EOK;
}

test$case(test_allocator_heap_scope_exit_panics)
{
    _heap_scope_panic_armed = true;
    if (setjmp(_heap_scope_panic_jmp) == 0) {
        mem$->scope_exit(mem$);
        tassert(false && "heap allocator scope_exit must panic");
    }
    _heap_scope_panic_armed = false;
    return EOK;
}
#endif

test$main();
