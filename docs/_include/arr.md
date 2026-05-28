

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



```c
/// Declares a dynamic array variable. `arr$(int) myarr` = `int* myarr`. Zero overhead, fully C-compatible.
#define arr$(T)

/// Returns element at index `i` (by value) with bounds checking via `uassert()`. Also works on `hm$`.
#define arr$at(a, i)

/// Returns the current allocated capacity (in elements). Returns 0 if array is NULL.
#define arr$cap(a)

/// Clears the array (sets length to 0). Does NOT free or shrink memory — use `arr$free` for that.
#define arr$clear(a)

/// Deletes element at index `i` by shifting subsequent elements left. Order preserved. O(n).
#define arr$del(a, i)

/// Deletes element at index `i` by swapping with the last element. Order NOT preserved, but O(1).
#define arr$delswap(a, i)

/// Frees the array memory and sets the pointer to NULL. Safe on NULL arrays (no-op).
#define arr$free(a)

/// Grows array so it can hold at least `add_len` more elements, with the absolute minimum of `min_cap`.
#define arr$grow(a, add_len, min_cap)

/// Checks if array has room for `add_extra` elements, growing if needed. Returns false on memory error.
#define arr$grow_check(a, add_extra)

/// Inserts element at index `i`, shifting subsequent elements right. Order preserved. O(n).
#define arr$ins(a, i, value...)

/// Returns the last element (by value). Asserts that the array is not empty.
#define arr$last(a)

/// Returns the number of elements. Works on `arr$`, `hm$`, static C arrays, and pointer+length slices.
#define arr$len(arr)

/// Initializes a dynamic array. Pass the array variable, an `IAllocator`, and optional `.capacity = N`. Returns the new pointer on success, NULL on memory error.
#define arr$new(a, allocator, kwargs...)

/// Pops and returns the last element (by value). Assert-fails on empty array. Decrements length.
#define arr$pop(a)

/// Appends a single element to the end. Automatically grows capacity if needed. Returns pointer to the new slot.
#define arr$push(a, value...)

/// Appends all elements from `array` (dynamic, static, or pointer+len) into `a`. `array_len` is optional for pointer+len.
#define arr$pusha(a, array, array_len...)

/// Appends multiple elements at once: `arr$pushm(arr, 1, 2, 3)`. Uses a compound-literal temporary array.
#define arr$pushm(a, items...)

/// Resizes the array capacity to at least `n` elements. No-op if current capacity >= n.
#define arr$setcap(a, n)

/// Sorts the array in-place using `qsort()` with the provided comparator.
#define arr$sort(a, qsort_cmp)




```
