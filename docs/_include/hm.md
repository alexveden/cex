

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



```c
/// Declares a hashmap variable. `hm$(char*, int) map` = `struct { char* key; int value; }*`.
#define hm$(_KeyType, _ValType)

/// Clears all entries from the hashmap. Frees copied string keys if `.copy_keys` was set. Does NOT free the hashmap itself.
#define hm$clear(t)

/// Deletes the entry for key `k`. IMPORTANT: the backing array may be reordered (swap-with-last). Frees copied string keys if applicable.
#define hm$del(t, k)

/// Frees all hashmap resources (entries, key copies, arena, hash table) and sets the pointer to NULL.
#define hm$free(t)

/// Gets the value for key `k` by value. Returns `def` (defaults to zero) if key not found.
#define hm$get(t, k, def...)

/// Gets a pointer to the value for key `k`. Returns NULL if key not found (no copy — direct pointer into hashmap storage).
#define hm$getp(t, k)

/// Gets a pointer to the full hashmap record (key+value struct) for key `k`. Returns NULL if not found.
#define hm$gets(t, k)

/// Returns the number of entries in the hashmap. Equivalent to `arr$len()`. Returns 0 if NULL.
#define hm$len(t)

/// Creates a new hashmap. Keyword args: `.capacity`, `.seed`, `.copy_keys` (for char* keys), `.copy_keys_arena_pgsize`. Returns the new pointer on success, NULL on memory error.
#define hm$new(t, allocator, kwargs...)

/// Declares a hashmap based on a custom struct that has a `.key` field. The struct itself becomes the key+value record.
#define hm$s(_StructType)

/// Sets `key` to `value` in the hashmap. Replaces if key already exists. Returns pointer to the record, or NULL on memory error.
#define hm$set(t, k, v...)

/// Adds or gets a key and returns a pointer to its value field for direct mutation. Returns NULL on memory error.
#define hm$setp(t, k)

/// Sets a full pre-initialized record (struct with `.key` field) into the hashmap. Returns pointer to the stored record, or NULL on memory error.
#define hm$sets(t, v...)




```
