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

- Arena Scope

```c
mem$arena(4096, arena)
{
    // This needs extra page
    u8* p2 = mem$malloc(arena, 10040);
    mem$scope(arena, tal)
    {
        u8* p3 = mem$malloc(tal, 100);
    }
}
```

- Arena Instance

```c
IAllocator arena = AllocatorArena.create(4096);

u8* p = mem$malloc(arena, 100); // direct use allowed

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
*/

#define __mem$

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

// clang-format off

/// Opens new memory scope using Arena-like allocator, frees all memory after scope exit
#define mem$scope(allocator, allc_var)                                                                                                                                           \
    u32 cex$tmpname(tallc_cnt) = 0;                                                                                                                                \
    for (IAllocator allc_var  \
        __attribute__ ((__cleanup__(_cex_allocator_memscope_cleanup))) =  \
                                                        (allocator)->scope_enter(allocator); \
        cex$tmpname(tallc_cnt) < 1; \
        cex$tmpname(tallc_cnt)++)

/// Creates new ArenaAllocator instance in scope, frees it at scope exit
#define mem$arena(page_size, allc_var)                                                                                                                                           \
    u32 cex$tmpname(tallc_cnt) = 0;                                                                                                                                \
    for (IAllocator allc_var  \
        __attribute__ ((__cleanup__(_cex_allocator_arena_cleanup))) =  \
                                                        AllocatorArena.create(page_size); \
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

#ifndef mem$asan_enabled
#        define mem$asan_enabled() 0
#endif
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
