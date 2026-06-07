#if !defined(cex$enable_minimal) || defined(cex$enable_ds)

#include "all.h"


#ifdef _CEXDS_STATISTICS
#    define _CEXDS_STATS(x) x
usize _cexds__array_grow;
usize _cexds__hash_grow;
usize _cexds__hash_shrink;
usize _cexds__hash_rebuild;
usize _cexds__hash_probes;
usize _cexds__hash_alloc;
usize _cexds__rehash_probes;
usize _cexds__rehash_items;
#else
#    define _CEXDS_STATS(x)
#endif

//
// _cexds__arr implementation
//

#define _cexds__item_ptr(t, i, elemsize) ((char*)a + elemsize * i)

static inline void*
_cexds__base(_cexds__array_header* hdr)
{
    if (hdr->el_align <= sizeof(_cexds__array_header)) {
        return hdr;
    } else {
        char* arr_ptr = (char*)hdr + sizeof(_cexds__array_header);
        return arr_ptr - hdr->el_align;
    }
}

bool
_cexds__arr_integrity(const void* arr, usize magic_num)
{
    (void)magic_num;
    (void)arr;

#ifndef NDEBUG
    _cexds__array_header* hdr = _cexds__header(arr);
    (void)hdr;

    uassert(arr != NULL && "array uninitialized or out-of-mem");
    // WARNING: next can trigger sanitizer with "stack/heap-buffer-underflow on address"
    //          when arr pointer is invalid arr$ / hm$ type pointer
    if (magic_num == 0) {
        uassert(((hdr->magic_num == _CEXDS_ARR_MAGIC) || hdr->magic_num == _CEXDS_HM_MAGIC));
    } else {
        uassert(hdr->magic_num == magic_num);
    }

    uassert(mem$asan_poison_check(hdr->__poison_area, sizeof(hdr->__poison_area)));

#endif


    return true;
}

inline usize
_cexds__arr_len(const void* arr)
{
    return (arr) ? _cexds__header(arr)->length : 0;
}

void*
_cexds__arrgrowf(
    void* arr,
    usize elemsize,
    usize addlen,
    usize min_cap,
    u16 el_align,
    IAllocator allc
)
{
    uassert(addlen < PTRDIFF_MAX && "negative or overflow");
    uassert(min_cap < PTRDIFF_MAX && "negative or overflow");
    uassert(el_align <= 64 && "alignment is too high");

    if (arr == NULL) {
        if (allc == NULL) {
            uassert(allc != NULL && "using uninitialized arr/hm or out-of-mem error");
            // unconditionally abort even in production
            abort();
        }
    } else {
        _cexds__arr_integrity(arr, 0);
    }
    usize min_len = (arr ? _cexds__header(arr)->length : 0) + addlen;

    // compute the minimum capacity needed
    if (min_len > min_cap) { min_cap = min_len; }

    // increase needed capacity to guarantee O(1) amortized
    if (min_cap < 2 * arr$cap(arr)) {
        min_cap = 2 * arr$cap(arr);
    } else if (min_cap < 16) {
        min_cap = 16;
    }
    uassert(min_cap < PTRDIFF_MAX && "negative or overflow after processing");
    uassert(addlen > 0 || min_cap > 0);

    if (min_cap <= arr$cap(arr)) { return arr; }

    // General types with alignment <= usize use generic realloc (less mem overhead + realloc faster)
    el_align = (el_align <= alignof(_cexds__array_header)) ? alignof(_cexds__array_header) : 64;

    void* new_arr;
    if (arr == NULL) {
        new_arr = mem$malloc(
            allc,
            mem$aligned_round(elemsize * min_cap + sizeof(_cexds__array_header), el_align),
            el_align
        );
    } else {
        _cexds__array_header* hdr = _cexds__header(arr);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        // NOTE: we must unpoison to prevent false ASAN use-after-poison check if data is copied
        mem$asan_unpoison(hdr->__poison_area, sizeof(hdr->__poison_area));
        new_arr = mem$realloc(
            _cexds__header(arr)->allocator,
            _cexds__base(hdr),
            mem$aligned_round(elemsize * min_cap + sizeof(_cexds__array_header), el_align),
            el_align
        );
    }

    if (new_arr == NULL) {
        // oops memory error
        return NULL;
    }

    new_arr = mem$aligned_pointer(new_arr + sizeof(_cexds__array_header), el_align);
    _cexds__array_header* hdr = _cexds__header(new_arr);
    if (arr == NULL) {
        hdr->length = 0;
        hdr->_hash_table = NULL;
        hdr->allocator = allc;
        hdr->magic_num = _CEXDS_ARR_MAGIC;
        hdr->allocator_scope_depth = allc->scope_depth(allc);
        hdr->el_align = el_align;
        mem$asan_poison(hdr->__poison_area, sizeof(hdr->__poison_area));
    } else {
        uassert(
            (hdr->magic_num == _CEXDS_ARR_MAGIC || hdr->magic_num == _CEXDS_HM_MAGIC) &&
            "bad magic after realloc"
        );
        mem$asan_poison(hdr->__poison_area, sizeof(hdr->__poison_area));
        _CEXDS_STATS(++_cexds__array_grow);
    }
    hdr->capacity = min_cap;

    return new_arr;
}

void
_cexds__arrfreef(void* a)
{
    if (a != NULL) {
        uassert(_cexds__header(a)->allocator != NULL);
        _cexds__array_header* h = _cexds__header(a);
        h->allocator->free(h->allocator, _cexds__base(h));
    }
}

//
// _cexds__hm hash table implementation
//

#define _CEXDS_BUCKET_LENGTH 8
#define _CEXDS_BUCKET_SHIFT (_CEXDS_BUCKET_LENGTH == 8 ? 3 : 2)
#define _CEXDS_BUCKET_MASK ((usize)(_CEXDS_BUCKET_LENGTH - 1))
#define _CEXDS_CACHE_LINE_SIZE 64

#define _cexds__hash_table(a) ((_cexds__hash_index*)_cexds__header(a)->_hash_table)


typedef struct
{
    u64 hash[_CEXDS_BUCKET_LENGTH];
    ptrdiff_t index[_CEXDS_BUCKET_LENGTH];
} _cexds__hash_bucket;
//static_assert(sizeof(_cexds__hash_bucket) % 64 == 0, "cacheline aligned");

typedef struct _cexds__hash_index
{
    usize slot_count;
    usize used_count;
    usize used_count_threshold;
    usize used_count_shrink_threshold;
    usize tombstone_count;
    usize tombstone_count_threshold;
    usize seed;
    usize slot_count_log2;
    enum _CexDsKeyType_e key_type;
    IAllocator key_arena;
    bool copy_keys;

    // not a separate allocation, just 64-byte aligned storage after this struct
    _cexds__hash_bucket* storage;
} _cexds__hash_index;

#define _CEXDS_INDEX_EMPTY -1
#define _CEXDS_INDEX_DELETED -2
#define _CEXDS_INDEX_IN_USE(x) ((x) >= 0)

#define _CEXDS_HASH_EMPTY 0
#define _CEXDS_HASH_DELETED 1

//#define _CEXDS_usize_BITS ((sizeof(usize)) * 8)
#define _CEXDS_usize_BITS ((sizeof(u64)) * 8)

static inline usize
_cexds__probe_position(u64 hash, usize slot_count, usize slot_log2)
{
    usize pos;
    (void)(slot_log2);
    pos = hash & ((u64)slot_count - 1);
#ifdef _CEXDS_INTERNAL_BUCKET_START
    pos &= ~_CEXDS_BUCKET_MASK;
#endif
    return pos;
}

static inline usize
_cexds__log2(usize slot_count)
{
    usize n = 0;
    while (slot_count > 1) {
        slot_count >>= 1;
        ++n;
    }
    return n;
}

void
_cexds__hmclear_func(struct _cexds__hash_index* t, _cexds__hash_index* old_table)
{
    if (t == NULL) {
        // typically external call of uninitialized table
        return;
    }

    uassert(t->slot_count > 0);
    t->tombstone_count = 0;
    t->used_count = 0;

    if (old_table) {
        t->key_arena = old_table->key_arena;
        t->seed = old_table->seed;
        t->key_arena = old_table->key_arena;
        t->copy_keys = old_table->copy_keys;
    } else {
        uassert(t->seed != 0);
    }

    usize i, j;
    for (i = 0; i < t->slot_count >> _CEXDS_BUCKET_SHIFT; ++i) {
        _cexds__hash_bucket* b = &t->storage[i];
        for (j = 0; j < _CEXDS_BUCKET_LENGTH; ++j) { b->hash[j] = _CEXDS_HASH_EMPTY; }
        for (j = 0; j < _CEXDS_BUCKET_LENGTH; ++j) { b->index[j] = _CEXDS_INDEX_EMPTY; }
    }
}

static _cexds__hash_index*
_cexds__make_hash_index(
    usize slot_count,
    _cexds__hash_index* old_table,
    IAllocator allc,
    usize hash_seed,
    enum _CexDsKeyType_e key_type
)
{
    _cexds__hash_index* t = mem$calloc(
        allc,
        1,
        (slot_count >> _CEXDS_BUCKET_SHIFT) * sizeof(_cexds__hash_bucket) +
            sizeof(_cexds__hash_index) + _CEXDS_CACHE_LINE_SIZE - 1
    );
    if (t == NULL) {
        return NULL; // memory error
    }
    t->storage = (_cexds__hash_bucket*)mem$aligned_pointer((usize)(t + 1), _CEXDS_CACHE_LINE_SIZE);
    t->slot_count = slot_count;
    t->slot_count_log2 = _cexds__log2(slot_count);
    t->tombstone_count = 0;
    t->used_count = 0;
    t->key_type = key_type;
    t->seed = hash_seed;

    // compute without overflowing
    t->used_count_threshold = slot_count - (slot_count >> 2);
    t->tombstone_count_threshold = (slot_count >> 3) + (slot_count >> 4);
    t->used_count_shrink_threshold = slot_count >> 2;

    if (slot_count <= _CEXDS_BUCKET_LENGTH) { t->used_count_shrink_threshold = 0; }
    // to avoid infinite loop, we need to guarantee that at least one slot is empty and will
    // terminate probes
    uassert(t->used_count_threshold + t->tombstone_count_threshold < t->slot_count);
    _CEXDS_STATS(++_cexds__hash_alloc);

    _cexds__hmclear_func(t, old_table);

    // copy out the old data, if any
    if (old_table) {
        usize i, j;
        t->used_count = old_table->used_count;
        for (i = 0; i < old_table->slot_count >> _CEXDS_BUCKET_SHIFT; ++i) {
            _cexds__hash_bucket* ob = &old_table->storage[i];
            for (j = 0; j < _CEXDS_BUCKET_LENGTH; ++j) {
                if (_CEXDS_INDEX_IN_USE(ob->index[j])) {
                    u64 hash = ob->hash[j];
                    usize pos = _cexds__probe_position(hash, t->slot_count, t->slot_count_log2);
                    usize step = _CEXDS_BUCKET_LENGTH;
                    _CEXDS_STATS(++_cexds__rehash_items);
                    for (;;) {
                        usize limit, z;
                        _cexds__hash_bucket* bucket;
                        bucket = &t->storage[pos >> _CEXDS_BUCKET_SHIFT];
                        _CEXDS_STATS(++_cexds__rehash_probes);

                        for (z = pos & _CEXDS_BUCKET_MASK; z < _CEXDS_BUCKET_LENGTH; ++z) {
                            if (bucket->hash[z] == 0) {
                                bucket->hash[z] = hash;
                                bucket->index[z] = ob->index[j];
                                goto done;
                            }
                        }

                        limit = pos & _CEXDS_BUCKET_MASK;
                        for (z = 0; z < limit; ++z) {
                            if (bucket->hash[z] == 0) {
                                bucket->hash[z] = hash;
                                bucket->index[z] = ob->index[j];
                                goto done;
                            }
                        }

                        pos += step; // quadratic probing
                        step += _CEXDS_BUCKET_LENGTH;
                        pos &= (t->slot_count - 1);
                    }
                }
            done:;
            }
        }
    }

    return t;
}

#define _CEXDS_ROTATE_LEFT(val, n) (((val) << (n)) | ((val) >> (_CEXDS_usize_BITS - (n))))
#define _CEXDS_ROTATE_RIGHT(val, n) (((val) >> (n)) | ((val) << (_CEXDS_usize_BITS - (n))))


#ifdef _CEXDS_SIPHASH_2_4
#    define _CEXDS_SIPHASH_C_ROUNDS 2
#    define _CEXDS_SIPHASH_D_ROUNDS 4
typedef int _CEXDS_SIPHASH_2_4_can_only_be_used_in_64_bit_builds[sizeof(usize) == 8 ? 1 : -1];
#endif

#ifndef _CEXDS_SIPHASH_C_ROUNDS
#    define _CEXDS_SIPHASH_C_ROUNDS 1
#endif
#ifndef _CEXDS_SIPHASH_D_ROUNDS
#    define _CEXDS_SIPHASH_D_ROUNDS 1
#endif

static inline u64
_cexds__hash_string(const char* str, usize str_cap, u64 seed)
{
    u64 hash = seed;
    // NOTE: using max buffer capacity capping, this allows using hash
    //       on char buf[N] - without stack overflowing
    for (usize i = 0; i < str_cap && *str; i++) {
        hash = _CEXDS_ROTATE_LEFT(hash, 9) + (unsigned char)*str++;
    }

    // Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
    hash ^= seed;
    hash = (~hash) + (hash << 18);
    hash ^= hash ^ _CEXDS_ROTATE_RIGHT(hash, 31);
    hash = hash * 21;
    hash ^= hash ^ _CEXDS_ROTATE_RIGHT(hash, 11);
    hash += (hash << 6);
    hash ^= _CEXDS_ROTATE_RIGHT(hash, 22);
    return hash + seed;
}

static inline u64
_cexds__siphash_bytes(const void* p, usize len, u64 seed)
{
    // hash that works on 32- or 64-bit registers without knowing which we have
    // (computes different results on 32-bit and 64-bit platform)
    // derived from siphash, but on 32-bit platforms very different as it uses 4 32-bit state not 4
    // 64-bit
    u64 v0 = ((((u64)0x736f6d65 << 16) << 16) + 0x70736575) ^ seed;
    u64 v1 = ((((u64)0x646f7261 << 16) << 16) + 0x6e646f6d) ^ ~seed;
    u64 v2 = ((((u64)0x6c796765 << 16) << 16) + 0x6e657261) ^ seed;
    u64 v3 = ((((u64)0x74656462 << 16) << 16) + 0x79746573) ^ ~seed;

#ifdef _CEXDS_TEST_SIPHASH_2_4
    // hardcoded with key material in the siphash test vectors
    v0 ^= 0x0706050403020100ull ^ seed;
    v1 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
    v2 ^= 0x0706050403020100ull ^ seed;
    v3 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
#endif

#define _CEXDS_SIPROUND()                                                                          \
    do {                                                                                           \
        v0 += v1;                                                                                  \
        v1 = _CEXDS_ROTATE_LEFT(v1, 13);                                                           \
        v1 ^= v0;                                                                                  \
        v0 = _CEXDS_ROTATE_LEFT(v0, _CEXDS_usize_BITS / 2);                                        \
        v2 += v3;                                                                                  \
        v3 = _CEXDS_ROTATE_LEFT(v3, 16);                                                           \
        v3 ^= v2;                                                                                  \
        v2 += v1;                                                                                  \
        v1 = _CEXDS_ROTATE_LEFT(v1, 17);                                                           \
        v1 ^= v2;                                                                                  \
        v2 = _CEXDS_ROTATE_LEFT(v2, _CEXDS_usize_BITS / 2);                                        \
        v0 += v3;                                                                                  \
        v3 = _CEXDS_ROTATE_LEFT(v3, 21);                                                           \
        v3 ^= v0;                                                                                  \
    } while (0)

    unsigned char* d = (unsigned char*)p;
    u64 data = 0;
    u64 i = 0;
    u64 j = 0;
    for (i = 0; i + sizeof(u64) <= len; i += sizeof(u64), d += sizeof(u64)) {
        data = (u64)d[0] | ((u64)d[1] << 8) | ((u64)d[2] << 16) | ((u64)d[3] << 24);
        data |= ((u64)d[4] | ((u64)d[5] << 8) | ((u64)d[6] << 16) | ((u64)d[7] << 24)) << 16 << 16;

        v3 ^= data;
        for (j = 0; j < _CEXDS_SIPHASH_C_ROUNDS; ++j) { _CEXDS_SIPROUND(); }
        v0 ^= data;
    }
    data = (u64)len << (_CEXDS_usize_BITS - 8);
    switch (len - i) {
        case 7:
            data |= ((u64)d[6] << 24) << 24; // fall through
        case 6:
            data |= ((u64)d[5] << 20) << 20; // fall through
        case 5:
            data |= ((u64)d[4] << 16) << 16; // fall through
        case 4:
            data |= ((u64)d[3] << 24); // fall through
        case 3:
            data |= (d[2] << 16); // fall through
        case 2:
            data |= (d[1] << 8); // fall through
        case 1:
            data |= d[0]; // fall through
        case 0:
            break;
    }
    v3 ^= data;
    for (j = 0; j < _CEXDS_SIPHASH_C_ROUNDS; ++j) { _CEXDS_SIPROUND(); }
    v0 ^= data;
    v2 ^= 0xff;
    for (j = 0; j < _CEXDS_SIPHASH_D_ROUNDS; ++j) { _CEXDS_SIPROUND(); }

#ifdef _CEXDS_SIPHASH_2_4
    return v0 ^ v1 ^ v2 ^ v3;
#else
    return v1 ^ v2 ^ v3;
#endif
}

u64
_cexds__hash_bytes(const void* p, usize len, u64 seed)
{
    if (unlikely(p == NULL || len == 0)) { return 0; }
#ifdef _CEXDS_SIPHASH_2_4
    return _cexds__siphash_bytes(p, len, seed);
#else
    unsigned char* d = (unsigned char*)p;

    if (unlikely(len == 4)) {
        u64 hash = (u32)d[0] | ((u32)d[1] << 8) | ((u32)d[2] << 16) | ((u32)d[3] << 24);
        // HASH32-BB  Bob Jenkin's presumably-accidental version of Thomas Wang hash with rotates
        // turned into shifts. Note that converting these back to rotates makes it run a lot slower,
        // presumably due to collisions, so I'm not really sure what's going on.
        hash ^= seed;
        hash = (hash ^ 61) ^ (hash >> 16);
        hash = hash + (hash << 3);
        hash = hash ^ (hash >> 4);
        hash = hash * 0x27d4eb2d;
        hash ^= seed;
        hash = hash ^ (hash >> 15);
        return (((u64)hash << 16 << 16) | hash) ^ seed;
    } else if (unlikely(len == 8)) {
        u64 hash = (u64)d[0] | ((u64)d[1] << 8) | ((u64)d[2] << 16) | ((u64)d[3] << 24);

        hash |= (u64)((u64)d[4] | ((u64)d[5] << 8) | ((u64)d[6] << 16) | ((u64)d[7] << 24))
             << 16 << 16;
        hash ^= seed;
        hash = (~hash) + (hash << 21);
        hash ^= _CEXDS_ROTATE_RIGHT(hash, 24);
        hash *= 265;
        hash ^= _CEXDS_ROTATE_RIGHT(hash, 14);
        hash ^= seed;
        hash *= 21;
        hash ^= _CEXDS_ROTATE_RIGHT(hash, 28);
        hash += (hash << 31);
        hash = (~hash) + (hash << 18);
        return hash;
    } else {
        return _cexds__siphash_bytes(p, len, seed);
    }
#endif
}

static inline u64
_cexds__hash(enum _CexDsKeyType_e key_type, const void* key, usize key_size, u64 seed)
{
    switch (key_type) {
        case _CexDsKeyType__generic:
            return _cexds__hash_bytes(key, key_size, seed);

        case _CexDsKeyType__charptr:
            // NOTE: we can't know char* length without touching it,
            // 65536 is a max key length in case of char was not null term
            return _cexds__hash_string(*(char**)key, 65536, seed);

        case _CexDsKeyType__charbuf:
            return _cexds__hash_string(key, key_size, seed);

        case _CexDsKeyType__cexstr: {
            str_s* s = (str_s*)key;
            return _cexds__hash_string(s->buf, s->len, seed);
        }
    }
    uassert(false && "unexpected key type");
    abort();
}

static bool
_cexds__is_key_equal(
    void* a,
    usize elemsize,
    void* key,
    usize keysize,
    usize keyoffset,
    enum _CexDsKeyType_e key_type,
    usize i
)
{
    void* hm_key = _cexds__item_ptr(a, i, elemsize) + keyoffset;

    switch (key_type) {
        case _CexDsKeyType__generic:
            return 0 == memcmp(key, hm_key, keysize);

        case _CexDsKeyType__charptr:
            return 0 == strcmp(*(char**)key, *(char**)hm_key);
        case _CexDsKeyType__charbuf:
            return 0 == strcmp((char*)key, (char*)hm_key);

        case _CexDsKeyType__cexstr: {
            str_s* _k = (str_s*)key;
            str_s* _hm = (str_s*)hm_key;
            if (_k->len != _hm->len) { return false; }
            return 0 == memcmp(_k->buf, _hm->buf, _k->len);
        }
    }
    uassert(false && "unexpected key type");
    abort();
}

static inline void*
_cexds__hmkey_ptr(void* a, usize elemsize, usize index, usize keyoffset)
{
    _cexds__hash_index* table = (_cexds__hash_index*)_cexds__header(a)->_hash_table;
    void* key_data_p = NULL;
    switch (table->key_type) {
        case _CexDsKeyType__generic:
            key_data_p = (char*)a + elemsize * index + keyoffset;
            break;

        case _CexDsKeyType__charbuf:
            key_data_p = (char*)((char*)a + elemsize * index + keyoffset);
            break;

        case _CexDsKeyType__charptr:
            key_data_p = *(char**)((char*)a + elemsize * index + keyoffset);
            break;

        case _CexDsKeyType__cexstr: {
            str_s* s = (str_s*)((char*)a + elemsize * index + keyoffset);
            key_data_p = s;
            break;
        }
        default:
            unreachable();
    }
    return key_data_p;
}

void
_cexds__hmfree_keys_func(void* a, usize elemsize, usize keyoffset)
{
    _cexds__array_header* h = _cexds__header(a);
    uassert(h->allocator != NULL);
    _cexds__hash_index* table = h->_hash_table;

    if (table != NULL) {
        if (table->copy_keys) {
            if (table->key_arena == NULL) {
                for (usize i = 0; i < h->length; ++i) {
                    h->allocator->free(h->allocator, *(char**)((char*)a + elemsize * i + keyoffset));
                }
            } else {
                uassert(table->key_arena->scope_depth(table->key_arena) > 0);
                // Exit + enter for arena is equivalent of cleaning up and poisoning
                table->key_arena->scope_exit(table->key_arena);
                table->key_arena->scope_enter(table->key_arena);
            }
        }
    }
}
void
_cexds__hmfree_func(void* a, usize elemsize, usize keyoffset)
{
    if (a == NULL) { return; }
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);

    _cexds__array_header* h = _cexds__header(a);
    _cexds__hmfree_keys_func(a, elemsize, keyoffset);
    if (h->_hash_table) {
        if (h->_hash_table->key_arena) { AllocatorArena.destroy(h->_hash_table->key_arena); }
        h->allocator->free(h->allocator, h->_hash_table);
    }
    h->allocator->free(h->allocator, _cexds__base(h));
}

static ptrdiff_t
_cexds__hm_find_slot(void* a, usize elemsize, void* key, usize keysize, usize keyoffset)
{
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);
    _cexds__hash_index* table = _cexds__hash_table(a);
    enum _CexDsKeyType_e key_type = table->key_type;
    u64 hash = _cexds__hash(key_type, key, keysize, table->seed);
    usize step = _CEXDS_BUCKET_LENGTH;

    if (hash < 2) {
        hash += 2; // stored hash values are forbidden from being 0, so we can detect empty slots
    }

    usize pos = _cexds__probe_position(hash, table->slot_count, table->slot_count_log2);

    // TODO: check when this could be infinite loop (due to overflow or something)?
    for (;;) {
        _CEXDS_STATS(++_cexds__hash_probes);
        _cexds__hash_bucket* bucket = &table->storage[pos >> _CEXDS_BUCKET_SHIFT];

        // start searching from pos to end of bucket, this should help performance on small hash
        // tables that fit in cache
        for (usize i = pos & _CEXDS_BUCKET_MASK; i < _CEXDS_BUCKET_LENGTH; ++i) {
            if (bucket->hash[i] == hash) {
                if (_cexds__is_key_equal(
                        a,
                        elemsize,
                        key,
                        keysize,
                        keyoffset,
                        key_type,
                        bucket->index[i]
                    )) {
                    return (pos & ~_CEXDS_BUCKET_MASK) + i;
                }
            } else if (bucket->hash[i] == _CEXDS_HASH_EMPTY) {
                return -1;
            }
        }

        // search from beginning of bucket to pos
        usize limit = pos & _CEXDS_BUCKET_MASK;
        for (usize i = 0; i < limit; ++i) {
            if (bucket->hash[i] == hash) {
                if (_cexds__is_key_equal(
                        a,
                        elemsize,
                        key,
                        keysize,
                        keyoffset,
                        key_type,
                        bucket->index[i]
                    )) {
                    return (pos & ~_CEXDS_BUCKET_MASK) + i;
                }
            } else if (bucket->hash[i] == _CEXDS_HASH_EMPTY) {
                return -1;
            }
        }

        // quadratic probing
        pos += step;
        step += _CEXDS_BUCKET_LENGTH;
        pos &= (table->slot_count - 1);
    }
}

void*
_cexds__hmget_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset)
{
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);

    _cexds__hash_index* table = (_cexds__hash_index*)_cexds__header(a)->_hash_table;
    if (table != NULL) {
        ptrdiff_t slot = _cexds__hm_find_slot(a, elemsize, key, keysize, keyoffset);
        if (slot >= 0) {
            _cexds__hash_bucket* b = &table->storage[slot >> _CEXDS_BUCKET_SHIFT];
            usize idx = b->index[slot & _CEXDS_BUCKET_MASK];
            return ((char*)a + elemsize * idx);
        }
    }
    return NULL;
}

void*
_cexds__hminit(
    usize elemsize,
    IAllocator allc,
    enum _CexDsKeyType_e key_type,
    u16 el_align,
    struct _cexds__hm_new_kwargs_s* kwargs
)
{
    usize capacity = 0;
    usize hm_seed = 0xBadB0dee;
    bool copy_keys = false;

    if (kwargs) {
        capacity = kwargs->capacity;
        hm_seed = kwargs->seed ? kwargs->seed : hm_seed;
        copy_keys = kwargs->copy_keys;
    }
    void* a = _cexds__arrgrowf(NULL, elemsize, 0, capacity, el_align, allc);
    if (a == NULL) {
        return NULL; // memory error
    }

    uassert(_cexds__header(a)->magic_num == _CEXDS_ARR_MAGIC);
    uassert(_cexds__header(a)->_hash_table == NULL);

    _cexds__header(a)->magic_num = _CEXDS_HM_MAGIC;

    _cexds__hash_index* table = _cexds__header(a)->_hash_table;

    // ensure slot counts must be pow of 2
    uassert(mem$is_power_of2(arr$cap(a)));
    table = _cexds__header(a)->_hash_table = _cexds__make_hash_index(
        arr$cap(a),
        NULL,
        _cexds__header(a)->allocator,
        hm_seed,
        key_type
    );

    if (table) {
        // NEW Table initialization here
        if (copy_keys) {
            uassert(table->key_type == _CexDsKeyType__charptr && "Only char* keys supported");
        }
        table->copy_keys = copy_keys;
        if (kwargs && kwargs->copy_keys_arena_pgsize > 0) {
            table->key_arena = AllocatorArena.create(
    &(AllocatorArena_kw){ .page_size = kwargs->copy_keys_arena_pgsize }
);
        }
    }

    return a;
}


void*
_cexds__hmput_key(
    void* a,
    usize elemsize,
    void* key,
    usize keysize,
    usize keyoffset,
    void* full_elem,
    void* result
)
{
    uassert(result != NULL);
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);

    void** out_result = (void**)result;
    _cexds__hash_index* table = (_cexds__hash_index*)_cexds__header(a)->_hash_table;
    uassert(table != NULL);
    if (table == NULL) { *out_result = NULL; goto end; }
    enum _CexDsKeyType_e key_type = table->key_type;
    *out_result = NULL;
    if (table->used_count >= table->used_count_threshold) {

        usize slot_count = (table == NULL) ? _CEXDS_BUCKET_LENGTH : table->slot_count * 2;
        _cexds__array_header* hdr = _cexds__header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        _cexds__hash_index* nt = _cexds__make_hash_index(
            slot_count,
            table,
            _cexds__header(a)->allocator,
            table->seed,
            table->key_type
        );

        if (nt == NULL) {
            uassert(nt != NULL && "new hash table memory error");
            *out_result = NULL;
            goto end;
        }

        _cexds__header(a)->allocator->free(_cexds__header(a)->allocator, table);
        _cexds__header(a)->_hash_table = table = nt;
        _CEXDS_STATS(++_cexds__hash_grow);
    }

    // we iterate hash table explicitly because we want to track if we saw a tombstone
    {
        u64 hash = _cexds__hash(key_type, key, keysize, table->seed);
        usize step = _CEXDS_BUCKET_LENGTH;
        usize pos;
        ptrdiff_t tombstone = -1;
        _cexds__hash_bucket* bucket;

        // stored hash values are forbidden from being 0, so we can detect empty slots to early out
        // quickly
        if (hash < 2) { hash += 2; }

        pos = _cexds__probe_position(hash, table->slot_count, table->slot_count_log2);

        for (;;) {
            usize limit, i;
            _CEXDS_STATS(++_cexds__hash_probes);
            bucket = &table->storage[pos >> _CEXDS_BUCKET_SHIFT];

            // start searching from pos to end of bucket
            for (i = pos & _CEXDS_BUCKET_MASK; i < _CEXDS_BUCKET_LENGTH; ++i) {
                if (bucket->hash[i] == hash) {
                    if (_cexds__is_key_equal(
                            a,
                            elemsize,
                            key,
                            keysize,
                            keyoffset,
                            key_type,
                            bucket->index[i]
                        )) {

                        *out_result = _cexds__item_ptr(a, bucket->index[i], elemsize);
                        goto process_key;
                    }
                } else if (bucket->hash[i] == 0) {
                    pos = (pos & ~_CEXDS_BUCKET_MASK) + i;
                    goto found_empty_slot;
                } else if (tombstone < 0) {
                    if (bucket->index[i] == _CEXDS_INDEX_DELETED) {
                        tombstone = (ptrdiff_t)((pos & ~_CEXDS_BUCKET_MASK) + i);
                    }
                }
            }

            // search from beginning of bucket to pos
            limit = pos & _CEXDS_BUCKET_MASK;
            for (i = 0; i < limit; ++i) {
                if (bucket->hash[i] == hash) {
                    if (_cexds__is_key_equal(
                            a,
                            elemsize,
                            key,
                            keysize,
                            keyoffset,
                            key_type,
                            bucket->index[i]
                        )) {
                        *out_result = _cexds__item_ptr(a, bucket->index[i], elemsize);
                        goto process_key;
                    }
                } else if (bucket->hash[i] == 0) {
                    pos = (pos & ~_CEXDS_BUCKET_MASK) + i;
                    goto found_empty_slot;
                } else if (tombstone < 0) {
                    if (bucket->index[i] == _CEXDS_INDEX_DELETED) {
                        tombstone = (ptrdiff_t)((pos & ~_CEXDS_BUCKET_MASK) + i);
                    }
                }
            }

            // quadratic probing
            pos += step;
            step += _CEXDS_BUCKET_LENGTH;
            pos &= (table->slot_count - 1);
        }
    found_empty_slot:
        if (tombstone >= 0) {
            pos = tombstone;
            --table->tombstone_count;
        }
        ++table->used_count;

        {
            ptrdiff_t i = (ptrdiff_t)_cexds__header(a)->length;
            // we want to do _cexds__arraddn(1), but we can't use the macros since we don't have
            // something of the right type
            if ((usize)i + 1 > arr$cap(a)) {
                *(void**)&a = _cexds__arrgrowf(a, elemsize, 1, 0, _cexds__header(a)->el_align, NULL);
                if (a == NULL) {
                    uassert(a != NULL && "new array for table memory error");
                    *out_result = NULL;
                    goto end;
                }
            }

            uassert((usize)i + 1 <= arr$cap(a));
            _cexds__header(a)->length = i + 1;
            bucket = &table->storage[pos >> _CEXDS_BUCKET_SHIFT];
            bucket->hash[pos & _CEXDS_BUCKET_MASK] = hash;
            bucket->index[pos & _CEXDS_BUCKET_MASK] = i;
            *out_result = _cexds__item_ptr(a, i, elemsize);
        }
        goto process_key;
    }

process_key:
    uassert(*out_result != NULL);
    if (full_elem) {
        memcpy(((char*)*out_result), full_elem, elemsize);
    } else {
        memcpy(((char*)*out_result) + keyoffset, key, keysize);
    }
    if (table->copy_keys) {
        uassert(key_type == _CexDsKeyType__charptr);
        uassert(keysize == sizeof(usize) && "expected pointer size");
        char** key_data_p = (char**)(*out_result + keyoffset);

        // Naive reimplementation of str.close() for optional isolation
        if (*key_data_p) {
            usize slen = strlen(*key_data_p);
            uassert(slen < PTRDIFF_MAX);
            IAllocator _allc = (table->key_arena) ? table->key_arena : _cexds__header(a)->allocator;

            char* result = mem$malloc(_allc, slen + 1);
            if (result) {
                memcpy(result, *key_data_p, slen);
                result[slen] = '\0';
            }
            *key_data_p = result;
        }
        

    }

end:
    return a;
}


bool
_cexds__hmdel_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset)
{
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);

    _cexds__hash_index* table = (_cexds__hash_index*)_cexds__header(a)->_hash_table;
    uassert(_cexds__header(a)->allocator != NULL);
    if (table == NULL) { return false; }

    ptrdiff_t slot = _cexds__hm_find_slot(a, elemsize, key, keysize, keyoffset);
    if (slot < 0) { return false; }

    _cexds__hash_bucket* b = &table->storage[slot >> _CEXDS_BUCKET_SHIFT];
    int i = slot & _CEXDS_BUCKET_MASK;
    ptrdiff_t old_index = b->index[i];
    ptrdiff_t final_index = (ptrdiff_t)_cexds__header(a)->length - 1;
    uassert(slot < (ptrdiff_t)table->slot_count);
    uassert(table->used_count > 0);
    --table->used_count;
    ++table->tombstone_count;
    // uassert(table->tombstone_count < table->slot_count/4);
    b->hash[i] = _CEXDS_HASH_DELETED;
    b->index[i] = _CEXDS_INDEX_DELETED;

    if (table->copy_keys) {
        if (table->key_arena == NULL) {
            IAllocator _allc = _cexds__header(a)->allocator;
            _allc->free(_allc, *(char**)((char*)a + elemsize * old_index + keyoffset));
        }
    }

    // if indices are the same, memcpy is a no-op, but back-pointer-fixup will fail, so
    // skip
    if (old_index != final_index) {
        // Replacing deleted element by last one, and update hashmap buckets for last element
        memmove((char*)a + elemsize * old_index, (char*)a + elemsize * final_index, elemsize);

        void* key_data_p = _cexds__hmkey_ptr(a, elemsize, old_index, keyoffset);
        uassert(key_data_p != NULL);
        slot = _cexds__hm_find_slot(a, elemsize, key_data_p, keysize, keyoffset);
        uassert(slot >= 0);
        b = &table->storage[slot >> _CEXDS_BUCKET_SHIFT];
        i = slot & _CEXDS_BUCKET_MASK;
        uassert(b->index[i] == final_index);
        b->index[i] = old_index;
    }
    _cexds__header(a)->length -= 1;

    if (table->used_count < table->used_count_shrink_threshold &&
        table->slot_count > _CEXDS_BUCKET_LENGTH) {
        _cexds__array_header* hdr = _cexds__header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        _cexds__header(a)->_hash_table = _cexds__make_hash_index(
            table->slot_count >> 1,
            table,
            _cexds__header(a)->allocator,
            table->seed,
            table->key_type
        );
        _cexds__header(a)->allocator->free(_cexds__header(a)->allocator, table);
        _CEXDS_STATS(++_cexds__hash_shrink);
    } else if (table->tombstone_count > table->tombstone_count_threshold) {
        _cexds__array_header* hdr = _cexds__header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        _cexds__header(a)->_hash_table = _cexds__make_hash_index(
            table->slot_count,
            table,
            _cexds__header(a)->allocator,
            table->seed,
            table->key_type
        );
        _cexds__header(a)->allocator->free(_cexds__header(a)->allocator, table);
        _CEXDS_STATS(++_cexds__hash_rebuild);
    }

    return a;
}

#endif
