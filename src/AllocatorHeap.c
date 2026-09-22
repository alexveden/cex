#if !defined(cex$enable_minimal) || defined(cex$enable_mem)

#include "AllocatorHeap.h"

// clang-format off
static void* _cex_allocator_heap__malloc(IAllocator self,usize size, usize alignment);
static void* _cex_allocator_heap__calloc(IAllocator self,usize nmemb, usize size, usize alignment);
static void* _cex_allocator_heap__realloc(IAllocator self,void* ptr, usize size, usize alignment);
static void* _cex_allocator_heap__free(IAllocator self,void* ptr);
static IAllocator  _cex_allocator_heap__scope_enter(IAllocator self);
static void _cex_allocator_heap__scope_exit(IAllocator self);
static u32 _cex_allocator_heap__scope_depth(IAllocator self);

AllocatorHeap_c _cex__default_global__allocator_heap = {
    .alloc = {
        .malloc = _cex_allocator_heap__malloc,
        .realloc = _cex_allocator_heap__realloc,
        .calloc = _cex_allocator_heap__calloc,
        .free = _cex_allocator_heap__free,
        .scope_enter = _cex_allocator_heap__scope_enter,
        .scope_exit = _cex_allocator_heap__scope_exit,
        .scope_depth = _cex_allocator_heap__scope_depth,
        .meta = {
            .magic_id =CEX_ALLOCATOR_HEAP_MAGIC,
            .is_arena = false,
            .is_temp = false, 
        }
    },
};
IAllocator const _cex__default_global__allocator_heap__allc = &_cex__default_global__allocator_heap.alloc;
// clang-format on


static void
_cex_allocator_heap__validate(IAllocator self)
{
    (void)self;
#ifndef NDEBUG
    uassert(self != NULL);
    uassert(
        self->meta.magic_id == CEX_ALLOCATOR_HEAP_MAGIC && "bad allocator pointer or mem corruption"
    );
#endif
}

static inline u64
_cex_allocator_heap__hdr_set(u64 size, u8 ptr_offset, u8 alignment)
{
    size &= 0xFFFFFFFFFFFFULL; // Mask to 48 bits
    return size | ((u64)ptr_offset << 48) | ((u64)alignment << 56);
}

static inline usize
_cex_allocator_heap__hdr_get_size(u64 alloc_hdr)
{
    return alloc_hdr & 0xFFFFFFFFFFFFULL;
}

static inline u8
_cex_allocator_heap__hdr_get_offset(u64 alloc_hdr)
{
    return (u8)((alloc_hdr >> 48) & 0xFF);
}

static inline u8
_cex_allocator_heap__hdr_get_alignment(u64 alloc_hdr)
{
    return (u8)(alloc_hdr >> 56);
}

static u64
_cex_allocator_heap__hdr_make(usize alloc_size, usize alignment)
{

    usize size = alloc_size;

    if (unlikely(alloc_size == 0 || alloc_size > PTRDIFF_MAX || alignment > 64)) {
        uassert(alloc_size > 0 && "zero size");
        uassert(alloc_size > PTRDIFF_MAX && "size is too high");
        uassert(alignment <= 64);
        return 0;
    }

#if UINTPTR_MAX > 0xFFFFFFFFU
    // Only 64 bit
    if (unlikely((u64)alloc_size > (u64)0xFFFFFFFFFFFFULL)) {
        uassert(
            (u64)alloc_size < (u64)0xFFFFFFFFFFFFULL && "size is too high, or negative overflow"
        );
        return 0;
    }
#endif

    if (alignment < 8) {
        static_assert(alignof(void*) <= 8, "unexpected ptr alignment");
        // We don't need to reserve extra room for base alignment
        alignment = 8;
    } else {
        uassert(mem$is_power_of2(alignment) && "must be pow2");

        if ((alloc_size & (alignment - 1)) != 0) {
            uassert(alloc_size % alignment == 0 && "requested size is not aligned");
            return 0;
        }
        // Adding extra alignment chunk because system malloc() may return misaligned pointer
        //   and we will need to offset it in order to return truly aligned address 
        size += alignment;
    }
    // room for header
    size += sizeof(u64);

    // extra area for poisoning
    size += sizeof(u64);

    size = mem$aligned_round(size, alignment);

    return _cex_allocator_heap__hdr_set(size, 0, alignment);
}

static void*
_cex_allocator_heap__alloc(IAllocator self, u8 fill_val, usize size, usize alignment)
{
    _cex_allocator_heap__validate(self);
    AllocatorHeap_c* a = (AllocatorHeap_c*)self;
    (void)a;

    u64 hdr = _cex_allocator_heap__hdr_make(size, alignment);
    if (unlikely(hdr == 0)) { return NULL; }

    usize full_size = _cex_allocator_heap__hdr_get_size(hdr);
    alignment = _cex_allocator_heap__hdr_get_alignment(hdr);

    // Memory alignment
    // |                 <hdr>|<poisn>|---<data>---
    // ^---malloc()
    u8* raw_result = NULL;
    if (fill_val != 0) {
        raw_result = cex$platform_malloc(full_size);
    } else {
        raw_result = cex$platform_calloc(1, full_size);
    }
    u8* result = raw_result;

    if (raw_result) {
        result = mem$aligned_pointer(raw_result + sizeof(u64) * 2, alignment);
        uassert(mem$aligned_pointer(result, 8) == result);
        uassert(mem$aligned_pointer(result, alignment) == result);

#ifdef CEX_TEST
        a->stats.n_allocs++;
        // intentionally set malloc to 0xf7 pattern to mark uninitialized data
        if (fill_val != 0) { memset(result, 0xf7, size); }
#endif
        usize ptr_offset = result - raw_result;
        uassert(ptr_offset >= sizeof(u64) * 2);
        uassert(ptr_offset <= 64 + 16);
        uassert(ptr_offset <= alignment + sizeof(u64) * 2);
        uassert(result + size <= raw_result + full_size);

        // poison area after header and before allocated pointer
        mem$asan_poison(result - sizeof(u64), sizeof(u64));
        ((u64*)result)[-2] = _cex_allocator_heap__hdr_set(size, ptr_offset, alignment);

        if (ptr_offset + size < full_size) {
            // Adding padding poison for non 8-byte aligned data
            mem$asan_poison(result + size, full_size - size - ptr_offset);
        }
    }

    return result;
}

static void*
_cex_allocator_heap__malloc(IAllocator self, usize size, usize alignment)
{
    return _cex_allocator_heap__alloc(self, 1, size, alignment);
}
static void*
_cex_allocator_heap__calloc(IAllocator self, usize nmemb, usize size, usize alignment)
{
    if (unlikely(nmemb == 0 || nmemb >= PTRDIFF_MAX)) {
        uassert(nmemb > 0 && "nmemb is zero");
        uassert(nmemb < PTRDIFF_MAX && "nmemb is too high or negative overflow");
        return NULL;
    }
    if (unlikely(size == 0 || size >= PTRDIFF_MAX)) {
        uassert(size > 0 && "size is zero");
        uassert(size < PTRDIFF_MAX && "size is too high or negative overflow");
        return NULL;
    }

    return _cex_allocator_heap__alloc(self, 0, size * nmemb, alignment);
}

static void*
_cex_allocator_heap__realloc(IAllocator self, void* ptr, usize size, usize alignment)
{
    _cex_allocator_heap__validate(self);
    if (unlikely(ptr == NULL)) {
        uassert(ptr != NULL);
        return NULL;
    }
    AllocatorHeap_c* a = (AllocatorHeap_c*)self;
    (void)a;

    // Memory alignment
    // |                 <hdr>|<poisn>|---<data>---
    // ^---realloc()
    char* p = ptr;
    uassert(
        mem$asan_poison_check(p - sizeof(u64), sizeof(u64)) &&
        "corrupted pointer or unallocated by mem$"
    );
    u64 old_hdr = *(u64*)(p - sizeof(u64) * 2);
    uassert(old_hdr > 0 && "bad poitner or corrupted malloced header?");
    u64 old_size = _cex_allocator_heap__hdr_get_size(old_hdr);
    (void)old_size;
    u8 old_offset = _cex_allocator_heap__hdr_get_offset(old_hdr);
    u8 old_alignment = _cex_allocator_heap__hdr_get_alignment(old_hdr);
    uassert((old_alignment >= 8 && old_alignment <= 64) && "corrupted header?");
    uassert(old_offset <= 64 + sizeof(u64) * 2 && "corrupted header?");
    if (unlikely(
            (alignment <= 8 && old_alignment != 8) || (alignment > 8 && alignment != old_alignment)
        )) {
        uassert(alignment == old_alignment && "given alignment doesn't match to old one");
        goto fail;
    }

    u64 new_hdr = _cex_allocator_heap__hdr_make(size, alignment);
    if (unlikely(new_hdr == 0)) { goto fail; }

    u8* raw_result = NULL;
    u8* result = NULL;
    usize new_full_size = _cex_allocator_heap__hdr_get_size(new_hdr);

    if (alignment <= _Alignof(max_align_t)) {
        uassert(new_full_size > size);
        raw_result = cex$platform_realloc(p - old_offset, new_full_size);
        if (unlikely(raw_result == NULL)) { goto fail; }
        result = mem$aligned_pointer(raw_result + sizeof(u64) * 2, old_alignment);
    } else {
        // fallback to malloc + memcpy because realloc doesn't guarantee alignment
        raw_result = cex$platform_malloc(new_full_size);
        if (unlikely(raw_result == NULL)) { goto fail; }
        result = mem$aligned_pointer(raw_result + sizeof(u64) * 2, old_alignment);
        memcpy(result, ptr, size > old_size ? old_size : size);
        cex$platform_free(ptr - old_offset);
    }
    uassert(mem$aligned_pointer(result, 8) == result);
    uassert(mem$aligned_pointer(result, old_alignment) == result);

    usize ptr_offset = result - raw_result;
    uassert(ptr_offset <= 64 + 16);
    uassert(ptr_offset <= old_alignment + sizeof(u64) * 2);
    // uassert(ptr_offset + size <= new_full_size);

#ifdef CEX_TEST
    a->stats.n_reallocs++;
    if (old_size < size) {
        // intentionally set unallocated to 0xf7 pattern to mark uninitialized data
        memset(result + old_size, 0xf7, size - old_size);
    }
#endif
    mem$asan_poison(result - sizeof(u64), sizeof(u64));
    ((u64*)result)[-2] = _cex_allocator_heap__hdr_set(size, ptr_offset, old_alignment);

    if (ptr_offset + size < new_full_size) {
        // Adding padding poison for non 8-byte aligned data
        mem$asan_poison(result + size, new_full_size - size - ptr_offset);
    }

    return result;

fail:
    _cex_allocator_heap__free(self, ptr);
    return NULL;
}

static void*
_cex_allocator_heap__free(IAllocator self, void* ptr)
{
    _cex_allocator_heap__validate(self);
    if (ptr != NULL) {
        AllocatorHeap_c* a = (AllocatorHeap_c*)self;
        (void)a;

        char* p = ptr;
        uassert(
            mem$asan_poison_check(p - sizeof(u64), sizeof(u64)) &&
            "corrupted pointer or unallocated by mem$"
        );
        u64 hdr = *(u64*)(p - sizeof(u64) * 2);
        uassert(hdr > 0 && "bad pointner or corrupted malloced header?");
        u8 offset = _cex_allocator_heap__hdr_get_offset(hdr);
        u8 alignment = _cex_allocator_heap__hdr_get_alignment(hdr);
        uassert(alignment >= 8 && "corrupted header?");
        uassert(alignment <= 64 && "corrupted header?");
        uassert(offset >= 16 && "corrupted header?");
        uassert(offset <= 64+16 && "corrupted header?");

#ifdef CEX_TEST
        a->stats.n_free++;
        u64 size = _cex_allocator_heap__hdr_get_size(hdr);
        u32 padding = mem$aligned_round(size + offset, alignment) - size - offset;
        if (padding > 0) {
            uassert(
                mem$asan_poison_check(p + size, padding) &&
                "corrupted area after unallocated size by mem$"
            );
        }
#endif

        cex$platform_free(p - offset);
    }
    return NULL;
}

static const struct Allocator_i*
_cex_allocator_heap__scope_enter(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    uassert(false && "this only supported by arenas");
    cex$platform_panic();
}

static void
_cex_allocator_heap__scope_exit(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    uassert(false && "this only supported by arenas");
    cex$platform_panic();
}

static u32
_cex_allocator_heap__scope_depth(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    return 1; // always 1
}

#endif
