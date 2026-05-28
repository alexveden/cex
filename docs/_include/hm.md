

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
