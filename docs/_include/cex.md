

- **GLOBAL CEX VARS / DEFINES / internals**

**NOTE**: run `cex config --help` for more information about configuration

```c

/// disables all asserts and safety checks (fast release mode)
#define NDEBUG

/// custom fprintf() function for asserts/logs/etc
#define __cex__fprintf(stream, prefix, filename, line, func, format, ...)

/// customize abort() behavior
#define __cex__abort()

/// customize uassert() behavior
#define __cex__assert()

/// Log verbosity level: 0=mute, 1=error, 2=warn, 3=info, 4=debug (default), 5=trace
#define CEX_LOG_LVL 4

/// disable ASAN memory poisoning and mem$asan_poison*
#define CEX_DISABLE_POISON 1

/// size of stack based buffer for small strings
#define CEX_SPRINTF_MIN 512

/// disables float printing for io.printf/et al functions (code size reduction)
#define CEX_SPRINTF_NOFLOAT

/// max AFL fuzzer input buffer size (default: 1024000)
#define CEX_FUZZ_MAX_BUF 1024000

/// enables AFL fuzzing mode (instead of libFuzzer)
#define CEX_FUZZ_AFL

/// max element byte-size for for$each value iteration (default: 64)
#define CEX_FOREACH_MAX_COPY_SIZE 64

/// temp allocator arena page size in bytes (default: 256 KB)
#define CEX_ALLOCATOR_TEMP_PAGE_SIZE 1024 * 256

/// max nesting depth for mem$scope() (default: 16)
#define CEX_ALLOCATOR_MAX_SCOPE_STACK 16

/// build-system mode flag (set automatically by cex.c)
#define CEX_BUILD

/// unit-test mode flag (enables extra checks and poison patterns)
#define CEX_TEST

/// new-project scaffolding mode (generates boilerplate)
#define CEX_NEW

/// single-header implementation mode (expands cex.h contents)
#define CEX_IMPLEMENTATION

/// skip CEX's own Win32 type/function declarations; let system <windows.h> provide them instead
/// use when your project includes <windows.h> (directly or via third-party libs like curl)
/// to avoid conflicts with CEX's predeclared Win32 types (e.g. _LARGE_INTEGER, _FILETIME)
#define CEX_NO_WIN32_TYPES

```



```c
/// Concatenate textually a##b
#define cex$concat(a, b)

/// Concatenate textually c##a##b
#define cex$concat3(c, a, b)

/// Enables CEX data structures: dynamic arrays, hashmaps, arr$* macros, for$each
#define cex$enable_ds

/// Enables CEX IO operations
#define cex$enable_io

/// Enables CEX memory allocators: tmem$, mem$
#define cex$enable_mem

/// Disables all key CEX capabilities, except core types, and macros, other functionality must be
/// re-enabled via `#define cex$enable_*`
#define cex$enable_minimal

/// Enables CEX os namespace + argparse
#define cex$enable_os

/// Enables CEX string operations
#define cex$enable_str

/// Set to 1 if current platform is freestanding (no OS, or libc)
#define cex$is_freestanding

/// Macro for redefining default platform calloc()
#define cex$platform_calloc

/// Macro for redefining default platform free()
#define cex$platform_free

/// Macro for redefining default platform malloc()
#define cex$platform_malloc

/// Macro for redefining panic function (used in assertions, and other CEX stuff)
#define cex$platform_panic

/// Macro for redefining default platform realloc()
#define cex$platform_realloc

/// Produces a literal string of any text inside the (...)
#define cex$stringize(...)

/// cex$tmpname - internal macro for generating temporary variable names (unique__line_num)
#define cex$tmpname(base)

/// makes a new variable with __cex__ prefix
#define cex$varname(a, b)

#define cex$version_date

#define cex$version_major

#define cex$version_minor

#define cex$version_patch

/// Code generator state: sbuf backing buffer, current indent level, and error state
typedef struct cex_codegen_s

/// Parsed declaration: name, docs, body, refined type/args, source location, and attributes
typedef struct cex_decl_s

/// Fuzz data fetcher (makes C-type payloads from random fuzz$case data)
typedef struct cex_fuzz_s

/// Generic iterator state (≤ 64 bytes). Used by `for$iter()` and custom iterator functions.
typedef struct cex_iterator_s

/// Token produced by the parser: a type tag + a slice into source content
typedef struct cex_token_s




```
