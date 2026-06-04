
Mem cheat-sheet

Global allocators:

- `mem$` - heap based allocator, typically used for long-living data, requires explicit mem$free
- `tmem$` - temporary allocator, based by ArenaAllocator, with 256kb page, requires `mem$scope`

Memory management hints:

- If function accept IAllocator as argument, it allocates memory
- If class/object accept IAllocator in constructor it should track allocator's instance
- `mem$scope()` - automatically free memory at scope exit by any reason (`return`, `goto` out,
`break`)
- consider `mem$malloc/mem$calloc/mem$realloc/mem$free/mem$new`
- You can init arena scope with `mem$arena(page_size, arena_var_name)`
- AllocatorArena grows dynamically if there is no room in existing page, but be careful when you use
many `realloc()`, it can grow arenas unexpectedly large.
- Use temp allocator as `mem$scope(tmem$, _) {}` it's a common CEX pattern, `_` is `tmem$`
short-alias
- Nested `mem$scope` are allowed, but memory freed at nested scope exit. NOTE: don't share pointers
across scopes.
- Use address sanitizers as often as possible


Examples:

- Vanilla heap allocator
```c
test$case(test_allocator_api)
{
    u8* p = mem$malloc(mem$, 100);
    tassert(p != NULL);

    // mem$free always nullifies pointer
    mem$free(mem$, p);
    tassert(p == NULL);

    p = mem$calloc(mem$, 100, 100, 32); // malloc with 32-byte alignment
    tassert(p != NULL);

    // Allocates new ZII struct based on given type
    auto my_item = mem$new(mem$, struct my_type_s);

    return EOK;
}

```

- Temporary memory scope

```c
mem$scope(tmem$, _)
{
    arr$(char*) incl_path = arr$new(incl_path, _);
    for$each (p, alt_include_path) {
        arr$push(incl_path, p);
        if (!os.path.exists(p)) { log$warn("alt_include_path not exists: %s\n", p); }
    }
}
```

- Arena Scope (two forms)

```c
// form 1 — integer page_size
mem$arena(4096, arena)
{
    u8* p = mem$malloc(arena, 100);
}

// form 2 — AllocatorArena_kw pointer
mem$arena(&(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }, arena)
{
    u8* p = mem$malloc(arena, 100);
}
```

- Arena Instance

```c
IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true });

u8* p = mem$malloc(arena, 100); // direct use allowed (disable_scopes = true)

mem$scope(arena, tal)
{
    // NOTE: this scope will be freed after exit
    u8* p2 = mem$malloc(tal, 100000);

    mem$scope(arena, tal)
    {
        u8* p3 = mem$malloc(tal, 100);
    }
}

AllocatorArena.destroy(arena);

```


```c
/// General purpose heap allocator
#define mem$

/// Gets address of a struct member via a single-element array compound literal
#define mem$addressof(typevar, value)

/// Checks if pointer address of `p` is aligned to `alignment`
#define mem$aligned_pointer(p, alignment)

/// Rounds `size` to the closest alignment
#define mem$aligned_round(size, alignment)

/// Creates new ArenaAllocator instance in scope, frees it at scope exit.
/// First argument: integer `page_size` or `const AllocatorArena_kw*` pointer.
#define mem$arena(ps, allc_var)

/// true - if program was compiled with address sanitizer support
#define mem$asan_enabled()

/// Poisons memory region with ASAN, or fill it with 0xf7 byte pattern (no ASAN)
#define mem$asan_poison(addr, size)

/// Check if previously poisoned address is consistent, and 0x7f pattern not overwritten (no ASAN)
#define mem$asan_poison_check(addr, size)

/// Unpoisons memory region with ASAN, or fill it with 0x00 byte pattern (no ASAN)
#define mem$asan_unpoison(addr, size)

/// Allocate zero initialized chunk of memory using `allocator`
#define mem$calloc(allocator, nmemb, size, alignment...)

/// Free previously allocated chunk of memory, `ptr` implicitly set to NULL
#define mem$free(allocator, ptr)

/// Checks if `s` value is power of 2
#define mem$is_power_of2(s)

/// Allocate uninitialized chunk of memory using `allocator`
#define mem$malloc(allocator, size, alignment...)

/// Allocates generic type instance using `allocator`, result is zero filled, size and alignment
/// derived from type T
#define mem$new(allocator, T)

/// Gets byte offset of a struct field
#define mem$offsetof(var, field)

/// Returns 32 for 32-bit platform, or 64 for 64-bit platform
#define mem$platform()

/// Reallocate chunk of memory using `allocator`
#define mem$realloc(allocator, old_ptr, size, alignment...)

/// Opens new memory scope using Arena-like allocator, frees all memory after scope exit
#define mem$scope(allocator, allc_var)




```
