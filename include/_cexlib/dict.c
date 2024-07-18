#include "dict.h"
#include "_hashmap.c"
#include <stdarg.h>
#include <time.h>
#include "list.h"

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
dict__hashfunc__u64_cmp(const void* a, const void* b, void* udata)
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
dict__hashfunc__u64_hash(const void* item, u64 seed0, u64 seed1)
{
    (void)seed0;
    (void)seed1;
    return hm_int_hash_simple(*(u64*)item);
}


/**
 * @brief Compares static char[] buffer keys **must be null terminated**
 *
 * @param a  char[N] string
 * @param b  char[N] string
 * @param udata  (unused)
 * @return compared int value
 */
static int
dict__hashfunc__str_cmp(const void* a, const void* b, void* udata)
{
    (void)udata;
    return strcmp(a, b);
}


/**
 * @brief Compares static char[] buffer keys **must be null terminated**
 *
 * @param item
 * @param seed0
 * @param seed1
 * @return
 */
static u64
dict__hashfunc__str_hash(const void* item, u64 seed0, u64 seed1)
{
    return hashmap_sip(item, strlen((char*)item), seed0, seed1);
}


Exception
dict_create(
    dict_c* self,
    size_t item_size,
    size_t item_align,
    size_t item_key_offsetof,
    size_t capacity,
    dict_hash_func_f hash_func,
    dict_compare_func_f compare_func,
    const Allocator_i* allocator,
    dict_elfree_func_f elfree,
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

    if (item_size <= sizeof(u64)) {
        uassert(item_size > sizeof(u64) && "item_size is too small");
        return Error.argument;
    }

    time_t now = time(NULL);

    self->hashmap = hashmap_new_with_allocator(
        allocator->malloc,
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
    if (self->hashmap == NULL) {
        return Error.memory;
    }

    return Error.ok;
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
    uassert(self != NULL);
    uassert(self->hashmap != NULL);

    const void* set_result = hashmap_set(self->hashmap, item);
    if (set_result == NULL && hashmap_oom(self->hashmap)) {
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
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return (void*)hashmap_get(self->hashmap, &key);
}

/**
 * @brief Get item by generic key pointer (including strings)
 *
 * @param self dict() instance
 * @param key generic pointer key
 */
void*
dict_get(dict_c* self, const void* key)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return (void*)hashmap_get(self->hashmap, key);
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
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return hashmap_count(self->hashmap);
}


/**
 * @brief Free dict() instance
 *
 * @param self  dict() instance
 * @return always NULL
 */
void
dict_destroy(dict_c* self)
{
    if (self != NULL) {
        if (self->hashmap != NULL) {
            hashmap_free(self->hashmap);
            self->hashmap = NULL;
        }
    }
}


/**
 * @brief Clear all elements in dict (but allocated capacity unchanged)
 *
 * @param self dict() instane
 */
void
dict_clear(dict_c* self)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    // clear all elements, but keeps old capacity unchanged
    hashmap_clear(self->hashmap, false);
}

/**
 * @brief Delete item by integer key
 *
 * @param self dict() instance
 * @param key u64 key
 */
void*
dict_deli(dict_c* self, u64 key)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return (void*)hashmap_delete(self->hashmap, &key);
}

/**
 * @brief Delete item by generic key pointer (including strings)
 *
 * @param self dict() instance
 * @param key generic pointer key
 */
void*
dict_del(dict_c* self, const void* key)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    return (void*)hashmap_delete(self->hashmap, key);
}


void*
dict_iter(dict_c* self, cex_iterator_s* iterator)
{
    uassert(self != NULL);
    uassert(self->hashmap != NULL);
    uassert(iterator != NULL);

    struct hashmap* hm = self->hashmap;

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t nbuckets;
        size_t count;
        size_t cursor;
        size_t counter;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) == 8, "ctx alignment mismatch");

    if (unlikely(iterator->val == NULL)) {
        if (hm->count == 0) {
            return NULL;
        }
        *ctx = (struct iter_ctx){
            .count = hm->count,
            .nbuckets = hm->nbuckets,
        };
    } else {
        ctx->counter++;
    }

    if (unlikely(ctx->count != hm->count || ctx->nbuckets != hm->nbuckets)) {
        uassert(ctx->count == hm->count && "hashmap changed during iteration");
        uassert(ctx->nbuckets == hm->nbuckets && "hashmap changed during iteration");
        return NULL;
    }

    if (hashmap_iter(self->hashmap, &ctx->cursor, &iterator->val)) {
        iterator->idx.i = ctx->counter;
        return iterator->val;
    } else {
        return NULL;
    }
}

Exception
dict_tolist(dict_c* self, void* listptr, const Allocator_i* allocator)
{
    if(self == NULL || listptr == NULL || allocator == NULL) {
        return Error.argument;
    }

    if(self->hashmap == NULL){
        uassert(self->hashmap == NULL);
        return Error.integrity;
    }


    struct hashmap* hm = self->hashmap;

    except_traceback(err, list.create((list_c*)listptr, hm->count, hm->elsize, alignof(size_t), allocator)){
        return err;
    }

    size_t hm_cursor = 0;
    void* item = NULL;
    
    while(hashmap_iter(self->hashmap, &hm_cursor, &item)) {
        except_traceback(err, list.append(listptr, item)){
            return err;
        }
    };

    return Error.ok;
}

const struct __module__dict dict = {
    // Autogenerated by CEX
    // clang-format off

    .hashfunc = {  // sub-module .hashfunc >>>
        .u64_cmp = dict__hashfunc__u64_cmp,
        .u64_hash = dict__hashfunc__u64_hash,
        .str_cmp = dict__hashfunc__str_cmp,
        .str_hash = dict__hashfunc__str_hash,
    },  // sub-module .hashfunc <<<
    .create = dict_create,
    .set = dict_set,
    .geti = dict_geti,
    .get = dict_get,
    .len = dict_len,
    .destroy = dict_destroy,
    .clear = dict_clear,
    .deli = dict_deli,
    .del = dict_del,
    .iter = dict_iter,
    .tolist = dict_tolist,
    // clang-format on
};