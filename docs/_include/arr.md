

### Dynamic array

Generic type-safe dynamic array backed by a heap header.

`arr$(T)` is just `T*` — zero overhead, fully C-array compatible with no hidden
pointer or fat-pointer indirection. The runtime header
(`_cexds__array_header`) lives *before* the user pointer at a negative offset.

#### Principles

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
   Note: the default heap allocator panics on real OOM (`cex$platform_oom_panic`); these `NULL`
   returns are for synthetic `test$alloc` OOM, custom allocators, or an opt-out build.

#### Examples

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



```c
/// Declares a dynamic array variable. `arr$(int) myarr` = `int* myarr`. Zero overhead, fully C-compatible.
#define arr$(T)

/// Returns element at index `i` (by value) with bounds checking via `uassert()`. Also works on `hm$`. Asserts on NULL array.
#define arr$at(a, i)

/// Returns the current allocated capacity (in elements). Returns 0 if array is NULL.
#define arr$cap(a)

/// Clears the array (sets length to 0). Does NOT free or shrink memory — use `arr$free` for that. NULL array is a no-op.
#define arr$clear(a)

/// Deletes element at index `i` by shifting subsequent elements left. Order preserved. O(n). NULL array is a no-op (returns 0).
#define arr$del(a, i)

/// Deletes element at index `i` by swapping with the last element. Order NOT preserved, but O(1). NULL array is a no-op.
#define arr$delswap(a, i)

/// Frees the array memory and sets the pointer to NULL. Safe on NULL arrays (no-op).
#define arr$free(a)

/// Grows array so it can hold at least `add_len` more elements, with the absolute minimum of `min_cap`. Returns the array pointer, or NULL on allocation failure / NULL array.
#define arr$grow(a, add_len, min_cap)

/// Checks if array has room for `add_extra` elements, growing if needed. Returns false on allocation failure, length overflow, or NULL array.
#define arr$grow_check(a, add_extra)

/// Inserts element at index `i`, shifting subsequent elements right. Order preserved. O(n). Returns pointer to the inserted slot, or NULL on allocation failure / NULL array.
#define arr$ins(a, i, value...)

/// Returns the last element (by value). Asserts that the array is not NULL or empty.
#define arr$last(a)

/// Returns the number of elements. Works on `arr$`, `hm$`, static C arrays, and pointer+length slices.
#define arr$len(arr)

/// Initializes a dynamic array. Pass the array variable, an `IAllocator`, and optional `.capacity = N`. Returns the new pointer on success, NULL on allocation failure.
#define arr$new(a, allocator, kwargs...)

/// Pops and returns the last element (by value). Asserts that the array is not NULL or empty.
#define arr$pop(a)

/// Appends a single element to the end. Automatically grows capacity if needed. Returns pointer to the new slot, or NULL on allocation failure / NULL array.
#define arr$push(a, value...)

/// Appends all elements from `array` (dynamic, static, or pointer+len) into `a`. `array_len` is optional for pointer+len. Returns pointer to the first appended slot, or NULL on allocation failure / NULL array / empty source.
#define arr$pusha(a, array, array_len...)

/// Appends multiple elements at once: `arr$pushm(arr, 1, 2, 3)`. Uses a compound-literal temporary array.
#define arr$pushm(a, items...)

/// Resizes the array capacity to at least `n` elements. No-op if current capacity >= n. Returns the array pointer, or NULL on allocation failure / NULL array.
#define arr$setcap(a, n)

/// Sorts the array in-place using `qsort()` with the provided comparator. NULL array is a no-op.
#define arr$sort(a, qsort_cmp)

/// Validates an `arr$` handle: `Error.memory` if NULL, `Error.integrity` on bad magic, `EOK` otherwise.
#define arr$validate(a)




```
