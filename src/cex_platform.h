

#ifndef cex$platform_malloc
///  Macro for redefining default platform malloc()
#    define cex$platform_malloc malloc
#endif

#ifndef cex$platform_calloc
///  Macro for redefining default platform calloc()
#    define cex$platform_calloc calloc
#endif

#ifndef cex$platform_free
///  Macro for redefining default platform free()
#    define cex$platform_free free
#endif

#ifndef cex$platform_realloc
///  Macro for redefining default platform realloc()
#    define cex$platform_realloc realloc
#endif

#ifndef cex$platform_panic
///  Macro for redefining panic function (used in assertions, and other CEX stuff)
#    define cex$platform_panic __cex__panic
#    define _cex$platform_panic_builtin
__attribute__((noinline, noreturn)) void __cex__panic(void);
#endif

/// Internal: installs crash signal handlers (POSIX/Windows), no-op where unsupported
void __cex__catch_signals(void);

#ifdef cex$enable_minimal
#    undef cex$enable_minimal
/// Disables all key CEX capabilities, except core types, and macros, other functionality must be
/// re-enabled via `#define cex$enable_*`
#    define cex$enable_minimal

#else
/// Enables CEX memory allocators: tmem$, mem$
#    define cex$enable_mem

/// Enables CEX data structures: dynamic arrays, hashmaps, arr$* macros, for$each
#    define cex$enable_ds

/// Enables CEX string operations
#    define cex$enable_str

/// Enables CEX IO operations
#    define cex$enable_io

/// Enables CEX os namespace + argparse
#    define cex$enable_os

#endif
