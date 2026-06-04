#include "AllocatorArena.h"

#if !defined(cex$enable_minimal) || defined(cex$enable_mem)


#define CEX_ARENA_MAX_ALLOC \
    (mem$platform() > 32 ? ((1ULL << 40) - 1000) : ((usize)-1 - 1000))
#define CEX_ARENA_MAX_ALIGN 64


static u64
_cex_arena_rec_get_size(const allocator_arena_rec_s* r)
{
    return ((u64)r->size_high << 32) | r->size_low;
}

static void
_cex_arena_rec_set_size(allocator_arena_rec_s* r, u64 s)
{
    r->size_low  = (u32)(u64)s;
    r->size_high = (u8)(((u64)s >> 32) & 0xff);
}

// bits 0-1: align_enc  {0→8, 1→16, 2→32, 3→64}
static u8
_cex_arena_rec_get_align(const allocator_arena_rec_s* r)
{
    return 8 << (r->flags & 0x3);
}

static void
_cex_arena_rec_set_align(allocator_arena_rec_s* r, u8 alignment)
{
    u8 e = (unsigned)__builtin_ctz((unsigned)(alignment)) - 3;
    r->flags = (r->flags & ~0x3) | e;
}

// bit 2: is_free
static bool
_cex_arena_rec_is_free(const allocator_arena_rec_s* r)
{
    return (r->flags >> 2) & 1;
}

static void
_cex_arena_rec_set_free(allocator_arena_rec_s* r)
{
    r->flags |= (1 << 2);
}

static void
_cex_arena_rec_set_used(allocator_arena_rec_s* r)
{
    r->flags &= ~(1 << 2);
}


static void
_cex_allocator_arena__validate(IAllocator self)
{
    (void)self;
#ifndef NDEBUG
    uassert(self != NULL);
    uassert(
        (self->meta.magic_id == CEX_ALLOCATOR_ARENA_MAGIC ||
         self->meta.magic_id == CEX_ALLOCATOR_TEMP_MAGIC) &&
        "bad allocator pointer or mem corruption"
    );
#endif
}


static inline usize
_cex_alloc_estimate_page_size(usize page_size, usize alloc_size)
{
    uassert(alloc_size < CEX_ARENA_MAX_ALLOC && "allocation is to big");
    usize base_page_size = mem$aligned_round(
        page_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN,
        alignof(allocator_arena_page_s)
    );
    uassert(base_page_size % alignof(allocator_arena_page_s) == 0 && "expected to be 64 aligned");

    if (alloc_size > 0.7 * base_page_size) {
        if (alloc_size > 1024 * 1024) {
            alloc_size *= 1.1;
            alloc_size += sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN;
        } else {
            alloc_size *= 2;
        }

        usize result = mem$aligned_round(
            alloc_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN,
            alignof(allocator_arena_page_s)
        );
        uassert(result % alignof(allocator_arena_page_s) == 0 && "expected to be 64 aligned");
        return result;
    } else {
        return base_page_size;
    }
}
static allocator_arena_rec_s
_cex_alloc_estimate_alloc_size(usize alloc_size, usize alignment)
{
    if (alloc_size == 0 || alloc_size > CEX_ARENA_MAX_ALLOC || alignment > CEX_ARENA_MAX_ALIGN) {
        uassert(alloc_size > 0);
        uassert(alloc_size <= CEX_ARENA_MAX_ALLOC && "allocation size is too high");
        uassert(alignment <= CEX_ARENA_MAX_ALIGN);
        return (allocator_arena_rec_s){ 0 };
    }
    usize size = alloc_size;

    if (alignment < 8) {
        static_assert(sizeof(allocator_arena_rec_s) == 8, "unexpected size");
        static_assert(alignof(void*) <= 8, "unexpected ptr alignment");
        alignment = 8;
        size = mem$aligned_round(alloc_size, 8);
    } else {
        uassert(mem$is_power_of2(alignment) && "must be pow2");
        if ((alloc_size & (alignment - 1)) != 0) {
            uassert(alloc_size % alignment == 0 && "requested size is not aligned");
            return (allocator_arena_rec_s){ 0 };
        }
    }

    size += sizeof(allocator_arena_rec_s);

    if (size - alloc_size == sizeof(allocator_arena_rec_s)) {
        size += sizeof(allocator_arena_rec_s); // adding extra bytes for ASAN poison
    }
    uassert(size - alloc_size >= sizeof(allocator_arena_rec_s));
    uassert(size - alloc_size <= 255 - sizeof(allocator_arena_rec_s) && "ptr_offset oveflow");
    uassert(size < alloc_size + 128 && "weird overflow");

    u8 align_enc = (unsigned)__builtin_ctz((unsigned)(alignment)) - 3;
    return (allocator_arena_rec_s){
        .size_low = (u32)alloc_size,
        .size_high = (u8)((u64)alloc_size >> 32),
        .flags = align_enc & 0x3,
        .ptr_padding = size - alloc_size - sizeof(allocator_arena_rec_s),
        .ptr_offset = 0,
    };
}

static inline allocator_arena_rec_s*
_cex_alloc_arena__get_rec(void* alloc_pointer)
{
    uassert(alloc_pointer != NULL);
    u8 offset = *((u8*)alloc_pointer - 1);
    uassert(offset <= CEX_ARENA_MAX_ALIGN);
    return (allocator_arena_rec_s*)((char*)alloc_pointer - offset);
}

static bool
_cex_allocator_arena__check_pointer_valid(AllocatorArena_c* self, void* old_ptr)
{
    uassert(self->scope_depth > 0);
    allocator_arena_page_s* page = self->last_page;
    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(old_ptr);
    while (page) {
        auto tpage = page->prev_page;
        if ((char*)rec > (char*)page &&
            (char*)rec < (((char*)page) + sizeof(allocator_arena_page_s) + page->capacity)) {
            uassert((char*)rec >= (char*)page + sizeof(allocator_arena_page_s));

            usize ptr_scope_mark =
                (((char*)rec) - ((char*)page) - sizeof(allocator_arena_page_s) + page->used_start);

            if (self->scope_depth < sizeof(self->scope_stack) / sizeof((self->scope_stack)[0])) {
                if (ptr_scope_mark < self->scope_stack[self->scope_depth - 1]) {
                    uassert(
                        ptr_scope_mark >= self->scope_stack[self->scope_depth - 1] &&
                        "trying to operate on pointer from different mem$scope() it will lead to use-after-free / ASAN poison issues"
                    );
                    return false; // using pointer out of scope of previous page
                }
            }
            return true;
        }
        page = tpage;
    }
    return false;
}

static allocator_arena_page_s*
_cex_allocator_arena__request_page_size(
    AllocatorArena_c* self,
    allocator_arena_rec_s new_rec,
    bool* out_is_allocated
)
{
    usize req_size = _cex_arena_rec_get_size(&new_rec)
                     + _cex_arena_rec_get_align(&new_rec)
                     + new_rec.ptr_padding;
    if (out_is_allocated) { *out_is_allocated = false; }

    if (self->last_page == NULL ||
        // self->last_page->capacity < req_size + mem$aligned_round(self->last_page->cursor, 8)) {
        self->last_page->capacity < req_size + self->last_page->cursor) {
        usize page_size = _cex_alloc_estimate_page_size(self->page_size, req_size);

        if (page_size == 0 || page_size > CEX_ARENA_MAX_ALLOC) {
            uassert(page_size > 0 && "page_size is zero");
            uassert(page_size <= CEX_ARENA_MAX_ALLOC && "page_size is to big");
            return NULL;
        }
        allocator_arena_page_s*
            page = mem$calloc(mem$, 1, page_size, alignof(allocator_arena_page_s));
        if (page == NULL) {
            return NULL; // memory error
        }

        uassert(mem$aligned_pointer(page, 64) == page);

        page->prev_page = self->last_page;
        page->used_start = self->used;
        page->capacity = page_size - sizeof(allocator_arena_page_s);
        mem$asan_poison(page->__poison_area, sizeof(page->__poison_area));
        mem$asan_poison(&page->data, page->capacity);

        self->last_page = page;
        self->stats.pages_created++;

        if (out_is_allocated) { *out_is_allocated = true; }
    }

    return self->last_page;
}

static void*
_cex_allocator_arena__malloc(IAllocator allc, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    uassert(
        (self->disable_scopes || self->scope_depth > 0)
        && "arena allocation must be performed in mem$scope() block!"
    );

    allocator_arena_rec_s rec = _cex_alloc_estimate_alloc_size(size, alignment);
    if (rec.size_low == 0 && rec.size_high == 0) { return NULL; }

    allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(self, rec, NULL);
    if (page == NULL) { return NULL; }
    uassert(page->capacity - page->cursor >= _cex_arena_rec_get_size(&rec) + rec.ptr_padding + _cex_arena_rec_get_align(&rec));
    uassert(page->cursor % 8 == 0);
    uassert(rec.ptr_padding <= 8);
    uassertf((usize)page->data % 8 == 0, "page.data offset: %zi\n", (page->data - (char*)page));

    allocator_arena_rec_s* page_rec = (allocator_arena_rec_s*)&page->data[page->cursor];
    uassert((((usize)(page_rec) & ((8) - 1)) == 0) && "unaligned pointer");
    static_assert(sizeof(allocator_arena_rec_s) == 8, "unexpected size");
    static_assert(alignof(allocator_arena_rec_s) <= 8, "unexpected alignment");

    mem$asan_unpoison(page_rec, sizeof(allocator_arena_rec_s));
    *page_rec = rec;

    u8 rec_align = _cex_arena_rec_get_align(&rec);
    void* result = mem$aligned_pointer(
        (char*)page_rec + sizeof(allocator_arena_rec_s),
        rec_align
    );

    uassert((char*)result >= ((char*)page_rec) + sizeof(allocator_arena_rec_s));
    rec.ptr_offset = (char*)result - (char*)page_rec;
    uassert(rec.ptr_offset <= rec_align);

    page_rec->ptr_offset = rec.ptr_offset;
    uassert(rec_align <= CEX_ARENA_MAX_ALIGN);

    u64 rec_size = _cex_arena_rec_get_size(&rec);
    mem$asan_unpoison(((char*)result) - 1, rec_size + 1);
    *(((char*)result) - 1) = rec.ptr_offset;

    usize bytes_alloc = rec.ptr_offset + rec_size + rec.ptr_padding;
    self->used += bytes_alloc;
    self->stats.bytes_alloc += bytes_alloc;
    page->cursor += bytes_alloc;
    page->last_alloc = result;
    uassert(page->cursor % 8 == 0);
    uassert(self->used % 8 == 0);
    uassert(((usize)(result) & ((rec_align) - 1)) == 0);


#ifdef CEX_TEST
    // intentionally set malloc to 0xf7 pattern to mark uninitialized data
    memset(result, 0xf7, _cex_arena_rec_get_size(&rec));
#endif

    return result;
}
static void*
_cex_allocator_arena__calloc(IAllocator allc, usize nmemb, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    if (nmemb > CEX_ARENA_MAX_ALLOC) {
        uassert(nmemb < CEX_ARENA_MAX_ALLOC);
        return NULL;
    }
    if (size > CEX_ARENA_MAX_ALLOC) {
        uassert(size < CEX_ARENA_MAX_ALLOC);
        return NULL;
    }
    usize alloc_size = nmemb * size;
    void* result = _cex_allocator_arena__malloc(allc, alloc_size, alignment);
    if (result != NULL) { memset(result, 0, alloc_size); }

    return result;
}

static void*
_cex_allocator_arena__free(IAllocator allc, void* ptr)
{
    (void)ptr;
    // NOTE: this intentionally does nothing, all memory releasing in scope_exit()
    _cex_allocator_arena__validate(allc);

    if (ptr == NULL) { return NULL; }

    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    (void)self;
    if (!self->disable_scopes) {
        uassert(
            _cex_allocator_arena__check_pointer_valid(self, ptr)
            && "pointer doesn't belong to arena"
        );
    }
    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(ptr);
    _cex_arena_rec_set_free(rec);
    mem$asan_poison(ptr, _cex_arena_rec_get_size(rec));

    return NULL;
}

static void*
_cex_allocator_arena__realloc(IAllocator allc, void* old_ptr, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    uassert(old_ptr != NULL);
    uassert(size > 0);
    if (size > CEX_ARENA_MAX_ALLOC) {
        uassert(size <= CEX_ARENA_MAX_ALLOC);
        goto fail;
    }

    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    uassert(
        (self->disable_scopes || self->scope_depth > 0)
        && "arena allocation must be performed in mem$scope() block!"
    );

    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(old_ptr);
    uassert(!_cex_arena_rec_is_free(rec) && "trying to realloc() already freed pointer");
    if (alignment < 8) {
        uassert(_cex_arena_rec_get_align(rec) == 8);
    } else {
        uassert(alignment == _cex_arena_rec_get_align(rec) && "realloc alignment mismatch with old_ptr");
        uassert(((usize)(old_ptr) & ((alignment)-1)) == 0 && "weird old_ptr not aligned");
        uassert(((usize)(size) & ((alignment)-1)) == 0 && "size is not aligned as expected");
    }

    if (!self->disable_scopes) {
        uassert(
            _cex_allocator_arena__check_pointer_valid(self, old_ptr) &&
            "pointer doesn't belong to arena"
        );
    }

    u64 rec_size = _cex_arena_rec_get_size(rec);
    if (unlikely(size <= rec_size)) {
        if (size == rec_size) { return old_ptr; }
        // NOTE: we can't change size/padding of this allocation, because this will break iterating
        // ptr_padding is only u8 size, we cant store size change.
        // We must NOT poison the tail here: a later realloc() growth path copies rec->size bytes
        // from old_ptr via memcpy, and stale poison from a prior shrink would cause use-after-poison.
        return old_ptr;
    }

    if (unlikely(self->last_page && self->last_page->last_alloc == old_ptr)) {
        // Faster path, when last allocation is current item for resizing
        allocator_arena_rec_s nrec = _cex_alloc_estimate_alloc_size(size, alignment);
        if (nrec.size_low == 0 && nrec.size_high == 0) { goto fail; }
        bool is_created = false;
        allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(
            self,
            nrec,
            &is_created
        );
        if (page == NULL) { goto fail; }
        if (!is_created) {
            // If new page was created, fall back to malloc/copy/free method
            //   but currently we have spare capacity for growth
            u64 extra_bytes = size - rec_size;
            mem$asan_unpoison((char*)old_ptr + rec_size, extra_bytes);
#ifdef CEX_TEST
            memset((char*)old_ptr + rec_size, 0xf7, extra_bytes);
#endif
            extra_bytes += (nrec.ptr_padding - rec->ptr_padding);
            page->cursor += extra_bytes;
            self->used += extra_bytes;
            self->stats.bytes_alloc += extra_bytes;
            rec_size = size;
            _cex_arena_rec_set_size(rec, rec_size);
            rec->ptr_padding = nrec.ptr_padding;

            uassert(
                (char*)rec + _cex_arena_rec_get_size(rec) + rec->ptr_padding + rec->ptr_offset ==
                &page->data[page->cursor]
            );
            uassert(page->cursor % 8 == 0);
            uassert(self->used % 8 == 0);
            mem$asan_poison((char*)old_ptr + size, rec->ptr_padding);
            return old_ptr;
        }
        // NOTE: fall through to default way
    }

    void* new_ptr = _cex_allocator_arena__malloc(allc, size, alignment);
    if (new_ptr == NULL) { goto fail; }
    memcpy(new_ptr, old_ptr, _cex_arena_rec_get_size(rec));
    _cex_allocator_arena__free(allc, old_ptr);
    return new_ptr;
fail:
    _cex_allocator_arena__free(allc, old_ptr);
    return NULL;
}


static const struct Allocator_i*
_cex_allocator_arena__scope_enter(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    if (self->disable_scopes) { return allc; }
    // NOTE: If scope_depth is higher CEX_ALLOCATOR_MAX_SCOPE_STACK, we stop marking
    //  all memory will be released after exiting scope_depth == CEX_ALLOCATOR_MAX_SCOPE_STACK
    if (self->scope_depth < sizeof(self->scope_stack) / sizeof((self->scope_stack)[0])) {
        self->scope_stack[self->scope_depth] = self->used;
    }
    self->scope_depth++;
    return allc;
}
static void
_cex_allocator_arena__scope_exit(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    if (self->disable_scopes) { return; }
    uassert(self->scope_depth > 0);

#ifdef CEX_TEST
    bool AllocatorArena_sanitize(IAllocator allc);
    uassert(AllocatorArena_sanitize(allc));
#endif
    self->scope_depth--;
    if (self->scope_depth >= sizeof(self->scope_stack) / sizeof((self->scope_stack)[0])) {
        // Scope overflow, wait until we reach CEX_ALLOCATOR_MAX_SCOPE_STACK
        return;
    }

    usize used_mark = self->scope_stack[self->scope_depth];

    allocator_arena_page_s* page = self->last_page;
    while (page) {
        auto tpage = page->prev_page;
        if (page->used_start == 0 || page->used_start < used_mark) {
            // last page, just set mark and poison
            usize free_offset = (used_mark - page->used_start);
            uassert(page->cursor >= free_offset);

            usize free_len = page->cursor - free_offset;
            page->cursor = free_offset;
            mem$asan_poison(&page->data[free_offset], free_len);

            uassert(self->used >= free_len);
            self->used -= free_len;
            self->stats.bytes_free += free_len;
            break; // we are done
        } else {
            usize free_len = page->cursor;
            uassert(self->used >= free_len);

            self->used -= free_len;
            self->stats.bytes_free += free_len;
            self->last_page = page->prev_page;
            self->stats.pages_free++;
            mem$free(mem$, page);
        }
        page = tpage;
    }
}
static u32
_cex_allocator_arena__scope_depth(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    return self->scope_depth;
}

/// Creates a new arena allocator with keyword args (AllocatorArena_kw), returns an IAllocator
IAllocator
AllocatorArena_create(const AllocatorArena_kw* kwargs)
{
    AllocatorArena_kw kw = {
        .page_size = CEX_ALLOCATOR_TEMP_PAGE_SIZE,
        .disable_scopes = false,
    };
    if (kwargs != NULL) {
        if (kwargs->page_size != 0) { kw.page_size = kwargs->page_size; }
        kw.disable_scopes = kwargs->disable_scopes;
    }

    if (kw.page_size < 1024 || kw.page_size >= CEX_ARENA_MAX_ALLOC) {
        uassert(kw.page_size >= 1024 && "page size is too small");
        uassert(kw.page_size < CEX_ARENA_MAX_ALLOC && "page size is too big");
        return NULL;
    }

    AllocatorArena_c template = {
        .alloc = {
            .malloc = _cex_allocator_arena__malloc,
            .realloc = _cex_allocator_arena__realloc,
            .calloc = _cex_allocator_arena__calloc,
            .free = _cex_allocator_arena__free,
            .scope_enter = _cex_allocator_arena__scope_enter,
            .scope_exit = _cex_allocator_arena__scope_exit,
            .scope_depth = _cex_allocator_arena__scope_depth,
            .meta = {
                .magic_id = CEX_ALLOCATOR_ARENA_MAGIC,
                .is_arena = true, 
                .is_temp = false, 
            }
        },
        .page_size = kw.page_size,
        .disable_scopes = kw.disable_scopes,
    };

    AllocatorArena_c* self = mem$new(mem$, AllocatorArena_c);
    if (self == NULL) {
        return NULL; // memory error
    }

    memcpy(self, &template, sizeof(AllocatorArena_c));
    uassert(self->alloc.meta.magic_id == CEX_ALLOCATOR_ARENA_MAGIC);
    uassert(self->alloc.malloc == _cex_allocator_arena__malloc);

    if (!self->disable_scopes) {
        _cex_allocator_arena__scope_enter(&self->alloc);
    }

    return &self->alloc;
}

/// Validates arena allocator internal state: record headers, poison markers, and page integrity
bool
AllocatorArena_sanitize(IAllocator allc)
{
    (void)allc;
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    if (self->scope_depth == 0 && !self->disable_scopes) {
        uassert(self->stats.bytes_alloc == self->stats.bytes_free && "memory leaks?");
    }
    allocator_arena_page_s* page = self->last_page;
    while (page) {
        uassert(page->cursor <= page->capacity);
        uassert(mem$asan_poison_check(page->__poison_area, sizeof(page->__poison_area)));

        usize i = 0;
        while (i < page->cursor) {
            allocator_arena_rec_s* rec = (allocator_arena_rec_s*)&page->data[i];
            u64 rec_size = _cex_arena_rec_get_size(rec);
            u8  rec_align = _cex_arena_rec_get_align(rec);
            (void)rec_align;
            uassert(rec_size <= page->capacity);
            uassert(rec_size <= page->cursor);
            uassert(rec->ptr_offset <= CEX_ARENA_MAX_ALIGN);
            uassert(rec->ptr_padding <= 16);
            uassert(rec_align <= CEX_ARENA_MAX_ALIGN);
            // is_free is a single bit, always 0 or 1 by construction
            uassert(mem$is_power_of2(rec_align));

            char* alloc_p = ((char*)rec) + rec->ptr_offset;
            u8 poffset = alloc_p[-1];
            (void)poffset;
            uassert(poffset == rec->ptr_offset && "near pointer offset mismatch to rec.ptr_offset");

            if (rec->ptr_padding) {
                uassert(
                    mem$asan_poison_check(alloc_p + rec_size, rec->ptr_padding) &&
                    "poison data overwrite past allocated item"
                );
            }

            if (_cex_arena_rec_is_free(rec)) {
                uassert(
                    mem$asan_poison_check(alloc_p, rec_size) &&
                    "poison data corruction in freed item area"
                );
            }
            i += rec->ptr_padding + rec->ptr_offset + rec_size;
        }
        if (page->cursor < page->capacity) {
            // unallocated page must be poisoned
            uassert(
                mem$asan_poison_check(&page->data[page->cursor], page->capacity - page->cursor) &&
                "poison data overwrite in unallocated area"
            );
        }

        page = page->prev_page;
    }

    return true;
}

/// Destroys arena allocator and frees all allocated pages
void
AllocatorArena_destroy(IAllocator self)
{
    _cex_allocator_arena__validate(self);
    AllocatorArena_c* allc = (AllocatorArena_c*)self;

    if (!allc->disable_scopes) {
        uassert(allc->scope_depth == 1 && "trying to destroy in mem$scope?");
    }
    _cex_allocator_arena__scope_exit(self);

#ifdef CEX_TEST
    uassert(AllocatorArena_sanitize(self));
#endif

    allocator_arena_page_s* page = allc->last_page;
    while (page) {
        auto tpage = page->prev_page;
        mem$free(mem$, page);
        page = tpage;
    }
    mem$free(mem$, allc);
}

#if !cex$is_freestanding
_Thread_local 
#endif
AllocatorArena_c _cex__default_global__allocator_temp = {
    .alloc = {
        .malloc = _cex_allocator_arena__malloc,
        .realloc = _cex_allocator_arena__realloc,
        .calloc = _cex_allocator_arena__calloc,
        .free = _cex_allocator_arena__free,
        .scope_enter = _cex_allocator_arena__scope_enter,
        .scope_exit = _cex_allocator_arena__scope_exit,
        .scope_depth = _cex_allocator_arena__scope_depth,
        .meta = {
            .magic_id = CEX_ALLOCATOR_TEMP_MAGIC,
            .is_arena = true,  // coming... soon
            .is_temp = true, 
        }, 
    },
    .page_size = CEX_ALLOCATOR_TEMP_PAGE_SIZE,
};

CEX_NAMESPACE_DEF struct __cex_namespace__AllocatorArena AllocatorArena = {
    // Autogenerated by CEX
    // clang-format off

    .create = AllocatorArena_create,
    .destroy = AllocatorArena_destroy,
    .sanitize = AllocatorArena_sanitize,

    // clang-format on
};
#endif
