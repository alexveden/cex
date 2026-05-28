#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_ds)
#include "cex_base.h"

/**

Generic type-safe dynamic array backed by a heap header.

`arr$(T)` is just `T*` — zero overhead, fully C-array compatible with no hidden
pointer or fat-pointer indirection. The runtime header
(`_cexds__array_header`) lives *before* the user pointer at a negative offset.

Principles:

1. **Zero overhead** — `arr$(T)` = `T*`. Pass them to any function expecting a C pointer+length.
2. **Allocator-backed** — Every array carries its `IAllocator`. Passed once at `arr$new`.
3. **O(1) amortized growth** — Capacity doubles when full (minimum 16).
4. **Unified length** — `arr$len()` works on dynamic `arr$`, static C arrays, and hashmaps.
5. **Unified iteration** — `for$each` / `for$eachp` iterate any array (arr$, static, pointer+len, hm$).
6. **Debug integrity** — Each array header has a magic number checked by every mutating macro
   (`_CEXDS_ARR_MAGIC = 0xC001DAAD`). Wrong magic triggers an assertion.
7. **ASAN-aware** — The 8-byte poison area after the header is marked poisoned so ASAN catches
   underflow reads/writes.

- Creating array
```c
int main(void)
{
    // heap allocator — must call arr$free() later, or use mem$scope() for automatic cleanup
    arr$(i32) array = arr$new(array, mem$);

    arr$pushm(array, 1, 2, 3);   // multiple elements at once (compound-literal temp array)
    arr$push(array, 4);          // single element

    io.printf("len=%zu\n", arr$len(array));  // works on arr$, hm$, static C arrays, pointer+len

    // iteration by value — copies each element into `it` (≤ CEX_FOREACH_MAX_COPY_SIZE bytes)
    for$each(it, array) {
        io.printf("el=%d\n", it);
    }

    // iteration by pointer — no copy, prefer for large structs
    // TIP: derive index from pointer subtraction
    for$eachp(it, array) {
        io.printf("el[%zu]=%d\n", (usize)(it - array), *it);
    }

    // gotcha: memory not freed until arr$free() — safe to call on NULL (no-op)
    arr$free(array);
    return 0;
}
```

- Array of structs
```c

typedef struct
{
    int key;
    float my_val;
    char* my_string;
    int value;
} my_struct;

int main(void)
{
    // pre-allocate capacity to avoid early reallocs; .capacity is optional,
    // defaults to 16 if omitted
    arr$(my_struct) array = arr$new(array, mem$, .capacity = 128);

    // gotcha: structs are copied by value into the array — the original `s` can
    // be reused or stack-allocated. For pointer-heavy structs you may need
    // deep-copy semantics handled by your own code.
    arr$push(array, ((my_struct){ 20, 5.0f, "hello", 0 }));
    arr$push(array, ((my_struct){ 40, 2.5f, "world", 0 }));

    // arr$len() works on both arr$ and static C arrays
    for (usize i = 0; i < arr$len(array); ++i) {
        io.printf("key: %d str: %s\n", array[i].key, array[i].my_string);
    }

    arr$free(array);
    return 0;
}
```

*/
#define __arr$

// this is a simple string arena allocator, initialize with e.g. 'cexds_string_arena my_arena={0}'.
typedef struct _cexds__string_arena _cexds__string_arena;
extern char* _cexds__stralloc(_cexds__string_arena* a, char* str);
extern void _cexds__strreset(_cexds__string_arena* a);

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
extern bool _cexds__arr_integrity(const void* arr, usize magic_num);
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

/// Declares a dynamic array variable. `arr$(int) myarr` = `int* myarr`. Zero overhead, fully C-compatible.
#define arr$(T) T*

struct _cexds__arr_new_kwargs_s
{
    usize capacity;
};
/// Initializes a dynamic array. Pass the array variable, an `IAllocator`, and optional `.capacity = N`. Returns the new pointer on success, NULL on memory error.
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
#define arr$free(a) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), _cexds__arrfreef((a)), (a) = NULL)

/// Resizes the array capacity to at least `n` elements. No-op if current capacity >= n.
#define arr$setcap(a, n) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), arr$grow(a, 0, n))

/// Clears the array (sets length to 0). Does NOT free or shrink memory — use `arr$free` for that.
#define arr$clear(a) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), _cexds__header(a)->length = 0)

/// Returns the current allocated capacity (in elements). Returns 0 if array is NULL.
#define arr$cap(a) ((a) ? (_cexds__header(a)->capacity) : 0)

/// Deletes element at index `i` by shifting subsequent elements left. Order preserved. O(n).
#define arr$del(a, i)                                                                              \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        memmove(&(a)[i], &(a)[(i) + 1], sizeof *(a) * (_cexds__header(a)->length - 1 - (i)));      \
        _cexds__header(a)->length--;                                                               \
    })

/// Deletes element at index `i` by swapping with the last element. Order NOT preserved, but O(1).
#define arr$delswap(a, i)                                                                          \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        (a)[i] = arr$last(a);                                                                      \
        _cexds__header(a)->length -= 1;                                                            \
    })

/// Returns the last element (by value). Asserts that the array is not empty.
#define arr$last(a)                                                                                \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert(_cexds__header(a)->length > 0 && "empty array");                                   \
        (a)[_cexds__header(a)->length - 1];                                                        \
    })

/// Returns element at index `i` (by value) with bounds checking via `uassert()`. Also works on `hm$`.
#define arr$at(a, i)                                                                               \
    ({                                                                                             \
        _cexds__arr_integrity(a, 0); /* may work also on hm$ */                                    \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        (a)[i];                                                                                    \
    })

/// Pops and returns the last element (by value). Assert-fails on empty array. Decrements length.
#define arr$pop(a)                                                                                 \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        _cexds__header(a)->length--;                                                               \
        (a)[_cexds__header(a)->length];                                                            \
    })

/// Appends a single element to the end. Automatically grows capacity if needed. Returns pointer to the new slot.
#define arr$push(a, value...)                                                                      \
    ({                                                                                             \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$push memory error");                                             \
            abort();                                                                               \
        }                                                                                          \
        (a)[_cexds__header(a)->length++] = (value);                                                \
        &(a)[_cexds__header(a)->length-1];                                                         \
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

/// Appends all elements from `array` (dynamic, static, or pointer+len) into `a`. `array_len` is optional for pointer+len.
#define arr$pusha(a, array, array_len...)                                                          \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassertf(array != NULL, "arr$pusha: array is NULL");                                       \
        usize _arr_len_va[] = { array_len };                                                       \
        usize arr_len = (sizeof(_arr_len_va) > 0) ? _arr_len_va[0] : arr$len(array);               \
        uassert(arr_len < PTRDIFF_MAX && "negative length or overflow");                           \
        if (unlikely(!arr$grow_check(a, arr_len))) {                                               \
            uassert(false && "arr$pusha memory error");                                            \
            abort();                                                                               \
        }                                                                                          \
        for (usize i = 0; i < arr_len; i++) { (a)[_cexds__header(a)->length++] = ((array)[i]); }   \
        /* NOLINTEND */                                                                            \
    })

/// Sorts the array in-place using `qsort()` with the provided comparator.
#define arr$sort(a, qsort_cmp)                                                                     \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        qsort((a), arr$len(a), sizeof(*a), qsort_cmp);                                             \
    })


/// Inserts element at index `i`, shifting subsequent elements right. Order preserved. O(n).
#define arr$ins(a, i, value...)                                                                    \
    do {                                                                                           \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$ins memory error");                                              \
            abort();                                                                               \
        }                                                                                          \
        _cexds__header(a)->length++;                                                               \
        uassert((usize)i < _cexds__header(a)->length && "i out of bounds");                        \
        memmove(&(a)[(i) + 1], &(a)[i], sizeof(*(a)) * (_cexds__header(a)->length - 1 - (i)));     \
        (a)[i] = (value);                                                                          \
    } while (0)

/// Checks if array has room for `add_extra` elements, growing if needed. Returns false on memory error.
#define arr$grow_check(a, add_extra)                                                               \
    ((_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC) &&                                                \
      _cexds__header(a)->length + (add_extra) > _cexds__header(a)->capacity)                       \
         ? (arr$grow(a, add_extra, 0), a != NULL)                                                  \
         : true)

/// Grows array so it can hold at least `add_len` more elements, with the absolute minimum of `min_cap`.
#define arr$grow(a, add_len, min_cap)                                                              \
    ((a) = _cexds__arrgrowf((a), sizeof *(a), (add_len), (min_cap), alignof(typeof(*a)), NULL))


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

Unified array / hashmap / slice iteration framework.

`for$` macros provide a single syntax for looping over any iterable data in CEX:

| Variant                              | Copies elements?          | Use case                                         |
|--------------------------------------|---------------------------|--------------------------------------------------|
| `for$each(it, array, len?)`          | By value (≤ 64 B)         | Small types / copy iteration / slices            |
| `for$eachp(it, array, len?)`         | By pointer (no copy)      | Large structs / avoid copy overhead / slices     |
| `for$iter(T, it, iter_func)`         | Custom (cex_iterator_s)   | Tokenizers, generators, splitters                |

All three work identically on `arr$`, `hm$`, static C arrays, and pointer+length slices.

- Using for$ as unified array iterator
```c

int main(void)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    // for$each copies elements by value (up to CEX_FOREACH_MAX_COPY_SIZE bytes)
    for$each(it, array) {
        io.printf("el=%d\n", it);
    }
    // Prints:
    // el=1
    // el=2
    // el=3

    // for$eachp provides a pointer — no copy, prefer for large structs
    for$eachp(it, array) {
        // TIP: derive index from pointer subtraction
        usize i = (usize)(it - array);

        io.printf("el[%zu]=%d\n", i, *it);
    }
    // Prints:
    // el[0]=1
    // el[1]=2
    // el[2]=3

    // Static C arrays work too — arr$len() inferred from sizeof
    i32 arr_int[] = {1, 2, 3, 4, 5};
    for$each(it, arr_int) {
        io.printf("static=%d\n", it);
    }
    // Prints:
    // static=1
    // static=2
    // static=3
    // static=4
    // static=5

    // Pointer+length slice — pass len as third arg
    i32* slice = &arr_int[2];
    for$each(it, slice, 2) {
        io.printf("slice=%d\n", it);
    }
    // Prints:
    // slice=3
    // slice=4

    // for$iter uses a custom iterator function and cex_iterator_s
    // NOTE: str_s is passed by value (stack-allocated slice)
    str_s s = str.sstr("123,456");
    for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {
        // gotcha: it.val is a non-null-terminated slice — use %S, not %s
        io.printf("it.value = %S\n", it.val);
    }
    // Prints:
    // it.value = 123
    // it.value = 456

    arr$free(array);
    return 0;
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

Generic type-safe hashmap backed by open-addressing with quadratic probing.

The hashmap shares the same backing engine as `arr$` (same header, same allocator).
This means every `hm$` is also an `arr$` — you can iterate, index, and take its length
just like a regular dynamic array.

Key features:

1. **Open-addressing** with bucketed hash table (8 slots per cache-line-aligned bucket).
2. **Quadratic probing** with tombstone tracking for efficient deletions.
3. **Key type auto-detection** via `_Generic` — numeric (memcmp), `char*` (strcmp),
   `char[N]` (strcmp), `str_s` (memcmp with length check).
4. **String key modes** — no-copy (default), copy via `malloc`/`strdup`
   (`.copy_keys = true`), or copy via arena allocator
   (`.copy_keys = true, .copy_keys_arena_pgsize = NNN`).
5. **Grow / shrink** — table doubles at 75% load factor, halves below 25%,
   tombstones trigger rebuild at ~12%.
6. **Dual index** — `hm$` data array is sorted by insertion order (stable until
   first delete). Hash table entries point into this array, so `arr$len()`,
   `for$each`, and bracket indexing all work transparently.

Principles:

1. `hm$(K,V)` is a struct `{ K key; V value; }*`.
2. `hm$s(S)` treats any struct with a `.key` field as a hashmap record.
3. `arr$len()`, `arr$cap()`, `for$each`, `for$eachp` all work on `hm$` types.
4. Array indexing `smap[i].key` / `smap[i].value` works but order may change after
   calls to `hm$del`.
5. `hm$new` can return `NULL` on memory error — always check (or use `uassert`).

- Basic usage
```c

int main(void)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);

    // hm$set replaces the value if the key already exists
    hm$set(intmap, 15, 7);
    hm$set(intmap, 11, 3);
    hm$set(intmap, 9, 5);

    // hm$len and arr$len are equivalent for hashmaps
    io.printf("len=%zu\n", hm$len(intmap));

    // get by value — returns a default (0 or custom) if key is missing
    io.printf("val for 9=%d\n", hm$get(intmap, 9, -1));

    // get by pointer — NULL if not found (no copy, direct pointer into storage)
    int* vp = hm$getp(intmap, 11);
    if (vp) io.printf("got %d\n", *vp);

    // deleting a non-existent key is safe (no-op)
    hm$del(intmap, 100);

    // gotcha: hm$del may reorder the backing array — do not rely on
    // insertion order after deletions

    // clear all entries (does not free the hashmap itself)
    hm$clear(intmap);

    // iteration works just like arr$
    for$each (it, intmap) {
        io.printf("key=%d, value=%d\n", it.key, it.value);
    }

    hm$free(intmap);
    return 0;
}
```

- Using hashmap as field of other struct
```c

typedef hm$(char*, int) MyHashmap;

struct my_hm_struct {
    MyHashmap hm;
};


int main(void)
{
    struct my_hm_struct hs = {0};

    // .copy_keys = true makes the hashmap duplicate char* keys internally
    // without it, the key pointer must outlive the hashmap
    hm$new(hs.hm, mem$, .copy_keys = true);

    // gotcha: "foo" is a string literal — with .copy_keys it is safe;
    // without .copy_keys, the literal pointer is stored directly (valid for
    // string literals, but not for stack buffers that go out of scope)
    hm$set(hs.hm, "foo", 3);

    hm$free(hs.hm);
    return 0;
}
```

- Storing string values in the arena
```c

int main(void)
{
    // .copy_keys_arena_pgsize = 1024 allocates key copies from an internal
    // arena with 1 KiB pages — avoids per-key malloc overhead
    hm$(char*, int) smap = hm$new(smap, mem$, .copy_keys = true,
                                   .copy_keys_arena_pgsize = 1024);

    char key2[10] = "foo";

    hm$set(smap, key2, 3);
    io.printf("len=%zu, val=%d\n", hm$len(smap), hm$get(smap, "foo", -1));

    // gotcha: after setting key2, the hashmap copied the string into the
    // arena. Overwriting the original buffer does NOT affect stored keys.
    memset(key2, 0, sizeof(key2));
    io.printf("after zero: key='%s' val=%d\n", smap[0].key, hm$get(smap, "foo", -1));

    hm$free(smap);   // also destroys the internal key arena
    return 0;
}
```

- Checking errors + custom struct backing
```c

struct my_rec_s
{
    usize key;    // .key field is mandatory for hm$s
    usize foo;
    usize bar;
};

int main(void)
{
    // hm$new returns NULL on memory error — always check in production code
    hm$(int, int) intmap;
    if (hm$new(intmap, mem$) == NULL) {
        io.printf("initialization error\n");
        return 1;
    }

    // custom struct as hashmap backend via hm$s(S)
    // gotcha: the struct MUST have a `.key` field, hm$s(S) uses offsetof()
    // to locate it. The rest of the struct is the value payload.
    hm$s(struct my_rec_s) smap = hm$new(smap, mem$);
    if (smap == NULL) {
        hm$free(intmap);
        return 1;
    }

    // hm$sets writes a full record (struct with .key)
    hm$sets(smap, ((struct my_rec_s){ .key = 1, .foo = 10, .bar = 20 }));
    io.printf("len=%zu, foo=%zu\n", hm$len(smap), smap[0].foo);

    // hm$gets returns pointer to full record, NULL if not found
    struct my_rec_s* r = hm$gets(smap, 1);
    if (r) io.printf("found: foo=%zu bar=%zu\n", r->foo, r->bar);

    hm$free(smap);
    hm$free(intmap);
    return 0;
}
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

/// hm$new(kwargs...) - default values always zeroed (ZII)
struct _cexds__hm_new_kwargs_s
{
    usize capacity; // initial hashmap capacity (default: 16)
    usize seed; // initial hashmap hash algorithm seed: (default: some const value)
    u32 copy_keys_arena_pgsize; // use arena for backing string keys copy (default: false)
    bool copy_keys; // duplicate/copy string keys when adding new records (default: false)
};


/// Creates a new hashmap. Keyword args: `.capacity`, `.seed`, `.copy_keys` (for char* keys), `.copy_keys_arena_pgsize`. Returns the new pointer on success, NULL on memory error.
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


/// Sets `key` to `value` in the hashmap. Replaces if key already exists. Returns pointer to the record, or NULL on memory error.
#define hm$set(t, k, v...)                                                                         \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = _cexds__hmput_key(                                                                   \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key),      /* offset of key in hm struct */                       \
            NULL,                           /* no full element set */                              \
            &result                         /* NULL on memory error */                             \
        );                                                                                         \
        if (result) result->value = (v);                                                           \
        result;                                                                                    \
    })

/// Adds or gets a key and returns a pointer to its value field for direct mutation. Returns NULL on memory error.
#define hm$setp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = _cexds__hmput_key(                                                                   \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key),      /* offset of key in hm struct */                       \
            NULL,                           /* no full element set */                              \
            &result                         /* NULL on memory error */                             \
        );                                                                                         \
        (result ? &result->value : NULL);                                                          \
    })

/// Sets a full pre-initialized record (struct with `.key` field) into the hashmap. Returns pointer to the stored record, or NULL on memory error.
#define hm$sets(t, v...)                                                                           \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        typeof(*t) _val = (v);                                                                     \
        (t) = _cexds__hmput_key(                                                                   \
            (t),                                                                                   \
            sizeof(*t),                /* size of hashmap item */                                  \
            &_val.key,                 /* temp on stack pointer to (k) value */                    \
            sizeof((t)->key),          /* size of key */                                           \
            offsetof(typeof(*t), key), /* offset of key in hm struct */                            \
            &(_val),                   /* full element write */                                    \
            &result                    /* NULL on memory error */                                  \
        );                                                                                         \
        result;                                                                                    \
    })

/// Gets the value for key `k` by value. Returns `def` (defaults to zero) if key not found.
#define hm$get(t, k, def...)                                                                       \
    ({                                                                                             \
        typeof(t) result = _cexds__hmget_key(                                                      \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key)       /* offset of key in hm struct */                       \
        );                                                                                         \
        typeof((t)->value) _def[1] = { def }; /* default value, always 0 if def... is empty! */    \
        result ? result->value : _def[0];                                                          \
    })

/// Gets a pointer to the value for key `k`. Returns NULL if key not found (no copy — direct pointer into hashmap storage).
#define hm$getp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = _cexds__hmget_key(                                                      \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
        result ? &result->value : NULL;                                                            \
    })

/// Gets a pointer to the full hashmap record (key+value struct) for key `k`. Returns NULL if not found.
#define hm$gets(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = _cexds__hmget_key(                                                      \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
        result;                                                                                    \
    })

/// Clears all entries from the hashmap. Frees copied string keys if `.copy_keys` was set. Does NOT free the hashmap itself.
#define hm$clear(t)                                                                                \
    ({                                                                                             \
        _cexds__arr_integrity(t, _CEXDS_HM_MAGIC);                                                 \
        _cexds__hmfree_keys_func((t), sizeof(*t), offsetof(typeof(*t), key));                      \
        _cexds__hmclear_func(_cexds__header((t))->_hash_table, NULL);                              \
        _cexds__header(t)->length = 0;                                                             \
        true;                                                                                      \
    })

/// Deletes the entry for key `k`. IMPORTANT: the backing array may be reordered (swap-with-last). Frees copied string keys if applicable.
#define hm$del(t, k)                                                                               \
    ({                                                                                             \
        _cexds__hmdel_key(                                                                         \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
    })


/// Frees all hashmap resources (entries, key copies, arena, hash table) and sets the pointer to NULL.
#define hm$free(t) (_cexds__hmfree_func((t), sizeof *(t), offsetof(typeof(*t), key)), (t) = NULL)

/// Returns the number of entries in the hashmap. Equivalent to `arr$len()`. Returns 0 if NULL.
#define hm$len(t)                                                                                  \
    ({                                                                                             \
        if (t != NULL) { _cexds__arr_integrity(t, _CEXDS_HM_MAGIC); }                              \
        (t) ? _cexds__header((t))->length : 0;                                                     \
    })

typedef struct _cexds__string_block
{
    struct _cexds__string_block* next;
    char storage[8];
} _cexds__string_block;

struct _cexds__string_arena
{
    _cexds__string_block* storage;
    usize remaining;
    unsigned char block;
    unsigned char mode; // this isn't used by the string arena itself
};

enum
{
    _CEXDS_SH_NONE,
    _CEXDS_SH_DEFAULT,
    _CEXDS_SH_STRDUP,
    _CEXDS_SH_ARENA
};

#define _cexds__shmode_func_wrapper(t, e, m) _cexds__shmode_func(e, m)

u64 _cexds__hash_bytes(const void* p, usize len, u64 seed);

#endif
