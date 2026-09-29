
### Memory management

#### Global allocators

- `mem$` - heap based allocator, typically used for long-living data, requires explicit mem$free
- `tmem$` - temporary allocator, backed by ArenaAllocator, with a 256KB page, requires `mem$scope`
- `test$alloc` - per-test-case arena (1 MB page, `disable_scopes`), created/destroyed by the test
runner, no manual free; `mem$scope` is a no-op; OOM simulation via
`test$alloc_set_oom_probability(prob)` (test mode only)

#### Memory management hints

- If a function accepts IAllocator as an argument, it allocates memory
- If a class/object accepts IAllocator in its constructor, it should track the allocator instance
- `mem$scope()` - frees memory at scope exit for any reason (`return`, `goto` out, `break`)
- consider `mem$malloc/mem$calloc/mem$realloc/mem$free/mem$new`
- You can init arena scope with `mem$arena_scope(page_size, arena_var_name)`
- AllocatorArena grows dynamically if there is no room in existing page, but be careful when you use
many `realloc()`, it can grow arenas unexpectedly large.
- Common CEX pattern: `mem$scope(tmem$, _) {}` — `_` is a short alias for `tmem$`
- Nested `mem$scope` are allowed, but memory is freed at the nested scope exit. NOTE: don't share
pointers across scopes.
- Never return a pointer allocated inside `mem$scope` (it's freed at scope exit)
- Never `realloc` a pointer from an outer scope inside a nested `mem$scope` (test mode asserts;
`arr$`/`hm$` resizing triggers it too)
- `mem$scope` is backed by a `for` loop: `break`/`continue` inside it exits the scope, not an outer
loop
- Arenas never reuse freed chunks; pre-allocate capacity instead of heavy `realloc`
- In test mode `mem$` tracks leaks, allocations are filled with `0xf7`, arenas are ASAN-poisoned;
switch `tmem$` to `mem$` to triage use-after-poison
- Use address sanitizers as often as possible


#### Examples

- Vanilla heap allocator
```c
u8* p = mem$malloc(mem$, 100);

// mem$free always nullifies the pointer
mem$free(mem$, p);
// p == NULL

p = mem$calloc(mem$, 100, 100, 32); // zeroed, 32-byte alignment
mem$free(mem$, p);

// Allocates a zero-initialized struct of the given type
auto my_item = mem$new(mem$, struct my_type_s);
mem$free(mem$, my_item);
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
mem$arena_scope(4096, arena)
{
    u8* p = mem$malloc(arena, 100);
}

// form 2 — AllocatorArena_kw pointer
mem$arena_scope(&(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true }, arena)
{
    u8* p = mem$malloc(arena, 100);
}
```

- Arena Instance

```c
// scoped arena (default): allocations are freed at mem$scope() exit
IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 });

u8* p = mem$malloc(arena, 100); // top-level allocation, freed at AllocatorArena.destroy()

mem$scope(arena, tal)
{
    u8* p2 = mem$malloc(tal, 100000); // freed at this scope exit

    mem$scope(arena, tal)
    {
        u8* p3 = mem$malloc(tal, 100); // freed at nested scope exit
    }
}

AllocatorArena.destroy(arena); // must not be called inside mem$scope

// manual mode: .disable_scopes = true makes mem$scope() a no-op, destroy() frees everything
IAllocator arena_manual =
    AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096, .disable_scopes = true });

u8* p4 = mem$malloc(arena_manual, 100); // direct use allowed

AllocatorArena.destroy(arena_manual);

```



```c
/// General purpose heap allocator
#define mem$

/// Overflow-checked addition: computes a + b, stores result through *res. Returns true on overflow.
#define mem$add_overflow(a, b, res)

/// Gets address of a struct member via a single-element array compound literal
#define mem$addressof(typevar, value)

/// Checks if pointer address of `p` is aligned to `alignment`
#define mem$aligned_pointer(p, alignment)

/// Rounds `size` to the closest alignment
#define mem$aligned_round(size, alignment)

/// Creates new ArenaAllocator instance in scope, frees it at scope exit.
/// First argument: integer `page_size` or `const AllocatorArena_kw*` pointer.
#define mem$arena_scope(ps, allc_var)

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

/// Overflow-checked multiplication: computes a * b, stores result through *res. Returns true on overflow.
#define mem$mul_overflow(a, b, res)

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

/// Overflow-checked subtraction: computes a - b, stores result through *res. Returns true on overflow.
#define mem$sub_overflow(a, b, res)




```
