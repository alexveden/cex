#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_ds)

#if defined(cex$enable_minimal) && !defined(cex$enable_mem)
#error "CEX ds namespace depends on `#define cex$enable_mem`"
#endif

#include "cex_base.h"

/**

## Dynamic array

Generic type-safe dynamic array backed by a heap header.

`arr$(T)` is just `T*` — zero overhead, fully C-array compatible with no hidden
pointer or fat-pointer indirection. The runtime header
(`_cexds__array_header`) lives *before* the user pointer at a negative offset.

### Principles

1. **Zero overhead** — `arr$(T)` = `T*`. Pass them to any function expecting a C pointer+length.
2. **Allocator-backed** — Every array carries its `IAllocator`. Passed once at `arr$new`.
3. **O(1) amortized growth** — Capacity doubles when full (minimum 16).
4. **Unified length** — `arr$len()` works on dynamic `arr$`, static C arrays, and hashmaps.
5. **Unified iteration** — `for$each` / `for$eachp` iterate any array (arr$, static, pointer+len, hm$).
6. **Debug integrity** — Each array header has a magic number (`_CEXDS_ARR_MAGIC = 0xC001DAAD`).
   Validate an explicit handle with `arr$validate()` / `hm$validate()`; wrong magic or a NULL
   handle returns an `Exception` (`Error.integrity` / `Error.memory`).
7. **ASAN-aware** — The 8-byte poison area after the header is marked poisoned so ASAN catches
   underflow reads/writes.
8. **Allocation-failure aware** — mutating macros return a pointer to the item slot or `NULL` on
   allocation failure; a grow-OOM frees the array and sets its variable to `NULL`. Most macros
   tolerate a `NULL` array; the accessors `arr$last()`, `arr$at()`, `arr$pop()` assert on it.
   Note: the default heap allocator panics on real OOM (`cex$platform_mem_panic`); these `NULL`
   returns are for synthetic `test$alloc` OOM, custom allocators, or an opt-out build.

### Examples

- Creating array
```c
// heap allocator — must call arr$free() later, or use mem$scope() for automatic cleanup
arr$(i32) array = arr$new(array, mem$);

arr$pushm(array, 1, 2, 3); // multiple elements at once (compound-literal temp array)
arr$push(array, 4);        // single element

for$each (it, array) {
    io.printf("el=%d\n", it);
}

arr$free(array); // safe to call on NULL (no-op)
```

- Array of structs

```c
// .capacity is optional, defaults to 16; pre-allocate to avoid early reallocs
arr$(my_struct) array = arr$new(array, mem$, .capacity = 128);

// structs are copied by value into the array — the source may be stack-allocated
arr$push(array, ((my_struct){ .key = 20, .my_string = "hello" }));

arr$free(array);
```

*/
#define __arr$

///////////////
//
// Everything below here is implementation details
//


// clang-format off
struct _cexds__hm_new_kwargs_s;
struct _cexds__arr_new_kwargs_s;
struct _cexds__hash_index;
enum _CexDsKeyType_e
{
    _CexDsKeyType__generic,
    _CexDsKeyType__charptr,
    _CexDsKeyType__charbuf,
    _CexDsKeyType__cexstr,
};
extern void* _cexds__arrgrowf(void* a, usize elemsize, usize addlen, usize min_cap, u16 el_align, IAllocator allc);
extern void _cexds__arrfreef(void* a);
extern Exception _cexds__arr_integrity(const void* arr, usize magic_num);
extern usize _cexds__arr_len(const void* arr);
extern void _cexds__hmfree_func(void* p, usize elemsize, usize keyoffset);
extern void _cexds__hmfree_keys_func(void* a, usize elemsize, usize keyoffset);
extern void _cexds__hmclear_func(struct _cexds__hash_index* t, struct _cexds__hash_index* old_table);
extern void* _cexds__hminit(usize elemsize, IAllocator allc, enum _CexDsKeyType_e key_type, u16 el_align, struct _cexds__hm_new_kwargs_s* kwargs);
extern void* _cexds__hmget_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset);
extern void* _cexds__hmput_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset, void* full_elem, void* result);
extern bool _cexds__hmdel_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset);
// clang-format on

#define _CEXDS_ARR_MAGIC 0xC001DAAD
#define _CEXDS_HM_MAGIC 0xF001C001


// cexds array alignment
// v malloc'd pointer                v-element 1
// |..... <_cexds__array_header>|====!====!====
//      ^ padding      ^cap^len ^         ^-element 2
//                              ^-- arr$ user space pointer (element 0)
//
typedef struct
{
    struct _cexds__hash_index* _hash_table;
    IAllocator allocator;
    u32 magic_num;
    u16 allocator_scope_depth;
    u16 el_align;
    usize capacity;
    usize length; // This MUST BE LAST before __poison_area
    u8 __poison_area[8];
} _cexds__array_header;
static_assert(alignof(_cexds__array_header) == alignof(usize), "align");
static_assert(
    sizeof(usize) == 8 ? sizeof(_cexds__array_header) == 48 : sizeof(_cexds__array_header) == 32,
    "size for x64 is 48 / for x32 is 32"
);

#define _cexds__header(t) ((_cexds__array_header*)(((char*)(t)) - sizeof(_cexds__array_header)))

/// Validates an `arr$` handle: `Error.memory` if NULL, `Error.integrity` on bad magic, `EOK` otherwise.
#define arr$validate(a) _cexds__arr_integrity((a), _CEXDS_ARR_MAGIC)

/// Declares a dynamic array variable. `arr$(int) myarr` = `int* myarr`. Zero overhead, fully C-compatible.
#define arr$(T) T*

struct _cexds__arr_new_kwargs_s
{
    usize capacity;
};
/// Initializes a dynamic array. Pass the array variable, an `IAllocator`, and optional `.capacity = N`. Returns the new pointer on success, NULL on allocation failure.
#define arr$new(a, allocator, kwargs...)                                                           \
    ({                                                                                             \
        static_assert(_Alignof(typeof(*a)) <= 64, "array item alignment too high");                \
        uassert(allocator != NULL);                                                                \
        struct _cexds__arr_new_kwargs_s _kwargs = { kwargs };                                      \
        (a) = (typeof(*a)*)_cexds__arrgrowf(                                                       \
            NULL,                                                                                  \
            sizeof(*a),                                                                            \
            _kwargs.capacity,                                                                      \
            0,                                                                                     \
            alignof(typeof(*a)),                                                                   \
            allocator                                                                              \
        );                                                                                         \
    })

/// Frees the array memory and sets the pointer to NULL. Safe on NULL arrays (no-op).
#define arr$free(a)                                                                                \
    ({                                                                                             \
        _cexds__arrfreef((a));                                                                     \
        (a) = NULL;                                                                                \
    })

/// Resizes the array capacity to at least `n` elements. No-op if current capacity >= n. Returns the array pointer, or NULL on allocation failure / NULL array.
#define arr$setcap(a, n) ((a) != NULL ? arr$grow(a, 0, n) : NULL)

/// Clears the array (sets length to 0). Does NOT free or shrink memory — use `arr$free` for that. NULL array is a no-op.
#define arr$clear(a)                                                                               \
    ({                                                                                             \
        if ((a) != NULL) {                                                                         \
            _cexds__header(a)->length = 0;                                                         \
        }                                                                                          \
    })

/// Returns the current allocated capacity (in elements). Returns 0 if array is NULL.
#define arr$cap(a) ((a) ? (_cexds__header(a)->capacity) : 0)

/// Deletes element at index `i` by shifting subsequent elements left. Order preserved. O(n). NULL array is a no-op (returns 0).
#define arr$del(a, i)                                                                              \
    ({                                                                                             \
        usize _cexds__len = 0;                                                                     \
        if ((a) != NULL) {                                                                         \
            _cexds__len = _cexds__header(a)->length;                                               \
            uassert_always((usize)i < _cexds__len && "out of bounds");                             \
            if ((usize)i + 1 < _cexds__len) {                                                      \
                memmove(&(a)[i], &(a)[(i) + 1], sizeof *(a) * (_cexds__len - 1 - (usize)i));       \
            }                                                                                      \
            _cexds__header(a)->length = _cexds__len - 1;                                           \
        }                                                                                          \
        _cexds__len;                                                                               \
    })

/// Deletes element at index `i` by swapping with the last element. Order NOT preserved, but O(1). NULL array is a no-op.
#define arr$delswap(a, i)                                                                          \
    ({                                                                                             \
        if ((a) != NULL) {                                                                         \
            uassert((usize)i < _cexds__header(a)->length && "out of bounds");                      \
            (a)[i] = arr$last(a);                                                                  \
            _cexds__header(a)->length -= 1;                                                        \
        }                                                                                          \
    })

/// Returns the last element (by value). Asserts that the array is not NULL or empty.
#define arr$last(a)                                                                                \
    ({                                                                                             \
        uassert((a) != NULL && "NULL array");                                                      \
        uassert(_cexds__header(a)->length > 0 && "empty array");                                   \
        (a)[_cexds__header(a)->length - 1];                                                        \
    })

/// Returns element at index `i` (by value) with bounds checking via `uassert()`. Also works on `hm$`. Asserts on NULL array.
#define arr$at(a, i)                                                                               \
    ({                                                                                             \
        uassert((a) != NULL && "NULL array");                                                      \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        (a)[i];                                                                                    \
    })

/// Pops and returns the last element (by value). Asserts that the array is not NULL or empty.
#define arr$pop(a)                                                                                 \
    ({                                                                                             \
        uassert((a) != NULL && "NULL array");                                                      \
        uassert(_cexds__header(a)->length > 0 && "empty array");                                   \
        if (_cexds__header(a)->length > 0) { _cexds__header(a)->length--; }                        \
        (a)[_cexds__header(a)->length];                                                            \
    })

/// Appends a single element to the end. Automatically grows capacity if needed. Returns pointer to the new slot, or NULL on allocation failure / NULL array.
#define arr$push(a, value...)                                                                      \
    ({                                                                                             \
        typeof(*a)* _cexds__ret = NULL;                                                            \
        if (arr$grow_check(a, 1)) {                                                                \
            (a)[_cexds__header(a)->length++] = (value);                                            \
            _cexds__ret = &(a)[_cexds__header(a)->length - 1];                                     \
        }                                                                                          \
        _cexds__ret;                                                                               \
    })

/// Appends multiple elements at once: `arr$pushm(arr, 1, 2, 3)`. Uses a compound-literal temporary array.
#define arr$pushm(a, items...)                                                                     \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        typeof(*a) _args[] = { items };                                                            \
        static_assert(sizeof(_args) > 0, "You must pass at least one item");                       \
        arr$pusha(a, _args, arr$len(_args));                                                       \
        /* NOLINTEND */                                                                            \
    })

/// Appends all elements from `array` (dynamic, static, or pointer+len) into `a`. `array_len` is optional for pointer+len. Returns pointer to the first appended slot, or NULL on allocation failure / NULL array / empty source.
#define arr$pusha(a, array, array_len...)                                                          \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        typeof(*a)* _cexds__ret = NULL;                                                            \
        if ((a) != NULL) {                                                                         \
            uassert(array != NULL && "arr$pusha: array is NULL");                                  \
            usize _arr_len_va[] = { array_len };                                                   \
            usize arr_len = (sizeof(_arr_len_va) > 0) ? _arr_len_va[0] : arr$len(array);           \
            uassert(arr_len < mem$MAX && "negative length or overflow");                       \
            if (arr_len > 0 && arr$grow_check(a, arr_len)) {                                       \
                typeof(*a)* _cexds__first = &(a)[_cexds__header(a)->length];                       \
                for (usize i = 0; i < arr_len; i++) {                                              \
                    (a)[_cexds__header(a)->length++] = ((array)[i]);                               \
                }                                                                                  \
                _cexds__ret = _cexds__first;                                                       \
            }                                                                                      \
        }                                                                                          \
        /* NOLINTEND */                                                                            \
        _cexds__ret;                                                                               \
    })

/// Sorts the array in-place using `qsort()` with the provided comparator. NULL array is a no-op.
#define arr$sort(a, qsort_cmp)                                                                     \
    ({                                                                                             \
        if ((a) != NULL) {                                                                         \
            qsort((a), arr$len(a), sizeof(*a), qsort_cmp);                                         \
        }                                                                                          \
    })


/// Inserts element at index `i`, shifting subsequent elements right. Order preserved. O(n). Returns pointer to the inserted slot, or NULL on allocation failure / NULL array.
#define arr$ins(a, i, value...)                                                                    \
    ({                                                                                             \
        typeof(*a)* _cexds__ret = NULL;                                                            \
        if (arr$grow_check(a, 1)) {                                                                \
            usize _cexds__len = _cexds__header(a)->length;                                         \
            uassert_always((usize)i < _cexds__len + 1 && "i out of bounds");                       \
            if ((usize)i < _cexds__len) {                                                          \
                memmove(&(a)[(i) + 1], &(a)[i], sizeof(*(a)) * (_cexds__len - (usize)i));          \
            }                                                                                      \
            _cexds__header(a)->length = _cexds__len + 1;                                           \
            (a)[i] = (value);                                                                      \
            _cexds__ret = &(a)[i];                                                                 \
        }                                                                                          \
        _cexds__ret;                                                                               \
    })

/// Checks if array has room for `add_extra` elements, growing if needed. Returns false on allocation failure, length overflow, or NULL array.
#define arr$grow_check(a, add_extra)                                                               \
    ({                                                                                             \
        bool _cexds__ok = false;                                                                   \
        if ((a) != NULL) {                                                                         \
            usize _cexds__add = (add_extra);                                                       \
            if (_cexds__add > (usize)-1 - _cexds__header(a)->length) {                             \
                _cexds__ok = false;                                                                \
            } else if (_cexds__header(a)->length + _cexds__add > _cexds__header(a)->capacity) {    \
                (void)arr$grow(a, _cexds__add, 0);                                                 \
                _cexds__ok = ((a) != NULL);                                                        \
            } else {                                                                               \
                _cexds__ok = true;                                                                 \
            }                                                                                      \
        }                                                                                          \
        _cexds__ok;                                                                                \
    })

/// Grows array so it can hold at least `add_len` more elements, with the absolute minimum of `min_cap`. Returns the array pointer, or NULL on allocation failure / NULL array.
#define arr$grow(a, add_len, min_cap)                                                              \
    ((a) != NULL                                                                                   \
         ? ((a) = _cexds__arrgrowf((a), sizeof *(a), (add_len), (min_cap), alignof(typeof(*a)),    \
                                   NULL),                                                          \
            (a))                                                                                   \
         : NULL)


#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 12)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsizeof-pointer-div"

#    define arr$len(arr)                                                                           \
        ({                                                                                         \
            __builtin_types_compatible_p(                                                          \
                typeof(arr),                                                                       \
                typeof(&(arr)[0])                                                                  \
            )                          /* check if array or ptr */                                 \
                ? _cexds__arr_len(arr) /* some pointer or arr$ */                                  \
                : (                                                                                \
                      sizeof(arr) / sizeof((arr)[0]) /* static array[] */                          \
                  );                                                                               \
        })
#else
/// Returns the number of elements. Works on `arr$`, `hm$`, static C arrays, and pointer+length slices.
#    define arr$len(arr)                                                                           \
        ({                                                                                         \
            _Pragma("GCC diagnostic push");                                                        \
            /* NOTE: temporary disable syntax error to support both static array length and        \
             * arr$(T) */                                                                          \
            _Pragma("GCC diagnostic ignored \"-Wsizeof-pointer-div\"");                            \
            /* NOLINTBEGIN */                                                                      \
            usize __cex_array_len = __builtin_types_compatible_p(                                                          \
                typeof(arr),                                                                       \
                typeof(&(arr)[0])                                                                  \
            )                          /* check if array or ptr */                                 \
                ? _cexds__arr_len(arr) /* some pointer or arr$ */                                  \
                : (                                                                                \
                      sizeof(arr) / sizeof((arr)[0]) /* static array[] */                          \
                  );                                                                               \
            /* NOLINTEND */                                                                        \
            _Pragma("GCC diagnostic pop");                                                         \
            __cex_array_len;                                                                       \
        })
#endif

static inline void*
_cex__get_buf_addr(void* a)
{
    return (a != NULL) ? &((char*)a)[0] : NULL;
}


/*
 *                  ARRAYS ITERATORS INTERFACE
 */

/**

## Iteration

Unified array / hashmap / slice iteration framework.

`for$` macros provide a single syntax for looping over any iterable data in CEX:

| Variant                              | Copies elements?          | Use case                                         |
|--------------------------------------|---------------------------|--------------------------------------------------|
| `for$each(it, array, len?)`          | By value (≤ 64 B)         | Small types / copy iteration / slices            |
| `for$eachp(it, array, len?)`         | By pointer (no copy)      | Large structs / avoid copy overhead / slices     |
| `for$iter(T, it, iter_func)`         | Custom (cex_iterator_s)   | Tokenizers, generators, splitters                |

`for$each` and `for$eachp` work identically on `arr$`, `hm$`, static C arrays, and pointer+length slices.

### Examples

- Using for$ as unified array iterator
```c
arr$(int) array = arr$new(array, mem$);
arr$pushm(array, 1, 2, 3);

// for$each copies elements by value (up to CEX_FOREACH_MAX_COPY_SIZE bytes)
for$each (it, array) {
    io.printf("el=%d\n", it);
}

// for$eachp provides a pointer — no copy, prefer for large structs
for$eachp (it, array) {
    // TIP: derive index from pointer subtraction
    usize i = (usize)(it - array);
    io.printf("el[%zu]=%d\n", i, *it);
}

arr$free(array);
```

- Custom iterator (tokens, generators, splitters)

```c
// for$iter uses a custom iterator function and cex_iterator_s
// NOTE: str_s is passed by value (stack-allocated slice)
str_s s = str.sstr("123,456");
for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {
    // gotcha: it.val is a non-null-terminated slice — use %S, not %s
    io.printf("it.val = %S\n", it.val);
}
```

*/
#define __for$

/**
 
Generic iterator state (≤ 64 bytes). Used by `for$iter()` and custom iterator functions.

Fields:
- `idx.i` — integer index (for array iteration)
- `idx.skey` — string key (for char*-keyed iteration)
- `idx.pkey` — opaque pointer key
- `_ctx[47]` — opaque per-iterator state
- `stopped` — set to 1 by the iterator function when exhausted
- `initialized` — set to 1 after first call
*/
typedef struct
{
    struct
    {
        union
        {
            usize i;
            char* skey;
            void* pkey;
        };
    } idx;
    char _ctx[47];
    u8 stopped;
    u8 initialized;
} cex_iterator_s;
static_assert(sizeof(usize) == sizeof(void*), "usize expected as sizeof ptr");
static_assert(alignof(usize) == alignof(void*), "alignof pointer != alignof usize");
static_assert(alignof(cex_iterator_s) == alignof(void*), "alignof");
static_assert(sizeof(cex_iterator_s) <= 64, "cex size");

/**
 
Iterates via a custom iterator function.

The iterator function receives a `cex_iterator_s*` and returns the next value
(or a zero-initialized sentinel when iteration is complete — the `.stopped`
field is set to 1).

Iterator function signature:
```c
MyType next_value(MyType array[], usize len, cex_iterator_s* iter);
```

Usage:
```c
for$iter(u32, it, array_iterator(arr2, arr$len(arr2), &it.iterator))
```
*/
#define for$iter(it_val_type, it, iter_func)                                                       \
    struct cex$tmpname(__cex_iter_)                                                                \
    {                                                                                              \
        it_val_type val;                                                                           \
        union /* NOTE:  iterator above and this struct shadow each other */                        \
        {                                                                                          \
            cex_iterator_s iterator;                                                               \
            struct                                                                                 \
            {                                                                                      \
                union                                                                              \
                {                                                                                  \
                    usize i;                                                                       \
                    char* skey;                                                                    \
                    void* pkey;                                                                    \
                };                                                                                 \
            } idx;                                                                                 \
        };                                                                                         \
    };                                                                                             \
                                                                                                   \
    for (struct cex$tmpname(__cex_iter_) it = { .val = (iter_func) }; !it.iterator.stopped;        \
         it.val = (iter_func))


#ifndef CEX_FOREACH_MAX_COPY_SIZE
/// Max element size (bytes) copied by for$each(), default 64
#define CEX_FOREACH_MAX_COPY_SIZE 64
#endif

/// Iterates over arrays by **value** (copies each element into `it`). Works on arr$, hm$, static arrays, and pointer+len. Capped at `CEX_FOREACH_MAX_COPY_SIZE` (64 B) per element.
#define for$each(it, array, array_len...)                                                          \
    /* NOLINTBEGIN*/                                                                               \
    static_assert(sizeof(typeof((array)[0])) <= CEX_FOREACH_MAX_COPY_SIZE,                          \
        "for$each() copies elements by value, but element type is too large "                      \
        "(sizeof(element) > CEX_FOREACH_MAX_COPY_SIZE). "                                          \
        "Use for$eachp() for pointer-based iteration, "                                            \
        "or increase CEX_FOREACH_MAX_COPY_SIZE.");                                                 \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    typeof((array)[0])* cex$tmpname(arr_arrp) = _cex__get_buf_addr(array);                         \
    usize cex$tmpname(arr_index) = 0;                                                              \
    for (typeof((array)[0]) it = { 0 };                                                            \
         (cex$tmpname(arr_index) < cex$tmpname(arr_length) &&                                      \
          ((it) = cex$tmpname(arr_arrp)[cex$tmpname(arr_index)], 1));                              \
         cex$tmpname(arr_index)++)
    /* NOLINTEND */                                                                                \


/// Iterates over arrays by **pointer** (no copy — `it` is `T*`). Best for large structs. Works on arr$, hm$, static arrays, and pointer+len.
#define for$eachp(it, array, array_len...)                                                         \
    /* NOLINTBEGIN*/                                                                               \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    typeof((array)[0])* cex$tmpname(arr_arrp) = _cex__get_buf_addr(array);                         \
    usize cex$tmpname(arr_index) = 0;                                                              \
    for (typeof((array)[0])* it = cex$tmpname(arr_arrp);                                           \
         cex$tmpname(arr_index) < cex$tmpname(arr_length);                                         \
         cex$tmpname(arr_index)++, it++)
    /* NOLINTEND */                                                                                \

/*
 *                 HASH MAP
 */
/**

## Hashmap

Generic type-safe hashmap backed by open-addressing with quadratic probing.

The hashmap shares the same backing engine as `arr$` (same header, same allocator).
This means every `hm$` is also an `arr$` — you can iterate, index, and take its length
just like a regular dynamic array.

### Key features

1. **Open-addressing** with bucketed hash table (8 slots per cache-line-aligned bucket).
2. **Quadratic probing** with tombstone tracking for efficient deletions.
3. **Key type auto-detection** via `_Generic` — numeric (memcmp), `char*` (strcmp),
   `char[N]` (strcmp), `str_s` (memcmp with length check).
4. **String key modes** — no-copy (default), copy via allocator (`mem$malloc`)
   (`.copy_keys = true`), or copy via arena allocator
   (`.copy_keys = true, .copy_keys_arena_pgsize = NNN`).
5. **Grow / shrink** — table doubles at 75% load factor, halves below 25%,
   tombstones trigger rebuild at ~20%.
6. **Dual index** — `hm$` data array is sorted by insertion order (stable until
   first delete). Hash table entries point into this array, so `arr$len()`,
   `for$each`, and bracket indexing all work transparently.

### Principles

1. `hm$(K,V)` is a struct `{ K key; V value; }*`.
2. `hm$s(S)` treats any struct with a `.key` field as a hashmap record.
3. `arr$len()`, `arr$cap()`, `for$each`, `for$eachp` all work on `hm$` types.
4. Array indexing `smap[i].key` / `smap[i].value` works but order may change after
   calls to `hm$del`.
5. `hm$new` can return `NULL` on allocation failure — always check (or use `uassert`).
6. **Allocation-failure aware** — `hm$set`/`hm$setp`/`hm$sets` return `NULL` on allocation
   failure; every `hm$` macro tolerates a `NULL` hashmap. Note: the default heap allocator panics
   on real OOM (`cex$platform_mem_panic`); these `NULL` returns are for synthetic `test$alloc`
   OOM, custom allocators, or an opt-out build.

### Examples

- Basic usage
```c
hm$(int, int) intmap = hm$new(intmap, mem$);

hm$set(intmap, 15, 7); // replaces the value if the key already exists
hm$set(intmap, 11, 3);

// get by value — returns a default (0 or custom) if key is missing
int v = hm$get(intmap, 9, -1);

// get by pointer — NULL if not found (no copy, direct pointer into storage)
int* vp = hm$getp(intmap, 11);

hm$del(intmap, 100); // deleting a non-existent key is safe (no-op)
hm$clear(intmap);    // clear all entries (does not free the hashmap)

// hm$len and arr$len are equivalent; iteration works like arr$
for$each (it, intmap) {
    io.printf("key=%d, value=%d\n", it.key, it.value);
}

hm$free(intmap);
```

- String keys (copy mode)

```c
// .copy_keys = true duplicates char* keys internally; otherwise the key
// pointer must outlive the hashmap
// .copy_keys_arena_pgsize = 1024 allocates key copies from an internal arena
hm$(char*, int) smap = hm$new(smap, mem$, .copy_keys = true,
                               .copy_keys_arena_pgsize = 1024);

char key[10] = "foo";
hm$set(smap, key, 3);
memset(key, 0, sizeof(key)); // stored key is a copy, unaffected

hm$free(smap); // also destroys the internal key arena
```

- Custom struct backing via hm$s

```c
struct my_rec_s
{
    usize key; // .key field is mandatory for hm$s (located via offsetof)
    usize foo;
    usize bar;
};

// hm$new returns NULL on allocation failure — always check (or use uassert)
hm$s(struct my_rec_s) smap = hm$new(smap, mem$);

hm$sets(smap, ((struct my_rec_s){ .key = 1, .foo = 10, .bar = 20 }));

// hm$gets returns a pointer to the full record, NULL if not found
struct my_rec_s* r = hm$gets(smap, 1);

hm$free(smap);
```

*/
#define __hm$

/// Declares a hashmap variable. `hm$(char*, int) map` = `struct { char* key; int value; }*`.
#define hm$(_KeyType, _ValType)                                                                    \
    struct                                                                                         \
    {                                                                                              \
        _KeyType key;                                                                              \
        _ValType value;                                                                            \
    }*

/// Declares a hashmap based on a custom struct that has a `.key` field. The struct itself becomes the key+value record.
#define hm$s(_StructType) _StructType*

/// Validates an `hm$` handle: `Error.memory` if NULL, `Error.integrity` on bad magic, `EOK` otherwise.
#define hm$validate(t) _cexds__arr_integrity((t), _CEXDS_HM_MAGIC)

/// hm$new(kwargs...) - default values always zeroed (ZII)
struct _cexds__hm_new_kwargs_s
{
    usize capacity; // initial hashmap capacity (default: 16)
    usize seed; // initial hashmap hash algorithm seed: (default: some const value)
    u32 copy_keys_arena_pgsize; // use arena for backing string keys copy (default: false)
    bool copy_keys; // duplicate/copy string keys when adding new records (default: false)
};


/// Creates a new hashmap. Keyword args: `.capacity`, `.seed`, `.copy_keys` (for char* keys), `.copy_keys_arena_pgsize`. Returns the new pointer on success, NULL on allocation failure.
#define hm$new(t, allocator, kwargs...)                                                            \
    ({                                                                                             \
        static_assert(_Alignof(typeof(*t)) <= 64, "hashmap record alignment too high");            \
        uassert(allocator != NULL);                                                                \
        enum _CexDsKeyType_e _key_type = _Generic(                                                 \
            &((t)->key),                                                                           \
            str_s *: _CexDsKeyType__cexstr,                                                        \
            char(**): _CexDsKeyType__charptr,                                                      \
            const char(**): _CexDsKeyType__charptr,                                                \
            char (*)[]: _CexDsKeyType__charbuf,                                                    \
            const char (*)[]: _CexDsKeyType__charbuf,                                              \
            default: _CexDsKeyType__generic                                                        \
        );                                                                                         \
        struct _cexds__hm_new_kwargs_s _kwargs = { kwargs };                                       \
        (t) = (typeof(*t)*)                                                                        \
            _cexds__hminit(sizeof(*t), (allocator), _key_type, alignof(typeof(*t)), &_kwargs);     \
    })


/// Sets `key` to `value` in the hashmap. Replaces if key already exists. Returns pointer to the record, or NULL on allocation failure / NULL hashmap.
#define hm$set(t, k, v...)                                                                         \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        if ((t) != NULL) {                                                                         \
            (t) = _cexds__hmput_key(                                                               \
                (t),                                                                               \
                sizeof(*t),                     /* size of hashmap item */                         \
                ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */           \
                sizeof((t)->key),               /* size of key */                                  \
                offsetof(typeof(*t), key),      /* offset of key in hm struct */                   \
                NULL,                           /* no full element set */                          \
                &result                         /* NULL on allocation failure */                         \
            );                                                                                     \
            if (result) result->value = (v);                                                       \
        }                                                                                          \
        result;                                                                                    \
    })

/// Adds or gets a key and returns a pointer to its value field for direct mutation. Returns NULL on allocation failure / NULL hashmap.
#define hm$setp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        if ((t) != NULL) {                                                                         \
            (t) = _cexds__hmput_key(                                                               \
                (t),                                                                               \
                sizeof(*t),                     /* size of hashmap item */                         \
                ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */           \
                sizeof((t)->key),               /* size of key */                                  \
                offsetof(typeof(*t), key),      /* offset of key in hm struct */                   \
                NULL,                           /* no full element set */                          \
                &result                         /* NULL on allocation failure */                         \
            );                                                                                     \
        }                                                                                          \
        (result ? &result->value : NULL);                                                          \
    })

/// Sets a full pre-initialized record (struct with `.key` field) into the hashmap. Returns pointer to the stored record, or NULL on allocation failure / NULL hashmap.
#define hm$sets(t, v...)                                                                           \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        if ((t) != NULL) {                                                                         \
            typeof(*t) _val = (v);                                                                 \
            (t) = _cexds__hmput_key(                                                               \
                (t),                                                                               \
                sizeof(*t),                /* size of hashmap item */                              \
                &_val.key,                 /* temp on stack pointer to (k) value */                \
                sizeof((t)->key),          /* size of key */                                       \
                offsetof(typeof(*t), key), /* offset of key in hm struct */                        \
                &(_val),                   /* full element write */                                \
                &result                    /* NULL on allocation failure */                              \
            );                                                                                     \
        }                                                                                          \
        result;                                                                                    \
    })

/// Gets the value for key `k` by value. Returns `def` (defaults to zero) if key not found or hashmap is NULL.
#define hm$get(t, k, def...)                                                                       \
    ({                                                                                             \
        typeof((t)->value) _def[1] = { def }; /* default value, always 0 if def... is empty! */    \
        typeof(t) result = NULL;                                                                   \
        if ((t) != NULL) {                                                                         \
            result = _cexds__hmget_key(                                                            \
                (t),                                                                               \
                sizeof(*t),                     /* size of hashmap item */                         \
                ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */           \
                sizeof((t)->key),               /* size of key */                                  \
                offsetof(typeof(*t), key)       /* offset of key in hm struct */                   \
            );                                                                                     \
        }                                                                                          \
        result ? result->value : _def[0];                                                          \
    })

/// Gets a pointer to the value for key `k`. Returns NULL if key not found or hashmap is NULL (no copy — direct pointer into hashmap storage).
#define hm$getp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        if ((t) != NULL) {                                                                         \
            result = _cexds__hmget_key(                                                            \
                (t),                                                                               \
                sizeof *(t),                                                                       \
                ((typeof((t)->key)[1]){ (k) }),                                                    \
                sizeof(t)->key,                                                                    \
                offsetof(typeof(*t), key)                                                          \
            );                                                                                     \
        }                                                                                          \
        result ? &result->value : NULL;                                                            \
    })

/// Gets a pointer to the full hashmap record (key+value struct) for key `k`. Returns NULL if not found or hashmap is NULL.
#define hm$gets(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        if ((t) != NULL) {                                                                         \
            result = _cexds__hmget_key(                                                            \
                (t),                                                                               \
                sizeof *(t),                                                                       \
                ((typeof((t)->key)[1]){ (k) }),                                                    \
                sizeof(t)->key,                                                                    \
                offsetof(typeof(*t), key)                                                          \
            );                                                                                     \
        }                                                                                          \
        result;                                                                                    \
    })

/// Clears all entries from the hashmap. Frees copied string keys if `.copy_keys` was set. Does NOT free the hashmap itself. NULL hashmap is a no-op.
#define hm$clear(t)                                                                                \
    ({                                                                                             \
        if ((t) != NULL) {                                                                         \
            _cexds__hmfree_keys_func((t), sizeof(*t), offsetof(typeof(*t), key));                  \
            _cexds__hmclear_func(_cexds__header((t))->_hash_table, NULL);                          \
            _cexds__header(t)->length = 0;                                                         \
        }                                                                                          \
        true;                                                                                      \
    })

/// Deletes the entry for key `k`. IMPORTANT: the backing array may be reordered (swap-with-last). Frees copied string keys if applicable. Returns false for a NULL hashmap.
#define hm$del(t, k)                                                                               \
    ({                                                                                             \
        bool _cexds__ret = false;                                                                  \
        if ((t) != NULL) {                                                                         \
            _cexds__ret = _cexds__hmdel_key(                                                       \
                (t),                                                                               \
                sizeof *(t),                                                                       \
                ((typeof((t)->key)[1]){ (k) }),                                                    \
                sizeof(t)->key,                                                                    \
                offsetof(typeof(*t), key)                                                          \
            );                                                                                     \
        }                                                                                          \
        _cexds__ret;                                                                               \
    })


/// Frees all hashmap resources (entries, key copies, arena, hash table) and sets the pointer to NULL.
#define hm$free(t) (_cexds__hmfree_func((t), sizeof *(t), offsetof(typeof(*t), key)), (t) = NULL)

/// Returns the number of entries in the hashmap. Equivalent to `arr$len()`. Returns 0 if NULL.
#define hm$len(t) ((t) ? _cexds__header((t))->length : 0)

u64 _cexds__hash_bytes(const void* p, usize len, u64 seed);

#endif
