#include "dict.h"
#include <time.h>

static inline u64
hm_int_hash_simple(u64 x)
{
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

/**
 * @brief Hashmap: int compare function
 *
 * @param a as u64
 * @param b as u64
 * @param udata (unused)
 * @return
 */
static int
hm_int_compare(const void* a, const void* b, void* udata)
{
    (void)udata;
    uassert(a != NULL && "must be set");
    uassert(b != NULL && "must be set");

    uassert((size_t)a % alignof(u64) == 0 && "a operand not aligned to u64");
    uassert((size_t)b % alignof(u64) == 0 && "b operand not aligned to u64");

    return (*(u64*)a) - (*(u64*)b);
}


/**
 * @brief Hashmap: int hash function, for making ticker_id -> hash (uniformly distrib)
 *
 * @param item (u64) item (ticker id)
 * @param seed0 (unused)
 * @param seed1 (unused)
 * @return hash value
 */
static u64
hm_int_hash(const void* item, u64 seed0, u64 seed1)
{
    (void)seed0;
    (void)seed1;
    return hm_int_hash_simple(*(u64*)item);
}


/**
 * @brief Hashmap: string value comparison (simply uses strcmp())
 *
 * @param a  char* string
 * @param b  char* string
 * @param udata  (unused)
 * @return compared int value
 */
static int
hm_str_compare(const void* a, const void* b, void* udata)
{
    (void)udata;
    return strcmp(a, b);
}

static int
hm_str_static_compare(const void* a, const void* b, void* udata)
{
    (void)udata;
    return strcmp(a, b);
}


/**
 * @brief Hashmap: string hash function (uses SIP hash)
 *
 * @param item char* string
 * @param seed0 hashmap_sip(..., seed0)
 * @param seed1 hashmap_sip(..., seed1)
 * @return string hash
 */
static u64
hm_str_hash(const void* item, u64 seed0, u64 seed1)
{
    return hashmap_sip(item, strlen((char*)item), seed0, seed1);
}

static u64
hm_str_static_hash(const void* item, u64 seed0, u64 seed1)
{
    return hashmap_sip(item, strlen((char*)item), seed0, seed1);
}

/**
 * @brief New dict() with u64 key
 *
 * @param item_size size of item strut
 * @param capacity dict capacity
 * @return dict malloc'ed pointer, or NULL on error
 */
dict_c*
dict_create_u64(size_t item_size, u32 capacity)
{
    uassert(item_size > sizeof(u64) && "item is too small");

    return hashmap_new_with_allocator(
        malloc,
        realloc,
        free,
        item_size,
        capacity,
        605948012320,   // seed0
        12093807012309, // seed1
        hm_int_hash,
        hm_int_compare,
        NULL,
        NULL
    );
}

Exception
dict_create(
    dict_c** self,
    size_t item_size,
    size_t item_align,
    size_t item_key_offsetof,
    size_t capacity,
    u64 (*hash_func)(const void* item, u64 seed0, u64 seed1),
    i32 (*compare_func)(const void* a, const void* b, void* udata),
    const Allocator_c* allocator,
    void (*elfree)(void* item),
    void* udata
)
{

    if (item_key_offsetof != 0) {
        uassert(
            item_key_offsetof != 0 &&
            "hashtable key offset must be 1st in struct, or set item_key_offsetof to 0 and custom hash/compare funcs"
        );
        return Error.integrity;
    }

    if (item_align > alignof(size_t)) {
        uassert(item_align <= alignof(size_t) && "item alignment exceed regular pointer alignment");
        return Error.argument;
    }

    if (item_size <= sizeof(u64)){
        uassert(item_size > sizeof(u64) && "item_size is too small");
        return Error.argument;
    }

    time_t now = time(NULL);

    *self = hashmap_new_with_allocator(
        allocator->alloc,
        allocator->realloc,
        allocator->free,
        item_size,
        capacity,
        now,                     // seed0
        hm_int_hash_simple(now), // seed1
        hash_func,
        compare_func,
        elfree,
        udata
    );
    if (*self == NULL) {
        return Error.memory;
    }

    return Error.ok;
}

/**
 * @brief New dict() with char[const N] key
 *
 * @param item_size size of item struct
 * @param capacity  initial capacity
 * @return  dict malloc'ed pointer or NULL
 */
dict_c*
dict_create_str(size_t item_size, u32 capacity)
{
    uassert(item_size > 20 && "item is too small");

    return hashmap_new_with_allocator(
        malloc,
        realloc,
        free,
        item_size,
        capacity,
        605948012320,   // seed0
        12093807012309, // seed1
        hm_str_hash,
        hm_str_compare,
        NULL,
        NULL
    );
}


/**
 * @brief Set or replace dict item
 *
 * @param self dict() instance
 * @param item  item key/value struct
 * @return error code, EOK (0!) on success, positive on failure
 */
Exception
dict_set(dict_c* self, const void* item)
{
    const void* set_result = hashmap_set(self, item);
    if (set_result == NULL && hashmap_oom(self)) {
        return Error.memory;
    }

    return EOK;
}

/**
 * @brief Get item by integer key
 *
 * @param self dict() instance
 * @param key u64 key
 */
void*
dict_geti(dict_c* self, u64 key)
{
    return (void*)hashmap_get(self, &key);
}

/**
 * @brief Get item by string key
 *
 * @param self dict_c instance
 * @param key string key
 */
void*
dict_gets(dict_c* self, const char* key)
{
    return (void*)hashmap_get(self, key);
}

/**
 * @brief Get item by generic key pointer
 *
 * @param self dict() instance
 * @param key generic pointer key
 */
void*
dict_get(dict_c* self, const void* key)
{
    return (void*)hashmap_get(self, key);
}


/**
 * @brief Number elements in dict()
 *
 * @param self  dict() instance
 * @return number
 */
size_t
dict_len(dict_c* self)
{
    return hashmap_count(self);
}


/**
 * @brief Free dict() instance
 *
 * @param self  dict() instance
 * @return always NULL
 */
dict_c*
dict_destroy(dict_c** self)
{
    if (self != NULL) {
        hashmap_free(*self);
        *self = NULL;
    }
    return NULL;
}


/**
 * @brief Clear all elements in dict (but allocated capacity unchanged)
 *
 * @param self dict() instane
 */
void
dict_clear(dict_c* self)
{
    // clear all elements, but keeps old capacity unchanged
    hashmap_clear(self, false);
}


const struct __module__dict dict = {
    // Autogenerated by CEX
    // clang-format off
    .create_u64 = dict_create_u64,
    .create = dict_create,
    .create_str = dict_create_str,
    .set = dict_set,
    .geti = dict_geti,
    .gets = dict_gets,
    .get = dict_get,
    .len = dict_len,
    .destroy = dict_destroy,
    .clear = dict_clear,
    // clang-format on
};
