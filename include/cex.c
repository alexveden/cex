#include "cex.h"

const struct _CEX_Error_struct Error = {
    .ok = EOK,                       // Success
    .memory = "MemoryError",         // memory allocation error
    .io = "IOError",                 // IO error
    .overflow = "OverflowError",     // buffer overflow
    .argument = "ArgumentError",     // function argument error
    .integrity = "IntegrityError",   // data integrity error
    .exists = "ExistsError",         // entity or key already exists
    .not_found = "NotFoundError",    // entity or key already exists
    .skip = "ShouldBeSkipped",       // NOT an error, function result must be skipped
    .sanity_check = "SanityCheckError",     // uerrcheck() failed
    .empty = "EmptyError",           // resource is empty
    .eof = "EOF",                    // end of file reached
    .argsparse = "ProgramArgsError", // program arguments empty or incorrect
};


/*
*                   _hashmap.c
*/
// Copyright 2020 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus

struct hashmap;

struct hashmap *hashmap_new(size_t elsize, size_t cap, uint64_t seed0,
    uint64_t seed1,
    uint64_t (*hash)(const void *item, uint64_t seed0, uint64_t seed1),
    int (*compare)(const void *a, const void *b, void *udata),
    void (*elfree)(void *item),
    void *udata);

struct hashmap *hashmap_new_with_allocator(void *(*malloc)(size_t),
    void *(*realloc)(void *, size_t), void (*free)(void*), size_t elsize,
    size_t cap, uint64_t seed0, uint64_t seed1,
    uint64_t (*hash)(const void *item, uint64_t seed0, uint64_t seed1),
    int (*compare)(const void *a, const void *b, void *udata),
    void (*elfree)(void *item),
    void *udata);

void hashmap_free(struct hashmap *map);
void hashmap_clear(struct hashmap *map, bool update_cap);
size_t hashmap_count(struct hashmap *map);
bool hashmap_oom(struct hashmap *map);
const void *hashmap_get(struct hashmap *map, const void *item);
const void *hashmap_set(struct hashmap *map, const void *item);
const void *hashmap_delete(struct hashmap *map, const void *item);
const void *hashmap_probe(struct hashmap *map, uint64_t position);
bool hashmap_scan(struct hashmap *map, bool (*iter)(const void *item, void *udata), void *udata);
bool hashmap_iter(struct hashmap *map, size_t *i, void **item);

uint64_t hashmap_sip(const void *data, size_t len, uint64_t seed0, uint64_t seed1);
uint64_t hashmap_murmur(const void *data, size_t len, uint64_t seed0, uint64_t seed1);
uint64_t hashmap_xxhash3(const void *data, size_t len, uint64_t seed0, uint64_t seed1);

const void *hashmap_get_with_hash(struct hashmap *map, const void *key, uint64_t hash);
const void *hashmap_delete_with_hash(struct hashmap *map, const void *key, uint64_t hash);
const void *hashmap_set_with_hash(struct hashmap *map, const void *item, uint64_t hash);
void hashmap_set_grow_by_power(struct hashmap *map, size_t power);
void hashmap_set_load_factor(struct hashmap *map, double load_factor);


// DEPRECATED: use `hashmap_new_with_allocator`
void hashmap_set_allocator(void *(*malloc)(size_t), void (*free)(void*));

#if defined(__cplusplus)
}
#endif  // __cplusplus

#endif  // HASHMAP_H

// Copyright 2020 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#define GROW_AT   0.60 /* 60% */
#define SHRINK_AT 0.10 /* 10% */

#ifndef HASHMAP_LOAD_FACTOR
#define HASHMAP_LOAD_FACTOR GROW_AT
#endif

static void *(*__malloc)(size_t) = NULL;
static void *(*__realloc)(void *, size_t) = NULL;
static void (*__free)(void *) = NULL;

// hashmap_set_allocator allows for configuring a custom allocator for
// all hashmap library operations. This function, if needed, should be called
// only once at startup and a prior to calling hashmap_new().
void hashmap_set_allocator(void *(*malloc)(size_t), void (*free)(void*)) {
    __malloc = malloc;
    __free = free;
}

struct bucket {
    uint64_t hash:48;
    uint64_t dib:16;
};

// hashmap is an open addressed hash map using robinhood hashing.
struct hashmap {
    void *(*malloc)(size_t);
    void *(*realloc)(void *, size_t);
    void (*free)(void *);
    size_t elsize;
    size_t cap;
    uint64_t seed0;
    uint64_t seed1;
    uint64_t (*hash)(const void *item, uint64_t seed0, uint64_t seed1);
    int (*compare)(const void *a, const void *b, void *udata);
    void (*elfree)(void *item);
    void *udata;
    size_t bucketsz;
    size_t nbuckets;
    size_t count;
    size_t mask;
    size_t growat;
    size_t shrinkat;
    uint8_t loadfactor;
    uint8_t growpower;
    bool oom;
    void *buckets;
    void *spare;
    void *edata;
};

void hashmap_set_grow_by_power(struct hashmap *map, size_t power) {
    map->growpower = power < 1 ? 1 : power > 16 ? 16 : power;
}

static double clamp_load_factor(double factor, double default_factor) {
    // Check for NaN and clamp between 50% and 90%
    return factor != factor ? default_factor :
           factor < 0.50 ? 0.50 :
           factor > 0.95 ? 0.95 :
           factor;
}

void hashmap_set_load_factor(struct hashmap *map, double factor) {
    factor = clamp_load_factor(factor, map->loadfactor / 100.0);
    map->loadfactor = factor * 100;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
}

static struct bucket *bucket_at0(void *buckets, size_t bucketsz, size_t i) {
    return (struct bucket*)(((char*)buckets)+(bucketsz*i));
}

static struct bucket *bucket_at(struct hashmap *map, size_t index) {
    return bucket_at0(map->buckets, map->bucketsz, index);
}

static void *bucket_item(struct bucket *entry) {
    return ((char*)entry)+sizeof(struct bucket);
}

static uint64_t clip_hash(uint64_t hash) {
    return hash & 0xFFFFFFFFFFFF;
}

static uint64_t get_hash(struct hashmap *map, const void *key) {
    return clip_hash(map->hash(key, map->seed0, map->seed1));
}


// hashmap_new_with_allocator returns a new hash map using a custom allocator.
// See hashmap_new for more information information
struct hashmap *hashmap_new_with_allocator(void *(*_malloc)(size_t),
    void *(*_realloc)(void*, size_t), void (*_free)(void*),
    size_t elsize, size_t cap, uint64_t seed0, uint64_t seed1,
    uint64_t (*hash)(const void *item, uint64_t seed0, uint64_t seed1),
    int (*compare)(const void *a, const void *b, void *udata),
    void (*elfree)(void *item),
    void *udata)
{
    _malloc = _malloc ? _malloc : __malloc ? __malloc : malloc;
    _realloc = _realloc ? _realloc : __realloc ? __realloc : realloc;
    _free = _free ? _free : __free ? __free : free;
    size_t ncap = 16;
    if (cap < ncap) {
        cap = ncap;
    } else {
        while (ncap < cap) {
            ncap *= 2;
        }
        cap = ncap;
    }
    size_t bucketsz = sizeof(struct bucket) + elsize;
    while (bucketsz & (sizeof(uintptr_t)-1)) {
        bucketsz++;
    }
    // hashmap + spare + edata
    size_t size = sizeof(struct hashmap)+bucketsz*2;
    struct hashmap *map = _malloc(size);
    if (!map) {
        return NULL;
    }
    memset(map, 0, sizeof(struct hashmap));
    map->elsize = elsize;
    map->bucketsz = bucketsz;
    map->seed0 = seed0;
    map->seed1 = seed1;
    map->hash = hash;
    map->compare = compare;
    map->elfree = elfree;
    map->udata = udata;
    map->spare = ((char*)map)+sizeof(struct hashmap);
    map->edata = (char*)map->spare+bucketsz;
    map->cap = cap;
    map->nbuckets = cap;
    map->mask = map->nbuckets-1;
    map->buckets = _malloc(map->bucketsz*map->nbuckets);
    if (!map->buckets) {
        _free(map);
        return NULL;
    }
    memset(map->buckets, 0, map->bucketsz*map->nbuckets);
    map->growpower = 1;
    map->loadfactor = clamp_load_factor(HASHMAP_LOAD_FACTOR, GROW_AT) * 100;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;
    map->malloc = _malloc;
    map->realloc = _realloc;
    map->free = _free;
    return map;
}

// hashmap_new returns a new hash map.
// Param `elsize` is the size of each element in the tree. Every element that
// is inserted, deleted, or retrieved will be this size.
// Param `cap` is the default lower capacity of the hashmap. Setting this to
// zero will default to 16.
// Params `seed0` and `seed1` are optional seed values that are passed to the
// following `hash` function. These can be any value you wish but it's often
// best to use randomly generated values.
// Param `hash` is a function that generates a hash value for an item. It's
// important that you provide a good hash function, otherwise it will perform
// poorly or be vulnerable to Denial-of-service attacks. This implementation
// comes with two helper functions `hashmap_sip()` and `hashmap_murmur()`.
// Param `compare` is a function that compares items in the tree. See the
// qsort stdlib function for an example of how this function works.
// The hashmap must be freed with hashmap_free().
// Param `elfree` is a function that frees a specific item. This should be NULL
// unless you're storing some kind of reference data in the hash.
struct hashmap *hashmap_new(size_t elsize, size_t cap, uint64_t seed0,
    uint64_t seed1,
    uint64_t (*hash)(const void *item, uint64_t seed0, uint64_t seed1),
    int (*compare)(const void *a, const void *b, void *udata),
    void (*elfree)(void *item),
    void *udata)
{
    return hashmap_new_with_allocator(NULL, NULL, NULL, elsize, cap, seed0,
        seed1, hash, compare, elfree, udata);
}

static void free_elements(struct hashmap *map) {
    if (map->elfree) {
        for (size_t i = 0; i < map->nbuckets; i++) {
            struct bucket *bucket = bucket_at(map, i);
            if (bucket->dib) map->elfree(bucket_item(bucket));
        }
    }
}

// hashmap_clear quickly clears the map.
// Every item is called with the element-freeing function given in hashmap_new,
// if present, to free any data referenced in the elements of the hashmap.
// When the update_cap is provided, the map's capacity will be updated to match
// the currently number of allocated buckets. This is an optimization to ensure
// that this operation does not perform any allocations.
void hashmap_clear(struct hashmap *map, bool update_cap) {
    map->count = 0;
    free_elements(map);
    if (update_cap) {
        map->cap = map->nbuckets;
    } else if (map->nbuckets != map->cap) {
        void *new_buckets = map->malloc(map->bucketsz*map->cap);
        if (new_buckets) {
            map->free(map->buckets);
            map->buckets = new_buckets;
        }
        map->nbuckets = map->cap;
    }
    memset(map->buckets, 0, map->bucketsz*map->nbuckets);
    map->mask = map->nbuckets-1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0) ;
    map->shrinkat = map->nbuckets * SHRINK_AT;
}

static bool resize0(struct hashmap *map, size_t new_cap) {
    struct hashmap *map2 = hashmap_new_with_allocator(map->malloc, map->realloc,
        map->free, map->elsize, new_cap, map->seed0, map->seed1, map->hash,
        map->compare, map->elfree, map->udata);
    if (!map2) return false;
    for (size_t i = 0; i < map->nbuckets; i++) {
        struct bucket *entry = bucket_at(map, i);
        if (!entry->dib) {
            continue;
        }
        entry->dib = 1;
        size_t j = entry->hash & map2->mask;
        while(1) {
            struct bucket *bucket = bucket_at(map2, j);
            if (bucket->dib == 0) {
                memcpy(bucket, entry, map->bucketsz);
                break;
            }
            if (bucket->dib < entry->dib) {
                memcpy(map2->spare, bucket, map->bucketsz);
                memcpy(bucket, entry, map->bucketsz);
                memcpy(entry, map2->spare, map->bucketsz);
            }
            j = (j + 1) & map2->mask;
            entry->dib += 1;
        }
    }
    map->free(map->buckets);
    map->buckets = map2->buckets;
    map->nbuckets = map2->nbuckets;
    map->mask = map2->mask;
    map->growat = map2->growat;
    map->shrinkat = map2->shrinkat;
    map->free(map2);
    return true;
}

static bool resize(struct hashmap *map, size_t new_cap) {
    return resize0(map, new_cap);
}

// hashmap_set_with_hash works like hashmap_set but you provide your
// own hash. The 'hash' callback provided to the hashmap_new function
// will not be called
const void *hashmap_set_with_hash(struct hashmap *map, const void *item,
    uint64_t hash)
{
    hash = clip_hash(hash);
    map->oom = false;
    if (map->count >= map->growat) {
        if (!resize(map, map->nbuckets*(1<<map->growpower))) {
            map->oom = true;
            return NULL;
        }
    }

    struct bucket *entry = map->edata;
    entry->hash = hash;
    entry->dib = 1;
    void *eitem = bucket_item(entry);
    memcpy(eitem, item, map->elsize);

    void *bitem;
    size_t i = entry->hash & map->mask;
    while(1) {
        struct bucket *bucket = bucket_at(map, i);
        if (bucket->dib == 0) {
            memcpy(bucket, entry, map->bucketsz);
            map->count++;
            return NULL;
        }
        bitem = bucket_item(bucket);
        if (entry->hash == bucket->hash && (!map->compare ||
            map->compare(eitem, bitem, map->udata) == 0))
        {
            memcpy(map->spare, bitem, map->elsize);
            memcpy(bitem, eitem, map->elsize);
            return map->spare;
        }
        if (bucket->dib < entry->dib) {
            memcpy(map->spare, bucket, map->bucketsz);
            memcpy(bucket, entry, map->bucketsz);
            memcpy(entry, map->spare, map->bucketsz);
            eitem = bucket_item(entry);
        }
        i = (i + 1) & map->mask;
        entry->dib += 1;
    }
}

// hashmap_set inserts or replaces an item in the hash map. If an item is
// replaced then it is returned otherwise NULL is returned. This operation
// may allocate memory. If the system is unable to allocate additional
// memory then NULL is returned and hashmap_oom() returns true.
const void *hashmap_set(struct hashmap *map, const void *item) {
    return hashmap_set_with_hash(map, item, get_hash(map, item));
}

// hashmap_get_with_hash works like hashmap_get but you provide your
// own hash. The 'hash' callback provided to the hashmap_new function
// will not be called
const void *hashmap_get_with_hash(struct hashmap *map, const void *key,
    uint64_t hash)
{
    hash = clip_hash(hash);
    size_t i = hash & map->mask;
    while(1) {
        struct bucket *bucket = bucket_at(map, i);
        if (!bucket->dib) return NULL;
        if (bucket->hash == hash) {
            void *bitem = bucket_item(bucket);
            if (!map->compare || map->compare(key, bitem, map->udata) == 0) {
                return bitem;
            }
        }
        i = (i + 1) & map->mask;
    }
}

// hashmap_get returns the item based on the provided key. If the item is not
// found then NULL is returned.
const void *hashmap_get(struct hashmap *map, const void *key) {
    return hashmap_get_with_hash(map, key, get_hash(map, key));
}

// hashmap_probe returns the item in the bucket at position or NULL if an item
// is not set for that bucket. The position is 'moduloed' by the number of
// buckets in the hashmap.
const void *hashmap_probe(struct hashmap *map, uint64_t position) {
    size_t i = position & map->mask;
    struct bucket *bucket = bucket_at(map, i);
    if (!bucket->dib) {
        return NULL;
    }
    return bucket_item(bucket);
}

// hashmap_delete_with_hash works like hashmap_delete but you provide your
// own hash. The 'hash' callback provided to the hashmap_new function
// will not be called
const void *hashmap_delete_with_hash(struct hashmap *map, const void *key,
    uint64_t hash)
{
    hash = clip_hash(hash);
    map->oom = false;
    size_t i = hash & map->mask;
    while(1) {
        struct bucket *bucket = bucket_at(map, i);
        if (!bucket->dib) {
            return NULL;
        }
        void *bitem = bucket_item(bucket);
        if (bucket->hash == hash && (!map->compare ||
            map->compare(key, bitem, map->udata) == 0))
        {
            memcpy(map->spare, bitem, map->elsize);
            bucket->dib = 0;
            while(1) {
                struct bucket *prev = bucket;
                i = (i + 1) & map->mask;
                bucket = bucket_at(map, i);
                if (bucket->dib <= 1) {
                    prev->dib = 0;
                    break;
                }
                memcpy(prev, bucket, map->bucketsz);
                prev->dib--;
            }
            map->count--;
            if (map->nbuckets > map->cap && map->count <= map->shrinkat) {
                // Ignore the return value. It's ok for the resize operation to
                // fail to allocate enough memory because a shrink operation
                // does not change the integrity of the data.
                resize(map, map->nbuckets/2);
            }
            return map->spare;
        }
        i = (i + 1) & map->mask;
    }
}

// hashmap_delete removes an item from the hash map and returns it. If the
// item is not found then NULL is returned.
const void *hashmap_delete(struct hashmap *map, const void *key) {
    return hashmap_delete_with_hash(map, key, get_hash(map, key));
}

// hashmap_count returns the number of items in the hash map.
size_t hashmap_count(struct hashmap *map) {
    return map->count;
}

// hashmap_free frees the hash map
// Every item is called with the element-freeing function given in hashmap_new,
// if present, to free any data referenced in the elements of the hashmap.
void hashmap_free(struct hashmap *map) {
    if (!map) return;
    free_elements(map);
    map->free(map->buckets);
    map->free(map);
}

// hashmap_oom returns true if the last hashmap_set() call failed due to the
// system being out of memory.
bool hashmap_oom(struct hashmap *map) {
    return map->oom;
}

// hashmap_scan iterates over all items in the hash map
// Param `iter` can return false to stop iteration early.
// Returns false if the iteration has been stopped early.
bool hashmap_scan(struct hashmap *map,
    bool (*iter)(const void *item, void *udata), void *udata)
{
    for (size_t i = 0; i < map->nbuckets; i++) {
        struct bucket *bucket = bucket_at(map, i);
        if (bucket->dib && !iter(bucket_item(bucket), udata)) {
            return false;
        }
    }
    return true;
}

// hashmap_iter iterates one key at a time yielding a reference to an
// entry at each iteration. Useful to write simple loops and avoid writing
// dedicated callbacks and udata structures, as in hashmap_scan.
//
// map is a hash map handle. i is a pointer to a size_t cursor that
// should be initialized to 0 at the beginning of the loop. item is a void
// pointer pointer that is populated with the retrieved item. Note that this
// is NOT a copy of the item stored in the hash map and can be directly
// modified.
//
// Note that if hashmap_delete() is called on the hashmap being iterated,
// the buckets are rearranged and the iterator must be reset to 0, otherwise
// unexpected results may be returned after deletion.
//
// This function has not been tested for thread safety.
//
// The function returns true if an item was retrieved; false if the end of the
// iteration has been reached.
bool hashmap_iter(struct hashmap *map, size_t *i, void **item) {
    struct bucket *bucket;
    do {
        if (*i >= map->nbuckets) return false;
        bucket = bucket_at(map, *i);
        (*i)++;
    } while (!bucket->dib);
    *item = bucket_item(bucket);
    return true;
}


//-----------------------------------------------------------------------------
// SipHash reference C implementation
//
// Copyright (c) 2012-2016 Jean-Philippe Aumasson
// <jeanphilippe.aumasson@gmail.com>
// Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
//
// To the extent possible under law, the author(s) have dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along
// with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
//
// default: SipHash-2-4
//-----------------------------------------------------------------------------
static uint64_t SIP64(const uint8_t *in, const size_t inlen, uint64_t seed0,
    uint64_t seed1)
{
#define U8TO64_LE(p) \
    {  (((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) | \
        ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) | \
        ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) | \
        ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56)) }
#define U64TO8_LE(p, v) \
    { U32TO8_LE((p), (uint32_t)((v))); \
      U32TO8_LE((p) + 4, (uint32_t)((v) >> 32)); }
#define U32TO8_LE(p, v) \
    { (p)[0] = (uint8_t)((v)); \
      (p)[1] = (uint8_t)((v) >> 8); \
      (p)[2] = (uint8_t)((v) >> 16); \
      (p)[3] = (uint8_t)((v) >> 24); }
#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))
#define SIPROUND \
    { v0 += v1; v1 = ROTL(v1, 13); \
      v1 ^= v0; v0 = ROTL(v0, 32); \
      v2 += v3; v3 = ROTL(v3, 16); \
      v3 ^= v2; \
      v0 += v3; v3 = ROTL(v3, 21); \
      v3 ^= v0; \
      v2 += v1; v1 = ROTL(v1, 17); \
      v1 ^= v2; v2 = ROTL(v2, 32); }
    uint64_t k0 = U8TO64_LE((uint8_t*)&seed0);
    uint64_t k1 = U8TO64_LE((uint8_t*)&seed1);
    uint64_t v3 = UINT64_C(0x7465646279746573) ^ k1;
    uint64_t v2 = UINT64_C(0x6c7967656e657261) ^ k0;
    uint64_t v1 = UINT64_C(0x646f72616e646f6d) ^ k1;
    uint64_t v0 = UINT64_C(0x736f6d6570736575) ^ k0;
    const uint8_t *end = in + inlen - (inlen % sizeof(uint64_t));
    for (; in != end; in += 8) {
        uint64_t m = U8TO64_LE(in);
        v3 ^= m;
        SIPROUND; SIPROUND;
        v0 ^= m;
    }
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;
    switch (left) {
    case 7: b |= ((uint64_t)in[6]) << 48; /* fall through */
    case 6: b |= ((uint64_t)in[5]) << 40; /* fall through */
    case 5: b |= ((uint64_t)in[4]) << 32; /* fall through */
    case 4: b |= ((uint64_t)in[3]) << 24; /* fall through */
    case 3: b |= ((uint64_t)in[2]) << 16; /* fall through */
    case 2: b |= ((uint64_t)in[1]) << 8; /* fall through */
    case 1: b |= ((uint64_t)in[0]); break;
    case 0: break;
    }
    v3 ^= b;
    SIPROUND; SIPROUND;
    v0 ^= b;
    v2 ^= 0xff;
    SIPROUND; SIPROUND; SIPROUND; SIPROUND;
    b = v0 ^ v1 ^ v2 ^ v3;
    uint64_t out = 0;
    U64TO8_LE((uint8_t*)&out, b);
    return out;
}

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// Murmur3_86_128
//-----------------------------------------------------------------------------
static uint64_t MM86128(const void *key, const int len, uint32_t seed) {
#define	ROTL32(x, r) ((x << r) | (x >> (32 - r)))
#define FMIX32(h) h^=h>>16; h*=0x85ebca6b; h^=h>>13; h*=0xc2b2ae35; h^=h>>16;
    const uint8_t * data = (const uint8_t*)key;
    const int nblocks = len / 16;
    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;
    uint32_t c1 = 0x239b961b;
    uint32_t c2 = 0xab0e9789;
    uint32_t c3 = 0x38b34ae5;
    uint32_t c4 = 0xa1e38b93;
    const uint32_t * blocks = (const uint32_t *)(data + nblocks*16);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i*4+0];
        uint32_t k2 = blocks[i*4+1];
        uint32_t k3 = blocks[i*4+2];
        uint32_t k4 = blocks[i*4+3];
        k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
        h1 = ROTL32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;
        k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
        h2 = ROTL32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;
        k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
        h3 = ROTL32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;
        k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
        h4 = ROTL32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
    }
    const uint8_t * tail = (const uint8_t*)(data + nblocks*16);
    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;
    switch(len & 15) {
    case 15: k4 ^= tail[14] << 16; /* fall through */
    case 14: k4 ^= tail[13] << 8; /* fall through */
    case 13: k4 ^= tail[12] << 0;
             k4 *= c4; k4  = ROTL32(k4,18); k4 *= c1; h4 ^= k4;
             /* fall through */
    case 12: k3 ^= tail[11] << 24; /* fall through */
    case 11: k3 ^= tail[10] << 16; /* fall through */
    case 10: k3 ^= tail[ 9] << 8; /* fall through */
    case  9: k3 ^= tail[ 8] << 0;
             k3 *= c3; k3  = ROTL32(k3,17); k3 *= c4; h3 ^= k3;
             /* fall through */
    case  8: k2 ^= tail[ 7] << 24; /* fall through */
    case  7: k2 ^= tail[ 6] << 16; /* fall through */
    case  6: k2 ^= tail[ 5] << 8; /* fall through */
    case  5: k2 ^= tail[ 4] << 0;
             k2 *= c2; k2  = ROTL32(k2,16); k2 *= c3; h2 ^= k2;
             /* fall through */
    case  4: k1 ^= tail[ 3] << 24; /* fall through */
    case  3: k1 ^= tail[ 2] << 16; /* fall through */
    case  2: k1 ^= tail[ 1] << 8; /* fall through */
    case  1: k1 ^= tail[ 0] << 0;
             k1 *= c1; k1  = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
             /* fall through */
    };
    h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    FMIX32(h1); FMIX32(h2); FMIX32(h3); FMIX32(h4);
    h1 += h2; h1 += h3; h1 += h4;
    h2 += h1; h3 += h1; h4 += h1;
    return (((uint64_t)h2)<<32)|h1;
}

//-----------------------------------------------------------------------------
// xxHash Library
// Copyright (c) 2012-2021 Yann Collet
// All rights reserved.
//
// BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
//
// xxHash3
//-----------------------------------------------------------------------------
#define XXH_PRIME_1 11400714785074694791ULL
#define XXH_PRIME_2 14029467366897019727ULL
#define XXH_PRIME_3 1609587929392839161ULL
#define XXH_PRIME_4 9650029242287828579ULL
#define XXH_PRIME_5 2870177450012600261ULL

static uint64_t XXH_read64(const void* memptr) {
    uint64_t val;
    memcpy(&val, memptr, sizeof(val));
    return val;
}

static uint32_t XXH_read32(const void* memptr) {
    uint32_t val;
    memcpy(&val, memptr, sizeof(val));
    return val;
}

static uint64_t XXH_rotl64(uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

static uint64_t xxh3(const void* data, size_t len, uint64_t seed) {
    const uint8_t* p = (const uint8_t*)data;
    const uint8_t* const end = p + len;
    uint64_t h64;

    if (len >= 32) {
        const uint8_t* const limit = end - 32;
        uint64_t v1 = seed + XXH_PRIME_1 + XXH_PRIME_2;
        uint64_t v2 = seed + XXH_PRIME_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - XXH_PRIME_1;

        do {
            v1 += XXH_read64(p) * XXH_PRIME_2;
            v1 = XXH_rotl64(v1, 31);
            v1 *= XXH_PRIME_1;

            v2 += XXH_read64(p + 8) * XXH_PRIME_2;
            v2 = XXH_rotl64(v2, 31);
            v2 *= XXH_PRIME_1;

            v3 += XXH_read64(p + 16) * XXH_PRIME_2;
            v3 = XXH_rotl64(v3, 31);
            v3 *= XXH_PRIME_1;

            v4 += XXH_read64(p + 24) * XXH_PRIME_2;
            v4 = XXH_rotl64(v4, 31);
            v4 *= XXH_PRIME_1;

            p += 32;
        } while (p <= limit);

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) +
            XXH_rotl64(v4, 18);

        v1 *= XXH_PRIME_2;
        v1 = XXH_rotl64(v1, 31);
        v1 *= XXH_PRIME_1;
        h64 ^= v1;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v2 *= XXH_PRIME_2;
        v2 = XXH_rotl64(v2, 31);
        v2 *= XXH_PRIME_1;
        h64 ^= v2;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v3 *= XXH_PRIME_2;
        v3 = XXH_rotl64(v3, 31);
        v3 *= XXH_PRIME_1;
        h64 ^= v3;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;

        v4 *= XXH_PRIME_2;
        v4 = XXH_rotl64(v4, 31);
        v4 *= XXH_PRIME_1;
        h64 ^= v4;
        h64 = h64 * XXH_PRIME_1 + XXH_PRIME_4;
    }
    else {
        h64 = seed + XXH_PRIME_5;
    }

    h64 += (uint64_t)len;

    while (p + 8 <= end) {
        uint64_t k1 = XXH_read64(p);
        k1 *= XXH_PRIME_2;
        k1 = XXH_rotl64(k1, 31);
        k1 *= XXH_PRIME_1;
        h64 ^= k1;
        h64 = XXH_rotl64(h64, 27) * XXH_PRIME_1 + XXH_PRIME_4;
        p += 8;
    }

    if (p + 4 <= end) {
        h64 ^= (uint64_t)(XXH_read32(p)) * XXH_PRIME_1;
        h64 = XXH_rotl64(h64, 23) * XXH_PRIME_2 + XXH_PRIME_3;
        p += 4;
    }

    while (p < end) {
        h64 ^= (*p) * XXH_PRIME_5;
        h64 = XXH_rotl64(h64, 11) * XXH_PRIME_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= XXH_PRIME_2;
    h64 ^= h64 >> 29;
    h64 *= XXH_PRIME_3;
    h64 ^= h64 >> 32;

    return h64;
}

// hashmap_sip returns a hash value for `data` using SipHash-2-4.
uint64_t hashmap_sip(const void *data, size_t len, uint64_t seed0,
    uint64_t seed1)
{
    return SIP64((uint8_t*)data, len, seed0, seed1);
}

// hashmap_murmur returns a hash value for `data` using Murmur3_86_128.
uint64_t hashmap_murmur(const void *data, size_t len, uint64_t seed0,
    uint64_t seed1)
{
    (void)seed1;
    return MM86128(data, len, seed0);
}

uint64_t hashmap_xxhash3(const void *data, size_t len, uint64_t seed0,
    uint64_t seed1)
{
    (void)seed1;
    return xxh3(data, len ,seed0);
}

//==============================================================================
// TESTS AND BENCHMARKS
// $ cc -DHASHMAP_TEST hashmap.c && ./a.out              # run tests
// $ cc -DHASHMAP_TEST -O3 hashmap.c && BENCH=1 ./a.out  # run benchmarks
//==============================================================================
#ifdef HASHMAP_TEST

static size_t deepcount(struct hashmap *map) {
    size_t count = 0;
    for (size_t i = 0; i < map->nbuckets; i++) {
        if (bucket_at(map, i)->dib) {
            count++;
        }
    }
    return count;
}

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wcompound-token-split-by-macro"
#pragma GCC diagnostic ignored "-Wgnu-statement-expression-from-macro-expansion"
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>

static bool rand_alloc_fail = false;
static int rand_alloc_fail_odds = 3; // 1 in 3 chance malloc will fail.
static uintptr_t total_allocs = 0;
static uintptr_t total_mem = 0;

static void *xmalloc(size_t size) {
    if (rand_alloc_fail && rand()%rand_alloc_fail_odds == 0) {
        return NULL;
    }
    void *mem = malloc(sizeof(uintptr_t)+size);
    assert(mem);
    *(uintptr_t*)mem = size;
    total_allocs++;
    total_mem += size;
    return (char*)mem+sizeof(uintptr_t);
}

static void xfree(void *ptr) {
    if (ptr) {
        total_mem -= *(uintptr_t*)((char*)ptr-sizeof(uintptr_t));
        free((char*)ptr-sizeof(uintptr_t));
        total_allocs--;
    }
}

static void shuffle(void *array, size_t numels, size_t elsize) {
    char tmp[elsize];
    char *arr = array;
    for (size_t i = 0; i < numels - 1; i++) {
        int j = i + rand() / (RAND_MAX / (numels - i) + 1);
        memcpy(tmp, arr + j * elsize, elsize);
        memcpy(arr + j * elsize, arr + i * elsize, elsize);
        memcpy(arr + i * elsize, tmp, elsize);
    }
}

static bool iter_ints(const void *item, void *udata) {
    int *vals = *(int**)udata;
    vals[*(int*)item] = 1;
    return true;
}

static int compare_ints_udata(const void *a, const void *b, void *udata) {
    return *(int*)a - *(int*)b;
}

static int compare_strs(const void *a, const void *b, void *udata) {
    return strcmp(*(char**)a, *(char**)b);
}

static uint64_t hash_int(const void *item, uint64_t seed0, uint64_t seed1) {
    return hashmap_xxhash3(item, sizeof(int), seed0, seed1);
    // return hashmap_sip(item, sizeof(int), seed0, seed1);
    // return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

static uint64_t hash_str(const void *item, uint64_t seed0, uint64_t seed1) {
    return hashmap_xxhash3(*(char**)item, strlen(*(char**)item), seed0, seed1);
    // return hashmap_sip(*(char**)item, strlen(*(char**)item), seed0, seed1);
    // return hashmap_murmur(*(char**)item, strlen(*(char**)item), seed0, seed1);
}

static void free_str(void *item) {
    xfree(*(char**)item);
}

static void all(void) {
    int seed = getenv("SEED")?atoi(getenv("SEED")):time(NULL);
    int N = getenv("N")?atoi(getenv("N")):2000;
    printf("seed=%d, count=%d, item_size=%zu\n", seed, N, sizeof(int));
    srand(seed);

    rand_alloc_fail = true;

    // test sip and murmur hashes
    assert(hashmap_sip("hello", 5, 1, 2) == 2957200328589801622);
    assert(hashmap_murmur("hello", 5, 1, 2) == 1682575153221130884);
    assert(hashmap_xxhash3("hello", 5, 1, 2) == 2584346877953614258);

    int *vals;
    while (!(vals = xmalloc(N * sizeof(int)))) {}
    for (int i = 0; i < N; i++) {
        vals[i] = i;
    }

    struct hashmap *map;

    while (!(map = hashmap_new(sizeof(int), 0, seed, seed,
                               hash_int, compare_ints_udata, NULL, NULL))) {}
    shuffle(vals, N, sizeof(int));
    for (int i = 0; i < N; i++) {
        // // printf("== %d ==\n", vals[i]);
        assert(map->count == (size_t)i);
        assert(map->count == hashmap_count(map));
        assert(map->count == deepcount(map));
        const int *v;
        assert(!hashmap_get(map, &vals[i]));
        assert(!hashmap_delete(map, &vals[i]));
        while (true) {
            assert(!hashmap_set(map, &vals[i]));
            if (!hashmap_oom(map)) {
                break;
            }
        }

        for (int j = 0; j < i; j++) {
            v = hashmap_get(map, &vals[j]);
            assert(v && *v == vals[j]);
        }
        while (true) {
            v = hashmap_set(map, &vals[i]);
            if (!v) {
                assert(hashmap_oom(map));
                continue;
            } else {
                assert(!hashmap_oom(map));
                assert(v && *v == vals[i]);
                break;
            }
        }
        v = hashmap_get(map, &vals[i]);
        assert(v && *v == vals[i]);
        v = hashmap_delete(map, &vals[i]);
        assert(v && *v == vals[i]);
        assert(!hashmap_get(map, &vals[i]));
        assert(!hashmap_delete(map, &vals[i]));
        assert(!hashmap_set(map, &vals[i]));
        assert(map->count == (size_t)(i+1));
        assert(map->count == hashmap_count(map));
        assert(map->count == deepcount(map));
    }

    int *vals2;
    while (!(vals2 = xmalloc(N * sizeof(int)))) {}
    memset(vals2, 0, N * sizeof(int));
    assert(hashmap_scan(map, iter_ints, &vals2));

    // Test hashmap_iter. This does the same as hashmap_scan above.
    size_t iter = 0;
    void *iter_val;
    while (hashmap_iter (map, &iter, &iter_val)) {
        assert (iter_ints(iter_val, &vals2));
    }
    for (int i = 0; i < N; i++) {
        assert(vals2[i] == 1);
    }
    xfree(vals2);

    shuffle(vals, N, sizeof(int));
    for (int i = 0; i < N; i++) {
        const int *v;
        v = hashmap_delete(map, &vals[i]);
        assert(v && *v == vals[i]);
        assert(!hashmap_get(map, &vals[i]));
        assert(map->count == (size_t)(N-i-1));
        assert(map->count == hashmap_count(map));
        assert(map->count == deepcount(map));
        for (int j = N-1; j > i; j--) {
            v = hashmap_get(map, &vals[j]);
            assert(v && *v == vals[j]);
        }
    }

    for (int i = 0; i < N; i++) {
        while (true) {
            assert(!hashmap_set(map, &vals[i]));
            if (!hashmap_oom(map)) {
                break;
            }
        }
    }

    assert(map->count != 0);
    size_t prev_cap = map->cap;
    hashmap_clear(map, true);
    assert(prev_cap < map->cap);
    assert(map->count == 0);


    for (int i = 0; i < N; i++) {
        while (true) {
            assert(!hashmap_set(map, &vals[i]));
            if (!hashmap_oom(map)) {
                break;
            }
        }
    }

    prev_cap = map->cap;
    hashmap_clear(map, false);
    assert(prev_cap == map->cap);

    hashmap_free(map);

    xfree(vals);


    while (!(map = hashmap_new(sizeof(char*), 0, seed, seed,
                               hash_str, compare_strs, free_str, NULL)));

    for (int i = 0; i < N; i++) {
        char *str;
        while (!(str = xmalloc(16)));
        snprintf(str, 16, "s%i", i);
        while(!hashmap_set(map, &str));
    }

    hashmap_clear(map, false);
    assert(hashmap_count(map) == 0);

    for (int i = 0; i < N; i++) {
        char *str;
        while (!(str = xmalloc(16)));
        snprintf(str, 16, "s%i", i);
        while(!hashmap_set(map, &str));
    }

    hashmap_free(map);

    if (total_allocs != 0) {
        fprintf(stderr, "total_allocs: expected 0, got %lu\n", total_allocs);
        exit(1);
    }
}

#define bench(name, N, code) {{ \
    if (strlen(name) > 0) { \
        printf("%-14s ", name); \
    } \
    size_t tmem = total_mem; \
    size_t tallocs = total_allocs; \
    uint64_t bytes = 0; \
    clock_t begin = clock(); \
    for (int i = 0; i < N; i++) { \
        (code); \
    } \
    clock_t end = clock(); \
    double elapsed_secs = (double)(end - begin) / CLOCKS_PER_SEC; \
    double bytes_sec = (double)bytes/elapsed_secs; \
    printf("%d ops in %.3f secs, %.0f ns/op, %.0f op/sec", \
        N, elapsed_secs, \
        elapsed_secs/(double)N*1e9, \
        (double)N/elapsed_secs \
    ); \
    if (bytes > 0) { \
        printf(", %.1f GB/sec", bytes_sec/1024/1024/1024); \
    } \
    if (total_mem > tmem) { \
        size_t used_mem = total_mem-tmem; \
        printf(", %.2f bytes/op", (double)used_mem/N); \
    } \
    if (total_allocs > tallocs) { \
        size_t used_allocs = total_allocs-tallocs; \
        printf(", %.2f allocs/op", (double)used_allocs/N); \
    } \
    printf("\n"); \
}}

static void benchmarks(void) {
    int seed = getenv("SEED")?atoi(getenv("SEED")):time(NULL);
    int N = getenv("N")?atoi(getenv("N")):5000000;
    printf("seed=%d, count=%d, item_size=%zu\n", seed, N, sizeof(int));
    srand(seed);


    int *vals = xmalloc(N * sizeof(int));
    for (int i = 0; i < N; i++) {
        vals[i] = i;
    }

    shuffle(vals, N, sizeof(int));

    struct hashmap *map;
    shuffle(vals, N, sizeof(int));

    map = hashmap_new(sizeof(int), 0, seed, seed, hash_int, compare_ints_udata,
                      NULL, NULL);
    bench("set", N, {
        const int *v = hashmap_set(map, &vals[i]);
        assert(!v);
    })
    shuffle(vals, N, sizeof(int));
    bench("get", N, {
        const int *v = hashmap_get(map, &vals[i]);
        assert(v && *v == vals[i]);
    })
    shuffle(vals, N, sizeof(int));
    bench("delete", N, {
        const int *v = hashmap_delete(map, &vals[i]);
        assert(v && *v == vals[i]);
    })
    hashmap_free(map);

    map = hashmap_new(sizeof(int), N, seed, seed, hash_int, compare_ints_udata,
                      NULL, NULL);
    bench("set (cap)", N, {
        const int *v = hashmap_set(map, &vals[i]);
        assert(!v);
    })
    shuffle(vals, N, sizeof(int));
    bench("get (cap)", N, {
        const int *v = hashmap_get(map, &vals[i]);
        assert(v && *v == vals[i]);
    })
    shuffle(vals, N, sizeof(int));
    bench("delete (cap)" , N, {
        const int *v = hashmap_delete(map, &vals[i]);
        assert(v && *v == vals[i]);
    })

    hashmap_free(map);


    xfree(vals);

    if (total_allocs != 0) {
        fprintf(stderr, "total_allocs: expected 0, got %lu\n", total_allocs);
        exit(1);
    }
}

int main(void) {
    hashmap_set_allocator(xmalloc, xfree);

    if (getenv("BENCH")) {
        printf("Running hashmap.c benchmarks...\n");
        benchmarks();
    } else {
        printf("Running hashmap.c tests...\n");
        all();
        printf("PASSED\n");
    }
}


#endif



/*
*                   _stb_sprintf.c
*/
// stb_sprintf - v1.10 - public domain snprintf() implementation
// originally by Jeff Roberts / RAD Game Tools, 2015/10/20
// http://github.com/nothings/stb
//
// allowed types:  sc uidBboXx p AaGgEef n
// lengths      :  hh h ll j z t I64 I32 I
//
// Contributors:
//    Fabian "ryg" Giesen (reformatting)
//    github:aganm (attribute format)
//
// Contributors (bugfixes):
//    github:d26435
//    github:trex78
//    github:account-login
//    Jari Komppa (SI suffixes)
//    Rohit Nirmal
//    Marcin Wojdyr
//    Leonard Ritter
//    Stefano Zanotti
//    Adam Allison
//    Arvid Gerstmann
//    Markus Kolb
//
// LICENSE:
//
//   See end of file for license information.

#ifndef STB_SPRINTF_H_INCLUDE
#define STB_SPRINTF_H_INCLUDE

// NOTE: CEX this file is slighly adjusted stb_sprintf.h,
// with alignment fixed + support of str_c as a parameter
#define STB_SPRINTF_NOUNALIGNED 1

/*
Single file sprintf replacement.

Originally written by Jeff Roberts at RAD Game Tools - 2015/10/20.
Hereby placed in public domain.

This is a full sprintf replacement that supports everything that
the C runtime sprintfs support, including float/double, 64-bit integers,
hex floats, field parameters (%*.*d stuff), length reads backs, etc.

Why would you need this if sprintf already exists?  Well, first off,
it's *much* faster (see below). It's also much smaller than the CRT
versions code-space-wise. We've also added some simple improvements
that are super handy (commas in thousands, callbacks at buffer full,
for example). Finally, the format strings for MSVC and GCC differ
for 64-bit integers (among other small things), so this lets you use
the same format strings in cross platform code.

It uses the standard single file trick of being both the header file
and the source itself. If you just include it normally, you just get
the header file function definitions. To get the code, you include
it from a C or C++ file and define STB_SPRINTF_IMPLEMENTATION first.

It only uses va_args macros from the C runtime to do it's work. It
does cast doubles to S64s and shifts and divides U64s, which does
drag in CRT code on most platforms.

It compiles to roughly 8K with float support, and 4K without.
As a comparison, when using MSVC static libs, calling sprintf drags
in 16K.

API:
====
int stbsp_sprintf( char * buf, char const * fmt, ... )
int stbsp_snprintf( char * buf, int count, char const * fmt, ... )
  Convert an arg list into a buffer.  stbsp_snprintf always returns
  a zero-terminated string (unlike regular snprintf).

int stbsp_vsprintf( char * buf, char const * fmt, va_list va )
int stbsp_vsnprintf( char * buf, int count, char const * fmt, va_list va )
  Convert a va_list arg list into a buffer.  stbsp_vsnprintf always returns
  a zero-terminated string (unlike regular snprintf).

int stbsp_vsprintfcb( STBSP_SPRINTFCB * callback, void * user, char * buf, char const * fmt, va_list
va ) typedef char * STBSP_SPRINTFCB( char const * buf, void * user, int len ); Convert into a
buffer, calling back every STB_SPRINTF_MIN chars. Your callback can then copy the chars out, print
them or whatever. This function is actually the workhorse for everything else. The buffer you pass
in must hold at least STB_SPRINTF_MIN characters.
    // you return the next buffer to use or 0 to stop converting

void stbsp_set_separators( char comma, char period )
  Set the comma and period characters to use.

FLOATS/DOUBLES:
===============
This code uses a internal float->ascii conversion method that uses
doubles with error correction (double-doubles, for ~105 bits of
precision).  This conversion is round-trip perfect - that is, an atof
of the values output here will give you the bit-exact double back.

One difference is that our insignificant digits will be different than
with MSVC or GCC (but they don't match each other either).  We also
don't attempt to find the minimum length matching float (pre-MSVC15
doesn't either).

If you don't need float or doubles at all, define STB_SPRINTF_NOFLOAT
and you'll save 4K of code space.

64-BIT INTS:
============
This library also supports 64-bit integers and you can use MSVC style or
GCC style indicators (%I64d or %lld).  It supports the C99 specifiers
for size_t and ptr_diff_t (%jd %zd) as well.

EXTRAS:
=======
Like some GCCs, for integers and floats, you can use a ' (single quote)
specifier and commas will be inserted on the thousands: "%'d" on 12345
would print 12,345.

For integers and floats, you can use a "$" specifier and the number
will be converted to float and then divided to get kilo, mega, giga or
tera and then printed, so "%$d" 1000 is "1.0 k", "%$.2d" 2536000 is
"2.53 M", etc. For byte values, use two $:s, like "%$$d" to turn
2536000 to "2.42 Mi". If you prefer JEDEC suffixes to SI ones, use three
$:s: "%$$$d" -> "2.42 M". To remove the space between the number and the
suffix, add "_" specifier: "%_$d" -> "2.53M".

In addition to octal and hexadecimal conversions, you can print
integers in binary: "%b" for 256 would print 100.

PERFORMANCE vs MSVC 2008 32-/64-bit (GCC is even slower than MSVC):
===================================================================
"%d" across all 32-bit ints (4.8x/4.0x faster than 32-/64-bit MSVC)
"%24d" across all 32-bit ints (4.5x/4.2x faster)
"%x" across all 32-bit ints (4.5x/3.8x faster)
"%08x" across all 32-bit ints (4.3x/3.8x faster)
"%f" across e-10 to e+10 floats (7.3x/6.0x faster)
"%e" across e-10 to e+10 floats (8.1x/6.0x faster)
"%g" across e-10 to e+10 floats (10.0x/7.1x faster)
"%f" for values near e-300 (7.9x/6.5x faster)
"%f" for values near e+300 (10.0x/9.1x faster)
"%e" for values near e-300 (10.1x/7.0x faster)
"%e" for values near e+300 (9.2x/6.0x faster)
"%.320f" for values near e-300 (12.6x/11.2x faster)
"%a" for random values (8.6x/4.3x faster)
"%I64d" for 64-bits with 32-bit values (4.8x/3.4x faster)
"%I64d" for 64-bits > 32-bit values (4.9x/5.5x faster)
"%s%s%s" for 64 char strings (7.1x/7.3x faster)
"...512 char string..." ( 35.0x/32.5x faster!)
*/

#if defined(__clang__)
#if defined(__has_feature) && defined(__has_attribute)
#if __has_feature(address_sanitizer)
#if __has_attribute(__no_sanitize__)
#define STBSP__ASAN __attribute__((__no_sanitize__("address")))
#elif __has_attribute(__no_sanitize_address__)
#define STBSP__ASAN __attribute__((__no_sanitize_address__))
#elif __has_attribute(__no_address_safety_analysis__)
#define STBSP__ASAN __attribute__((__no_address_safety_analysis__))
#endif
#endif
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
#if defined(__SANITIZE_ADDRESS__) && __SANITIZE_ADDRESS__
#define STBSP__ASAN __attribute__((__no_sanitize_address__))
#endif
#endif

#ifndef STBSP__ASAN
#define STBSP__ASAN
#endif

#ifdef STB_SPRINTF_STATIC
#define STBSP__PUBLICDEC static
#define STBSP__PUBLICDEF static STBSP__ASAN
#else
#ifdef __cplusplus
#define STBSP__PUBLICDEC extern "C"
#define STBSP__PUBLICDEF extern "C" STBSP__ASAN
#else
#define STBSP__PUBLICDEC extern
#define STBSP__PUBLICDEF STBSP__ASAN
#endif
#endif

#if defined(__has_attribute)
#if __has_attribute(format)
#define STBSP__ATTRIBUTE_FORMAT(fmt, va) __attribute__((format(printf, fmt, va)))
#endif
#endif

#ifndef STBSP__ATTRIBUTE_FORMAT
#define STBSP__ATTRIBUTE_FORMAT(fmt, va)
#endif

#ifdef _MSC_VER
#define STBSP__NOTUSED(v) (void)(v)
#else
#define STBSP__NOTUSED(v) (void)sizeof(v)
#endif

#include <stdarg.h> // for va_arg(), va_list()
#include <stddef.h> // size_t, ptrdiff_t

#ifndef STB_SPRINTF_MIN
#define STB_SPRINTF_MIN 512 // how many characters per callback
#endif
typedef char* STBSP_SPRINTFCB(const char* buf, void* user, int len);

#ifndef STB_SPRINTF_DECORATE
#define STB_SPRINTF_DECORATE(name)                                                                 \
    stbsp_##name // define this before including if you want to change the names
#endif

STBSP__PUBLICDEF int
STB_SPRINTF_DECORATE(vfprintf)(FILE* stream, const char* format, va_list va);
STBSP__PUBLICDEF int
STB_SPRINTF_DECORATE(fprintf)(FILE* stream, const char* format, ...);

STBSP__PUBLICDEC int STB_SPRINTF_DECORATE(vsprintf)(char* buf, char const* fmt, va_list va);
STBSP__PUBLICDEC int
    STB_SPRINTF_DECORATE(vsnprintf)(char* buf, int count, char const* fmt, va_list va);
STBSP__PUBLICDEC int STB_SPRINTF_DECORATE(sprintf)(char* buf, char const* fmt, ...)
    STBSP__ATTRIBUTE_FORMAT(2, 3);
STBSP__PUBLICDEC int STB_SPRINTF_DECORATE(snprintf)(char* buf, int count, char const* fmt, ...)
    STBSP__ATTRIBUTE_FORMAT(3, 4);

STBSP__PUBLICDEC int STB_SPRINTF_DECORATE(vsprintfcb)(
    STBSP_SPRINTFCB* callback,
    void* user,
    char* buf,
    char const* fmt,
    va_list va
);
STBSP__PUBLICDEC void STB_SPRINTF_DECORATE(set_separators)(char comma, char period);

#endif // STB_SPRINTF_H_INCLUDE


#define stbsp__uint32 unsigned int
#define stbsp__int32 signed int

#ifdef _MSC_VER
#define stbsp__uint64 unsigned __int64
#define stbsp__int64 signed __int64
#else
#define stbsp__uint64 unsigned long long
#define stbsp__int64 signed long long
#endif
#define stbsp__uint16 unsigned short

#ifndef stbsp__uintptr
#if defined(__ppc64__) || defined(__powerpc64__) || defined(__aarch64__) || defined(_M_X64) ||     \
    defined(__x86_64__) || defined(__x86_64) || defined(__s390x__)
#define stbsp__uintptr stbsp__uint64
#else
#define stbsp__uintptr stbsp__uint32
#endif
#endif

#ifndef STB_SPRINTF_MSVC_MODE // used for MSVC2013 and earlier (MSVC2015 matches GCC)
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define STB_SPRINTF_MSVC_MODE
#endif
#endif

#ifdef STB_SPRINTF_NOUNALIGNED // define this before inclusion to force stbsp_sprintf to always use
                               // aligned accesses
#define STBSP__UNALIGNED(code)
#else
#define STBSP__UNALIGNED(code) code
#endif

#ifndef STB_SPRINTF_NOFLOAT
// internal float utility functions
static stbsp__int32 stbsp__real_to_str(
    char const** start,
    stbsp__uint32* len,
    char* out,
    stbsp__int32* decimal_pos,
    double value,
    stbsp__uint32 frac_digits
);
static stbsp__int32 stbsp__real_to_parts(stbsp__int64* bits, stbsp__int32* expo, double value);
#define STBSP__SPECIAL 0x7000
#endif

static char stbsp__period = '.';
static char stbsp__comma = ',';
static struct
{
    short temp; // force next field to be 2-byte aligned
    char pair[201];
} stbsp__digitpair = { 0,
                       "00010203040506070809101112131415161718192021222324"
                       "25262728293031323334353637383940414243444546474849"
                       "50515253545556575859606162636465666768697071727374"
                       "75767778798081828384858687888990919293949596979899" };

STBSP__PUBLICDEF void
STB_SPRINTF_DECORATE(set_separators)(char pcomma, char pperiod)
{
    stbsp__period = pperiod;
    stbsp__comma = pcomma;
}

#define STBSP__LEFTJUST 1
#define STBSP__LEADINGPLUS 2
#define STBSP__LEADINGSPACE 4
#define STBSP__LEADING_0X 8
#define STBSP__LEADINGZERO 16
#define STBSP__INTMAX 32
#define STBSP__TRIPLET_COMMA 64
#define STBSP__NEGATIVE 128
#define STBSP__METRIC_SUFFIX 256
#define STBSP__HALFWIDTH 512
#define STBSP__METRIC_NOSPACE 1024
#define STBSP__METRIC_1024 2048
#define STBSP__METRIC_JEDEC 4096

static void
stbsp__lead_sign(stbsp__uint32 fl, char* sign)
{
    sign[0] = 0;
    if (fl & STBSP__NEGATIVE) {
        sign[0] = 1;
        sign[1] = '-';
    } else if (fl & STBSP__LEADINGSPACE) {
        sign[0] = 1;
        sign[1] = ' ';
    } else if (fl & STBSP__LEADINGPLUS) {
        sign[0] = 1;
        sign[1] = '+';
    }
}

static STBSP__ASAN stbsp__uint32
stbsp__strlen_limited(char const* s, stbsp__uint32 limit)
{
    char const* sn = s;

    // get up to 4-byte alignment
    for (;;) {
        if (((stbsp__uintptr)sn & 3) == 0) {
            break;
        }

        if (!limit || *sn == 0) {
            return (stbsp__uint32)(sn - s);
        }

        ++sn;
        --limit;
    }

    // scan over 4 bytes at a time to find terminating 0
    // this will intentionally scan up to 3 bytes past the end of buffers,
    // but becase it works 4B aligned, it will never cross page boundaries
    // (hence the STBSP__ASAN markup; the over-read here is intentional
    // and harmless)
    while (limit >= 4) {
        stbsp__uint32 v = *(stbsp__uint32*)sn;
        // bit hack to find if there's a 0 byte in there
        if ((v - 0x01010101) & (~v) & 0x80808080UL) {
            break;
        }

        sn += 4;
        limit -= 4;
    }

    // handle the last few characters to find actual size
    while (limit && *sn) {
        ++sn;
        --limit;
    }

    return (stbsp__uint32)(sn - s);
}

STBSP__PUBLICDEF int
STB_SPRINTF_DECORATE(vsprintfcb)(
    STBSP_SPRINTFCB* callback,
    void* user,
    char* buf,
    char const* fmt,
    va_list va
)
{
    static char hex[] = "0123456789abcdefxp";
    static char hexu[] = "0123456789ABCDEFXP";
    char* bf;
    char const* f;
    int tlen = 0;

    bf = buf;
    f = fmt;
    for (;;) {
        stbsp__int32 fw, pr, tz;
        stbsp__uint32 fl;

// macros for the callback buffer stuff
#define stbsp__chk_cb_bufL(bytes)                                                                  \
    {                                                                                              \
        int len = (int)(bf - buf);                                                                 \
        if ((len + (bytes)) >= STB_SPRINTF_MIN) {                                                  \
            tlen += len;                                                                           \
            if (0 == (bf = buf = callback(buf, user, len)))                                        \
                goto done;                                                                         \
        }                                                                                          \
    }
#define stbsp__chk_cb_buf(bytes)                                                                   \
    {                                                                                              \
        if (callback) {                                                                            \
            stbsp__chk_cb_bufL(bytes);                                                             \
        }                                                                                          \
    }
#define stbsp__flush_cb()                                                                          \
    {                                                                                              \
        stbsp__chk_cb_bufL(STB_SPRINTF_MIN - 1);                                                   \
    } // flush if there is even one byte in the buffer
#define stbsp__cb_buf_clamp(cl, v)                                                                 \
    cl = v;                                                                                        \
    if (callback) {                                                                                \
        int lg = STB_SPRINTF_MIN - (int)(bf - buf);                                                \
        if (cl > lg)                                                                               \
            cl = lg;                                                                               \
    }

        // fast copy everything up to the next % (or end of string)
        for (;;) {
            while (((stbsp__uintptr)f) & 3) {
            schk1:
                if (f[0] == '%') {
                    goto scandd;
                }
            schk2:
                if (f[0] == 0) {
                    goto endfmt;
                }
                stbsp__chk_cb_buf(1);
                *bf++ = f[0];
                ++f;
            }
            for (;;) {
                // Check if the next 4 bytes contain %(0x25) or end of string.
                // Using the 'hasless' trick:
                // https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
                stbsp__uint32 v, c;
                v = *(stbsp__uint32*)f;
                c = (~v) & 0x80808080;
                if (((v ^ 0x25252525) - 0x01010101) & c) {
                    goto schk1;
                }
                if ((v - 0x01010101) & c) {
                    goto schk2;
                }
                if (callback) {
                    if ((STB_SPRINTF_MIN - (int)(bf - buf)) < 4) {
                        goto schk1;
                    }
                }
#ifdef STB_SPRINTF_NOUNALIGNED
                if (((stbsp__uintptr)bf) & 3) {
                    bf[0] = f[0];
                    bf[1] = f[1];
                    bf[2] = f[2];
                    bf[3] = f[3];
                } else
#endif
                {
                    *(stbsp__uint32*)bf = v;
                }
                bf += 4;
                f += 4;
            }
        }
    scandd:

        ++f;

        // ok, we have a percent, read the modifiers first
        fw = 0;
        pr = -1;
        fl = 0;
        tz = 0;

        // flags
        for (;;) {
            switch (f[0]) {
                // if we have left justify
                case '-':
                    fl |= STBSP__LEFTJUST;
                    ++f;
                    continue;
                // if we have leading plus
                case '+':
                    fl |= STBSP__LEADINGPLUS;
                    ++f;
                    continue;
                // if we have leading space
                case ' ':
                    fl |= STBSP__LEADINGSPACE;
                    ++f;
                    continue;
                // if we have leading 0x
                case '#':
                    fl |= STBSP__LEADING_0X;
                    ++f;
                    continue;
                // if we have thousand commas
                case '\'':
                    fl |= STBSP__TRIPLET_COMMA;
                    ++f;
                    continue;
                // if we have kilo marker (none->kilo->kibi->jedec)
                case '$':
                    if (fl & STBSP__METRIC_SUFFIX) {
                        if (fl & STBSP__METRIC_1024) {
                            fl |= STBSP__METRIC_JEDEC;
                        } else {
                            fl |= STBSP__METRIC_1024;
                        }
                    } else {
                        fl |= STBSP__METRIC_SUFFIX;
                    }
                    ++f;
                    continue;
                // if we don't want space between metric suffix and number
                case '_':
                    fl |= STBSP__METRIC_NOSPACE;
                    ++f;
                    continue;
                // if we have leading zero
                case '0':
                    fl |= STBSP__LEADINGZERO;
                    ++f;
                    goto flags_done;
                default:
                    goto flags_done;
            }
        }
    flags_done:

        // get the field width
        if (f[0] == '*') {
            fw = va_arg(va, stbsp__uint32);
            ++f;
        } else {
            while ((f[0] >= '0') && (f[0] <= '9')) {
                fw = fw * 10 + f[0] - '0';
                f++;
            }
        }
        // get the precision
        if (f[0] == '.') {
            ++f;
            if (f[0] == '*') {
                pr = va_arg(va, stbsp__uint32);
                ++f;
            } else {
                pr = 0;
                while ((f[0] >= '0') && (f[0] <= '9')) {
                    pr = pr * 10 + f[0] - '0';
                    f++;
                }
            }
        }

        // handle integer size overrides
        switch (f[0]) {
            // are we halfwidth?
            case 'h':
                fl |= STBSP__HALFWIDTH;
                ++f;
                if (f[0] == 'h') {
                    ++f; // QUARTERWIDTH
                }
                break;
            // are we 64-bit (unix style)
            case 'l':
                fl |= ((sizeof(long) == 8) ? STBSP__INTMAX : 0);
                ++f;
                if (f[0] == 'l') {
                    fl |= STBSP__INTMAX;
                    ++f;
                }
                break;
            // are we 64-bit on intmax? (c99)
            case 'j':
                fl |= (sizeof(size_t) == 8) ? STBSP__INTMAX : 0;
                ++f;
                break;
            // are we 64-bit on size_t or ptrdiff_t? (c99)
            case 'z':
                fl |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
                ++f;
                break;
            case 't':
                fl |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
                ++f;
                break;
            // are we 64-bit (msft style)
            case 'I':
                if ((f[1] == '6') && (f[2] == '4')) {
                    fl |= STBSP__INTMAX;
                    f += 3;
                } else if ((f[1] == '3') && (f[2] == '2')) {
                    f += 3;
                } else {
                    fl |= ((sizeof(void*) == 8) ? STBSP__INTMAX : 0);
                    ++f;
                }
                break;
            default:
                break;
        }

        // handle each replacement
        switch (f[0]) {
#define STBSP__NUMSZ 512 // big enough for e308 (with commas) or e-307
            char num[STBSP__NUMSZ];
            char lead[8];
            char tail[8];
            char* s;
            char const* h;
            stbsp__uint32 l, n, cs;
            stbsp__uint64 n64;
#ifndef STB_SPRINTF_NOFLOAT
            double fv;
#endif
            stbsp__int32 dp;
            char const* sn;

            case 's':
                // get the string
                s = va_arg(va, char*);
                if ((void*)s <= (void*)(1024 * 1024)) {
                    if (s == 0) {
                        s = (char*)"(null)";
                    }
#if defined(__x86_64__) || defined(__x86_64)
                    else {
                        // NOTE: cex is str_c passed as %s, s will be length
                        // try to double check sensible value of pointer
                        s = (char*)"(str_c->%S)";
                    }
#endif
                }
                // get the length, limited to desired precision
                // always limit to ~0u chars since our counts are 32b
                l = stbsp__strlen_limited(s, (pr >= 0) ? (unsigned)pr : ~0u);
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                // copy the string in
                goto scopy;
            case 'S': {
                // NOTE: CEX extra (support of str_c)
                str_c sv = va_arg(va, str_c);
                s = sv.buf;
                if (s == 0) {
                    s = (char*)"(null)";
                    l = stbsp__strlen_limited(s, (pr >= 0) ? (unsigned)pr : ~0u);
                } else {
                    l = sv.len > 0xffffffff ? 0xffffffff : sv.len;
                }
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                // copy the string in
                goto scopy;
            }
            case 'c': // char
                // get the character
                s = num + STBSP__NUMSZ - 1;
                *s = (char)va_arg(va, int);
                l = 1;
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                goto scopy;

            case 'n': // weird write-bytes specifier
            {
                int* d = va_arg(va, int*);
                *d = tlen + (int)(bf - buf);
            } break;

#ifdef STB_SPRINTF_NOFLOAT
            case 'A':               // float
            case 'a':               // hex float
            case 'G':               // float
            case 'g':               // float
            case 'E':               // float
            case 'e':               // float
            case 'f':               // float
                va_arg(va, double); // eat it
                s = (char*)"No float";
                l = 8;
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                cs = 0;
                STBSP__NOTUSED(dp);
                goto scopy;
#else
            case 'A': // hex float
            case 'a': // hex float
                h = (f[0] == 'A') ? hexu : hex;
                fv = va_arg(va, double);
                if (pr == -1) {
                    pr = 6; // default is 6
                }
                // read the double into a string
                if (stbsp__real_to_parts((stbsp__int64*)&n64, &dp, fv)) {
                    fl |= STBSP__NEGATIVE;
                }

                s = num + 64;

                stbsp__lead_sign(fl, lead);

                if (dp == -1023) {
                    dp = (n64) ? -1022 : 0;
                } else {
                    n64 |= (((stbsp__uint64)1) << 52);
                }
                n64 <<= (64 - 56);
                if (pr < 15) {
                    n64 += ((((stbsp__uint64)8) << 56) >> (pr * 4));
                }
                // add leading chars

#ifdef STB_SPRINTF_MSVC_MODE
                *s++ = '0';
                *s++ = 'x';
#else
                lead[1 + lead[0]] = '0';
                lead[2 + lead[0]] = 'x';
                lead[0] += 2;
#endif
                *s++ = h[(n64 >> 60) & 15];
                n64 <<= 4;
                if (pr) {
                    *s++ = stbsp__period;
                }
                sn = s;

                // print the bits
                n = pr;
                if (n > 13) {
                    n = 13;
                }
                if (pr > (stbsp__int32)n) {
                    tz = pr - n;
                }
                pr = 0;
                while (n--) {
                    *s++ = h[(n64 >> 60) & 15];
                    n64 <<= 4;
                }

                // print the expo
                tail[1] = h[17];
                if (dp < 0) {
                    tail[2] = '-';
                    dp = -dp;
                } else {
                    tail[2] = '+';
                }
                n = (dp >= 1000) ? 6 : ((dp >= 100) ? 5 : ((dp >= 10) ? 4 : 3));
                tail[0] = (char)n;
                for (;;) {
                    tail[n] = '0' + dp % 10;
                    if (n <= 3) {
                        break;
                    }
                    --n;
                    dp /= 10;
                }

                dp = (int)(s - sn);
                l = (int)(s - (num + 64));
                s = num + 64;
                cs = 1 + (3 << 24);
                goto scopy;

            case 'G': // float
            case 'g': // float
                h = (f[0] == 'G') ? hexu : hex;
                fv = va_arg(va, double);
                if (pr == -1) {
                    pr = 6;
                } else if (pr == 0) {
                    pr = 1; // default is 6
                }
                // read the double into a string
                if (stbsp__real_to_str(&sn, &l, num, &dp, fv, (pr - 1) | 0x80000000)) {
                    fl |= STBSP__NEGATIVE;
                }

                // clamp the precision and delete extra zeros after clamp
                n = pr;
                if (l > (stbsp__uint32)pr) {
                    l = pr;
                }
                while ((l > 1) && (pr) && (sn[l - 1] == '0')) {
                    --pr;
                    --l;
                }

                // should we use %e
                if ((dp <= -4) || (dp > (stbsp__int32)n)) {
                    if (pr > (stbsp__int32)l) {
                        pr = l - 1;
                    } else if (pr) {
                        --pr; // when using %e, there is one digit before the decimal
                    }
                    goto doexpfromg;
                }
                // this is the insane action to get the pr to match %g semantics for %f
                if (dp > 0) {
                    pr = (dp < (stbsp__int32)l) ? l - dp : 0;
                } else {
                    pr = -dp + ((pr > (stbsp__int32)l) ? (stbsp__int32)l : pr);
                }
                goto dofloatfromg;

            case 'E': // float
            case 'e': // float
                h = (f[0] == 'E') ? hexu : hex;
                fv = va_arg(va, double);
                if (pr == -1) {
                    pr = 6; // default is 6
                }
                // read the double into a string
                if (stbsp__real_to_str(&sn, &l, num, &dp, fv, pr | 0x80000000)) {
                    fl |= STBSP__NEGATIVE;
                }
            doexpfromg:
                tail[0] = 0;
                stbsp__lead_sign(fl, lead);
                if (dp == STBSP__SPECIAL) {
                    s = (char*)sn;
                    cs = 0;
                    pr = 0;
                    goto scopy;
                }
                s = num + 64;
                // handle leading chars
                *s++ = sn[0];

                if (pr) {
                    *s++ = stbsp__period;
                }

                // handle after decimal
                if ((l - 1) > (stbsp__uint32)pr) {
                    l = pr + 1;
                }
                for (n = 1; n < l; n++) {
                    *s++ = sn[n];
                }
                // trailing zeros
                tz = pr - (l - 1);
                pr = 0;
                // dump expo
                tail[1] = h[0xe];
                dp -= 1;
                if (dp < 0) {
                    tail[2] = '-';
                    dp = -dp;
                } else {
                    tail[2] = '+';
                }
#ifdef STB_SPRINTF_MSVC_MODE
                n = 5;
#else
                n = (dp >= 100) ? 5 : 4;
#endif
                tail[0] = (char)n;
                for (;;) {
                    tail[n] = '0' + dp % 10;
                    if (n <= 3) {
                        break;
                    }
                    --n;
                    dp /= 10;
                }
                cs = 1 + (3 << 24); // how many tens
                goto flt_lead;

            case 'f': // float
                fv = va_arg(va, double);
            doafloat:
                // do kilos
                if (fl & STBSP__METRIC_SUFFIX) {
                    double divisor;
                    divisor = 1000.0f;
                    if (fl & STBSP__METRIC_1024) {
                        divisor = 1024.0;
                    }
                    while (fl < 0x4000000) {
                        if ((fv < divisor) && (fv > -divisor)) {
                            break;
                        }
                        fv /= divisor;
                        fl += 0x1000000;
                    }
                }
                if (pr == -1) {
                    pr = 6; // default is 6
                }
                // read the double into a string
                if (stbsp__real_to_str(&sn, &l, num, &dp, fv, pr)) {
                    fl |= STBSP__NEGATIVE;
                }
            dofloatfromg:
                tail[0] = 0;
                stbsp__lead_sign(fl, lead);
                if (dp == STBSP__SPECIAL) {
                    s = (char*)sn;
                    cs = 0;
                    pr = 0;
                    goto scopy;
                }
                s = num + 64;

                // handle the three decimal varieties
                if (dp <= 0) {
                    stbsp__int32 i;
                    // handle 0.000*000xxxx
                    *s++ = '0';
                    if (pr) {
                        *s++ = stbsp__period;
                    }
                    n = -dp;
                    if ((stbsp__int32)n > pr) {
                        n = pr;
                    }
                    i = n;
                    while (i) {
                        if ((((stbsp__uintptr)s) & 3) == 0) {
                            break;
                        }
                        *s++ = '0';
                        --i;
                    }
                    while (i >= 4) {
                        *(stbsp__uint32*)s = 0x30303030;
                        s += 4;
                        i -= 4;
                    }
                    while (i) {
                        *s++ = '0';
                        --i;
                    }
                    if ((stbsp__int32)(l + n) > pr) {
                        l = pr - n;
                    }
                    i = l;
                    while (i) {
                        *s++ = *sn++;
                        --i;
                    }
                    tz = pr - (n + l);
                    cs = 1 + (3 << 24); // how many tens did we write (for commas below)
                } else {
                    cs = (fl & STBSP__TRIPLET_COMMA) ? ((600 - (stbsp__uint32)dp) % 3) : 0;
                    if ((stbsp__uint32)dp >= l) {
                        // handle xxxx000*000.0
                        n = 0;
                        for (;;) {
                            if ((fl & STBSP__TRIPLET_COMMA) && (++cs == 4)) {
                                cs = 0;
                                *s++ = stbsp__comma;
                            } else {
                                *s++ = sn[n];
                                ++n;
                                if (n >= l) {
                                    break;
                                }
                            }
                        }
                        if (n < (stbsp__uint32)dp) {
                            n = dp - n;
                            if ((fl & STBSP__TRIPLET_COMMA) == 0) {
                                while (n) {
                                    if ((((stbsp__uintptr)s) & 3) == 0) {
                                        break;
                                    }
                                    *s++ = '0';
                                    --n;
                                }
                                while (n >= 4) {
                                    *(stbsp__uint32*)s = 0x30303030;
                                    s += 4;
                                    n -= 4;
                                }
                            }
                            while (n) {
                                if ((fl & STBSP__TRIPLET_COMMA) && (++cs == 4)) {
                                    cs = 0;
                                    *s++ = stbsp__comma;
                                } else {
                                    *s++ = '0';
                                    --n;
                                }
                            }
                        }
                        cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
                        if (pr) {
                            *s++ = stbsp__period;
                            tz = pr;
                        }
                    } else {
                        // handle xxxxx.xxxx000*000
                        n = 0;
                        for (;;) {
                            if ((fl & STBSP__TRIPLET_COMMA) && (++cs == 4)) {
                                cs = 0;
                                *s++ = stbsp__comma;
                            } else {
                                *s++ = sn[n];
                                ++n;
                                if (n >= (stbsp__uint32)dp) {
                                    break;
                                }
                            }
                        }
                        cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
                        if (pr) {
                            *s++ = stbsp__period;
                        }
                        if ((l - dp) > (stbsp__uint32)pr) {
                            l = pr + dp;
                        }
                        while (n < l) {
                            *s++ = sn[n];
                            ++n;
                        }
                        tz = pr - (l - dp);
                    }
                }
                pr = 0;

                // handle k,m,g,t
                if (fl & STBSP__METRIC_SUFFIX) {
                    char idx;
                    idx = 1;
                    if (fl & STBSP__METRIC_NOSPACE) {
                        idx = 0;
                    }
                    tail[0] = idx;
                    tail[1] = ' ';
                    {
                        if (fl >> 24) { // SI kilo is 'k', JEDEC and SI kibits are 'K'.
                            if (fl & STBSP__METRIC_1024) {
                                tail[idx + 1] = "_KMGT"[fl >> 24];
                            } else {
                                tail[idx + 1] = "_kMGT"[fl >> 24];
                            }
                            idx++;
                            // If printing kibits and not in jedec, add the 'i'.
                            if (fl & STBSP__METRIC_1024 && !(fl & STBSP__METRIC_JEDEC)) {
                                tail[idx + 1] = 'i';
                                idx++;
                            }
                            tail[0] = idx;
                        }
                    }
                };

            flt_lead:
                // get the length that we copied
                l = (stbsp__uint32)(s - (num + 64));
                s = num + 64;
                goto scopy;
#endif

            case 'B': // upper binary
            case 'b': // lower binary
                h = (f[0] == 'B') ? hexu : hex;
                lead[0] = 0;
                if (fl & STBSP__LEADING_0X) {
                    lead[0] = 2;
                    lead[1] = '0';
                    lead[2] = h[0xb];
                }
                l = (8 << 4) | (1 << 8);
                goto radixnum;

            case 'o': // octal
                h = hexu;
                lead[0] = 0;
                if (fl & STBSP__LEADING_0X) {
                    lead[0] = 1;
                    lead[1] = '0';
                }
                l = (3 << 4) | (3 << 8);
                goto radixnum;

            case 'p': // pointer
                fl |= (sizeof(void*) == 8) ? STBSP__INTMAX : 0;
                pr = sizeof(void*) * 2;
                fl &= ~STBSP__LEADINGZERO; // 'p' only prints the pointer with zeros
                                           // fall through - to X

            case 'X': // upper hex
            case 'x': // lower hex
                h = (f[0] == 'X') ? hexu : hex;
                l = (4 << 4) | (4 << 8);
                lead[0] = 0;
                if (fl & STBSP__LEADING_0X) {
                    lead[0] = 2;
                    lead[1] = '0';
                    lead[2] = h[16];
                }
            radixnum:
                // get the number
                if (fl & STBSP__INTMAX) {
                    n64 = va_arg(va, stbsp__uint64);
                } else {
                    n64 = va_arg(va, stbsp__uint32);
                }

                s = num + STBSP__NUMSZ;
                dp = 0;
                // clear tail, and clear leading if value is zero
                tail[0] = 0;
                if (n64 == 0) {
                    lead[0] = 0;
                    if (pr == 0) {
                        l = 0;
                        cs = 0;
                        goto scopy;
                    }
                }
                // convert to string
                for (;;) {
                    *--s = h[n64 & ((1 << (l >> 8)) - 1)];
                    n64 >>= (l >> 8);
                    if (!((n64) || ((stbsp__int32)((num + STBSP__NUMSZ) - s) < pr))) {
                        break;
                    }
                    if (fl & STBSP__TRIPLET_COMMA) {
                        ++l;
                        if ((l & 15) == ((l >> 4) & 15)) {
                            l &= ~15;
                            *--s = stbsp__comma;
                        }
                    }
                };
                // get the tens and the comma pos
                cs = (stbsp__uint32)((num + STBSP__NUMSZ) - s) + ((((l >> 4) & 15)) << 24);
                // get the length that we copied
                l = (stbsp__uint32)((num + STBSP__NUMSZ) - s);
                // copy it
                goto scopy;

            case 'u': // unsigned
            case 'i':
            case 'd': // integer
                // get the integer and abs it
                if (fl & STBSP__INTMAX) {
                    stbsp__int64 i64 = va_arg(va, stbsp__int64);
                    n64 = (stbsp__uint64)i64;
                    if ((f[0] != 'u') && (i64 < 0)) {
                        n64 = (stbsp__uint64)-i64;
                        fl |= STBSP__NEGATIVE;
                    }
                } else {
                    stbsp__int32 i = va_arg(va, stbsp__int32);
                    n64 = (stbsp__uint32)i;
                    if ((f[0] != 'u') && (i < 0)) {
                        n64 = (stbsp__uint32)-i;
                        fl |= STBSP__NEGATIVE;
                    }
                }

#ifndef STB_SPRINTF_NOFLOAT
                if (fl & STBSP__METRIC_SUFFIX) {
                    if (n64 < 1024) {
                        pr = 0;
                    } else if (pr == -1) {
                        pr = 1;
                    }
                    fv = (double)(stbsp__int64)n64;
                    goto doafloat;
                }
#endif

                // convert to string
                s = num + STBSP__NUMSZ;
                l = 0;

                for (;;) {
                    // do in 32-bit chunks (avoid lots of 64-bit divides even with constant
                    // denominators)
                    char* o = s - 8;
                    if (n64 >= 100000000) {
                        n = (stbsp__uint32)(n64 % 100000000);
                        n64 /= 100000000;
                    } else {
                        n = (stbsp__uint32)n64;
                        n64 = 0;
                    }
                    if ((fl & STBSP__TRIPLET_COMMA) == 0) {
                        do {
                            s -= 2;
                            *(stbsp__uint16*)
                                s = *(stbsp__uint16*)&stbsp__digitpair.pair[(n % 100) * 2];
                            n /= 100;
                        } while (n);
                    }
                    while (n) {
                        if ((fl & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                            l = 0;
                            *--s = stbsp__comma;
                            --o;
                        } else {
                            *--s = (char)(n % 10) + '0';
                            n /= 10;
                        }
                    }
                    if (n64 == 0) {
                        if ((s[0] == '0') && (s != (num + STBSP__NUMSZ))) {
                            ++s;
                        }
                        break;
                    }
                    while (s != o) {
                        if ((fl & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                            l = 0;
                            *--s = stbsp__comma;
                            --o;
                        } else {
                            *--s = '0';
                        }
                    }
                }

                tail[0] = 0;
                stbsp__lead_sign(fl, lead);

                // get the length that we copied
                l = (stbsp__uint32)((num + STBSP__NUMSZ) - s);
                if (l == 0) {
                    *--s = '0';
                    l = 1;
                }
                cs = l + (3 << 24);
                if (pr < 0) {
                    pr = 0;
                }

            scopy:
                // get fw=leading/trailing space, pr=leading zeros
                if (pr < (stbsp__int32)l) {
                    pr = l;
                }
                n = pr + lead[0] + tail[0] + tz;
                if (fw < (stbsp__int32)n) {
                    fw = n;
                }
                fw -= n;
                pr -= l;

                // handle right justify and leading zeros
                if ((fl & STBSP__LEFTJUST) == 0) {
                    if (fl & STBSP__LEADINGZERO) // if leading zeros, everything is in pr
                    {
                        pr = (fw > pr) ? fw : pr;
                        fw = 0;
                    } else {
                        fl &= ~STBSP__TRIPLET_COMMA; // if no leading zeros, then no commas
                    }
                }

                // copy the spaces and/or zeros
                if (fw + pr) {
                    stbsp__int32 i;
                    stbsp__uint32 c;

                    // copy leading spaces (or when doing %8.4d stuff)
                    if ((fl & STBSP__LEFTJUST) == 0) {
                        while (fw > 0) {
                            stbsp__cb_buf_clamp(i, fw);
                            fw -= i;
                            while (i) {
                                if ((((stbsp__uintptr)bf) & 3) == 0) {
                                    break;
                                }
                                *bf++ = ' ';
                                --i;
                            }
                            while (i >= 4) {
                                *(stbsp__uint32*)bf = 0x20202020;
                                bf += 4;
                                i -= 4;
                            }
                            while (i) {
                                *bf++ = ' ';
                                --i;
                            }
                            stbsp__chk_cb_buf(1);
                        }
                    }

                    // copy leader
                    sn = lead + 1;
                    while (lead[0]) {
                        stbsp__cb_buf_clamp(i, lead[0]);
                        lead[0] -= (char)i;
                        while (i) {
                            *bf++ = *sn++;
                            --i;
                        }
                        stbsp__chk_cb_buf(1);
                    }

                    // copy leading zeros
                    c = cs >> 24;
                    cs &= 0xffffff;
                    cs = (fl & STBSP__TRIPLET_COMMA) ? ((stbsp__uint32)(c - ((pr + cs) % (c + 1))))
                                                     : 0;
                    while (pr > 0) {
                        stbsp__cb_buf_clamp(i, pr);
                        pr -= i;
                        if ((fl & STBSP__TRIPLET_COMMA) == 0) {
                            while (i) {
                                if ((((stbsp__uintptr)bf) & 3) == 0) {
                                    break;
                                }
                                *bf++ = '0';
                                --i;
                            }
                            while (i >= 4) {
                                *(stbsp__uint32*)bf = 0x30303030;
                                bf += 4;
                                i -= 4;
                            }
                        }
                        while (i) {
                            if ((fl & STBSP__TRIPLET_COMMA) && (cs++ == c)) {
                                cs = 0;
                                *bf++ = stbsp__comma;
                            } else {
                                *bf++ = '0';
                            }
                            --i;
                        }
                        stbsp__chk_cb_buf(1);
                    }
                }

                // copy leader if there is still one
                sn = lead + 1;
                while (lead[0]) {
                    stbsp__int32 i;
                    stbsp__cb_buf_clamp(i, lead[0]);
                    lead[0] -= (char)i;
                    while (i) {
                        *bf++ = *sn++;
                        --i;
                    }
                    stbsp__chk_cb_buf(1);
                }

                // copy the string
                n = l;
                while (n) {
                    stbsp__int32 i;
                    stbsp__cb_buf_clamp(i, n);
                    n -= i;
                    STBSP__UNALIGNED(while (i >= 4) {
                        *(stbsp__uint32 volatile*)bf = *(stbsp__uint32 volatile*)s;
                        bf += 4;
                        s += 4;
                        i -= 4;
                    })
                    while (i) {
                        *bf++ = *s++;
                        --i;
                    }
                    stbsp__chk_cb_buf(1);
                }

                // copy trailing zeros
                while (tz) {
                    stbsp__int32 i;
                    stbsp__cb_buf_clamp(i, tz);
                    tz -= i;
                    while (i) {
                        if ((((stbsp__uintptr)bf) & 3) == 0) {
                            break;
                        }
                        *bf++ = '0';
                        --i;
                    }
                    while (i >= 4) {
                        *(stbsp__uint32*)bf = 0x30303030;
                        bf += 4;
                        i -= 4;
                    }
                    while (i) {
                        *bf++ = '0';
                        --i;
                    }
                    stbsp__chk_cb_buf(1);
                }

                // copy tail if there is one
                sn = tail + 1;
                while (tail[0]) {
                    stbsp__int32 i;
                    stbsp__cb_buf_clamp(i, tail[0]);
                    tail[0] -= (char)i;
                    while (i) {
                        *bf++ = *sn++;
                        --i;
                    }
                    stbsp__chk_cb_buf(1);
                }

                // handle the left justify
                if (fl & STBSP__LEFTJUST) {
                    if (fw > 0) {
                        while (fw) {
                            stbsp__int32 i;
                            stbsp__cb_buf_clamp(i, fw);
                            fw -= i;
                            while (i) {
                                if ((((stbsp__uintptr)bf) & 3) == 0) {
                                    break;
                                }
                                *bf++ = ' ';
                                --i;
                            }
                            while (i >= 4) {
                                *(stbsp__uint32*)bf = 0x20202020;
                                bf += 4;
                                i -= 4;
                            }
                            while (i--) {
                                *bf++ = ' ';
                            }
                            stbsp__chk_cb_buf(1);
                        }
                    }
                }
                break;

            default: // unknown, just copy code
                s = num + STBSP__NUMSZ - 1;
                *s = f[0];
                l = 1;
                fw = fl = 0;
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                goto scopy;
        }
        ++f;
    }
endfmt:

    if (!callback) {
        *bf = 0;
    } else {
        stbsp__flush_cb();
    }

done:
    return tlen + (int)(bf - buf);
}

// cleanup
#undef STBSP__LEFTJUST
#undef STBSP__LEADINGPLUS
#undef STBSP__LEADINGSPACE
#undef STBSP__LEADING_0X
#undef STBSP__LEADINGZERO
#undef STBSP__INTMAX
#undef STBSP__TRIPLET_COMMA
#undef STBSP__NEGATIVE
#undef STBSP__METRIC_SUFFIX
#undef STBSP__NUMSZ
#undef stbsp__chk_cb_bufL
#undef stbsp__chk_cb_buf
#undef stbsp__flush_cb
#undef stbsp__cb_buf_clamp

// ============================================================================
//   wrapper functions

STBSP__PUBLICDEF int
STB_SPRINTF_DECORATE(sprintf)(char* buf, char const* fmt, ...)
{
    int result;
    va_list va;
    va_start(va, fmt);
    result = STB_SPRINTF_DECORATE(vsprintfcb)(0, 0, buf, fmt, va);
    va_end(va);
    return result;
}

typedef struct stbsp__context
{
    char* buf;
    FILE* file;
    int count;
    int length;
    int has_error;
    char tmp[STB_SPRINTF_MIN];
} stbsp__context;

static char*
stbsp__clamp_callback(const char* buf, void* user, int len)
{
    stbsp__context* c = (stbsp__context*)user;
    c->length += len;

    if (len > c->count) {
        len = c->count;
    }

    if (len) {
        if (buf != c->buf) {
            const char *s, *se;
            char* d;
            d = c->buf;
            s = buf;
            se = buf + len;
            do {
                *d++ = *s++;
            } while (s < se);
        }
        c->buf += len;
        c->count -= len;
    }

    if (c->count <= 0) {
        return c->tmp;
    }
    return (c->count >= STB_SPRINTF_MIN) ? c->buf : c->tmp; // go direct into buffer if you can
}

static char*
stbsp__count_clamp_callback(const char* buf, void* user, int len)
{
    stbsp__context* c = (stbsp__context*)user;
    (void)sizeof(buf);

    c->length += len;
    return c->tmp; // go direct into buffer if you can
}

STBSP__PUBLICDEF int
STB_SPRINTF_DECORATE(vsnprintf)(char* buf, int count, char const* fmt, va_list va)
{
    stbsp__context c;

    if ((count == 0) && !buf) {
        c.length = 0;

        STB_SPRINTF_DECORATE(vsprintfcb)(stbsp__count_clamp_callback, &c, c.tmp, fmt, va);
    } else {
        int l;

        c.buf = buf;
        c.count = count;
        c.length = 0;

        STB_SPRINTF_DECORATE(vsprintfcb)
        (stbsp__clamp_callback, &c, stbsp__clamp_callback(0, &c, 0), fmt, va);

        // zero-terminate
        l = (int)(c.buf - buf);
        if (l >= count) { // should never be greater, only equal (or less) than count
            l = count - 1;
            c.length = l;
        }
        buf[l] = 0;
    }

    return c.length;
}

STBSP__PUBLICDEF int
STB_SPRINTF_DECORATE(snprintf)(char* buf, int count, char const* fmt, ...)
{
    int result;
    va_list va;
    va_start(va, fmt);

    result = STB_SPRINTF_DECORATE(vsnprintf)(buf, count, fmt, va);
    va_end(va);

    return result;
}

STBSP__PUBLICDEF int
STB_SPRINTF_DECORATE(vsprintf)(char* buf, char const* fmt, va_list va)
{
    return STB_SPRINTF_DECORATE(vsprintfcb)(0, 0, buf, fmt, va);
}

static char*
stbsp__fprintf_callback(const char* buf, void* user, int len)
{
    stbsp__context* c = (stbsp__context*)user;
    c->length += len;
    if (len) {
        if(fwrite(buf, sizeof(char), len, c->file) != (size_t)len){
            c->has_error = 1;
        }
    }
    return c->tmp;
}

STBSP__PUBLICDEF int
STB_SPRINTF_DECORATE(vfprintf)(FILE* stream, const char* format, va_list va)
{
    stbsp__context c = {.file = stream, .length = 0};

    STB_SPRINTF_DECORATE(vsprintfcb)
    (stbsp__fprintf_callback, &c, stbsp__fprintf_callback(0, &c, 0), format, va);

    return c.has_error == 0 ? c.length : -1;
}

STBSP__PUBLICDEF int
STB_SPRINTF_DECORATE(fprintf)(FILE* stream, const char* format, ...)
{
    int result;
    va_list va;
    va_start(va, format);
    result = STB_SPRINTF_DECORATE(vfprintf)(stream, format, va);
    va_end(va);
    return result;
}

// =======================================================================
//   low level float utility functions

#ifndef STB_SPRINTF_NOFLOAT

// copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
#define STBSP__COPYFP(dest, src)                                                                   \
    {                                                                                              \
        int cn;                                                                                    \
        for (cn = 0; cn < 8; cn++)                                                                 \
            ((char*)&dest)[cn] = ((char*)&src)[cn];                                                \
    }

// get float info
static stbsp__int32
stbsp__real_to_parts(stbsp__int64* bits, stbsp__int32* expo, double value)
{
    double d;
    stbsp__int64 b = 0;

    // load value and round at the frac_digits
    d = value;

    STBSP__COPYFP(b, d);

    *bits = b & ((((stbsp__uint64)1) << 52) - 1);
    *expo = (stbsp__int32)(((b >> 52) & 2047) - 1023);

    return (stbsp__int32)((stbsp__uint64)b >> 63);
}

static double const stbsp__bot[23] = { 1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005,
                                       1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
                                       1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017,
                                       1e+018, 1e+019, 1e+020, 1e+021, 1e+022 };
static double const stbsp__negbot[22] = { 1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006,
                                          1e-007, 1e-008, 1e-009, 1e-010, 1e-011, 1e-012,
                                          1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018,
                                          1e-019, 1e-020, 1e-021, 1e-022 };
static double const stbsp__negboterr[22] = {
    -5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020,
    -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
    4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026,
    -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
    -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032,
    2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
    2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,
    -4.8596774326570872e-039
};
static double const stbsp__top[13] = { 1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161,
                                       1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299 };
static double const stbsp__negtop[13] = { 1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161,
                                          1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299 };
static double const stbsp__toperr[13] = { 8388608,
                                          6.8601809640529717e+028,
                                          -7.253143638152921e+052,
                                          -4.3377296974619174e+075,
                                          -1.5559416129466825e+098,
                                          -3.2841562489204913e+121,
                                          -3.7745893248228135e+144,
                                          -1.7356668416969134e+167,
                                          -3.8893577551088374e+190,
                                          -9.9566444326005119e+213,
                                          6.3641293062232429e+236,
                                          -5.2069140800249813e+259,
                                          -5.2504760255204387e+282 };
static double const stbsp__negtoperr[13] = { 3.9565301985100693e-040,  -2.299904345391321e-063,
                                             3.6506201437945798e-086,  1.1875228833981544e-109,
                                             -5.0644902316928607e-132, -6.7156837247865426e-155,
                                             -2.812077463003139e-178,  -5.7778912386589953e-201,
                                             7.4997100559334532e-224,  -4.6439668915134491e-247,
                                             -6.3691100762962136e-270, -9.436808465446358e-293,
                                             8.0970921678014997e-317 };

#if defined(_MSC_VER) && (_MSC_VER <= 1200)
static stbsp__uint64 const stbsp__powten[20] = { 1,
                                                 10,
                                                 100,
                                                 1000,
                                                 10000,
                                                 100000,
                                                 1000000,
                                                 10000000,
                                                 100000000,
                                                 1000000000,
                                                 10000000000,
                                                 100000000000,
                                                 1000000000000,
                                                 10000000000000,
                                                 100000000000000,
                                                 1000000000000000,
                                                 10000000000000000,
                                                 100000000000000000,
                                                 1000000000000000000,
                                                 10000000000000000000U };
#define stbsp__tento19th ((stbsp__uint64)1000000000000000000)
#else
static stbsp__uint64 const stbsp__powten[20] = { 1,
                                                 10,
                                                 100,
                                                 1000,
                                                 10000,
                                                 100000,
                                                 1000000,
                                                 10000000,
                                                 100000000,
                                                 1000000000,
                                                 10000000000ULL,
                                                 100000000000ULL,
                                                 1000000000000ULL,
                                                 10000000000000ULL,
                                                 100000000000000ULL,
                                                 1000000000000000ULL,
                                                 10000000000000000ULL,
                                                 100000000000000000ULL,
                                                 1000000000000000000ULL,
                                                 10000000000000000000ULL };
#define stbsp__tento19th (1000000000000000000ULL)
#endif

#define stbsp__ddmulthi(oh, ol, xh, yh)                                                            \
    {                                                                                              \
        double ahi = 0, alo, bhi = 0, blo;                                                         \
        stbsp__int64 bt;                                                                           \
        oh = xh * yh;                                                                              \
        STBSP__COPYFP(bt, xh);                                                                     \
        bt &= ((~(stbsp__uint64)0) << 27);                                                         \
        STBSP__COPYFP(ahi, bt);                                                                    \
        alo = xh - ahi;                                                                            \
        STBSP__COPYFP(bt, yh);                                                                     \
        bt &= ((~(stbsp__uint64)0) << 27);                                                         \
        STBSP__COPYFP(bhi, bt);                                                                    \
        blo = yh - bhi;                                                                            \
        ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo;                               \
    }

#define stbsp__ddtoS64(ob, xh, xl)                                                                 \
    {                                                                                              \
        double ahi = 0, alo, vh, t;                                                                \
        ob = (stbsp__int64)xh;                                                                     \
        vh = (double)ob;                                                                           \
        ahi = (xh - vh);                                                                           \
        t = (ahi - xh);                                                                            \
        alo = (xh - (ahi - t)) - (vh + t);                                                         \
        ob += (stbsp__int64)(ahi + alo + xl);                                                      \
    }

#define stbsp__ddrenorm(oh, ol)                                                                    \
    {                                                                                              \
        double s;                                                                                  \
        s = oh + ol;                                                                               \
        ol = ol - (s - oh);                                                                        \
        oh = s;                                                                                    \
    }

#define stbsp__ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#define stbsp__ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);

static void
stbsp__raise_to_power10(double* ohi, double* olo, double d, stbsp__int32 power) // power can be -323
                                                                                // to +350
{
    double ph, pl;
    if ((power >= 0) && (power <= 22)) {
        stbsp__ddmulthi(ph, pl, d, stbsp__bot[power]);
    } else {
        stbsp__int32 e, et, eb;
        double p2h, p2l;

        e = power;
        if (power < 0) {
            e = -e;
        }
        et = (e * 0x2c9) >> 14; /* %23 */
        if (et > 13) {
            et = 13;
        }
        eb = e - (et * 23);

        ph = d;
        pl = 0.0;
        if (power < 0) {
            if (eb) {
                --eb;
                stbsp__ddmulthi(ph, pl, d, stbsp__negbot[eb]);
                stbsp__ddmultlos(ph, pl, d, stbsp__negboterr[eb]);
            }
            if (et) {
                stbsp__ddrenorm(ph, pl);
                --et;
                stbsp__ddmulthi(p2h, p2l, ph, stbsp__negtop[et]);
                stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__negtop[et], stbsp__negtoperr[et]);
                ph = p2h;
                pl = p2l;
            }
        } else {
            if (eb) {
                e = eb;
                if (eb > 22) {
                    eb = 22;
                }
                e -= eb;
                stbsp__ddmulthi(ph, pl, d, stbsp__bot[eb]);
                if (e) {
                    stbsp__ddrenorm(ph, pl);
                    stbsp__ddmulthi(p2h, p2l, ph, stbsp__bot[e]);
                    stbsp__ddmultlos(p2h, p2l, stbsp__bot[e], pl);
                    ph = p2h;
                    pl = p2l;
                }
            }
            if (et) {
                stbsp__ddrenorm(ph, pl);
                --et;
                stbsp__ddmulthi(p2h, p2l, ph, stbsp__top[et]);
                stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__top[et], stbsp__toperr[et]);
                ph = p2h;
                pl = p2l;
            }
        }
    }
    stbsp__ddrenorm(ph, pl);
    *ohi = ph;
    *olo = pl;
}

// given a float value, returns the significant bits in bits, and the position of the
//   decimal point in decimal_pos.  +/-INF and NAN are specified by special values
//   returned in the decimal_pos parameter.
// frac_digits is absolute normally, but if you want from first significant digits (got %g and %e),
// or in 0x80000000
static stbsp__int32
stbsp__real_to_str(
    char const** start,
    stbsp__uint32* len,
    char* out,
    stbsp__int32* decimal_pos,
    double value,
    stbsp__uint32 frac_digits
)
{
    double d;
    stbsp__int64 bits = 0;
    stbsp__int32 expo, e, ng, tens;

    d = value;
    STBSP__COPYFP(bits, d);
    expo = (stbsp__int32)((bits >> 52) & 2047);
    ng = (stbsp__int32)((stbsp__uint64)bits >> 63);
    if (ng) {
        d = -d;
    }

    if (expo == 2047) // is nan or inf?
    {
        // CEX: lower case nan/inf
        *start = (bits & ((((stbsp__uint64)1) << 52) - 1)) ? "nan" : "inf";
        *decimal_pos = STBSP__SPECIAL;
        *len = 3;
        return ng;
    }

    if (expo == 0) // is zero or denormal
    {
        if (((stbsp__uint64)bits << 1) == 0) // do zero
        {
            *decimal_pos = 1;
            *start = out;
            out[0] = '0';
            *len = 1;
            return ng;
        }
        // find the right expo for denormals
        {
            stbsp__int64 v = ((stbsp__uint64)1) << 51;
            while ((bits & v) == 0) {
                --expo;
                v >>= 1;
            }
        }
    }

    // find the decimal exponent as well as the decimal bits of the value
    {
        double ph, pl;

        // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of
        // log10 of all expos 1..2046
        tens = expo - 1023;
        tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

        // move the significant bits into position and stick them into an int
        stbsp__raise_to_power10(&ph, &pl, d, 18 - tens);

        // get full as much precision from double-double as possible
        stbsp__ddtoS64(bits, ph, pl);

        // check if we undershot
        if (((stbsp__uint64)bits) >= stbsp__tento19th) {
            ++tens;
        }
    }

    // now do the rounding in integer land
    frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1)
                                             : (tens + frac_digits);
    if ((frac_digits < 24)) {
        stbsp__uint32 dg = 1;
        if ((stbsp__uint64)bits >= stbsp__powten[9]) {
            dg = 10;
        }
        while ((stbsp__uint64)bits >= stbsp__powten[dg]) {
            ++dg;
            if (dg == 20) {
                goto noround;
            }
        }
        if (frac_digits < dg) {
            stbsp__uint64 r;
            // add 0.5 at the right position and round
            e = dg - frac_digits;
            if ((stbsp__uint32)e >= 24) {
                goto noround;
            }
            r = stbsp__powten[e];
            bits = bits + (r / 2);
            if ((stbsp__uint64)bits >= stbsp__powten[dg]) {
                ++tens;
            }
            bits /= r;
        }
    noround:;
    }

    // kill long trailing runs of zeros
    if (bits) {
        stbsp__uint32 n;
        for (;;) {
            if (bits <= 0xffffffff) {
                break;
            }
            if (bits % 1000) {
                goto donez;
            }
            bits /= 1000;
        }
        n = (stbsp__uint32)bits;
        while ((n % 1000) == 0) {
            n /= 1000;
        }
        bits = n;
    donez:;
    }

    // convert to string
    out += 64;
    e = 0;
    for (;;) {
        stbsp__uint32 n;
        char* o = out - 8;
        // do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant
        // denomiators be damned)
        if (bits >= 100000000) {
            n = (stbsp__uint32)(bits % 100000000);
            bits /= 100000000;
        } else {
            n = (stbsp__uint32)bits;
            bits = 0;
        }
        while (n) {
            out -= 2;
            *(stbsp__uint16*)out = *(stbsp__uint16*)&stbsp__digitpair.pair[(n % 100) * 2];
            n /= 100;
            e += 2;
        }
        if (bits == 0) {
            if ((e) && (out[0] == '0')) {
                ++out;
                --e;
            }
            break;
        }
        while (out != o) {
            *--out = '0';
            ++e;
        }
    }

    *decimal_pos = tens;
    *start = out;
    *len = e;
    return ng;
}

#undef stbsp__ddmulthi
#undef stbsp__ddrenorm
#undef stbsp__ddmultlo
#undef stbsp__ddmultlos
#undef STBSP__SPECIAL
#undef STBSP__COPYFP

#endif // STB_SPRINTF_NOFLOAT

// clean up
#undef stbsp__uint16
#undef stbsp__uint32
#undef stbsp__int32
#undef stbsp__uint64
#undef stbsp__int64
#undef STBSP__UNALIGNED


/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

/*
*                   allocators.c
*/
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>

// struct Allocator_i;
#define ALLOCATOR_HEAP_MAGIC 0xFEED0001U
#define ALLOCATOR_STACK_MAGIC 0xFEED0002U
#define ALLOCATOR_STATIC_ARENA_MAGIC 0xFEED0003U

static void* allocator_heap__malloc(size_t size);
static void* allocator_heap__calloc(size_t nmemb, size_t size);
static void* allocator_heap__aligned_malloc(size_t alignment, size_t size);
static void* allocator_heap__realloc(void* ptr, size_t size);
static void* allocator_heap__aligned_realloc(void* ptr, size_t alignment, size_t size);
static void allocator_heap__free(void* ptr);
static FILE* allocator_heap__fopen(const char* filename, const char* mode);
static int allocator_heap__fclose(FILE* f);
static int allocator_heap__open(const char* pathname, int flags, mode_t mode);
static int allocator_heap__close(int fd);

static void* allocator_staticarena__malloc(size_t size);
static void* allocator_staticarena__calloc(size_t nmemb, size_t size);
static void* allocator_staticarena__aligned_malloc(size_t alignment, size_t size);
static void* allocator_staticarena__realloc(void* ptr, size_t size);
static void* allocator_staticarena__aligned_realloc(void* ptr, size_t alignment, size_t size);
static void allocator_staticarena__free(void* ptr);
static FILE* allocator_staticarena__fopen(const char* filename, const char* mode);
static int allocator_staticarena__fclose(FILE* f);
static int allocator_staticarena__open(const char* pathname, int flags, mode_t mode);
static int allocator_staticarena__close(int fd);

static allocator_heap_s
    allocator__heap_data = { .base = {
                                 .malloc = allocator_heap__malloc,
                                 .malloc_aligned = allocator_heap__aligned_malloc,
                                 .realloc = allocator_heap__realloc,
                                 .realloc_aligned = allocator_heap__aligned_realloc,
                                 .calloc = allocator_heap__calloc,
                                 .free = allocator_heap__free,
                                 .fopen = allocator_heap__fopen,
                                 .fclose = allocator_heap__fclose,
                                 .open = allocator_heap__open,
                                 .close = allocator_heap__close,
                             } };
static allocator_staticarena_s
    allocator__staticarena_data = { .base = {
                                         .malloc = allocator_staticarena__malloc,
                                         .malloc_aligned = allocator_staticarena__aligned_malloc,
                                         .calloc = allocator_staticarena__calloc,
                                         .realloc = allocator_staticarena__realloc,
                                         .realloc_aligned = allocator_staticarena__aligned_realloc,
                                         .free = allocator_staticarena__free,
                                         .fopen = allocator_staticarena__fopen,
                                         .fclose = allocator_staticarena__fclose,
                                         .open = allocator_staticarena__open,
                                         .close = allocator_staticarena__close,
                                     } };

/*
 *                  HEAP ALLOCATOR
 */

/**
 * @brief  heap-based allocator (simple proxy for malloc/free/realloc)
 */
const Allocator_i*
allocators__heap__create(void)
{
    uassert(allocator__heap_data.magic == 0 && "Already initialized");

    allocator__heap_data.magic = ALLOCATOR_HEAP_MAGIC;

#ifndef NDEBUG
    memset(&allocator__heap_data.stats, 0, sizeof(allocator__heap_data.stats));
#endif

    return &allocator__heap_data.base;
}

const Allocator_i*
allocators__heap__destroy(void)
{
    uassert(allocator__heap_data.magic != 0 && "Already destroyed");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    allocator__heap_data.magic = 0;

#ifndef NDEBUG
    allocator_heap_s* a = &allocator__heap_data;
    // NOTE: this message only shown if no DNDEBUG
    if (a->stats.n_allocs != a->stats.n_free) {
        utracef(
            "Allocator: Possible memory leaks/double free: memory allocator->allocs() [%d] != allocator->free() [%d] count! \n",
            a->stats.n_allocs,
            a->stats.n_free
        );
    }
    if (a->stats.n_fopen != a->stats.n_fclose) {
        utracef(
            "Allocator: Possible FILE* leaks: allocator->fopen() [%d] != allocator->fclose() [%d]!\n",
            a->stats.n_fopen,
            a->stats.n_fclose
        );
    }
    if (a->stats.n_open != a->stats.n_close) {
        utracef(
            "Allocator: Possible file descriptor leaks: allocator->open() [%d] != allocator->close() [%d]!\n",
            a->stats.n_open,
            a->stats.n_close
        );
    }

    memset(&allocator__heap_data.stats, 0, sizeof(allocator__heap_data.stats));
#endif

    return NULL;
}

static void*
allocator_heap__malloc(size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    allocator__heap_data.stats.n_allocs++;
#endif

    return malloc(size);
}

static void*
allocator_heap__calloc(size_t nmemb, size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    allocator__heap_data.stats.n_allocs++;
#endif

    return calloc(nmemb, size);
}

static void*
allocator_heap__aligned_malloc(size_t alignment, size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(size % alignment == 0 && "size must be rounded to align");

#ifndef NDEBUG
    allocator__heap_data.stats.n_allocs++;
#endif

    return aligned_alloc(alignment, size);
}

static void*
allocator_heap__realloc(void* ptr, size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    allocator__heap_data.stats.n_reallocs++;
#endif

    return realloc(ptr, size);
}

static void*
allocator_heap__aligned_realloc(void* ptr, size_t alignment, size_t size)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(((size_t)ptr % alignment) == 0 && "aligned_realloc existing pointer unaligned");
    uassert(size % alignment == 0 && "size must be rounded to align");

#ifndef NDEBUG
    allocator__heap_data.stats.n_reallocs++;
#endif

    // TODO: implement #ifdef MSVC it supports _aligned_realloc()

    void* result = NULL;

    // Check if we have available space for realloc'ing new size
    //
    size_t new_size = malloc_usable_size(ptr);
    // NOTE: malloc_usable_size() returns a value no less than the size of
    // the block of allocated memory pointed to by ptr.

    if (new_size >= size) {
        // This should return extended memory
        result = realloc(ptr, size);
        if (result == NULL) {
            // memory error
            return NULL;
        }
        if (result != ptr || (size_t)result % alignment != 0) {
            // very rare case, when some thread acquired space or returned unaligned result
            // Pessimistic case
            void* aligned = aligned_alloc(alignment, size);
            memcpy(aligned, result, size);
            free(result);
            return aligned;
        }
    } else {
        // No space available, alloc new memory + copy + release old
        result = aligned_alloc(alignment, size);
        if (result == NULL) {
            return NULL;
        }
        memcpy(result, ptr, new_size);
        free(ptr);
    }

    return result;
}

static void
allocator_heap__free(void* ptr)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    if (ptr != NULL) {
        allocator__heap_data.stats.n_free++;
    }
#endif

    free(ptr);
}

static FILE*
allocator_heap__fopen(const char* filename, const char* mode)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(filename != NULL);
    uassert(mode != NULL);

    FILE* res = fopen(filename, mode);

#ifndef NDEBUG
    if (res != NULL) {
        allocator__heap_data.stats.n_fopen++;
    }
#endif

    return res;
}

static int
allocator_heap__open(const char* pathname, int flags, mode_t mode)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(pathname != NULL);

    int fd = open(pathname, flags, mode);

#ifndef NDEBUG
    if (fd != -1) {
        allocator__heap_data.stats.n_open++;
    }
#endif

    return fd;
}

static int
allocator_heap__close(int fd)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    int ret = close(fd);

#ifndef NDEBUG
    if (ret != -1) {
        allocator__heap_data.stats.n_close++;
    }
#endif

    return ret;
}

static int
allocator_heap__fclose(FILE* f)
{
    uassert(allocator__heap_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__heap_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    uassert(f != NULL);
    uassert(f != stdin && "closing stdin");
    uassert(f != stdout && "closing stdout");
    uassert(f != stderr && "closing stderr");

#ifndef NDEBUG
    allocator__heap_data.stats.n_fclose++;
#endif

    return fclose(f);
}


/*
 *                  STATIC ARENA ALLOCATOR
 */

/**
 * @brief Static arena allocator (can be heap or stack arena)
 *
 * Static allocator should be created at the start of the application (maybe in main()),
 * and freed after app shutdown.
 *
 * Note: memory leaks are not caught by sanitizers, if you forget to call
 * allocators.staticarena.destroy() sanitizers will be silent.
 *
 * No realloc() supported by this arena!
 *
 * @param buffer - pointer to memory buffer
 * @param capacity - capacity of a buffer (minimal requires is 1024)
 * @return  allocator instance
 */
const Allocator_i*
allocators__staticarena__create(char* buffer, size_t capacity)
{
    uassert(allocator__staticarena_data.magic == 0 && "Already initialized");

    uassert(capacity >= 1024 && "capacity is too low");
    uassert(((capacity & (capacity - 1)) == 0) && "must be power of 2");

    allocator_staticarena_s* a = &allocator__staticarena_data;

    a->magic = ALLOCATOR_STATIC_ARENA_MAGIC;

    a->mem = buffer;
    if (a->mem == NULL) {
        memset(a, 0, sizeof(*a));
        return NULL;
    }

    memset(a->mem, 0, capacity);

    a->max = (char*)a->mem + capacity;
    a->next = a->mem;

    u32 offset = ((u64)a->next % sizeof(size_t));
    a->next = (char*)a->next + (offset ? sizeof(size_t) - offset : 0);

    uassert(((u64)a->next % sizeof(size_t) == 0) && "alloca/malloc() returned non word aligned ptr");

    return &a->base;
}

const Allocator_i*
allocators__staticarena__destroy(void)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");


    allocator_staticarena_s* a = &allocator__staticarena_data;
    a->magic = 0;
    a->mem = NULL;
    a->next = NULL;
    a->max = NULL;

#ifndef NDEBUG
    if (a->stats.n_allocs != a->stats.n_free) {
        utracef(
            "Allocator: Possible memory leaks/double free: memory allocator->allocs() [%d] != allocator->free() [%d] count! \n",
            a->stats.n_allocs,
            a->stats.n_free
        );
    }
    if (a->stats.n_fopen != a->stats.n_fclose) {
        utracef(
            "Allocator: Possible FILE* leaks: allocator->fopen() [%d] != allocator->fclose() [%d]!\n",
            a->stats.n_fopen,
            a->stats.n_fclose
        );
    }
    if (a->stats.n_open != a->stats.n_close) {
        utracef(
            "Allocator: Possible file descriptor leaks: allocator->open() [%d] != allocator->close() [%d]!\n",
            a->stats.n_open,
            a->stats.n_close
        );
    }

    memset(&allocator__staticarena_data.stats, 0, sizeof(allocator__staticarena_data.stats));
#endif

    return NULL;
}


static void*
allocator_staticarena__aligned_realloc(void* ptr, size_t alignment, size_t size)
{
    (void)ptr;
    (void)size;
    (void)alignment;
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}
static void*
allocator_staticarena__realloc(void* ptr, size_t size)
{
    (void)ptr;
    (void)size;
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}

static void
allocator_staticarena__free(void* ptr)
{
    (void)ptr;
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");

#ifndef NDEBUG
    allocator_staticarena_s* a = &allocator__staticarena_data;
    if(ptr != NULL){
        a->stats.n_free++;
    }
#endif
}

static void*
allocator_staticarena__aligned_malloc(size_t alignment, size_t size)
{
    uassert(allocator__staticarena_data.magic != 0 && "not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");

    allocator_staticarena_s* a = &allocator__staticarena_data;


    if (size == 0) {
        uassert(size > 0 && "zero size");
        return NULL;
    }
    if (size % alignment != 0) {
        uassert(size >= alignment && "size not aligned, must be a rounded to alignment");
        return NULL;
    }

    size_t offset = ((size_t)a->next % alignment);

    alignment = (offset ? alignment - offset : 0);

    if ((char*)a->next + size + alignment > (char*)a->max) {
        // not enough capacity
        return NULL;
    }

    void* ptr = (char*)a->next + alignment;
    a->next = (char*)ptr + size;

#ifndef NDEBUG
    a->stats.n_allocs++;
#endif

    return ptr;
}

static void*
allocator_staticarena__malloc(size_t size)
{
    uassert(allocator__staticarena_data.magic != 0 && "not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    allocator_staticarena_s* a = &allocator__staticarena_data;

    if (size == 0) {
        uassert(size > 0 && "zero size");
        return NULL;
    }

    void* ptr = a->next;
    if ((char*)a->next + size > (char*)a->max) {
        // not enough capacity
        return NULL;
    }

    u32 offset = (size % sizeof(size_t));
    a->next = (char*)a->next + size + (offset ? sizeof(size_t) - offset : 0);

#ifndef NDEBUG
    a->stats.n_allocs++;
#endif

    return ptr;
}

static void*
allocator_staticarena__calloc(size_t nmemb, size_t size)
{
    uassert(allocator__staticarena_data.magic != 0 && "not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    allocator_staticarena_s* a = &allocator__staticarena_data;

    size_t alloc_size = nmemb * size;
    if (nmemb != 0 && alloc_size / nmemb != size) {
        // overflow handling
        return NULL;
    }

    if (alloc_size == 0) {
        uassert(alloc_size > 0 && "zero size");
        return NULL;
    }

    void* ptr = a->next;
    if ((char*)a->next + alloc_size > (char*)a->max) {
        // not enough capacity
        return NULL;
    }

    u32 offset = (alloc_size % sizeof(size_t));
    a->next = (char*)a->next + alloc_size + (offset ? sizeof(size_t) - offset : 0);

    memset(ptr, 0, alloc_size);

#ifndef NDEBUG
    a->stats.n_allocs++;
#endif

    return ptr;
}

static FILE*
allocator_staticarena__fopen(const char* filename, const char* mode)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");
    uassert(filename != NULL);
    uassert(mode != NULL);

    FILE* res = fopen(filename, mode);

#ifndef NDEBUG
    if (res != NULL) {
        allocator_staticarena_s* a = &allocator__staticarena_data;
        a->stats.n_fopen++;
    }
#endif

    return res;
}

static int
allocator_staticarena__fclose(FILE* f)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    uassert(f != NULL);
    uassert(f != stdin && "closing stdin");
    uassert(f != stdout && "closing stdout");
    uassert(f != stderr && "closing stderr");

#ifndef NDEBUG
    allocator_staticarena_s* a = &allocator__staticarena_data;
    a->stats.n_fclose++;
#endif

    return fclose(f);
}
static int
allocator_staticarena__open(const char* pathname, int flags, mode_t mode)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");
    uassert(pathname != NULL);

    int fd = open(pathname, flags, mode);

#ifndef NDEBUG
    if (fd != -1) {
        allocator__staticarena_data.stats.n_open++;
    }
#endif
    return fd;
}

static int
allocator_staticarena__close(int fd)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    int ret = close(fd);

#ifndef NDEBUG
    if (ret != -1) {
        allocator__staticarena_data.stats.n_close++;
    }
#endif

    return ret;
}

const struct __module__allocators allocators = {
    // Autogenerated by CEX
    // clang-format off

    .heap = {  // sub-module .heap >>>
        .create = allocators__heap__create,
        .destroy = allocators__heap__destroy,
    },  // sub-module .heap <<<

    .staticarena = {  // sub-module .staticarena >>>
        .create = allocators__staticarena__create,
        .destroy = allocators__staticarena__destroy,
    },  // sub-module .staticarena <<<
    // clang-format on
};

/*
*                   argparse.c
*/
/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const u32 ARGPARSE_OPT_UNSET = 1;
const u32 ARGPARSE_OPT_LONG = (1 << 1);

// static const u8 1 /*ARGPARSE_OPT_GROUP*/  = 1;
// static const u8 2 /*ARGPARSE_OPT_BOOLEAN*/ = 2;
// static const u8 3 /*ARGPARSE_OPT_BIT*/ = 3;
// static const u8 4 /*ARGPARSE_OPT_INTEGER*/ = 4;
// static const u8 5 /*ARGPARSE_OPT_FLOAT*/ = 5;
// static const u8 6 /*ARGPARSE_OPT_STRING*/ = 6;

static const char*
argparse__prefix_skip(const char* str, const char* prefix)
{
    size_t len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static int
argparse__prefix_cmp(const char* str, const char* prefix)
{
    for (;; str++, prefix++) {
        if (!*prefix) {
            return 0;
        } else if (*str != *prefix) {
            return (unsigned char)*prefix - (unsigned char)*str;
        }
    }
}

static Exception
argparse__error(argparse_c* self, const argparse_opt_s* opt, const char* reason, int flags)
{
    (void)self;
    if (flags & ARGPARSE_OPT_LONG) {
        fprintf(stdout, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        fprintf(stdout, "error: option `-%c` %s\n", opt->short_name, reason);
    }

    return Error.argument;
}

void
argparse_usage(argparse_c* self)
{
    uassert(self->_ctx.argv != NULL && "usage before parse!");

    fprintf(stdout, "Usage:\n");
    if (self->usage) {

        for$iter(str_c, it, str.iter_split(s$(self->usage), "\n", &it.iterator))
        {
            if (it.val->len == 0) {
                break;
            }

            char* fn = strrchr(self->program_name, '/');
            if (fn != NULL) {
                fprintf(stdout, "%s ", fn + 1);
            } else {
                fprintf(stdout, "%s ", self->program_name);
            }

            if (fwrite(it.val->buf, sizeof(char), it.val->len, stdout)) {
                ;
            }

            fputc('\n', stdout);
        }
    } else {
        fprintf(stdout, "%s [options] [--] [arg1 argN]\n", self->program_name);
    }

    // print description
    if (self->description) {
        fprintf(stdout, "%s\n", self->description);
    }

    fputc('\n', stdout);

    const argparse_opt_s* options;

    // figure out best width
    size_t usage_opts_width = 0;
    size_t len;
    options = self->options;
    for (u32 i = 0; i < self->options_len; i++, options++) {
        len = 0;
        if ((options)->short_name) {
            len += 2;
        }
        if ((options)->short_name && (options)->long_name) {
            len += 2; // separator ", "
        }
        if ((options)->long_name) {
            len += strlen((options)->long_name) + 2;
        }
        if (options->type == 4 /*ARGPARSE_OPT_INTEGER*/) {
            len += strlen("=<int>");
        }
        if (options->type == 5 /*ARGPARSE_OPT_FLOAT*/) {
            len += strlen("=<flt>");
        } else if (options->type == 6 /*ARGPARSE_OPT_STRING*/) {
            len += strlen("=<str>");
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) {
            usage_opts_width = len;
        }
    }
    usage_opts_width += 4; // 4 spaces prefix

    options = self->options;
    for (u32 i = 0; i < self->options_len; i++, options++) {
        size_t pos = 0;
        size_t pad = 0;
        if (options->type == 1 /*ARGPARSE_OPT_GROUP*/) {
            fputc('\n', stdout);
            fprintf(stdout, "%s", options->help);
            fputc('\n', stdout);
            continue;
        }
        pos = fprintf(stdout, "    ");
        if (options->short_name) {
            pos += fprintf(stdout, "-%c", options->short_name);
        }
        if (options->long_name && options->short_name) {
            pos += fprintf(stdout, ", ");
        }
        if (options->long_name) {
            pos += fprintf(stdout, "--%s", options->long_name);
        }
        if (options->type == 4 /*ARGPARSE_OPT_INTEGER*/) {
            pos += fprintf(stdout, "=<int>");
        } else if (options->type == 5 /*ARGPARSE_OPT_FLOAT*/) {
            pos += fprintf(stdout, "=<flt>");
        } else if (options->type == 6 /*ARGPARSE_OPT_STRING*/) {
            pos += fprintf(stdout, "=<str>");
        }
        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            fputc('\n', stdout);
            pad = usage_opts_width;
        }
        fprintf(stdout, "%*s%s\n", (int)pad + 2, "", options->help);
    }

    // print epilog
    if (self->epilog) {
        fprintf(stdout, "%s\n", self->epilog);
    }
}
static Exception
argparse__getvalue(argparse_c* self, argparse_opt_s* opt, int flags)
{
    const char* s = NULL;
    if (!opt->value) {
        goto skipped;
    }

    switch (opt->type) {
        case 2 /*ARGPARSE_OPT_BOOLEAN*/:
            if (flags & ARGPARSE_OPT_UNSET) {
                *(int*)opt->value = 0;
            } else {
                *(int*)opt->value = 1;
            }
            opt->is_present = true;
            break;
        case 3 /*ARGPARSE_OPT_BIT*/:
            if (flags & ARGPARSE_OPT_UNSET) {
                *(int*)opt->value &= ~opt->data;
            } else {
                *(int*)opt->value |= opt->data;
            }
            opt->is_present = true;
            break;
        case 6 /*ARGPARSE_OPT_STRING*/:
            if (self->_ctx.optvalue) {
                *(const char**)opt->value = self->_ctx.optvalue;
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                *(const char**)opt->value = *++self->_ctx.argv;
            } else {
                return argparse__error(self, opt, "requires a value", flags);
            }
            opt->is_present = true;
            break;
        case 4 /*ARGPARSE_OPT_INTEGER*/:
            errno = 0;
            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return argparse__error(self, opt, "requires a value", flags);
                }

                *(int*)opt->value = strtol(self->_ctx.optvalue, (char**)&s, 0);
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                *(int*)opt->value = strtol(*++self->_ctx.argv, (char**)&s, 0);
            } else {
                return argparse__error(self, opt, "requires a value", flags);
            }
            if (errno == ERANGE) {
                return argparse__error(self, opt, "numerical result out of range", flags);
            }
            if (s[0] != '\0') { // no digits or contains invalid characters
                return argparse__error(self, opt, "expects an integer value", flags);
            }
            opt->is_present = true;
            break;
        case 5 /*ARGPARSE_OPT_FLOAT*/:
            errno = 0;

            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return argparse__error(self, opt, "requires a value", flags);
                }
                *(float*)opt->value = strtof(self->_ctx.optvalue, (char**)&s);
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                *(float*)opt->value = strtof(*++self->_ctx.argv, (char**)&s);
            } else {
                return argparse__error(self, opt, "requires a value", flags);
            }
            if (errno == ERANGE) {
                return argparse__error(self, opt, "numerical result out of range", flags);
            }
            if (s[0] != '\0') { // no digits or contains invalid characters
                return argparse__error(self, opt, "expects a numerical value", flags);
            }
            opt->is_present = true;
            break;
        default:
            uassert(false && "unhandled");
            return Error.sanity_check;
    }

skipped:
    if (opt->callback) {
        opt->is_present = true;
        return opt->callback(self, opt);
    } else {
        if (opt->short_name == 'h') {
            argparse_usage(self);
            return Error.argsparse;
        }
    }

    return Error.ok;
}

static Exception
argparse__options_check(argparse_c* self, bool reset)
{
    var options = self->options;

    for (u32 i = 0; i < self->options_len; i++, options++) {
        switch (options->type) {
            case 1 /*ARGPARSE_OPT_GROUP*/:
                continue;
            case 2 /*ARGPARSE_OPT_BOOLEAN*/:
            case 3 /*ARGPARSE_OPT_BIT*/:
            case 4 /*ARGPARSE_OPT_INTEGER*/:
            case 5 /*ARGPARSE_OPT_FLOAT*/:
            case 6 /*ARGPARSE_OPT_STRING*/:
                if (reset) {
                    options->is_present = 0;
                    if (!(options->short_name || options->long_name)) {
                        uassert(
                            (options->short_name || options->long_name) &&
                            "options both long/short_name NULL"
                        );
                        return Error.sanity_check;
                    }
                    if (options->value == NULL && options->short_name != 'h') {
                        uassertf(
                            options->value != NULL,
                            "option value [%c/%s] is null\n",
                            options->short_name,
                            options->long_name
                        );
                        return Error.sanity_check;
                    }
                } else {
                    if (options->required && !options->is_present) {
                        fprintf(
                            stdout,
                            "Error: missing required option: -%c/--%s\n",
                            options->short_name,
                            options->long_name
                        );
                        return Error.argsparse;
                    }
                }
                continue;
            default:
                uassertf(false, "wrong option type: %d", options->type);
        }
    }

    return Error.ok;
}

static Exception
argparse__short_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        if (options->short_name == *self->_ctx.optvalue) {
            self->_ctx.optvalue = self->_ctx.optvalue[1] ? self->_ctx.optvalue + 1 : NULL;
            return argparse__getvalue(self, options, 0);
        }
    }
    return Error.not_found;
}

static Exception
argparse__long_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        const char* rest;
        int opt_flags = 0;
        if (!options->long_name) {
            continue;
        }

        rest = argparse__prefix_skip(self->_ctx.argv[0] + 2, options->long_name);
        if (!rest) {
            // negation disabled?
            if (options->flags & ARGPARSE_OPT_NONEG) {
                continue;
            }
            // only OPT_BOOLEAN/OPT_BIT supports negation
            if (options->type != 2 /*ARGPARSE_OPT_BOOLEAN*/ &&
                options->type != 3 /*ARGPARSE_OPT_BIT*/) {
                continue;
            }

            if (argparse__prefix_cmp(self->_ctx.argv[0] + 2, "no-")) {
                continue;
            }
            rest = argparse__prefix_skip(self->_ctx.argv[0] + 2 + 3, options->long_name);
            if (!rest) {
                continue;
            }
            opt_flags |= ARGPARSE_OPT_UNSET;
        }
        if (*rest) {
            if (*rest != '=') {
                continue;
            }
            self->_ctx.optvalue = rest + 1;
        }
        return argparse__getvalue(self, options, opt_flags | ARGPARSE_OPT_LONG);
    }
    return Error.not_found;
}


static Exception
argparse__report_error(argparse_c* self, Exc err)
{
    // invalidate argc
    self->_ctx.argc = 0;

    if (err == Error.not_found) {
        fprintf(stdout, "error: unknown option `%s`\n", self->_ctx.argv[0]);
    } else if (err == Error.integrity) {
        fprintf(stdout, "error: option `%s` follows argument\n", self->_ctx.argv[0]);
    }
    return Error.argsparse;
}

Exception
argparse_parse(argparse_c* self, int argc, char** argv)
{
    if (self->options_len == 0) {
        return raise_exc(
            Error.sanity_check,
            "zero options provided or self.options_len field not set\n"
        );
    }
    uassert(argc > 0);
    uassert(argv != NULL);
    uassert(argv[0] != NULL);

    if (self->program_name == NULL) {
        self->program_name = argv[0];
    }

    // reset if we have several runs
    memset(&self->_ctx, 0, sizeof(self->_ctx));

    self->_ctx.argc = argc - 1;
    self->_ctx.argv = argv + 1;
    self->_ctx.out = argv;

    except(err, argparse__options_check(self, true))
    {
        return err;
    }

    for (; self->_ctx.argc; self->_ctx.argc--, self->_ctx.argv++) {
        const char* arg = self->_ctx.argv[0];
        if (arg[0] != '-' || !arg[1]) {
            self->_ctx.has_argument = true;

            if (self->flags.stop_at_non_option) {
                self->_ctx.argc--;
                self->_ctx.argv++;
                break;
            }
            continue;
        }
        // short option
        if (arg[1] != '-') {
            if (self->_ctx.has_argument) {
                // options are not allowed after arguments
                return argparse__report_error(self, Error.integrity);
            }

            self->_ctx.optvalue = arg + 1;
            self->_ctx.cpidx++;
            except(err, argparse__short_opt(self, self->options))
            {
                return argparse__report_error(self, err);
            }
            while (self->_ctx.optvalue) {
                except(err, argparse__short_opt(self, self->options))
                {
                    return argparse__report_error(self, err);
                }
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            self->_ctx.argc--;
            self->_ctx.argv++;
            self->_ctx.cpidx++;
            break;
        }
        // long option
        if (self->_ctx.has_argument) {
            // options are not allowed after arguments
            return argparse__report_error(self, Error.integrity);
        }
        except(err, argparse__long_opt(self, self->options))
        {
            return argparse__report_error(self, err);
        }
        self->_ctx.cpidx++;
        continue;
    }

    except(err, argparse__options_check(self, false))
    {
        return err;
    }

    self->_ctx.out += self->_ctx.cpidx + 1; // excludes 1st argv[0], program_name
    self->_ctx.argc = argc - self->_ctx.cpidx - 1;

    return Error.ok;
}

u32
argparse_argc(argparse_c* self)
{
    uassert(self->_ctx.out != NULL && "argparse.parse() not called?");
    return self->_ctx.argc;
}

char**
argparse_argv(argparse_c* self)
{
    uassert(self->_ctx.out != NULL && "argparse.parse() not called?");
    return self->_ctx.out;
}


const struct __module__argparse argparse = {
    // Autogenerated by CEX
    // clang-format off
    .usage = argparse_usage,
    .parse = argparse_parse,
    .argc = argparse_argc,
    .argv = argparse_argv,
    // clang-format on
};

/*
*                   dict.c
*/
#include <stdarg.h>
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
 * @brief Compares static char[] buffer keys **must be null terminated**
 *
 * @param a  char[N] string
 * @param b  char[N] string
 * @param udata  (unused)
 * @return compared int value
 */
static int
hm_str_static_compare(const void* a, const void* b, void* udata)
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
hm_str_static_hash(const void* item, u64 seed0, u64 seed1)
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

/*
*                   io.c
*/
#include <errno.h>
#include <unistd.h>

Exception
io_fopen(io_c* self, const char* filename, const char* mode, const Allocator_i* allocator)
{
    if (self == NULL) {
        uassert(self != NULL);
        return Error.argument;
    }
    if (filename == NULL) {
        uassert(filename != NULL);
        return Error.argument;
    }
    if (mode == NULL) {
        uassert(mode != NULL);
        return Error.argument;
    }
    if (allocator == NULL) {
        uassert(allocator != NULL);
        return Error.argument;
    }


    *self = (io_c){
        ._fh = allocator->fopen(filename, mode),
        ._allocator = allocator,
    };

    if (self->_fh == NULL) {
        memset(self, 0, sizeof(*self));
        return raise_exc(Error.io, "%s, file: %s\n", strerror(errno), filename);
    }

    return Error.ok;
}

Exception
io_fattach(io_c* self, FILE* fh, const Allocator_i* allocator)
{
    if (self == NULL) {
        uassert(self != NULL);
        return Error.argument;
    }
    if (allocator == NULL) {
        uassert(allocator != NULL);
        return Error.argument;
    }
    if (fh == NULL) {
        uassert(fh != NULL);
        return Error.argument;
    }

    *self = (io_c){ ._fh = fh,
                    ._allocator = allocator,
                    ._flags = {
                        .is_attached = true,
                    } };

    return Error.ok;
}

int
io_fileno(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    return fileno(self->_fh);
}

bool
io_isatty(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    return isatty(fileno(self->_fh)) == 1;
}

Exception
io_flush(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    int ret = fflush(self->_fh);
    if (unlikely(ret == -1)) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

Exception
io_seek(io_c* self, long offset, int whence)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    int ret = fseek(self->_fh, offset, whence);
    if (unlikely(ret == -1)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
    } else {
        return Error.ok;
    }
}

void
io_rewind(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    rewind(self->_fh);
}

Exception
io_tell(io_c* self, size_t* size)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    long ret = ftell(self->_fh);
    if (unlikely(ret < 0)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
        *size = 0;
    } else {
        *size = ret;
        return Error.ok;
    }
}

size_t
io_size(io_c* self)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    if (self->_fsize == 0) {
        // Do some caching
        size_t old_pos = 0;
        except(err, io_tell(self, &old_pos))
        {
            return 0;
        }
        except(err, io_seek(self, 0, SEEK_END))
        {
            return 0;
        }
        except(err, io_tell(self, &self->_fsize))
        {
            return 0;
        }
        except(err, io_seek(self, old_pos, SEEK_SET))
        {
            return 0;
        }
    }

    return self->_fsize;
}

Exception
io_read(io_c* self, void* restrict obj_buffer, size_t obj_el_size, size_t* obj_count)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == NULL || *obj_count == 0) {
        return Error.argument;
    }

    const size_t ret_count = fread(obj_buffer, obj_el_size, *obj_count, self->_fh);

    if (ret_count != *obj_count) {
        if (ferror(self->_fh)) {
            *obj_count = 0;
            return Error.io;
        } else {
            *obj_count = ret_count;
            return (ret_count == 0) ? Error.eof : Error.ok;
        }
    }

    return Error.ok;
}

Exception
io_readall(io_c* self, str_c* s)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);
    uassert(s != NULL);

    // invalidate result if early exit
    *s = (str_c){
        .buf = NULL,
        .len = 0,
    };

    // Forbid console and stdin
    if (io_isatty(self)) {
        return raise_exc(Error.io, "io.readall() not allowed for pipe/socket/std[in/out/err]\n");
    }

    self->_fsize = io_size(self);

    if (unlikely(self->_fsize == 0)) {

        *s = (str_c){
            .buf = "",
            .len = 0,
        };
        except_traceback(err, io_seek(self, 0, SEEK_END))
        {
            ;
        }
        return Error.eof;
    }
    // allocate extra 16 bytes, to catch condition when file size grows
    // this may be indication we are trying to read stream
    size_t exp_size = self->_fsize + 1 + 15;

    if (self->_fbuf == NULL) {
        self->_fbuf = self->_allocator->malloc(exp_size);
        self->_fbuf_size = exp_size;
    } else {
        if (self->_fbuf_size < exp_size) {
            self->_fbuf = self->_allocator->realloc(self->_fbuf, exp_size);
            self->_fbuf_size = exp_size;
        }
    }
    if (unlikely(self->_fbuf == NULL)) {
        self->_fbuf_size = 0;
        return Error.memory;
    }

    size_t read_size = self->_fbuf_size;
    except(err, io_read(self, self->_fbuf, sizeof(char), &read_size))
    {
        return err;
    }

    if (read_size != self->_fsize) {
        utracef("%ld != %ld: %s\n", read_size, self->_fsize, strerror(errno));
        return "File size changed";
    }

    *s = (str_c){
        .buf = self->_fbuf,
        .len = read_size,
    };

    // Always null terminate
    self->_fbuf[read_size] = '\0';

    return read_size == 0 ? Error.eof : Error.ok;
}

Exception
io_readline(io_c* self, str_c* s)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);
    uassert(s != NULL);


    Exc result = Error.ok;
    size_t cursor = 0;
    FILE* fh = self->_fh;
    char* buf = self->_fbuf;
    size_t buf_size = self->_fbuf_size;

    int c = EOF;
    while ((c = fgetc(fh)) != EOF) {
        if (unlikely(c == '\n')) {
            // Handle windows \r\n new lines also
            if (cursor > 0 && buf[cursor - 1] == '\r') {
                cursor--;
            }
            break;
        }
        if (unlikely(c == '\0')) {
            // plain text file should not have any zero bytes in there
            result = Error.integrity;
            goto fail;
        }

        if (unlikely(cursor >= buf_size)) {
            if (self->_fbuf == NULL) {
                uassert(cursor == 0 && "no buf, cursor expected 0");

                self->_fbuf = buf = self->_allocator->malloc(4096);
                if (self->_fbuf == NULL) {
                    result = Error.memory;
                    goto fail;
                }
                self->_fbuf_size = buf_size = 4096 - 1; // keep extra for null
                self->_fbuf[self->_fbuf_size] = '\0';
                self->_fbuf[cursor] = '\0';
            } else {
                uassert(cursor > 0 && "no buf, cursor expected 0");
                uassert(self->_fbuf_size > 0 && "empty buffer, weird");

                if (self->_fbuf_size + 1 < 4096) {
                    // Cap minimal buf size
                    self->_fbuf_size = 4095;
                }

                // Grow initial size by factor of 2
                self->_fbuf = buf = self->_allocator->realloc(
                    self->_fbuf,
                    (self->_fbuf_size + 1) * 2
                );
                if (self->_fbuf == NULL) {
                    self->_fbuf_size = 0;
                    result = Error.memory;
                    goto fail;
                }
                self->_fbuf_size = buf_size = (self->_fbuf_size + 1) * 2 - 1;
                self->_fbuf[self->_fbuf_size] = '\0';
            }
        }
        buf[cursor] = c;
        cursor++;
    }

    if (self->_fbuf != NULL) {
        self->_fbuf[cursor] = '\0';
    }

    if (ferror(self->_fh)) {
        result = Error.io;
        goto fail;
    }

    if (cursor == 0) {
        // return valid str_c, but empty string
        *s = (str_c){
            .buf = "",
            .len = cursor,
        };
        return (feof(self->_fh) ? Error.eof : Error.ok);
    } else {
        *s = (str_c){
            .buf = self->_fbuf,
            .len = cursor,
        };
        return Error.ok;
    }

fail:
    *s = (str_c){
        .buf = NULL,
        .len = 0,
    };
    return result;
}

Exception
io_fprintf(io_c* self, const char* format, ...)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    va_list va;
    va_start(va, format);
    int result = STB_SPRINTF_DECORATE(vfprintf)(self->_fh, format, va);
    va_end(va);

    if (result == -1) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

Exception
io_write(io_c* self, void* restrict obj_buffer, size_t obj_el_size, size_t obj_count)
{
    uassert(self != NULL);
    uassert(self->_fh != NULL);

    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == 0) {
        return Error.argument;
    }

    const size_t ret_count = fwrite(obj_buffer, obj_el_size, obj_count, self->_fh);

    if (ret_count != obj_count) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

void
io_close(io_c* self)
{
    if (self != NULL) {
        uassert(self->_allocator != NULL && "allocator not set");

        if (self->_fh != NULL && !self->_flags.is_attached) {
            // prevent closing attached FILE* (i.e. stdin/out or other)
            self->_allocator->fclose(self->_fh);
        }

        if (self->_fbuf != NULL) {
            self->_allocator->free(self->_fbuf);
        }

        memset(self, 0, sizeof(*self));
    }
}


const struct __module__io io = {
    // Autogenerated by CEX
    // clang-format off
    .fopen = io_fopen,
    .fattach = io_fattach,
    .fileno = io_fileno,
    .isatty = io_isatty,
    .flush = io_flush,
    .seek = io_seek,
    .rewind = io_rewind,
    .tell = io_tell,
    .size = io_size,
    .read = io_read,
    .readall = io_readall,
    .readline = io_readline,
    .fprintf = io_fprintf,
    .write = io_write,
    .close = io_close,
    // clang-format on
};

/*
*                   list.c
*/

static inline list_head_s*
list__head(list_c* self)
{
    uassert(self != NULL);
    uassert(self->arr != NULL && "array is not initialized");
    list_head_s* head = (list_head_s*)((char*)self->arr - sizeof(list_head_s));
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    uassert(head->capacity > 0 && "zero capacity or memory corruption");
    uassert(head->count <= head->capacity && "count > capacity");

    return head;
}
static inline size_t
list__alloc_capacity(size_t capacity)
{
    uassert(capacity > 0);

    if (capacity < 4) {
        return 4;
    } else if (capacity >= 1024) {
        return capacity * 1.2;
    } else {
        // Round up to closest pow*2 int
        u64 p = 4;
        while (p < capacity) {
            p *= 2;
        }
        return p;
    }
}

static inline void*
list__elidx(list_head_s* head, size_t idx)
{
    // Memory alignment
    // |-----|head|--el1--|--el2--|--elN--|
    //  ^^^^ - head can moved to el1, because of el1 alignment
    void* result = (char*)head + sizeof(list_head_s) + (head->header.elsize * idx);

    uassert((size_t)result % head->header.elalign == 0 && "misaligned array index pointer");

    return result;
}

static inline list_head_s*
list__realloc(list_head_s* head, size_t alloc_size)
{
    // Memory alignment
    // |-----|head|--el1--|--el2--|--elN--|
    //  ^^^^ - head can moved to el1, because of el1 alignment
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    size_t offset = 0;
    size_t align = alignof(list_head_s);
    if (head->header.elalign > sizeof(list_head_s)) {
        align = head->header.elalign;
        offset = head->header.elalign - sizeof(list_head_s);
    }
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    void* mptr = (char*)head - offset;
    uassert((size_t)mptr % alignof(size_t) == 0 && "misaligned malloc pointer");
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    uassert(alloc_size % head->header.elalign == 0 && "misaligned size");

    void* result = head->allocator->realloc_aligned(mptr, align, alloc_size);
    uassert((size_t)result % align == 0 && "misaligned after realloc");

    head = (list_head_s*)((char*)result + offset);
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    return head;
}

static inline size_t
list__alloc_size(size_t capacity, size_t elsize, size_t elalign)
{
    uassert(capacity > 0 && "zero capacity");
    uassert(elsize > 0 && "zero element size");
    uassert(elalign > 0 && "zero element align");
    uassert(elsize % elalign == 0 && "element size has to be rounded to elalign");

    size_t result = (capacity * elsize) +
                    (elalign > sizeof(list_head_s) ? elalign : sizeof(list_head_s));
    uassert(result % elalign == 0 && "alloc_size is unaligned");
    return result;
}

Exception
list_create(
    list_c* self,
    size_t capacity,
    size_t elsize,
    size_t elalign,
    const Allocator_i* allocator
)
{
    if (self == NULL) {
        uassert(self != NULL && "must not be NULL");
        return Error.argument;
    }

    if (allocator == NULL) {
        uassert(allocator != NULL && "allocator invalid");
        return Error.argument;
    }

    if (elsize == 0 || elsize > INT16_MAX) {
        uassert(elsize > 0 && "zero elsize");
        uassert(elsize < INT16_MAX && "element size if too high");
        return Error.argument;
    }
    if (elalign == 0 || (elalign & (elalign - 1)) != 0) {
        uassert((elalign & (elalign - 1)) == 0 && "elalign must be power of 2");
        uassert(elsize > 0 && "zero elsize");
        uassert(elalign > 0 && "zero elalign");
        return Error.argument;
    }

    // Clear dlist pointer
    memset(self, 0, sizeof(list_c));

    capacity = list__alloc_capacity(capacity);
    size_t alloc_size = list__alloc_size(capacity, elsize, elalign);
    char* buf = allocator->malloc_aligned(elalign, alloc_size);

    if (buf == NULL) {
        return Error.memory;
    }

    if (elalign > sizeof(list_head_s)) {
        buf += elalign - sizeof(list_head_s);
    }

    list_head_s* head = (list_head_s*)buf;
    *head = (list_head_s){
        .header = {
            .magic = 0x1eed,
            .elsize = elsize,
            .elalign = elalign,
        },
        .count = 0,
        .capacity = capacity,
        .allocator = allocator,
    };

    list_c* d = (list_c*)self;
    d->len = 0;
    d->arr = list__elidx(head, 0);

    return Error.ok;
}

Exception
list_create_static(list_c* self, void* buf, size_t buf_len, size_t elsize, size_t elalign)
{
    if (self == NULL) {
        uassert(self != NULL && "must not be NULL");
        return Error.argument;
    }

    if (elsize == 0 || elsize > INT16_MAX) {
        uassert(elsize > 0 && "zero elsize");
        uassert(elsize < INT16_MAX && "element size if too high");
        return Error.argument;
    }
    if (elalign == 0 || elalign > 64 || (elalign & (elalign - 1)) != 0) {
        uassert(elalign <= 64 && "el align is too high");
        uassert((elalign & (elalign - 1)) == 0 && "elalign must be power of 2");
        uassert(elsize > 0 && "zero elsize");
        uassert(elalign > 0 && "zero elalign");
        return Error.argument;
    }

    // Clear dlist pointer
    memset(self, 0, sizeof(list_c));

    size_t offset = ((size_t)buf + sizeof(list_head_s)) % elalign;
    if (offset != 0) {
        offset = elalign - offset;
    }

    if (buf_len < sizeof(list_head_s) + offset + elsize) {
        return Error.overflow;
    }
    size_t max_capacity = (buf_len - sizeof(list_head_s) - offset) / elsize;

    list_head_s* head = (list_head_s*)((char*)buf + offset);
    *head = (list_head_s){
        .header = {
            .magic = 0x1eed,
            .elsize = elsize,
            .elalign = elalign,
        },
        .count = 0,
        .capacity = max_capacity,
        .allocator = NULL,
    };

    list_c* d = (list_c*)self;
    d->len = 0;
    d->arr = list__elidx(head, 0);

    return Error.ok;
}


Exception
list_insert(void* self, void* item, size_t index)
{
    uassert(self != NULL);
    list_c* d = (list_c*)self;
    list_head_s* head = list__head(self);

    if (index > head->count) {
        return Error.argument;
    }

    if (head->count == head->capacity) {
        if (head->allocator == NULL) {
            return Error.overflow;
        }
        size_t new_cap = list__alloc_capacity(head->capacity + 1);
        size_t alloc_size = list__alloc_size(new_cap, head->header.elsize, head->header.elalign);
        head = list__realloc(head, alloc_size);
        uassert(head->header.magic == 0x1eed && "head missing after realloc");

        if (head == NULL) {
            d->len = 0;
            return Error.memory;
        }

        d->arr = list__elidx(head, 0);
        head->capacity = new_cap;
    }

    if (index < head->count) {
        memmove(
            list__elidx(head, index + 1),
            list__elidx(head, index),
            head->header.elsize * (head->count - index)
        );
    }

    memcpy(list__elidx(head, index), item, head->header.elsize);

    head->count++;
    d->len = head->count;

    return Error.ok;
}

Exception
list_del(void* self, size_t index)
{
    uassert(self != NULL);
    list_c* d = (list_c*)self;
    list_head_s* head = list__head(self);

    if (index >= head->count) {
        return Error.argument;
    }

    if (index < head->count) {
        memmove(
            list__elidx(head, index),
            list__elidx(head, index + 1),
            head->header.elsize * (head->count - index - 1)
        );
    }

    head->count--;
    d->len = head->count;

    return Error.ok;
}

void
list_sort(void* self, int (*comp)(const void*, const void*))
{
    uassert(self != NULL);
    uassert(comp != NULL);
    list_c* d = (list_c*)self;
    list_head_s* head = list__head(self);
    qsort(d->arr, head->count, head->header.elsize, comp);
}


Exception
list_append(void* self, void* item)
{
    uassert(self != NULL);
    list_head_s* head = list__head(self);
    return list_insert(self, item, head->count);
}

void
list_clear(void* self)
{
    uassert(self != NULL);

    list_c* d = (list_c*)self;
    list_head_s* head = list__head(self);
    head->count = 0;
    d->len = 0;
}

Exception
list_extend(void* self, void* items, size_t nitems)
{
    uassert(self != NULL);

    list_c* d = (list_c*)self;
    list_head_s* head = list__head(self);

    if (unlikely(items == NULL || nitems == 0)) {
        return Error.argument;
    }

    if (head->count + nitems > head->capacity) {
        if (head->allocator == NULL) {
            return Error.overflow;
        }

        size_t new_cap = list__alloc_capacity(head->capacity + nitems);

        size_t alloc_size = list__alloc_size(new_cap, head->header.elsize, head->header.elalign);
        head = list__realloc(head, alloc_size);
        uassert(head->header.magic == 0x1eed && "head missing after realloc");

        if (d->arr == NULL) {
            d->len = 0;
            return Error.memory;
        }

        d->arr = list__elidx(head, 0);
        head->capacity = new_cap;
    }
    memcpy(list__elidx(head, d->len), items, head->header.elsize * nitems);
    head->count += nitems;
    d->len = head->count;

    return Error.ok;
}

size_t
list_len(void* self)
{
    list_c* d = (list_c*)self;
    list_head_s* head = list__head(self);
    d->len = head->count;
    return head->count;
}

size_t
list_capacity(void* self)
{
    list_head_s* head = list__head(self);
    return head->capacity;
}

void*
list_destroy(void* self)
{
    list_c* d = (list_c*)self;

    if (self != NULL) {
        if (d->arr != NULL) {
            list_head_s* head = list__head(self);

            size_t offset = 0;
            if (head->header.elalign > sizeof(list_head_s)) {
                offset = head->header.elalign - sizeof(list_head_s);
            }

            void* mptr = (char*)head - offset;
            if (head->allocator != NULL) {
                // free only if it's a dynamic array
                head->allocator->free(mptr);
            } else {
                // in static list reset head
                memset(head, 0, sizeof(*head));
            }
            d->arr = NULL;
            d->len = 0;
        }
    }

    return NULL;
}

void*
list_iter(void* self, cex_iterator_s* iterator)
{
    uassert(self != NULL && "self == NULL");
    uassert(iterator != NULL && "null iterator");

    list_c* d = (list_c*)self;
    list_head_s* head = list__head(self);

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) == 8, "ctx alignment mismatch");

    if (unlikely(iterator->val == NULL)) {
        ctx->cursor = 0;
    } else {
        ctx->cursor++;
    }

    if (ctx->cursor >= d->len) {
        return NULL;
    }

    iterator->idx.i = ctx->cursor;
    iterator->val = (char*)d->arr + ctx->cursor * head->header.elsize;
    return iterator->val;
}

const struct __module__list list = {
    // Autogenerated by CEX
    // clang-format off
    .create = list_create,
    .create_static = list_create_static,
    .insert = list_insert,
    .del = list_del,
    .sort = list_sort,
    .append = list_append,
    .clear = list_clear,
    .extend = list_extend,
    .len = list_len,
    .capacity = list_capacity,
    .destroy = list_destroy,
    .iter = list_iter,
    // clang-format on
};

/*
*                   sbuf.c
*/
#include <stdarg.h>

struct _sbuf__sprintf_ctx
{
    sbuf_head_s* head;
    char* buf;
    Exc err;
    int count;
    int length;
    char tmp[STB_SPRINTF_MIN];
};

static inline sbuf_head_s*
sbuf__head(sbuf_c self)
{
    uassert(self != NULL);
    sbuf_head_s* head = (sbuf_head_s*)(self - sizeof(sbuf_head_s));

    uassert(head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");
    uassert(head->capacity > 0 && "zero capacity or memory corruption");
    uassert(head->length <= head->capacity && "count > capacity");
    uassert(head->header.nullterm == 0 && "nullterm != 0");

    return head;
}
static inline size_t
sbuf__alloc_capacity(size_t capacity)
{
    uassert(capacity < INT32_MAX && "requested capacity exceeds 2gb, maybe overflow?");

    capacity += sizeof(sbuf_head_s) + 1; // also +1 for nullterm

    if (capacity >= 512) {
        return capacity * 1.2;
    } else {
        // Round up to closest pow*2 int
        u64 p = 4;
        while (p < capacity) {
            p *= 2;
        }
        return p;
    }
}
static inline Exception
sbuf__grow_buffer(sbuf_c* self, u32 length)
{
    sbuf_head_s* head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));

    if (unlikely(head->allocator == NULL)) {
        // sbuf is static, bad luck, overflow
        return Error.overflow;
    }

    u32 new_capacity = sbuf__alloc_capacity(length);
    head = head->allocator->realloc(head, new_capacity);
    if (unlikely(head == NULL)) {
        *self = NULL;
        return Error.memory;
    }

    head->capacity = new_capacity - sizeof(sbuf_head_s) - 1,
    *self = (char*)head + sizeof(sbuf_head_s);
    (*self)[head->capacity] = '\0';
    return Error.ok;
}

Exception
sbuf_create(sbuf_c* self, u32 capacity, const Allocator_i* allocator)
{
    uassert(self != NULL);
    uassert(capacity != 0);
    uassert(allocator != NULL);

    if (capacity < 512) {
        // NOTE: if buffer is too small, allocate more size based on sbuf__alloc_capacity() formula
        // otherwise use exact number from a user to build the initial buffer
        capacity = sbuf__alloc_capacity(capacity);
    }

    char* buf = allocator->malloc(capacity);

    if (buf == NULL) {
        return Error.memory;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = capacity - sizeof(sbuf_head_s) - 1,
        .allocator = allocator,
    };

    *self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (*self)[0] = '\0';
    (*self)[head->capacity] = '\0';

    return Error.ok;
}

Exception
sbuf_create_static(sbuf_c* self, char* buf, size_t buf_size)
{
    if (unlikely(self == NULL)) {
        uassert(self != NULL);
        return Error.argument;
    }
    if (unlikely(buf == NULL)) {
        uassert(buf != NULL);
        return Error.argument;
    }
    if (unlikely(buf_size == 0)) {
        uassert(buf_size > 0);
        return Error.argument;
    }
    if (unlikely(buf_size <= sizeof(sbuf_head_s) + 1)) {
        uassert(buf_size > sizeof(sbuf_head_s) + 1);
        return Error.overflow;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = buf_size - sizeof(sbuf_head_s) - 1,
        .allocator = NULL,
    };

    *self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (*self)[0] = '\0';
    (*self)[head->capacity] = '\0';

    return Error.ok;
}

Exception
sbuf_grow(sbuf_c* self, u32 capacity)
{
    sbuf_head_s* head = sbuf__head(*self);
    if (capacity <= head->capacity) {
        // capacity is enough, no need to grow
        return Error.ok;
    }
    return sbuf__grow_buffer(self, capacity);
}

Exception
sbuf_append_c(sbuf_c* self, char* s)
{
    uassert(self != NULL);
    if (s == NULL) {
        return Error.argument;
    }

    sbuf_head_s* head = sbuf__head(*self);

    u32 length = head->length;
    u32 capacity = head->capacity;
    uassert(capacity > 0);


    while (*s != '\0') {

        // Try resize
        if (length > capacity - 1) {
            except(err, sbuf__grow_buffer(self, length + 1))
            {
                return err;
            }
        }

        (*self)[length] = *s;
        length++;
        s++;
    }
    // always null terminate
    (*self)[length] = '\0';

    // re-fetch head in case of realloc
    head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
    head->length = length;

    return Error.ok;
}
Exception
sbuf_replace(sbuf_c* self, const str_c oldstr, const str_c newstr)
{
    uassert(self != NULL);
    uassert(oldstr.buf != newstr.buf && "old and new overlap");
    uassert(*self != newstr.buf && "self and new overlap");
    uassert(*self != oldstr.buf && "self and old overlap");

    sbuf_head_s* head = sbuf__head(*self);

    if (unlikely(!str.is_valid(oldstr) || !str.is_valid(newstr) || oldstr.len == 0)) {
        return Error.argument;
    }

    str_c s = str.cbuf(*self, head->length);

    if (unlikely(s.len == 0)) {
        return Error.ok;
    }

    u32 capacity = head->capacity;

    ssize_t idx = -1;
    while ((idx = str.find(s, oldstr, idx + 1, 0)) != -1) {
        // pointer to start of the found `old`

        char* f = &((*self)[idx]);

        if (oldstr.len == newstr.len) {
            // Tokens exact match just replace
            memcpy(f, newstr.buf, newstr.len);
        } else if (newstr.len < oldstr.len) {
            // Move remainder of a string to fill the gap
            memcpy(f, newstr.buf, newstr.len);
            memmove(f + newstr.len, f + oldstr.len, s.len - idx - oldstr.len);
            s.len -= (oldstr.len - newstr.len);
            if (newstr.len == 0) {
                // NOTE: Edge case: replacing all by empty string, reset index again
                idx--;
            }
        } else {
            // Try resize
            if (unlikely(s.len + (newstr.len - oldstr.len) > capacity - 1)) {
                except(err, sbuf__grow_buffer(self, s.len + (newstr.len - oldstr.len)))
                {
                    return err;
                }
                // re-fetch head in case of realloc
                head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
                s.buf = *self;
                f = &((*self)[idx]);
            }
            // Move exceeding string to avoid overwriting
            memmove(f + newstr.len, f + oldstr.len, s.len - idx - oldstr.len + 1);
            memcpy(f, newstr.buf, newstr.len);
            s.len += (newstr.len - oldstr.len);
        }
    }

    head->length = s.len;
    // always null terminate
    (*self)[s.len] = '\0';

    return Error.ok;
}

Exception
sbuf_append(sbuf_c* self, str_c s)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    if (s.buf == NULL) {
        return Error.argument;
    }

    if (s.len == 0) {
        return Error.ok;
    }

    uassert(*self != s.buf && "buffer overlap");

    u64 length = head->length;
    u64 capacity = head->capacity;

    // Try resize
    if (length + s.len > capacity - 1) {
        except(err, sbuf__grow_buffer(self, length + s.len))
        {
            return err;
        }
    }
    memcpy((*self + length), s.buf, s.len);
    length += s.len;

    // always null terminate
    (*self)[length] = '\0';

    // re-fetch head in case of realloc
    head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
    head->length = length;

    return Error.ok;
}

void
sbuf_clear(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    head->length = 0;
    (*self)[head->length] = '\0';
}

u32
sbuf_len(const sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    return head->length;
}

u32
sbuf_capacity(const sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    return head->capacity;
}

sbuf_c
sbuf_destroy(sbuf_c* self)
{
    uassert(self != NULL);

    if (*self != NULL) {
        sbuf_head_s* head = sbuf__head(*self);

        // NOTE: null-terminate string to avoid future usage,
        // it will appear as empty string if references anywhere else
        ((char*)head)[0] = '\0';
        (*self)[0] = '\0';
        *self = NULL;

        if (head->allocator != NULL) {
            // allocator is NULL for static sbuf
            head->allocator->free(head);
        }
    }
    return NULL;
}

static char*
sbuf__sprintf_callback(const char* buf, void* user, int len)
{
    struct _sbuf__sprintf_ctx* ctx = (struct _sbuf__sprintf_ctx*)user;
    sbuf_c sbuf = ((char*)ctx->head + sizeof(sbuf_head_s));
    uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");

    if (unlikely(ctx->err != EOK)) {
        return ctx->tmp;
    }

    uassert((buf != ctx->buf) || (sbuf + ctx->length + len <= sbuf + ctx->count && "out of bounds"));

    if (unlikely(ctx->length + len > ctx->count)) {
        bool buf_is_tmp = buf != ctx->buf;

        if (len < 0 || ctx->length + len > INT32_MAX) {
            ctx->err = Error.integrity;
            return ctx->tmp;
        }

        // NOTE: sbuf likely changed after realloc
        except(err, sbuf__grow_buffer(&sbuf, ctx->length + len + 1))
        {
            ctx->err = err;
            return ctx->tmp;
        }
        // re-fetch head in case of realloc
        ctx->head = (sbuf_head_s*)(sbuf - sizeof(sbuf_head_s));
        uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");

        ctx->buf = sbuf + ctx->head->length;
        ctx->count = ctx->head->capacity;

        uassert(ctx->count >= ctx->length);

        if (!buf_is_tmp) {
            // If we use the same buffer for sprintf() prevent use-after-free issue
            // if ctx->buf was reallocated to  another pointer
            buf = ctx->buf;
        }
    }

    ctx->length += len;
    ctx->head->length += len;

    if (len > 0) {
        if (buf != ctx->buf) {
            // NOTE: copy data only if previously tmp buffer used
            memcpy(ctx->buf, buf, len);
        }
        ctx->buf += len;
    }

    // NOTE: if string buffer is small, uses stack-based tmp buffer (about 512bytes)
    // // and then copy into sbuf_s buffer when ready. When string grows, uses heap allocated
    // // sbuf_c directly without copy
    return ((ctx->count - ctx->length) >= STB_SPRINTF_MIN) ? ctx->buf : ctx->tmp;
}

Exception
sbuf_sprintf(sbuf_c* self, const char* format, ...)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    // utracef("head s: %s, len: %d\n", *self, head->length);

    struct _sbuf__sprintf_ctx ctx = {
        .head = head,
        .err = EOK,
        .buf = *self + head->length,
        .length = head->length,
        .count = head->capacity,
    };

    va_list va;
    va_start(va, format);

    STB_SPRINTF_DECORATE(vsprintfcb)
    (sbuf__sprintf_callback, &ctx, sbuf__sprintf_callback(NULL, &ctx, 0), format, va);

    // re-fetch self in case of realloc in sbuf__sprintf_callback
    *self = ((char*)ctx.head + sizeof(sbuf_head_s));

    // always null terminate
    (*self)[ctx.head->length] = '\0';
    (*self)[ctx.head->capacity] = '\0';

    va_end(va);

    return ctx.err;
}

str_c
sbuf_tostr(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    return (str_c){
        .buf = *self,
        .len = head->length,
    };
}


const struct __module__sbuf sbuf = {
    // Autogenerated by CEX
    // clang-format off
    .create = sbuf_create,
    .create_static = sbuf_create_static,
    .grow = sbuf_grow,
    .append_c = sbuf_append_c,
    .replace = sbuf_replace,
    .append = sbuf_append,
    .clear = sbuf_clear,
    .len = sbuf_len,
    .capacity = sbuf_capacity,
    .destroy = sbuf_destroy,
    .sprintf = sbuf_sprintf,
    .tostr = sbuf_tostr,
    // clang-format on
};

/*
*                   str.c
*/
#include <ctype.h>


static inline bool
str__isvalid(const str_c* s)
{
    return s->buf != NULL;
}

static inline ssize_t
str__index(str_c* s, const char* c, u8 clen)
{
    ssize_t result = -1;

    if (!str__isvalid(s)) {
        return -1;
    }

    u8 split_by_idx[UINT8_MAX] = { 0 };
    for (u8 i = 0; i < clen; i++) {
        split_by_idx[(u8)c[i]] = 1;
    }

    for (size_t i = 0; i < s->len; i++) {
        if (split_by_idx[(u8)s->buf[i]]) {
            result = i;
            break;
        }
    }

    return result;
}

str_c
str_cstr(const char* ccharptr)
{
    if (unlikely(ccharptr == NULL)) {
        return (str_c){ 0 };
    }

    return (str_c){
        .buf = (char*)ccharptr,
        .len = strlen(ccharptr),
    };
}

str_c
str_cbuf(char* s, size_t length)
{
    if (unlikely(s == NULL)) {
        return (str_c){ 0 };
    }

    return (str_c){
        .buf = s,
        .len = strnlen(s, length),
    };
}

str_c
str_sub(str_c s, ssize_t start, ssize_t end)
{
    if (unlikely(s.len == 0)) {
        if (start == 0 && end == 0) {
            return s;
        } else {
            return (str_c){ 0 };
        }
    }

    if (start < 0) {
        // negative start uses offset from the end (-1 - last char)
        start += s.len;
    }

    if (end == 0) {
        // if end is zero, uses remainder
        end = s.len;
    } else if (end < 0) {
        // negative end, uses offset from the end
        end += s.len;
    }

    if (unlikely((size_t)start >= s.len || end > (ssize_t)s.len || end < start)) {
        return (str_c){ 0 };
    }

    return (str_c){
        .buf = s.buf + start,
        .len = (size_t)(end - start),
    };
}

Exception
str_copy(str_c s, char* dest, size_t destlen)
{
    uassert(dest != s.buf && "self copy is not allowed");

    if (unlikely(dest == NULL || destlen == 0)) {
        return Error.argument;
    }

    // If we fail next, it still writes empty string
    dest[0] = '\0';

    if (unlikely(!str__isvalid(&s))) {
        return Error.sanity_check;
    }

    size_t slen = s.len;
    size_t len_to_copy = MIN(destlen - 1, slen);
    memcpy(dest, s.buf, len_to_copy);

    // Null terminate end of buffer
    dest[len_to_copy] = '\0';

    if (unlikely(slen >= destlen)) {
        return Error.overflow;
    }

    return Error.ok;
}

size_t
str_len(str_c s)
{
    return s.len;
}

bool
str_is_valid(str_c s)
{
    return str__isvalid(&s);
}

char*
str_iter(str_c s, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) == 8, "ctx alignment mismatch");

    if (unlikely(iterator->val == NULL)) {
        // Iterator is not initialized set to 1st item
        if (unlikely(!str__isvalid(&s) || s.len == 0)) {
            return NULL;
        }
        iterator->val = s.buf;
        iterator->idx.i = 0;
        return &s.buf[0];
    }

    if (ctx->cursor >= s.len - 1) {
        return NULL;
    }

    ctx->cursor++;
    iterator->idx.i++;
    iterator->val = &s.buf[iterator->idx.i];
    return iterator->val;
}

ssize_t
str_find(str_c s, str_c needle, size_t start, size_t end)
{
    if (needle.len == 0 || needle.len > s.len) {
        return -1;
    }
    if (start >= s.len) {
        return -1;
    }

    if (end == 0 || end > s.len) {
        end = s.len;
    }

    for (size_t i = start; i < end - needle.len + 1; i++) {
        // check the 1st letter
        if (s.buf[i] == needle.buf[0]) {
            // 1 char
            if (needle.len == 1) {
                return i;
            }
            // check whole word
            if (memcmp(&s.buf[i], needle.buf, needle.len) == 0) {
                return i;
            }
        }
    }

    return -1;
}

ssize_t
str_rfind(str_c s, str_c needle, size_t start, size_t end)
{
    if (needle.len == 0 || needle.len > s.len) {
        return -1;
    }
    if (start >= s.len) {
        return -1;
    }

    if (end == 0 || end > s.len) {
        end = s.len;
    }

    for (size_t i = end - needle.len + 1; i-- > start;) {
        // check the 1st letter
        if (s.buf[i] == needle.buf[0]) {
            // 1 char
            if (needle.len == 1) {
                return i;
            }
            // check whole word
            if (memcmp(&s.buf[i], needle.buf, needle.len) == 0) {
                return i;
            }
        }
    }

    return -1;
}

bool
str_contains(str_c s, str_c needle)
{
    return str_find(s, needle, 0, 0) != -1;
}

bool
str_starts_with(str_c s, str_c needle)
{
    return str_find(s, needle, 0, needle.len) != -1;
}

bool
str_ends_with(str_c s, str_c needle)
{
    if (needle.len > s.len) {
        return false;
    }

    return str_find(s, needle, s.len - needle.len, 0) != -1;
}

str_c
str_remove_prefix(str_c s, str_c prefix)
{
    ssize_t idx = str_find(s, prefix, 0, prefix.len);
    if (idx == -1) {
        return s;
    }

    return (str_c){
        .buf = s.buf + prefix.len,
        .len = s.len - prefix.len,
    };
}

str_c
str_remove_suffix(str_c s, str_c suffix)
{
    if (suffix.len > s.len) {
        return s;
    }

    ssize_t idx = str_find(s, suffix, s.len - suffix.len, 0);

    if (idx == -1) {
        return s;
    }

    return (str_c){
        .buf = s.buf,
        .len = s.len - suffix.len,
    };
}


static inline void
str__strip_left(str_c* s)
{
    char* cend = s->buf + s->len;

    while (s->buf < cend) {
        switch (*s->buf) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->buf++;
                s->len--;
                break;
            default:
                return;
        }
    }
}

static inline void
str__strip_right(str_c* s)
{
    while (s->len > 0) {
        switch (s->buf[s->len - 1]) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->len--;
                break;
            default:
                return;
        }
    }
}

str_c
str_lstrip(str_c s)
{
    if (s.buf == NULL) {
        return (str_c){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_c){
            .buf = "",
            .len = 0,
        };
    }

    str_c result = (str_c){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_left(&result);
    return result;
}

str_c
str_rstrip(str_c s)
{
    if (s.buf == NULL) {
        return (str_c){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_c){
            .buf = "",
            .len = 0,
        };
    }

    str_c result = (str_c){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_right(&result);
    return result;
}

str_c
str_strip(str_c s)
{
    if (s.buf == NULL) {
        return (str_c){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_c){
            .buf = "",
            .len = 0,
        };
    }

    str_c result = (str_c){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_left(&result);
    str__strip_right(&result);
    return result;
}

int
str_cmp(str_c self, str_c other)
{
    if (unlikely(self.buf == NULL)) {
        if (other.buf == NULL) {
            return 0;
        } else {
            return -1;
        }
    }

    if (unlikely(self.len == 0)) {
        if (other.buf == NULL) {
            return 1;
        } else if (other.len == 0) {
            return 0;
        } else {
            return -other.buf[0];
        }
    }

    size_t min_len = MIN(self.len, other.len);
    int cmp = memcmp(self.buf, other.buf, min_len);

    if (cmp == 0 && self.len != other.len) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

int
str_cmpi(str_c self, str_c other)
{
    if (unlikely(self.buf == NULL)) {
        if (other.buf == NULL) {
            return 0;
        } else {
            return -1;
        }
    }

    if (unlikely(self.len == 0)) {
        if (other.buf == NULL) {
            return 1;
        } else if (other.len == 0) {
            return 0;
        } else {
            return -other.buf[0];
        }
    }

    size_t min_len = MIN(self.len, other.len);

    int cmp = 0;
    char* s = self.buf;
    char* o = other.buf;
    for (size_t i = 0; i < min_len; i++) {
        cmp = tolower(*s) - tolower(*o);
        s++;
        o++;
    }

    if (cmp == 0 && self.len != other.len) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

str_c*
str_iter_split(str_c s, const char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        size_t cursor;
        size_t split_by_len;
        str_c str;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) <= alignof(size_t), "cex_iterator_s _ctx misalign");

    if (unlikely(iterator->val == NULL)) {
        // First run handling
        if (unlikely(!str__isvalid(&s))) {
            return NULL;
        }
        ctx->split_by_len = strlen(split_by);

        if (ctx->split_by_len == 0) {
            return NULL;
        }
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        ssize_t idx = str__index(&s, split_by, ctx->split_by_len);
        if (idx < 0) {
            idx = s.len;
        }
        ctx->cursor = idx;
        ctx->str = str.sub(s, 0, idx);

        iterator->val = &ctx->str;
        iterator->idx.i = 0;
        return iterator->val;
    } else {
        uassert(iterator->val == &ctx->str);

        if (ctx->cursor >= s.len) {
            return NULL; // reached the end stops
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == s.len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            ctx->str = str.cstr("");
            return iterator->val;
        }

        // Get remaining string after prev split_by char
        str_c tok = str.sub(s, ctx->cursor, 0);
        ssize_t idx = str__index(&tok, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->str = tok;
            ctx->cursor = s.len;
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->str = str.sub(tok, 0, idx);
            ctx->cursor += idx;
        }

        return iterator->val;
    }
}

const struct __module__str str = {
    // Autogenerated by CEX
    // clang-format off
    .cstr = str_cstr,
    .cbuf = str_cbuf,
    .sub = str_sub,
    .copy = str_copy,
    .len = str_len,
    .is_valid = str_is_valid,
    .iter = str_iter,
    .find = str_find,
    .rfind = str_rfind,
    .contains = str_contains,
    .starts_with = str_starts_with,
    .ends_with = str_ends_with,
    .remove_prefix = str_remove_prefix,
    .remove_suffix = str_remove_suffix,
    .lstrip = str_lstrip,
    .rstrip = str_rstrip,
    .strip = str_strip,
    .cmp = str_cmp,
    .cmpi = str_cmpi,
    .iter_split = str_iter_split,
    // clang-format on
};