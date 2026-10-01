#pragma once
#if !defined(cex$enable_minimal) || defined(cex$enable_mem)
#include "all.h"

/// Heap allocator block magic marker
#define CEX_ALLOCATOR_HEAP_MAGIC 0xF00dBa01
/// Temp allocator block magic marker
#define CEX_ALLOCATOR_TEMP_MAGIC 0xF00dBeef
/// Arena allocator block magic marker
#define CEX_ALLOCATOR_ARENA_MAGIC 0xFeedF001
/// Default page size (256 KB) for the temp allocator arena
#define CEX_ALLOCATOR_TEMP_PAGE_SIZE 1024 * 256



void _cex_allocator_memscope_cleanup(IAllocator* allc);
void _cex_allocator_arena_cleanup(IAllocator* allc);

/**
## Memory management

### Global allocators

- `mem$` - heap based allocator, typically used for long-living data, requires explicit mem$free
- `tmem$` - temporary allocator, backed by ArenaAllocator, with a 256KB page, requires `mem$scope`
- `test$alloc` - per-test-case arena (1 MB page, `disable_scopes`), created/destroyed by the test
runner, no manual free; `mem$scope` is a no-op; OOM simulation via
`test$alloc_set_oom_probability(prob)` (test mode only)

### Memory management hints

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


### Examples

- Vanilla heap allocator
```c
u8* p = mem$malloc(mem$, 100);
mem$free(mem$, p); // mem$free always nullifies the pointer (p == NULL)

p = mem$calloc(mem$, 100, 100, 32); // zeroed, 32-byte alignment
mem$free(mem$, p);

auto my_item = mem$new(mem$, struct my_type_s); // zero-initialized struct
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

- Arena scope

```c
mem$arena_scope(4096, arena)
{
    u8* p = mem$malloc(arena, 100);
}
```

- Arena instance

```c
// scoped arena (default): mem$scope() frees its allocations, destroy() frees the rest
// .backing_alloc overrides where arena pages come from (default: mem$);
// the backing allocator must outlive the arena
IAllocator arena = AllocatorArena.create(&(AllocatorArena_kw){ .page_size = 4096 });

u8* p = mem$malloc(arena, 100); // top-level allocation, freed at destroy()

mem$scope(arena, tal)
{
    u8* p2 = mem$malloc(tal, 100); // freed at this scope exit
}

AllocatorArena.destroy(arena); // must not be called inside mem$scope
// .disable_scopes = true makes mem$scope() a no-op; destroy() frees everything
```

*/

#define __mem$

#ifndef mem$asan_enabled
#    if defined(__has_feature)
#        if __has_feature(address_sanitizer)
/// true - if program was compiled with address sanitizer support
#            define mem$asan_enabled() 1
#        else
#            define mem$asan_enabled() 0
#        endif
#    else
#        if defined(__SANITIZE_ADDRESS__)
#            define mem$asan_enabled() 1
#        else
#            define mem$asan_enabled() 0
#        endif
#    endif
#endif // mem$asan_enabled

/// Temporary allocator arena (use only in `mem$scope(tmem$, _))`
#define tmem$ ((IAllocator)(&_cex__default_global__allocator_temp.alloc))

/// General purpose heap allocator
#define mem$ _cex__default_global__allocator_heap__allc

/// Allocate uninitialized chunk of memory using `allocator`
#define mem$malloc(allocator, size, alignment...)                                                  \
    ({                                                                                             \
        /* NOLINTBEGIN*/                                                                           \
        usize _alignment[] = { alignment };                                                        \
        (allocator)->malloc((allocator), size, (sizeof(_alignment) > 0) ? _alignment[0] : 0);      \
        /* NOLINTEND*/                                                                             \
    })

/// Allocate zero initialized chunk of memory using `allocator`
#define mem$calloc(allocator, nmemb, size, alignment...)                                           \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        usize _alignment[] = { alignment };                                                        \
        (allocator)                                                                                \
            ->calloc((allocator), nmemb, size, (sizeof(_alignment) > 0) ? _alignment[0] : 0);      \
        /* NOLINTEND*/                                                                             \
    })

/// Reallocate chunk of memory using `allocator`
#define mem$realloc(allocator, old_ptr, size, alignment...)                                        \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        usize _alignment[] = { alignment };                                                        \
        (allocator)                                                                                \
            ->realloc((allocator), old_ptr, size, (sizeof(_alignment) > 0) ? _alignment[0] : 0);   \
        /* NOLINTEND*/                                                                             \
    })

/// Free previously allocated chunk of memory, `ptr` implicitly set to NULL
#define mem$free(allocator, ptr)                                                                   \
    ({                                                                                             \
        (ptr) = (allocator)->free((allocator), ptr);                                               \
        (ptr) = NULL;                                                                              \
        (ptr);                                                                                     \
    })

/// Allocates generic type instance using `allocator`, result is zero filled, size and alignment
/// derived from type T
#define mem$new(allocator, T)                                                                      \
    (typeof(T)*)(allocator)->calloc((allocator), 1, sizeof(T), _Alignof(T))

/// Overflow-checked addition: computes a + b, stores result through *res. Returns true on overflow.
#define mem$add_overflow(a, b, res) __builtin_add_overflow((a), (b), (res))

/// Overflow-checked subtraction: computes a - b, stores result through *res. Returns true on overflow.
#define mem$sub_overflow(a, b, res) __builtin_sub_overflow((a), (b), (res))

/// Overflow-checked multiplication: computes a * b, stores result through *res. Returns true on overflow.
#define mem$mul_overflow(a, b, res) __builtin_mul_overflow((a), (b), (res))

// clang-format off

/// Opens new memory scope using Arena-like allocator, frees all memory after scope exit
#define mem$scope(allocator, allc_var)                                                                                                                                           \
    u32 cex$tmpname(tallc_cnt) = 0;                                                                                                                                \
    for (IAllocator allc_var  \
        __attribute__ ((__cleanup__(_cex_allocator_memscope_cleanup))) =  \
                                                        (allocator)->scope_enter(allocator); \
        cex$tmpname(tallc_cnt) < 1; \
        cex$tmpname(tallc_cnt)++)

/// Creates new ArenaAllocator instance in scope, frees it at scope exit.
/// First argument: integer `page_size` or `const AllocatorArena_kw*` pointer.
#define mem$arena_scope(ps, allc_var)                                                                                                                                           \
    u32 cex$tmpname(tallc_cnt) = 0;                                                                                                                                \
    for (IAllocator allc_var  \
        __attribute__ ((__cleanup__(_cex_allocator_arena_cleanup))) =  \
        ({                                                                                                                   \
            AllocatorArena_kw _mem$arena_kw_val = { .page_size = (usize)(ps), .disable_scopes = false };                    \
            const AllocatorArena_kw* _mem$arena_kw = _Generic((ps),                                                                                                  \
                const AllocatorArena_kw*: (ps),                                                                             \
                AllocatorArena_kw*: (ps),                                                                                   \
                default: &_mem$arena_kw_val                                                                                  \
            );                                                                                                               \
            AllocatorArena.create(_mem$arena_kw);                                                                            \
        });                                                                                                                   \
        cex$tmpname(tallc_cnt) < 1; \
        cex$tmpname(tallc_cnt)++)
// clang-format on

/// Checks if `s` value is power of 2
#define mem$is_power_of2(s) (((s) != 0) && (((s) & ((s) - 1)) == 0))

/// Rounds `size` to the closest alignment
#define mem$aligned_round(size, alignment)                                                         \
    (usize)((((usize)(size)) + ((usize)alignment) - 1) & ~(((usize)alignment) - 1))

/// Checks if pointer address of `p` is aligned to `alignment`
#define mem$aligned_pointer(p, alignment) (void*)mem$aligned_round(p, alignment)

/// Returns 32 for 32-bit platform, or 64 for 64-bit platform
#define mem$platform() __SIZEOF_SIZE_T__ * 8

/// Gets address of a struct member via a single-element array compound literal
#define mem$addressof(typevar, value) ((typeof(typevar)[1]){ (value) })

/// Gets byte offset of a struct field
#define mem$offsetof(var, field) ((char*)&(var)->field - (char*)(var))


#ifndef NDEBUG
#    ifndef CEX_DISABLE_POISON
#        define CEX_DISABLE_POISON 0
#    endif
#else // #ifndef NDEBUG
#    ifndef CEX_DISABLE_POISON
#        define CEX_DISABLE_POISON 1
#    endif
#endif


#ifdef CEX_TEST
#    define _mem$asan_poison_mark(addr, c, size) memset(addr, c, size)
#    define _mem$asan_poison_check_mark(addr, len)                                                 \
        ({                                                                                         \
            usize _len = (len);                                                                    \
            u8* _addr = (void*)(addr);                                                             \
            bool result = _addr != NULL && _len > 0;                                               \
            for (usize i = 0; i < _len; i++) {                                                     \
                if (_addr[i] != 0xf7) {                                                            \
                    result = false;                                                                \
                    break;                                                                         \
                }                                                                                  \
            }                                                                                      \
            result;                                                                                \
        })

#else // #ifdef CEX_TEST
#    define _mem$asan_poison_mark(addr, c, size) (void)0
#    define _mem$asan_poison_check_mark(addr, len) (1)
#endif

// NOTE: ASAN poisoning works inadequate on WASM
#if CEX_DISABLE_POISON  
#    define mem$asan_poison(addr, len)
#    define mem$asan_unpoison(addr, len)
#    define mem$asan_poison_check(addr, len) (1)
#else
void __asan_poison_memory_region(void const volatile* addr, size_t size);
void __asan_unpoison_memory_region(void const volatile* addr, size_t size);
void* __asan_region_is_poisoned(void* beg, size_t size);

#    if mem$asan_enabled() &&  !defined(__EMSCRIPTEN__)

/// Poisons memory region with ASAN, or fill it with 0xf7 byte pattern (no ASAN)
#        define mem$asan_poison(addr, size)                                                        \
            ({                                                                                     \
                void* _addr = (addr);                                                              \
                size_t _size = (size);                                                             \
                if (__asan_region_is_poisoned(_addr, (size)) == NULL) {                            \
                    _mem$asan_poison_mark(                                                         \
                        _addr,                                                                     \
                        0xf7,                                                                      \
                        _size                                                                      \
                    ); /* Marks are only enabled in CEX_TEST */                                    \
                }                                                                                  \
                __asan_poison_memory_region(_addr, _size);                                         \
            })

/// Unpoisons memory region with ASAN, or fill it with 0x00 byte pattern (no ASAN)
#        define mem$asan_unpoison(addr, size)                                                      \
            ({                                                                                     \
                void* _addr = (addr);                                                              \
                size_t _size = (size);                                                             \
                __asan_unpoison_memory_region(_addr, _size);                                       \
                _mem$asan_poison_mark(                                                             \
                    _addr,                                                                         \
                    0x00,                                                                          \
                    _size                                                                          \
                ); /* Marks are only enabled in CEX_TEST */                                        \
            })

/// Check if previously poisoned address is consistent, and 0x7f pattern not overwritten (no ASAN)
#        define mem$asan_poison_check(addr, size)                                                  \
            ({                                                                                     \
                void* _addr = addr;                                                                \
                __asan_region_is_poisoned(_addr, (size)) == _addr;                                 \
            })

#    else // #if defined(__SANITIZE_ADDRESS__)

#        define mem$asan_poison(addr, len) _mem$asan_poison_mark((addr), 0xf7, (len))
#        define mem$asan_unpoison(addr, len) _mem$asan_poison_mark((addr), 0x00, (len))

#        ifdef CEX_TEST
#            define mem$asan_poison_check(addr, len) _mem$asan_poison_check_mark((addr), (len))
#        else // #ifdef CEX_TEST
#            define mem$asan_poison_check(addr, len) (1)
#        endif

#    endif // #if defined(__SANITIZE_ADDRESS__)

#endif // #if CEX_DISABLE_POISON
#endif
