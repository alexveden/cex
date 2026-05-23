#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_ds)
#include "cex_base.h"

/**

* Creating array
```c
    // Using heap allocator (need to free later!)
    arr$(i32) array = arr$new(array, mem$);

    // adding elements
    arr$pushm(array, 1, 2, 3); // multiple at once
    arr$push(array, 4); // single element

    // length of array
    arr$len(array);

    // getting i-th elements
    array[1];

    // iterating array (by value)
    for$each(it, array) {
        io.printf("el=%d\n", it);
    }

    // iterating array (by pointer - prefer for bigger structs to avoid copying)
    for$eachp(it, array) {
        // TIP: making array index out of `it`
        usize i = it - array;

        // NOTE: 'it' now is a pointer
        io.printf("el[%zu]=%d\n", i, *it);
    }

    // free resources
    arr$free(array);
```

* Array of structs
```c

typedef struct
{
    int key;
    float my_val;
    char* my_string;
    int value;
} my_struct;

void somefunc(void)
{
    arr$(my_struct) array = arr$new(array, mem$, .capacity = 128);
    uassert(arr$cap(array), 128);

    my_struct s;
    s = (my_struct){ 20, 5.0, "hello ", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 2.5, "failure", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 1.1, "world!", 0 };
    arr$push(array, s);

    for (usize i = 0; i < arr$len(array); ++i) {
        io.printf("key: %d str: %s\n", array[i].key, array[i].my_string);
    }
    arr$free(array);

    return EOK;
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

/// Generic array type definition. Use arr$(int) myarr - defines new myarr variable, as int array
#define arr$(T) T*

struct _cexds__arr_new_kwargs_s
{
    usize capacity;
};
/// Array initialization: use arr$(int) arr = arr$new(arr, mem$, .capacity = , ...)
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

/// Free resources for dynamic array (only needed if mem$ allocator was used)
#define arr$free(a) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), _cexds__arrfreef((a)), (a) = NULL)

/// Set array capacity and resize if needed
#define arr$setcap(a, n) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), arr$grow(a, 0, n))

/// Clear array contents
#define arr$clear(a) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), _cexds__header(a)->length = 0)

/// Returns current array capacity
#define arr$cap(a) ((a) ? (_cexds__header(a)->capacity) : 0)

/// Delete array elements by index (memory will be shifted, order preserved)
#define arr$del(a, i)                                                                              \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        memmove(&(a)[i], &(a)[(i) + 1], sizeof *(a) * (_cexds__header(a)->length - 1 - (i)));      \
        _cexds__header(a)->length--;                                                               \
    })

/// Delete element by swapping with last one (no memory overhear, element order changes)
#define arr$delswap(a, i)                                                                          \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        (a)[i] = arr$last(a);                                                                      \
        _cexds__header(a)->length -= 1;                                                            \
    })

/// Return last element of array
#define arr$last(a)                                                                                \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert(_cexds__header(a)->length > 0 && "empty array");                                   \
        (a)[_cexds__header(a)->length - 1];                                                        \
    })

/// Get element at index (bounds checking with uassert())
#define arr$at(a, i)                                                                               \
    ({                                                                                             \
        _cexds__arr_integrity(a, 0); /* may work also on hm$ */                                    \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        (a)[i];                                                                                    \
    })

/// Pop element from the end
#define arr$pop(a)                                                                                 \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        _cexds__header(a)->length--;                                                               \
        (a)[_cexds__header(a)->length];                                                            \
    })

/// Push element to the end
#define arr$push(a, value...)                                                                      \
    ({                                                                                             \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$push memory error");                                             \
            abort();                                                                               \
        }                                                                                          \
        (a)[_cexds__header(a)->length++] = (value);                                                \
        &(a)[_cexds__header(a)->length-1];                                                         \
    })

/// Push many elements to the end
#define arr$pushm(a, items...)                                                                     \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        typeof(*a) _args[] = { items };                                                            \
        static_assert(sizeof(_args) > 0, "You must pass at least one item");                       \
        arr$pusha(a, _args, arr$len(_args));                                                       \
        /* NOLINTEND */                                                                            \
    })

/// Push another array into a. array can be dynamic or static or pointer+len
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

/// Sort array with qsort() libc function
#define arr$sort(a, qsort_cmp)                                                                     \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        qsort((a), arr$len(a), sizeof(*a), qsort_cmp);                                             \
    })


/// Inserts element into array at index `i`
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

/// Check array capacity and return false on memory error
#define arr$grow_check(a, add_extra)                                                               \
    ((_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC) &&                                                \
      _cexds__header(a)->length + (add_extra) > _cexds__header(a)->capacity)                       \
         ? (arr$grow(a, add_extra, 0), a != NULL)                                                  \
         : true)

/// Grows array capacity
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
/// Versatile array length, works with dynamic (arr$) and static compile time arrays
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

- using for$ as unified array iterator

```c

test$case(test_array_iteration)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    for$each(it, array) {
        io.printf("el=%d\n", it);
    }
    // Prints:
    // el=1
    // el=2
    // el=3

    // NOTE: prefer this when you work with bigger structs to avoid extra memory copying
    for$eachp(it, array) {
        // TIP: making array index out of `it`
        usize i = it - array;

        // NOTE: it now is a pointer
        io.printf("el[%zu]=%d\n", i, *it);
    }
    // Prints:
    // el[0]=1
    // el[1]=2
    // el[2]=3

    // Static arrays work as well (arr$len inferred)
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


    // Simple pointer+length also works (let's do a slice)
    i32* slice = &arr_int[2];
    for$each(it, slice, 2) {
        io.printf("slice=%d\n", it);
    }
    // Prints:
    // slice=3
    // slice=4


    // it is type of cex_iterator_s
    // NOTE: run in shell: ➜ ./cex help cex_iterator_s
    s = str.sstr("123,456");
    for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {
        io.printf("it.value = %S\n", it.val);
    }
    // Prints:
    // it.value = 123
    // it.value = 456

    arr$free(array);
    return EOK;
}

```

*/
#define __for$

/**
 * @brief cex_iterator_s - CEX iterator interface
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
 * @brief Iterates via iterator function (see usage below)
 *
 * Iterator function signature:
 * u32* array_iterator(u32 array[], u32 len, cex_iterator_s* iter)
 *
 * for$iter(u32, it, array_iterator(arr2, arr$len(arr2), &it.iterator))
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


/// Iterates over arrays `it` is iterated **value**, array may be arr$/or static / or pointer,
/// array_len is only required for pointer+len use case
#define for$each(it, array, array_len...)                                                          \
    /* NOLINTBEGIN*/                                                                               \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    typeof((array)[0])* cex$tmpname(arr_arrp) = _cex__get_buf_addr(array);                         \
    usize cex$tmpname(arr_index) = 0;                                                              \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    for (typeof((array)[0]) it = { 0 };                                                            \
         (cex$tmpname(arr_index) < cex$tmpname(arr_length) &&                                      \
          ((it) = cex$tmpname(arr_arrp)[cex$tmpname(arr_index)], 1));                              \
         cex$tmpname(arr_index)++)
    /* NOLINTEND */                                                                                \


/// Iterates over arrays `it` is iterated by **pointer**, array may be arr$/or static / or pointer,
/// array_len is only required for pointer+len use case
#define for$eachp(it, array, array_len...)                                                         \
    /* NOLINTBEGIN*/                                                                               \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
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
Generic type-safe hashmap

Principles:

1. Data is backed by engine similar to arr$
2. `arr$len()` works with hashmap too
3. Array indexing works with hashmap
4. `for$each`/`for$eachp` is applicable
5. `hm$` generic type is essentially a struct with `key` and `value` fields
6. `hm$` supports following keys: numeric (by default it's just binary representation), char*,
char[N], str_s (CEX sting slice).
7. `hm$` with string keys are stored without copy, use  `hm$new(hm, mem$, .copy_keys = true)` for
copy-mode.
8. `hm$` can store string keys inside an Arena allocator when  `hm$new(hm, mem$, .copy_keys = true,
.copy_keys_arena_pgsize = NNN)`


```c

test$case(test_simple_hashmap)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);

    // Setting items
    hm$set(intmap, 15, 7);
    hm$set(intmap, 11, 3);
    hm$set(intmap, 9, 5);

    // Length
    tassert_eq(hm$len(intmap), 3);
    tassert_eq(arr$len(intmap), 3);

    // Getting items **by value**
    tassert(hm$get(intmap, 9) == 5);
    tassert(hm$get(intmap, 11) == 3);
    tassert(hm$get(intmap, 15) == 7);

    // Getting items **pointer** - NULL on missing
    tassert(hm$getp(intmap, 1) == NULL);

    // Getting with default if not found
    tassert_eq(hm$get(intmap64, -1, 999), 999);

    // Accessing hashmap as array by i-th index
    // NOTE: hashmap elements are ordered until first deletion
    tassert_eq(intmap[0].key, 1);
    tassert_eq(intmap[0].value, 3);

    // removing items
    hm$del(intmap, 100);

    // cleanup
    hm$clear(intmap);

    // basic iteration **by value**
    for$each (it, intmap) {
        io.printf("key=%d, value=%d\n", it.key, it.value);
    }

    // basic iteration **by pointer**
    for$each (it, intmap) {
        io.printf("key=%d, value=%d\n", it->key, it->value);
    }

    hm$free(intmap);
}

```

- Using hashmap as field of other struct
```c

typedef hm$(char* , int) MyHashmap;

struct my_hm_struct {
    MyHashmap hm;
};


test$case(test_hashmap_string_copy_clear_cleanup)
{
    struct my_hm_struct hs = {0};
    // NOTE: .copy_keys - makes sure that key string was copied
    hm$new(hs.hm, mem$, .copy_keys = true);
    hm$set(hs.hm, "foo", 3);
}
```

- Storing string values in the arena
```c

test$case(test_hashmap_string_copy_arena)
{
    hm$(char*, int) smap = hm$new(smap, mem$, .copy_keys = true, .copy_keys_arena_pgsize = 1024);

    char key2[10] = "foo";

    hm$set(smap, key2, 3);
    tassert_eq(hm$len(smap), 1);
    tassert_eq(hm$get(smap, "foo"), 3);
    tassert_eq(hm$get(smap, key2), 3);
    tassert_eq(smap[0].key, "foo");

    memset(key2, 0, sizeof(key2));
    tassert_eq(smap[0].key, "foo");
    tassert_eq(hm$get(smap, "foo"), 3);

    hm$free(smap);
    return EOK;
}

```

- Checking errors + custom struct backing
```c

test$case(test_hashmap_basic)
{
    hm$(int, int) intmap;
    if(hm$new(intmap, mem$) == NULL) {
        // initialization error
    }

    // struct as a value
    struct test64_s
    {
        usize foo;
        usize bar;
    };
    hm$(int, struct test64_s) intmap = hm$new(intmap, mem$);

    // custom struct as hashmap backend
    struct test64_s
    {
        usize fooa;
        usize key; // this field `key` is mandatory
    };

    hm$s(struct test64_s) smap = hm$new(smap, mem$);
    tassert(smap != NULL);

    // Setting hashmap as a whole struct key/value record
    tassert(hm$sets(smap, (struct test64_s){ .key = 1, .fooa = 10 }));
    tassert_eq(hm$len(smap), 1);
    tassert_eq(smap[0].key, 1);
    tassert_eq(smap[0].fooa, 10);

    // Getting full struct by .key value
    struct test64_s* r = hm$gets(smap, 1);
    tassert(r != NULL);
    tassert(r == &smap[0]);
    tassert_eq(r->key, 1);
    tassert_eq(r->fooa, 10);

}

```

*/
#define __hm$

/// Defines hashmap generic type
#define hm$(_KeyType, _ValType)                                                                    \
    struct                                                                                         \
    {                                                                                              \
        _KeyType key;                                                                              \
        _ValType value;                                                                            \
    }*

/// Defines hashmap type based on _StructType, must have `key` field
#define hm$s(_StructType) _StructType*

/// hm$new(kwargs...) - default values always zeroed (ZII)
struct _cexds__hm_new_kwargs_s
{
    usize capacity; // initial hashmap capacity (default: 16)
    usize seed; // initial hashmap hash algorithm seed: (default: some const value)
    u32 copy_keys_arena_pgsize; // use arena for backing string keys copy (default: false)
    bool copy_keys; // duplicate/copy string keys when adding new records (default: false)
};


/// Creates new hashmap of hm$(KType, VType) using allocator, kwargs: .capacity, .seed,
/// .copy_keys_arena_pgsize, .copy_keys
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


/// Set hashmap key/value, replaces if exists 
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

/// Add new item and returns pointer of hashmap record for `k`, for further editing  
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

/// Set full record, must be initialized by user
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

/// Get item by value, def - default value (zeroed by default), can be any type
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

/// Get item by pointer (no copy, direct pointer inside hashmap)
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

/// Get a pointer to full hashmap record, NULL if not found
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

/// Clears hashmap contents
#define hm$clear(t)                                                                                \
    ({                                                                                             \
        _cexds__arr_integrity(t, _CEXDS_HM_MAGIC);                                                 \
        _cexds__hmfree_keys_func((t), sizeof(*t), offsetof(typeof(*t), key));                      \
        _cexds__hmclear_func(_cexds__header((t))->_hash_table, NULL);                              \
        _cexds__header(t)->length = 0;                                                             \
        true;                                                                                      \
    })

/// Deletes items, IMPORTANT hashmap array may be reordered after this call
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


/// Frees hashmap resources
#define hm$free(t) (_cexds__hmfree_func((t), sizeof *(t), offsetof(typeof(*t), key)), (t) = NULL)

/// Returns hashmap length, also you can use arr$len()
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
#endif
