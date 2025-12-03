#pragma once
#ifndef CEX_HEADER_H
#define CEX_HEADER_H
/*
# CEX.C - Comprehensively EXtended C Language (cex-c.org)
                                                                MOCCA - Make Old C Cexy Again!

>    MIT License 2023-2025 (c) Alex Veden (see license information at the end of this file)
>    https://github.com/alexveden/cex/

CEX is self-contained C language extension, the only dependency is one of gcc/clang compilers.
cex.h contains build system, unit test runner, small standard lib and help system.

Visit https://cex-c.org for more information

## GETTING STARTED
(existing project, when cex.c exists in the project root directory)
```
1. > cd project_dir
2. > gcc/clang ./cex.c -o ./cex     (need only once, then cex will rebuil itself)
3. > ./cex --help                   get info about available commands
```

## GETTING STARTED
(bare cex.h file, and nothing else)
```
1. > download https://cex-c.org/cex.h or copy existing one
2. > mkdir project_dir
3. > cd project_dir
4. > gcc/clang -D CEX_NEW -x c ./cex.h    prime cex.c and build system
5. > ./cex                                creates boilerplate project
6. > ./cex test run all                   runs sample unit tests
7. > ./cex app run myapp                  runs sample app
```

## cex tool usage:
```
> ./cex --help
Usage:
cex  [-D] [-D<ARG1>] [-D<ARG2>] command [options] [args]

CEX language (cexy$) build and project management system

help                Search cex.h and project symbols and extract help
process             Create CEX namespaces from project source code
new                 Create new CEX project
config              Check project and system environment and config
test                Test running
app                 App runner

You may try to get help for commands as well, try `cex process --help`
Use `cex -DFOO -DBAR config` to set project config flags
Use `cex -D config` to reset all project config flags to defaults

```
*/

/*
 *                  GLOBAL CEX VARS / DEFINES
 *
 * NOTE: run `cex config --help` for more information about configuration
 */

/// disables all asserts and safety checks (fast release mode)
// #define NDEBUG

/// custom fprintf() function for asserts/logs/etc
// #define __cex__fprintf(stream, prefix, filename, line, func, format, ...)

/// customize abort() behavior
// #define __cex__abort()

// customize uassert() behavior
// #define __cex__assert()

// you may override this level to manage log$* verbosity
// #define CEX_LOG_LVL 5

/// disable ASAN memory poisoning and mem$asan_poison*
// #define CEX_DISABLE_POISON 1

/// size of stack based buffer for small strings
// #define CEX_SPRINTF_MIN 512

/// disables float printing for io.printf/et al functions (code size reduction)
// #define CEX_SPRINTF_NOFLOAT

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__APPLE__) || defined(__MACH__)
// NOTE: Apple SDK defines sprintf as a macro, this messes str.sprintf() calls, because
//      sprintf() part is expanded as macro.
#    ifdef sprintf
#        undef sprintf
#    endif
#    ifdef vsprintf
#        undef vsprintf
#    endif
#endif

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__) || defined(__GNUG__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define cex$version_major 0
#define cex$version_minor 18
#define cex$version_patch 0
#define cex$version_date "2025-12-03"



/*
*                          src/cex_platform.h
*/


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
__attribute__((noinline)) void __cex__panic(void);
#endif

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



/*
*                          src/cex_base.h
*/
/**
 * @file cex/cex.h
 * @brief CEX core file
 */

/*
 *                 CORE TYPES
 */
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef size_t usize;
typedef ptrdiff_t isize;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L
/// automatic variable type, supported by GCC/Clang or C23
#    define auto __auto_type
#endif

// clang-format off
#define IAllocator const struct Allocator_i* 
typedef struct Allocator_i
{
    // >>> cacheline
    alignas(64) void* (*const malloc)(IAllocator self, usize size, usize alignment);
    void* (*const calloc)(IAllocator self, usize nmemb, usize size, usize alignment);
    void* (*const realloc)(IAllocator self, void* ptr, usize new_size, usize alignment);
    void* (*const free)(IAllocator self, void* ptr);
    const struct Allocator_i* (*const scope_enter)(IAllocator self);   /* Only for arenas/temp alloc! */
    void (*const scope_exit)(IAllocator self);    /* Only for arenas/temp alloc! */
    u32 (*const scope_depth)(IAllocator self);  /* Current mem$scope depth */
    struct {
        u32 magic_id;
        bool is_arena;
        bool is_temp;
    } meta;
    //<<< 64 byte cacheline
} Allocator_i;
// clang-format on
static_assert(alignof(Allocator_i) == 64, "size");
static_assert(sizeof(Allocator_i) == 64, "size");
static_assert(sizeof((Allocator_i){ 0 }.meta) == 8, "size");


/// Represents char* slice (string view) + may not be null-term at len!
typedef struct
{
    usize len;
    char* buf;
} str_s;

static_assert(alignof(str_s) == alignof(usize), "align");
static_assert(sizeof(str_s) == sizeof(usize) * 2, "size");


/**
 * @brief creates str_s, instance from string literals/constants: str$s("my string")
 *
 * Uses compile time string length calculation, only literals
 *
 */
#define str$s(string)                                                                              \
    (str_s){ .buf = /* WARNING: only literals!!!*/ "" string, .len = sizeof((string)) - 1 }

/*
 *                 BRANCH MANAGEMENT
 * `if(unlikely(condition)) {...}` is helpful for error management, to let compiler
 *  know that the scope inside in if() is less likely to occur (or exceptionally unlikely)
 *  this allows compiler to organize code with less failed branch predictions and faster
 *  performance overall.
 *
 *  Example:
 *  char* s = malloc(128);
 *  if(unlikely(s == NULL)) {
 *      printf("Memory error\n");
 *      abort();
 *  }
 */
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define likely(expr) __builtin_expect(!!(expr), 1)
#define fallthrough() __attribute__((fallthrough));

/*
 *                 ERRORS
 */

/**
CEX Error handling cheat sheet:

1. Errors can be any `char*`, or string literals.
2. EOK / Error.ok - is NULL, means no error
3. Exception return type forced to be checked by compiler
4. Error is built-in generic error type
5. Errors should be checked by pointer comparison, not string contents.
6. `e$` are helper macros for error handling
7.  DO NOT USE break/continue inside e\$except/e\$except_* scopes (these macros are for loops too)!


Generic errors:

```c
Error.ok = EOK;                       // Success
Error.memory = "MemoryError";         // memory allocation error
Error.io = "IOError";                 // IO error
Error.overflow = "OverflowError";     // buffer overflow
Error.argument = "ArgumentError";     // function argument error
Error.integrity = "IntegrityError";   // data integrity error
Error.exists = "ExistsError";         // entity or key already exists
Error.not_found = "NotFoundError";    // entity or key already exists
Error.skip = "ShouldBeSkipped";       // NOT an error, function result must be skipped
Error.empty = "EmptyError";           // resource is empty
Error.eof = "EOF";                    // end of file reached
Error.argsparse = "ProgramArgsError"; // program arguments empty or incorrect
Error.runtime = "RuntimeError";       // generic runtime error
Error.assert = "AssertError";         // generic runtime check
Error.os = "OSError";                 // generic OS check
Error.timeout = "TimeoutError";       // await interval timeout
Error.permission = "PermissionError"; // Permission denied
Error.try_again = "TryAgainError";    // EAGAIN / EWOULDBLOCK errno analog for async operations
```

```c

Exception
remove_file(char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;  // Empty of null file
    }
    if (!os.path.exists(path)) {
        return "Not exists" // literal error are allowed, but must be handled as strcmp()
    }
    if (str.eq(path, "magic.file")) {
        // Returns an Error.integrity and logs error at current line to stdout
        return e$raise(Error.integrity, "Removing magic file is not allowed!");
    }
    if (remove(path) < 0) {
        return strerror(errno); // using system error text (arbitrary!)
    }
    return EOK;
}

Exception read_file(char* filename) {
    e$assert(buff != NULL);

    int fd = 0;
    e$except_errno(fd = open(filename, O_RDONLY)) { return Error.os; }
    return EOK;
}

Exception do_stuff(char* filename) {
    // return immediately with error + prints traceback
    e$ret(read_file("foo.txt"));

    // jumps to label if read_file() fails + prints traceback
    e$goto(read_file(NULL), fail);

    // silent error handing without tracebacks
    e$except_silent (err, foo(0)) {

        // Nesting of error handlers is allowed
        e$except_silent (err, foo(2)) {
            return err;
        }

        // NOTE: `err` is address of char* compared with address Error.os (not by string contents!)
        if (err == Error.os) {
            // Special handing
            io.print("Ooops OS problem\n");
        } else {
            // propagate
            return err;
        }
    }
    return EOK;

fail:
    // TODO: cleanup here
    return Error.io;
}
```
*/
#define __e$

/// Generic CEX error is a char*, where NULL means success(no error)
typedef char* Exc;

/// Equivalent of Error.ok, execution success
#define EOK (Exc) NULL

/// Use `Exception` in function signatures, to force developer to check return value
/// of the function.
#define Exception Exc __attribute__((warn_unused_result))


/**
 * @brief Generic errors list, used as constant pointers, errors must be checked as
 * pointer comparison, not as strcmp() !!!
 */
extern const struct _CEX_Error_struct
{
    Exc ok; // NOTE: must be the 1st, same as EOK
    Exc memory;
    Exc io;
    Exc overflow;
    Exc argument;
    Exc integrity;
    Exc exists;
    Exc not_found;
    Exc skip;
    Exc empty;
    Exc eof;
    Exc argsparse;
    Exc runtime;
    Exc assert;
    Exc os;
    Exc timeout;
    Exc permission;
    Exc try_again;
} Error;

#ifndef __cex__fprintf

// NOTE: you may try to define our own fprintf
#    define __cex__fprintf(stream, prefix, filename, line, func, format, ...)                      \
        cexsp__fprintf(                                                                            \
            stream,                                                                                \
            "%s ( %s:%d %s() ) " format,                                                           \
            prefix,                                                                                \
            filename,                                                                              \
            line,                                                                                  \
            func,                                                                                  \
            ##__VA_ARGS__                                                                          \
        )

static inline bool
__cex__fprintf_dummy(void)
{
    return true; // WARN: must always return true!
}

#endif

#ifndef __FILE_NAME__
#    define __FILE_NAME__                                                                          \
        (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifndef _WIN32
#    define CEX_NAMESPACE __attribute__((visibility("hidden"))) extern const
#else
#    define CEX_NAMESPACE extern const
#endif

/**
Simple console logging engine:

- Prints file:line + log type: `[INFO]    ( file.c:14 cexy_fun() ) Message format: ./cex`
- Supports CEX formatting engine
- Can be regulated using compile time level, e.g. `#define CEX_LOG_LVL 4`


Log levels (CEX_LOG_LVL value):

- 0 - mute all including assert messages, tracebacks, errors
- 1 - allow log$error + assert messages, tracebacks
- 2 - allow log$warn
- 3 - allow log$info
- 4 - allow log$debug (default level if CEX_LOG_LVL is not set)
- 5 - allow log$trace

*/
#define __log$

#ifndef CEX_LOG_LVL
// LVL Value
// 0 - mute all including assert messages, tracebacks, errors
// 1 - allow log$error + assert messages, tracebacks
// 2 - allow log$warn
// 3 - allow log$info
// 4 - allow log$debug  (default level if CEX_LOG_LVL is not set)
// 5 - allow log$trace
// NOTE: you may override this level to manage log$* verbosity
#    define CEX_LOG_LVL 4
#endif

#if CEX_LOG_LVL > 0
/// Log error (when CEX_LOG_LVL > 0)
#    define log$error(format, ...)                                                                 \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[ERROR]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$error(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 1
/// Log warning  (when CEX_LOG_LVL > 1)
#    define log$warn(format, ...)                                                                  \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[WARN]   ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$warn(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 2
/// Log info  (when CEX_LOG_LVL > 2)
#    define log$info(format, ...)                                                                  \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[INFO]   ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$info(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 3
/// Log debug (when CEX_LOG_LVL > 3)
#    define log$debug(format, ...)                                                                 \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[DEBUG]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$debug(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 4
/// Log tace (when CEX_LOG_LVL > 4)
#    define log$trace(format, ...)                                                                 \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[TRACE]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$trace(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 0
#    define __cex__traceback(uerr, fail_func)                                                      \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[^STCK]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            "^^^^^ [%s] in function call `%s`\n",                                                  \
            uerr,                                                                                  \
            fail_func                                                                              \
        ))

/// Non disposable assert, returns Error.assert CEX exception when failed
#    define e$assert(A)                                                                             \
        ({                                                                                          \
            if (unlikely(!((A)))) {                                                                 \
                __cex__fprintf(stdout, "[ASSERT] ", __FILE_NAME__, __LINE__, __func__, "%s\n", #A); \
                return Error.assert;                                                                \
            }                                                                                       \
        })


/// Non disposable assert, returns Error.assert CEX exception when failed (supports formatting)
#    define e$assertf(A, format, ...)                                                              \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                __cex__fprintf(                                                                    \
                    stdout,                                                                        \
                    "[ASSERT] ",                                                                   \
                    __FILE_NAME__,                                                                 \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    format "\n",                                                                   \
                    ##__VA_ARGS__                                                                  \
                );                                                                                 \
                return Error.assert;                                                               \
            }                                                                                      \
        })
#else // #if CEX_LOG_LVL > 0
#    define __cex__traceback(uerr, fail_func) __cex__fprintf_dummy()
#    define e$assert(A)                                                                            \
        ({                                                                                         \
            if (unlikely(!((A)))) { return Error.assert; }                                         \
        })


#    define e$assertf(A, format, ...)                                                              \
        ({                                                                                         \
            if (unlikely(!((A)))) { return Error.assert; }                                         \
        })
#endif // #if CEX_LOG_LVL > 0


/**
 *                 ASSERTIONS MACROS
 */
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

#if mem$asan_enabled()
// This should be linked when gcc sanitizer enabled
void __sanitizer_print_stack_trace();
#    define sanitizer_stack_trace() __sanitizer_print_stack_trace()
#else
#    define sanitizer_stack_trace() ((void)(0))
#endif

#if defined(__clang_analyzer__)
#    include <assert.h>
#    define uassert(cond) assert(cond)
#    define uassertf(cond, format, ...) assert(cond)
#    define uassert_disable() ((void)0)
#    define uassert_enable() ((void)0)
#    define __cex_test_postmortem_exists() 0
#elif defined(NDEBUG)
#    define uassertf(cond, format, ...) ((void)(0))
#    define uassert(cond) ((void)(0))
#    define uassert_disable() ((void)0)
#    define uassert_enable() ((void)0)
#    define __cex_test_postmortem_exists() 0
#else

#    ifdef CEX_TEST
// this prevents spamming on stderr (i.e. cextest.h output stream in silent mode)
int __cex_test_uassert_enabled = 1;
#        define uassert_disable() __cex_test_uassert_enabled = 0
#        define uassert_enable() __cex_test_uassert_enabled = 1
#        define uassert_is_enabled() (__cex_test_uassert_enabled)
#    else
#        define uassert_disable()                                                                  \
            static_assert(false, "uassert_disable() allowed only when compiled with -DCEX_TEST")
#        define uassert_enable() (void)0
#        define uassert_is_enabled() true
#        define __cex_test_postmortem_ctx NULL
#        define __cex_test_postmortem_exists() 0
#        define __cex_test_postmortem_f(ctx)
#    endif // #ifdef CEX_TEST


/**
 * @def uassert(A)
 * @brief Custom assertion, with support of sanitizer call stack printout at failure.
 */
#    define uassert(A)                                                                             \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                __cex__fprintf(                                                                    \
                    (uassert_is_enabled() ? stderr : stdout),                                      \
                    "[ASSERT] ",                                                                   \
                    __FILE_NAME__,                                                                 \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    "%s\n",                                                                        \
                    #A                                                                             \
                );                                                                                 \
                if (uassert_is_enabled()) { cex$platform_panic(); }                                \
            }                                                                                      \
        })

#    define uassertf(A, format, ...)                                                               \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                __cex__fprintf(                                                                    \
                    (uassert_is_enabled() ? stderr : stdout),                                      \
                    "[ASSERT] ",                                                                   \
                    __FILE_NAME__,                                                                 \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    format "\n",                                                                   \
                    ##__VA_ARGS__                                                                  \
                );                                                                                 \
                if (uassert_is_enabled()) { cex$platform_panic(); }                                \
            }                                                                                      \
        })
#endif


#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#    undef unreachable
#endif

#ifdef NDEBUG
#    define unreachable() __builtin_unreachable()
#else
#    define unreachable()                                                                          \
        ({                                                                                         \
            __cex__fprintf(stderr, "[UNREACHABLE] ", __FILE_NAME__, __LINE__, __func__, "\n");     \
            cex$platform_panic();                                                                  \
            __builtin_unreachable();                                                               \
        })
#endif

#if defined(_WIN32) || defined(_WIN64)
#    define breakpoint() __debugbreak()
#elif defined(__APPLE__)
#    define breakpoint() __builtin_debugtrap()
#elif defined(__linux__) || defined(__unix__)
#    define breakpoint() __builtin_trap()
#else
#    warning "breakpoint() is not supported by this arch"
#    define breakpoint()
#endif

/// Concatenate textually c##a##b
#define cex$concat3(c, a, b) c##a##b

/// Concatenate textually a##b
#define cex$concat(a, b) a##b

#define _cex$stringize(...) #__VA_ARGS__

/// Produces a literal string of any text inside the (...)
#define cex$stringize(...) _cex$stringize(__VA_ARGS__)

/// makes a new variable with __cex__ prefix
#define cex$varname(a, b) cex$concat3(__cex__, a, b)

/// cex$tmpname - internal macro for generating temporary variable names (unique__line_num)
#define cex$tmpname(base) cex$varname(base, __LINE__)

/// raises an error, code: `return e$raise(Error.integrity, "ooops: %d", i);`
#define e$raise(return_uerr, error_msg, ...)                                                       \
    (log$error("[%s] " error_msg "\n", return_uerr, ##__VA_ARGS__), (return_uerr))

/// catches the error of function inside scope + prints traceback
#define e$except(_var_name, _func)                                                                 \
    for (Exc _var_name = _func;                                                                    \
         unlikely((_var_name != EOK) && (__cex__traceback(_var_name, #_func), 1));                 \
         _var_name = EOK)

#if defined(CEX_TEST) || defined(CEX_BUILD)
#    define e$except_silent(_var_name, _func) e$except (_var_name, _func)
#else
/// catches the error of function inside scope (without traceback)
#    define e$except_silent(_var_name, _func)                                                      \
        for (Exc _var_name = _func; unlikely(_var_name != EOK); _var_name = EOK)
#endif

/// catches the error of system function (if negative value + errno), prints errno error
#define e$except_errno(_expression)                                                                \
    for (int _tmp_errno = 0; unlikely(                                                             \
             ((_tmp_errno == 0) && ((_expression) < 0) && ((_tmp_errno = errno), 1) &&             \
              (log$error(                                                                          \
                   "`%s` failed errno: %d, msg: %s\n",                                             \
                   #_expression,                                                                   \
                   _tmp_errno,                                                                     \
                   strerror(_tmp_errno)                                                            \
               ),                                                                                  \
               1) &&                                                                               \
              (errno = _tmp_errno, 1))                                                             \
         );                                                                                        \
         _tmp_errno = 1)

/// catches the error is expression returned null
#define e$except_null(_expression)                                                                 \
    if (unlikely(((_expression) == NULL) && (log$error("`%s` returned NULL\n", #_expression), 1)))

/// catches the error is expression returned true
#define e$except_true(_expression)                                                                 \
    if (unlikely(((_expression)) && (log$error("`%s` returned non zero\n", #_expression), 1)))

/// immediately returns from function with _func error + prints traceback
#define e$ret(_func)                                                                               \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func), 1)                      \
         );                                                                                        \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    return cex$tmpname(__cex_err_traceback_)

/// `goto _label` when _func returned error + prints traceback
#define e$goto(_func, _label)                                                                      \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func), 1)                      \
         );                                                                                        \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    goto _label


#if defined(__GNUC__) && !defined(__clang__)
// NOTE: GCC < 12, has some weird warnings for arr$len temp pragma push + missing-field-initializers
#    if (__GNUC__ < 12)
#        pragma GCC diagnostic ignored "-Wsizeof-pointer-div"
#        pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#    endif
#endif

#if defined(__STDC_HOSTED__)
#    if __STDC_HOSTED__ == 0
/// Set to 1 if current platform is freestanding (no OS, or libc)
#        define cex$is_freestanding 1
#    else
#        define cex$is_freestanding 0
#    endif
#else
// If __STDC_HOSTED__ is not defined, we're likely freestanding
#    define cex$is_freestanding 1
#endif



/*
*                          src/mem.h
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_mem)

#define CEX_ALLOCATOR_HEAP_MAGIC 0xF00dBa01
#define CEX_ALLOCATOR_TEMP_MAGIC 0xF00dBeef
#define CEX_ALLOCATOR_ARENA_MAGIC 0xFeedF001
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

/// Gets address of struct member
#define mem$addressof(typevar, value) ((typeof(typevar)[1]){ (value) })

/// Gets offset in bytes of struct member
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



/*
*                          src/AllocatorHeap.h
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_mem)


typedef struct
{
    alignas(64) const Allocator_i alloc;
    // below goes sanity check stuff
    struct
    {
        u32 n_allocs;
        u32 n_reallocs;
        u32 n_free;
    } stats;
} AllocatorHeap_c;

static_assert(sizeof(AllocatorHeap_c) == 128, "size!");
static_assert(offsetof(AllocatorHeap_c, alloc) == 0, "base must be the 1st struct member");

extern AllocatorHeap_c _cex__default_global__allocator_heap;
extern IAllocator const _cex__default_global__allocator_heap__allc;

#endif



/*
*                          src/AllocatorArena.h
*/

#if !defined(cex$enable_minimal) || defined(cex$enable_mem)

#    define CEX_ALLOCATOR_MAX_SCOPE_STACK 16

typedef struct allocator_arena_page_s allocator_arena_page_s;


typedef struct
{
    alignas(64) const Allocator_i alloc;

    allocator_arena_page_s* last_page;
    usize used;

    usize page_size;
    u32 scope_depth; // current scope mark, used by mem$scope
    struct
    {
        usize bytes_alloc;
        usize bytes_realloc;
        usize bytes_free;
        u32 pages_created;
        u32 pages_free;
    } stats;

    // each mark is a `used` value at alloc.scope_enter()
    usize scope_stack[CEX_ALLOCATOR_MAX_SCOPE_STACK];

} AllocatorArena_c;

static_assert(sizeof(AllocatorArena_c) <= 256, "size!");
static_assert(offsetof(AllocatorArena_c, alloc) == 0, "base must be the 1st struct member");

typedef struct allocator_arena_page_s
{
    alignas(64) allocator_arena_page_s* prev_page;
    usize used_start; // as of AllocatorArena.used field
    u32 cursor;       // current allocated size of this page
    u32 capacity;     // max capacity of this page (excluding header)
    void* last_alloc; // last allocated pointer (viable for realloc)
    u8 __poison_area[(sizeof(usize) == 8 ? 32 : 44)]; // barrier of sanitizer poison + space reserve
    char data[];                                      // trailing chunk of data
} allocator_arena_page_s;
static_assert(sizeof(allocator_arena_page_s) == 64, "size!");
static_assert(alignof(allocator_arena_page_s) == 64, "align");
static_assert(offsetof(allocator_arena_page_s, data) == 64, "data must be aligned to 64");

typedef struct allocator_arena_rec_s
{
    u32 size;         // allocation size
    u8 ptr_padding;   // padding in bytes to next rec (also poisoned!)
    u8 ptr_alignment; // requested pointer alignment
    u8 is_free;       // indication that address has been free()'d
    u8 ptr_offset;    // byte offset for allocated pointer for this item
} allocator_arena_rec_s;
static_assert(sizeof(allocator_arena_rec_s) == 8, "size!");
static_assert(offsetof(allocator_arena_rec_s, ptr_offset) == 7, "ptr_offset must be last");

extern
#    if !cex$is_freestanding
    _Thread_local
#    endif
    AllocatorArena_c _cex__default_global__allocator_temp;

struct __cex_namespace__AllocatorArena
{
    // Autogenerated by CEX
    // clang-format off

    IAllocator      (*create)(usize page_size);
    void            (*destroy)(IAllocator self);
    bool            (*sanitize)(IAllocator allc);

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__AllocatorArena AllocatorArena;

#endif



/*
*                          src/ds.h
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_ds)

/**

* Creating array
```c
    // Using heap allocator (need to free later!)
    arr$(i32) array = arr$new(array, mem$);

    // adding elements
    arr$pushm(array, 1, 2, 3); // multiple at once
    arr$push(array, 4); // single element

    // length of array
    arr$len(array);

    // getting i-th elements
    array[1];

    // iterating array (by value)
    for$each(it, array) {
        io.printf("el=%d\n", it);
    }

    // iterating array (by pointer - prefer for bigger structs to avoid copying)
    for$eachp(it, array) {
        // TIP: making array index out of `it`
        usize i = it - array;

        // NOTE: 'it' now is a pointer
        io.printf("el[%zu]=%d\n", i, *it);
    }

    // free resources
    arr$free(array);
```

* Array of structs
```c

typedef struct
{
    int key;
    float my_val;
    char* my_string;
    int value;
} my_struct;

void somefunc(void)
{
    arr$(my_struct) array = arr$new(array, mem$, .capacity = 128);
    uassert(arr$cap(array), 128);

    my_struct s;
    s = (my_struct){ 20, 5.0, "hello ", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 2.5, "failure", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 1.1, "world!", 0 };
    arr$push(array, s);

    for (usize i = 0; i < arr$len(array); ++i) {
        io.printf("key: %d str: %s\n", array[i].key, array[i].my_string);
    }
    arr$free(array);

    return EOK;
}
```

*/
#define __arr$

// this is a simple string arena allocator, initialize with e.g. 'cexds_string_arena my_arena={0}'.
typedef struct _cexds__string_arena _cexds__string_arena;
extern char* _cexds__stralloc(_cexds__string_arena* a, char* str);
extern void _cexds__strreset(_cexds__string_arena* a);

///////////////
//
// Everything below here is implementation details
//


// clang-format off
struct _cexds__hm_new_kwargs_s;
struct _cexds__arr_new_kwargs_s;
struct _cexds__hash_index;
enum _CexDsKeyType_e
{
    _CexDsKeyType__generic,
    _CexDsKeyType__charptr,
    _CexDsKeyType__charbuf,
    _CexDsKeyType__cexstr,
};
extern void* _cexds__arrgrowf(void* a, usize elemsize, usize addlen, usize min_cap, u16 el_align, IAllocator allc);
extern void _cexds__arrfreef(void* a);
extern bool _cexds__arr_integrity(const void* arr, usize magic_num);
extern usize _cexds__arr_len(const void* arr);
extern void _cexds__hmfree_func(void* p, usize elemsize, usize keyoffset);
extern void _cexds__hmfree_keys_func(void* a, usize elemsize, usize keyoffset);
extern void _cexds__hmclear_func(struct _cexds__hash_index* t, struct _cexds__hash_index* old_table);
extern void* _cexds__hminit(usize elemsize, IAllocator allc, enum _CexDsKeyType_e key_type, u16 el_align, struct _cexds__hm_new_kwargs_s* kwargs);
extern void* _cexds__hmget_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset);
extern void* _cexds__hmput_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset, void* full_elem, void* result);
extern bool _cexds__hmdel_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset);
// clang-format on

#define _CEXDS_ARR_MAGIC 0xC001DAAD
#define _CEXDS_HM_MAGIC 0xF001C001


// cexds array alignment
// v malloc'd pointer                v-element 1
// |..... <_cexds__array_header>|====!====!====
//      ^ padding      ^cap^len ^         ^-element 2
//                              ^-- arr$ user space pointer (element 0)
//
typedef struct
{
    struct _cexds__hash_index* _hash_table;
    IAllocator allocator;
    u32 magic_num;
    u16 allocator_scope_depth;
    u16 el_align;
    usize capacity;
    usize length; // This MUST BE LAST before __poison_area
    u8 __poison_area[8];
} _cexds__array_header;
static_assert(alignof(_cexds__array_header) == alignof(usize), "align");
static_assert(
    sizeof(usize) == 8 ? sizeof(_cexds__array_header) == 48 : sizeof(_cexds__array_header) == 32,
    "size for x64 is 48 / for x32 is 32"
);

#define _cexds__header(t) ((_cexds__array_header*)(((char*)(t)) - sizeof(_cexds__array_header)))

/// Generic array type definition. Use arr$(int) myarr - defines new myarr variable, as int array
#define arr$(T) T*

struct _cexds__arr_new_kwargs_s
{
    usize capacity;
};
/// Array initialization: use arr$(int) arr = arr$new(arr, mem$, .capacity = , ...)
#define arr$new(a, allocator, kwargs...)                                                           \
    ({                                                                                             \
        static_assert(_Alignof(typeof(*a)) <= 64, "array item alignment too high");                \
        uassert(allocator != NULL);                                                                \
        struct _cexds__arr_new_kwargs_s _kwargs = { kwargs };                                      \
        (a) = (typeof(*a)*)_cexds__arrgrowf(                                                       \
            NULL,                                                                                  \
            sizeof(*a),                                                                            \
            _kwargs.capacity,                                                                      \
            0,                                                                                     \
            alignof(typeof(*a)),                                                                   \
            allocator                                                                              \
        );                                                                                         \
    })

/// Free resources for dynamic array (only needed if mem$ allocator was used)
#define arr$free(a) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), _cexds__arrfreef((a)), (a) = NULL)

/// Set array capacity and resize if needed
#define arr$setcap(a, n) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), arr$grow(a, 0, n))

/// Clear array contents
#define arr$clear(a) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), _cexds__header(a)->length = 0)

/// Returns current array capacity
#define arr$cap(a) ((a) ? (_cexds__header(a)->capacity) : 0)

/// Delete array elements by index (memory will be shifted, order preserved)
#define arr$del(a, i)                                                                              \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        memmove(&(a)[i], &(a)[(i) + 1], sizeof *(a) * (_cexds__header(a)->length - 1 - (i)));      \
        _cexds__header(a)->length--;                                                               \
    })

/// Delete element by swapping with last one (no memory overhear, element order changes)
#define arr$delswap(a, i)                                                                          \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        (a)[i] = arr$last(a);                                                                      \
        _cexds__header(a)->length -= 1;                                                            \
    })

/// Return last element of array
#define arr$last(a)                                                                                \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert(_cexds__header(a)->length > 0 && "empty array");                                   \
        (a)[_cexds__header(a)->length - 1];                                                        \
    })

/// Get element at index (bounds checking with uassert())
#define arr$at(a, i)                                                                               \
    ({                                                                                             \
        _cexds__arr_integrity(a, 0); /* may work also on hm$ */                                    \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        (a)[i];                                                                                    \
    })

/// Pop element from the end
#define arr$pop(a)                                                                                 \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        _cexds__header(a)->length--;                                                               \
        (a)[_cexds__header(a)->length];                                                            \
    })

/// Push element to the end
#define arr$push(a, value...)                                                                      \
    ({                                                                                             \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$push memory error");                                             \
            abort();                                                                               \
        }                                                                                          \
        (a)[_cexds__header(a)->length++] = (value);                                                \
    })

/// Push many elements to the end
#define arr$pushm(a, items...)                                                                     \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        typeof(*a) _args[] = { items };                                                            \
        static_assert(sizeof(_args) > 0, "You must pass at least one item");                       \
        arr$pusha(a, _args, arr$len(_args));                                                       \
        /* NOLINTEND */                                                                            \
    })

/// Push another array into a. array can be dynamic or static or pointer+len
#define arr$pusha(a, array, array_len...)                                                          \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassertf(array != NULL, "arr$pusha: array is NULL");                                       \
        usize _arr_len_va[] = { array_len };                                                       \
        usize arr_len = (sizeof(_arr_len_va) > 0) ? _arr_len_va[0] : arr$len(array);               \
        uassert(arr_len < PTRDIFF_MAX && "negative length or overflow");                           \
        if (unlikely(!arr$grow_check(a, arr_len))) {                                               \
            uassert(false && "arr$pusha memory error");                                            \
            abort();                                                                               \
        }                                                                                          \
        for (usize i = 0; i < arr_len; i++) { (a)[_cexds__header(a)->length++] = ((array)[i]); }   \
        /* NOLINTEND */                                                                            \
    })

/// Sort array with qsort() libc function
#define arr$sort(a, qsort_cmp)                                                                     \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        qsort((a), arr$len(a), sizeof(*a), qsort_cmp);                                             \
    })


/// Inserts element into array at index `i`
#define arr$ins(a, i, value...)                                                                    \
    do {                                                                                           \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$ins memory error");                                              \
            abort();                                                                               \
        }                                                                                          \
        _cexds__header(a)->length++;                                                               \
        uassert((usize)i < _cexds__header(a)->length && "i out of bounds");                        \
        memmove(&(a)[(i) + 1], &(a)[i], sizeof(*(a)) * (_cexds__header(a)->length - 1 - (i)));     \
        (a)[i] = (value);                                                                          \
    } while (0)

/// Check array capacity and return false on memory error
#define arr$grow_check(a, add_extra)                                                               \
    ((_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC) &&                                                \
      _cexds__header(a)->length + (add_extra) > _cexds__header(a)->capacity)                       \
         ? (arr$grow(a, add_extra, 0), a != NULL)                                                  \
         : true)

/// Grows array capacity
#define arr$grow(a, add_len, min_cap)                                                              \
    ((a) = _cexds__arrgrowf((a), sizeof *(a), (add_len), (min_cap), alignof(typeof(*a)), NULL))


#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 12)
#    define arr$len(arr)                                                                           \
        ({                                                                                         \
            __builtin_types_compatible_p(                                                          \
                typeof(arr),                                                                       \
                typeof(&(arr)[0])                                                                  \
            )                          /* check if array or ptr */                                 \
                ? _cexds__arr_len(arr) /* some pointer or arr$ */                                  \
                : (                                                                                \
                      sizeof(arr) / sizeof((arr)[0]) /* static array[] */                          \
                  );                                                                               \
        })
#else
/// Versatile array length, works with dynamic (arr$) and static compile time arrays
#    define arr$len(arr)                                                                           \
        ({                                                                                         \
            _Pragma("GCC diagnostic push");                                                        \
            /* NOTE: temporary disable syntax error to support both static array length and        \
             * arr$(T) */                                                                          \
            _Pragma("GCC diagnostic ignored \"-Wsizeof-pointer-div\"");                            \
            /* NOLINTBEGIN */                                                                      \
            __builtin_types_compatible_p(                                                          \
                typeof(arr),                                                                       \
                typeof(&(arr)[0])                                                                  \
            )                          /* check if array or ptr */                                 \
                ? _cexds__arr_len(arr) /* some pointer or arr$ */                                  \
                : (                                                                                \
                      sizeof(arr) / sizeof((arr)[0]) /* static array[] */                          \
                  );                                                                               \
            /* NOLINTEND */                                                                        \
            _Pragma("GCC diagnostic pop");                                                         \
        })
#endif

static inline void*
_cex__get_buf_addr(void* a)
{
    return (a != NULL) ? &((char*)a)[0] : NULL;
}


/*
 *                  ARRAYS ITERATORS INTERFACE
 */

/**

- using for$ as unified array iterator

```c

test$case(test_array_iteration)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    for$each(it, array) {
        io.printf("el=%d\n", it);
    }
    // Prints:
    // el=1
    // el=2
    // el=3

    // NOTE: prefer this when you work with bigger structs to avoid extra memory copying
    for$eachp(it, array) {
        // TIP: making array index out of `it`
        usize i = it - array;

        // NOTE: it now is a pointer
        io.printf("el[%zu]=%d\n", i, *it);
    }
    // Prints:
    // el[0]=1
    // el[1]=2
    // el[2]=3

    // Static arrays work as well (arr$len inferred)
    i32 arr_int[] = {1, 2, 3, 4, 5};
    for$each(it, arr_int) {
        io.printf("static=%d\n", it);
    }
    // Prints:
    // static=1
    // static=2
    // static=3
    // static=4
    // static=5


    // Simple pointer+length also works (let's do a slice)
    i32* slice = &arr_int[2];
    for$each(it, slice, 2) {
        io.printf("slice=%d\n", it);
    }
    // Prints:
    // slice=3
    // slice=4


    // it is type of cex_iterator_s
    // NOTE: run in shell: ➜ ./cex help cex_iterator_s
    s = str.sstr("123,456");
    for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {
        io.printf("it.value = %S\n", it.val);
    }
    // Prints:
    // it.value = 123
    // it.value = 456

    arr$free(array);
    return EOK;
}

```

*/
#define __for$

/**
 * @brief cex_iterator_s - CEX iterator interface
 */
typedef struct
{
    struct
    {
        union
        {
            usize i;
            char* skey;
            void* pkey;
        };
    } idx;
    char _ctx[47];
    u8 stopped;
    u8 initialized;
} cex_iterator_s;
static_assert(sizeof(usize) == sizeof(void*), "usize expected as sizeof ptr");
static_assert(alignof(usize) == alignof(void*), "alignof pointer != alignof usize");
static_assert(alignof(cex_iterator_s) == alignof(void*), "alignof");
static_assert(sizeof(cex_iterator_s) <= 64, "cex size");

/**
 * @brief Iterates via iterator function (see usage below)
 *
 * Iterator function signature:
 * u32* array_iterator(u32 array[], u32 len, cex_iterator_s* iter)
 *
 * for$iter(u32, it, array_iterator(arr2, arr$len(arr2), &it.iterator))
 */
#define for$iter(it_val_type, it, iter_func)                                                       \
    struct cex$tmpname(__cex_iter_)                                                                \
    {                                                                                              \
        it_val_type val;                                                                           \
        union /* NOTE:  iterator above and this struct shadow each other */                        \
        {                                                                                          \
            cex_iterator_s iterator;                                                               \
            struct                                                                                 \
            {                                                                                      \
                union                                                                              \
                {                                                                                  \
                    usize i;                                                                       \
                    char* skey;                                                                    \
                    void* pkey;                                                                    \
                };                                                                                 \
            } idx;                                                                                 \
        };                                                                                         \
    };                                                                                             \
                                                                                                   \
    for (struct cex$tmpname(__cex_iter_) it = { .val = (iter_func) }; !it.iterator.stopped;        \
         it.val = (iter_func))


/// Iterates over arrays `it` is iterated **value**, array may be arr$/or static / or pointer,
/// array_len is only required for pointer+len use case
#define for$each(it, array, array_len...)                                                          \
    /* NOLINTBEGIN*/                                                                               \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    typeof((array)[0])* cex$tmpname(arr_arrp) = _cex__get_buf_addr(array);                         \
    usize cex$tmpname(arr_index) = 0;                                                              \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    for (typeof((array)[0]) it = { 0 };                                                            \
         (cex$tmpname(arr_index) < cex$tmpname(arr_length) &&                                      \
          ((it) = cex$tmpname(arr_arrp)[cex$tmpname(arr_index)], 1));                              \
         cex$tmpname(arr_index)++)
    /* NOLINTEND */                                                                                \


/// Iterates over arrays `it` is iterated by **pointer**, array may be arr$/or static / or pointer,
/// array_len is only required for pointer+len use case
#define for$eachp(it, array, array_len...)                                                         \
    /* NOLINTBEGIN*/                                                                               \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    typeof((array)[0])* cex$tmpname(arr_arrp) = _cex__get_buf_addr(array);                         \
    usize cex$tmpname(arr_index) = 0;                                                              \
    for (typeof((array)[0])* it = cex$tmpname(arr_arrp);                                           \
         cex$tmpname(arr_index) < cex$tmpname(arr_length);                                         \
         cex$tmpname(arr_index)++, it++)
    /* NOLINTEND */                                                                                \

/*
 *                 HASH MAP
 */

/**
Generic type-safe hashmap

Principles:

1. Data is backed by engine similar to arr$
2. `arr$len()` works with hashmap too
3. Array indexing works with hashmap
4. `for$each`/`for$eachp` is applicable
5. `hm$` generic type is essentially a struct with `key` and `value` fields
6. `hm$` supports following keys: numeric (by default it's just binary representation), char*,
char[N], str_s (CEX sting slice).
7. `hm$` with string keys are stored without copy, use  `hm$new(hm, mem$, .copy_keys = true)` for
copy-mode.
8. `hm$` can store string keys inside an Arena allocator when  `hm$new(hm, mem$, .copy_keys = true,
.copy_keys_arena_pgsize = NNN)`


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

*/
#define __hm$

/// Defines hashmap generic type
#define hm$(_KeyType, _ValType)                                                                    \
    struct                                                                                         \
    {                                                                                              \
        _KeyType key;                                                                              \
        _ValType value;                                                                            \
    }*

/// Defines hashmap type based on _StructType, must have `key` field
#define hm$s(_StructType) _StructType*

/// hm$new(kwargs...) - default values always zeroed (ZII)
struct _cexds__hm_new_kwargs_s
{
    usize capacity; // initial hashmap capacity (default: 16)
    usize seed; // initial hashmap hash algorithm seed: (default: some const value)
    u32 copy_keys_arena_pgsize; // use arena for backing string keys copy (default: false)
    bool copy_keys; // duplicate/copy string keys when adding new records (default: false)
};


/// Creates new hashmap of hm$(KType, VType) using allocator, kwargs: .capacity, .seed,
/// .copy_keys_arena_pgsize, .copy_keys
#define hm$new(t, allocator, kwargs...)                                                            \
    ({                                                                                             \
        static_assert(_Alignof(typeof(*t)) <= 64, "hashmap record alignment too high");            \
        uassert(allocator != NULL);                                                                \
        enum _CexDsKeyType_e _key_type = _Generic(                                                 \
            &((t)->key),                                                                           \
            str_s *: _CexDsKeyType__cexstr,                                                        \
            char(**): _CexDsKeyType__charptr,                                                      \
            const char(**): _CexDsKeyType__charptr,                                                \
            char (*)[]: _CexDsKeyType__charbuf,                                                    \
            const char (*)[]: _CexDsKeyType__charbuf,                                              \
            default: _CexDsKeyType__generic                                                        \
        );                                                                                         \
        struct _cexds__hm_new_kwargs_s _kwargs = { kwargs };                                       \
        (t) = (typeof(*t)*)                                                                        \
            _cexds__hminit(sizeof(*t), (allocator), _key_type, alignof(typeof(*t)), &_kwargs);     \
    })


/// Set hashmap key/value, replaces if exists 
#define hm$set(t, k, v...)                                                                         \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = _cexds__hmput_key(                                                                   \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key),      /* offset of key in hm struct */                       \
            NULL,                           /* no full element set */                              \
            &result                         /* NULL on memory error */                             \
        );                                                                                         \
        if (result) result->value = (v);                                                           \
        result;                                                                                    \
    })

/// Add new item and returns pointer of hashmap record for `k`, for further editing  
#define hm$setp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = _cexds__hmput_key(                                                                   \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key),      /* offset of key in hm struct */                       \
            NULL,                           /* no full element set */                              \
            &result                         /* NULL on memory error */                             \
        );                                                                                         \
        (result ? &result->value : NULL);                                                          \
    })

/// Set full record, must be initialized by user
#define hm$sets(t, v...)                                                                           \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        typeof(*t) _val = (v);                                                                     \
        (t) = _cexds__hmput_key(                                                                   \
            (t),                                                                                   \
            sizeof(*t),                /* size of hashmap item */                                  \
            &_val.key,                 /* temp on stack pointer to (k) value */                    \
            sizeof((t)->key),          /* size of key */                                           \
            offsetof(typeof(*t), key), /* offset of key in hm struct */                            \
            &(_val),                   /* full element write */                                    \
            &result                    /* NULL on memory error */                                  \
        );                                                                                         \
        result;                                                                                    \
    })

/// Get item by value, def - default value (zeroed by default), can be any type
#define hm$get(t, k, def...)                                                                       \
    ({                                                                                             \
        typeof(t) result = _cexds__hmget_key(                                                      \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key)       /* offset of key in hm struct */                       \
        );                                                                                         \
        typeof((t)->value) _def[1] = { def }; /* default value, always 0 if def... is empty! */    \
        result ? result->value : _def[0];                                                          \
    })

/// Get item by pointer (no copy, direct pointer inside hashmap)
#define hm$getp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = _cexds__hmget_key(                                                      \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
        result ? &result->value : NULL;                                                            \
    })

/// Get a pointer to full hashmap record, NULL if not found
#define hm$gets(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = _cexds__hmget_key(                                                      \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
        result;                                                                                    \
    })

/// Clears hashmap contents
#define hm$clear(t)                                                                                \
    ({                                                                                             \
        _cexds__arr_integrity(t, _CEXDS_HM_MAGIC);                                                 \
        _cexds__hmfree_keys_func((t), sizeof(*t), offsetof(typeof(*t), key));                      \
        _cexds__hmclear_func(_cexds__header((t))->_hash_table, NULL);                              \
        _cexds__header(t)->length = 0;                                                             \
        true;                                                                                      \
    })

/// Deletes items, IMPORTANT hashmap array may be reordered after this call
#define hm$del(t, k)                                                                               \
    ({                                                                                             \
        _cexds__hmdel_key(                                                                         \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
    })


/// Frees hashmap resources
#define hm$free(t) (_cexds__hmfree_func((t), sizeof *(t), offsetof(typeof(*t), key)), (t) = NULL)

/// Returns hashmap length, also you can use arr$len()
#define hm$len(t)                                                                                  \
    ({                                                                                             \
        if (t != NULL) { _cexds__arr_integrity(t, _CEXDS_HM_MAGIC); }                              \
        (t) ? _cexds__header((t))->length : 0;                                                     \
    })

typedef struct _cexds__string_block
{
    struct _cexds__string_block* next;
    char storage[8];
} _cexds__string_block;

struct _cexds__string_arena
{
    _cexds__string_block* storage;
    usize remaining;
    unsigned char block;
    unsigned char mode; // this isn't used by the string arena itself
};

enum
{
    _CEXDS_SH_NONE,
    _CEXDS_SH_DEFAULT,
    _CEXDS_SH_STRDUP,
    _CEXDS_SH_ARENA
};

#define _cexds__shmode_func_wrapper(t, e, m) _cexds__shmode_func(e, m)
#endif



/*
*                          src/_sprintf.h
*/
// stb_sprintf - v1.10 - public domain snprintf() implementation
// originally by Jeff Roberts / RAD Game Tools, 2015/10/20
// http://github.com/nothings/stb
//
// allowed types:  sc uidBboXx p AaGgEef n
// lengths      :  hh h ll j z t I64 I32 I
//
// Contributors:
//    Fabian "ryg" Giesen (reformatting)
//    github:aganm (attribute format)
//
// Contributors (bugfixes):
//    github:d26435
//    github:trex78
//    github:account-login
//    Jari Komppa (SI suffixes)
//    Rohit Nirmal
//    Marcin Wojdyr
//    Leonard Ritter
//    Stefano Zanotti
//    Adam Allison
//    Arvid Gerstmann
//    Markus Kolb
//
// LICENSE:
//
//   See end of file for license information.


/*
Single file sprintf replacement.

Originally written by Jeff Roberts at RAD Game Tools - 2015/10/20.
Hereby placed in public domain.

This is a full sprintf replacement that supports everything that
the C runtime sprintfs support, including float/double, 64-bit integers,
hex floats, field parameters (%*.*d stuff), length reads backs, etc.

It compiles to roughly 8K with float support, and 4K without.
As a comparison, when using MSVC static libs, calling sprintf drags
in 16K.


FLOATS/DOUBLES:
===============
This code uses a internal float->ascii conversion method that uses
doubles with error correction (double-doubles, for ~105 bits of
precision).  This conversion is round-trip perfect - that is, an atof
of the values output here will give you the bit-exact double back.

If you don't need float or doubles at all, define STB_SPRINTF_NOFLOAT
and you'll save 4K of code space.

64-BIT INTS:
============
This library also supports 64-bit integers and you can use MSVC style or
GCC style indicators (%I64d or %lld).  It supports the C99 specifiers
for size_t and ptr_diff_t (%jd %zd) as well.

EXTRAS:
=======
Like some GCCs, for integers and floats, you can use a ' (single quote)
specifier and commas will be inserted on the thousands: "%'d" on 12345
would print 12,345.

For integers and floats, you can use a "$" specifier and the number
will be converted to float and then divided to get kilo, mega, giga or
tera and then printed, so "%$d" 1000 is "1.0 k", "%$.2d" 2536000 is
"2.53 M", etc. For byte values, use two $:s, like "%$$d" to turn
2536000 to "2.42 Mi". If you prefer JEDEC suffixes to SI ones, use three
$:s: "%$$$d" -> "2.42 M". To remove the space between the number and the
suffix, add "_" specifier: "%_$d" -> "2.53M".

In addition to octal and hexadecimal conversions, you can print
integers in binary: "%b" for 256 would print 100.
*/

// SETTINGS

// #define CEX_SPRINTF_NOFLOAT // disables floating point code (2x less in size)
#ifndef CEX_SPRINTF_MIN
#    define CEX_SPRINTF_MIN 512 // size of stack based buffer for small strings
#endif

// #define CEXSP_STATIC   // makes all functions static

#ifdef CEXSP_STATIC
#    define CEXSP__PUBLICDEC static
#    define CEXSP__PUBLICDEF static
#else
#    define CEXSP__PUBLICDEC extern
#    define CEXSP__PUBLICDEF
#endif

#define CEXSP__ATTRIBUTE_FORMAT(fmt, va) __attribute__((format(printf, fmt, va)))

typedef char* cexsp_callback_f(char* buf, void* user, u32 len);

typedef struct cexsp__context
{
    char* buf;
    FILE* file;
    IAllocator allc;
    u32 capacity;
    u32 length;
    u32 has_error;
    char tmp[CEX_SPRINTF_MIN];
} cexsp__context;

// clang-format off
CEXSP__PUBLICDEF int cexsp__vfprintf(FILE* stream, const char* format, va_list va);
CEXSP__PUBLICDEF int cexsp__fprintf(FILE* stream, const char* format, ...);
CEXSP__PUBLICDEC int cexsp__vsnprintf(char* buf, int count, char const* fmt, va_list va);
CEXSP__PUBLICDEC int cexsp__snprintf(char* buf, int count, char const* fmt, ...) CEXSP__ATTRIBUTE_FORMAT(3, 4);
CEXSP__PUBLICDEC int cexsp__vsprintfcb(cexsp_callback_f* callback, void* user, char* buf, char const* fmt, va_list va);
CEXSP__PUBLICDEC void cexsp__set_separators(char comma, char period);
// clang-format on



/*
*                          src/str.h
*/
/**
 * @file
 * @brief
 */

#if !defined(cex$enable_minimal) || defined(cex$enable_str)


/// Joins parts of strings using a separator str$join(allc, ",", "a", "b", "c") -> "a,b,c"
#define str$join(allocator, str_join_by, str_parts...)                                             \
    ({                                                                                             \
        char* _args[] = { str_parts };                                                             \
        usize _args_len = arr$len(_args);                                                          \
        str.join(_args, _args_len, str_join_by, allocator);                                        \
    })

/// Parses string contents as value type based on generic numeric type of out_var_ptr
#define str$convert(str_or_slice, out_var_ptr)                                                     \
    _Generic((str_or_slice), \
    char*: _Generic((out_var_ptr), \
        i8*:  str.convert.to_i8, \
        u8*:  str.convert.to_u8, \
        i16*:  str.convert.to_i16, \
        u16*:  str.convert.to_u16, \
        i32*:  str.convert.to_i32, \
        u32*:  str.convert.to_u32, \
        i64*:  str.convert.to_i64, \
        u64*:  str.convert.to_u64, \
        f32*:  str.convert.to_f32, \
        f64*:  str.convert.to_f64 \
    ), \
    str_s: _Generic((out_var_ptr), \
        i8*:  str.convert.to_i8s, \
        u8*:  str.convert.to_u8s, \
        i16*:  str.convert.to_i16s, \
        u16*:  str.convert.to_u16s, \
        i32*:  str.convert.to_i32s, \
        u32*:  str.convert.to_u32s, \
        i64*:  str.convert.to_i64s, \
        u64*:  str.convert.to_u64s, \
        f32*:  str.convert.to_f32s, \
        f64*:  str.convert.to_f64s \
    ) \
)(str_or_slice, out_var_ptr)

/**

CEX string principles:

- `str` namespace is build for compatibility with C strings
- all string functions are NULL resilient
- all string functions can return NULL on error
- you don't have to check every operation for NULL every time, just at the end
- all string format operations support CEX specific specificators (see below)

String slices:

- Slices are backed by `(str_s){.buf = s, .len = NNN}` struct
- Slices are passed by value and allocated on stack
- Slices can be made from null-terminated strings, or buffers, or literals
- str$s("hello") - use this for compile time defined slices/constants
- Slices are not guaranteed to be null-terminated
- Slices support operations which allowed by read-only string view representation
- CEX formatting uses `%S` for slices: `io.print("Hello %S\n", str$s("world"))`


- Working with slices:
```c

test$case(test_cstr)
{
    char* cstr = "hello";
    str_s s = str.sstr(cstr);
    tassert_eq(s.buf, cstr);
    tassert_eq(s.len, 5);
    tassert(s.buf == cstr);
    tassert_eq(str.len(s.buf), 5);
}

```

- Getting substring as slices
```c

str.sub("123456", 0, 0); // slice: 123456
str.sub("123456", 1, 0); // slice: 23456
str.sub("123456", 1, -1); // slice: 2345
str.sub("123456", -3, -1); // slice: 345
str.sub("123456", -30, 2000); // slice: (str_s){.buf = NULL, .len = 0} (error, but no crash)

// works with slices too
str_s s = str.sstr("123456");
str_s sub = str.slice.sub(s, 1, 2);

```

- Splitting / iterating via tokens

```c

// Working without mem allocation
s = str.sstr("123,456");
for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {
    io.printf("%S\n", it.val); // NOTE: it.val is non null-terminated slice
}


// Mem allocating split
mem$scope(tmem$, _)
{

    // NOTE: each `res` item will be allocated C-string, use tmem$ or deallocate independently
    arr$(char*) res = str.split("123,456,789", ",", _);
    tassert(res != NULL); // NULL on error

    for$each (v, res) {
        io.printf("%s\n", v); // NOTE: strings now cloned and null-terminated
    }
}

```

- Chaining string operations
```c

mem$scope(tmem$, _)
{
    char* s = str.fmt(_, "hi there"); // NULL on error
    s = str.replace(s, "hi", "hello", _); // NULL tolerant, NULL on error
    s = str.fmt(_, "result is: %s", s); // NULL tolerant, NULL on error
    if (s == NULL) {
        // TODO: oops error occurred, in one of three operations, but we don't need to check each one
    }

    tassert_eq(s, "result is: hello there");
}
```

- Pattern matching

```c
// Pattern matching 101
// * - one or more characters
// ? - one character
// [abc] - one character a or b or c
// [!abc] - one character, but not a or b or c
// [abc+] - one or more characters a or b or c
// [a-cA-C0-9] - one character in a range of characters
// \\* - escaping literal '*'
// (abc|def|xyz) - matching combination of words abc or def or xyz

tassert(str.match("test.txt", "*?txt"));
tassert(str.match("image.png", "image.[jp][pn]g"));
tassert(str.match("backup.txt", "[!a]*.txt"));
tassert(!str.match("D", "[a-cA-C0-9]"));
tassert(str.match("1234567890abcdefABCDEF", "[0-9a-fA-F+]"));
tassert(str.match("create", "(run|build|create|clean)"));


// Works with slices
str_s src = str$s("my_test __String.txt");
tassert(str.slice.match(src, "*"));
tassert(str.slice.match(src, "*.txt*"));
tassert(str.slice.match(src, "my_test*.txt"));

```

*/

struct __cex_namespace__str
{
    // Autogenerated by CEX
    // clang-format off

    /// Clones string using allocator, null tolerant, returns NULL on error.
    char*           (*clone)(char* s, IAllocator allc);
    /// Makes a copy of initial `src`, into `dest` buffer constrained by `destlen`. NULL tolerant,
    /// always null-terminated, overflow checked.
    Exception       (*copy)(char* dest, char* src, usize destlen);
    /// Checks if string ends with prefix, returns false on error, NULL tolerant
    bool            (*ends_with)(char* s, char* suffix);
    /// Compares two null-terminated strings (null tolerant)
    bool            (*eq)(char* a, char* b);
    /// Compares two strings, case insensitive, null tolerant
    bool            (*eqi)(char* a, char* b);
    /// Find a substring in a string, returns pointer to first element. NULL tolerant, and NULL on err.
    char*           (*find)(char* haystack, char* needle);
    /// Find substring from the end , NULL tolerant, returns NULL on error.
    char*           (*findr)(char* haystack, char* needle);
    /// Formats string and allocates it dynamically using allocator, supports CEX format engine
    char*           (*fmt)(IAllocator allc, char* format,...);
    /// Joins string using a separator (join_by), NULL tolerant, returns NULL on error.
    char*           (*join)(char** str_arr, usize str_arr_len, char* join_by, IAllocator allc);
    /// Calculates string length, NULL tolerant.
    usize           (*len)(char* s);
    /// Returns new lower case string, returns NULL on error, null tolerant
    char*           (*lower)(char* s, IAllocator allc);
    /// String pattern matching check (see ./cex help str$ for examples)
    bool            (*match)(char* s, char* pattern);
    /// libc `qsort()` comparator functions, for arrays of `char*`, sorting alphabetical
    int             (*qscmp)(const void* a, const void* b);
    /// libc `qsort()` comparator functions, for arrays of `char*`, sorting alphabetical case insensitive
    int             (*qscmpi)(const void* a, const void* b);
    /// Replaces substring occurrence in a string
    char*           (*replace)(char* s, char* old_sub, char* new_sub, IAllocator allc);
    /// Creates string slice from a buf+len
    str_s           (*sbuf)(char* s, usize length);
    /// Splits string using split_by (allows many) chars, returns new dynamic array of split char*
    /// tokens, allocates memory with allc, returns NULL on error. NULL tolerant. Items of array are
    /// cloned, so you need free them independently or better use arena or tmem$.
    arr$(char*)     (*split)(char* s, char* split_by, IAllocator allc);
    /// Splits string by lines, result allocated by allc, as dynamic array of cloned lines, Returns NULL
    /// on error, NULL tolerant. Items of array are cloned, so you need free them independently or
    /// better use arena or tmem$. Supports \n or \r\n.
    arr$(char*)     (*split_lines)(char* s, IAllocator allc);
    /// Analog of sprintf() uses CEX sprintf engine. NULL tolerant, overflow safe.
    Exc             (*sprintf)(char* dest, usize dest_len, char* format,...);
    /// Creates string slice of input C string (NULL tolerant, (str_s){0} on error)
    str_s           (*sstr)(char* ccharptr);
    /// Checks if string starts with prefix, returns false on error, NULL tolerant
    bool            (*starts_with)(char* s, char* prefix);
    /// Makes slices of `s` char* string, start/end are indexes, can be negative from the end, if end=0
    /// mean full length of the string. `s` may be not null-terminated. function is NULL tolerant,
    /// return (str_s){0} on error
    str_s           (*sub)(char* s, isize start, isize end);
    /// Returns new upper case string, returns NULL on error, null tolerant
    char*           (*upper)(char* s, IAllocator allc);
    /// Analog of vsprintf() uses CEX sprintf engine. NULL tolerant, overflow safe.
    Exception       (*vsprintf)(char* dest, usize dest_len, char* format, va_list va);

    struct {
        Exception       (*to_f32)(char* s, f32* num);
        Exception       (*to_f32s)(str_s s, f32* num);
        Exception       (*to_f64)(char* s, f64* num);
        Exception       (*to_f64s)(str_s s, f64* num);
        Exception       (*to_i16)(char* s, i16* num);
        Exception       (*to_i16s)(str_s s, i16* num);
        Exception       (*to_i32)(char* s, i32* num);
        Exception       (*to_i32s)(str_s s, i32* num);
        Exception       (*to_i64)(char* s, i64* num);
        Exception       (*to_i64s)(str_s s, i64* num);
        Exception       (*to_i8)(char* s, i8* num);
        Exception       (*to_i8s)(str_s s, i8* num);
        Exception       (*to_u16)(char* s, u16* num);
        Exception       (*to_u16s)(str_s s, u16* num);
        Exception       (*to_u32)(char* s, u32* num);
        Exception       (*to_u32s)(str_s s, u32* num);
        Exception       (*to_u64)(char* s, u64* num);
        Exception       (*to_u64s)(str_s s, u64* num);
        Exception       (*to_u8)(char* s, u8* num);
        Exception       (*to_u8s)(str_s s, u8* num);
    } convert;

    struct {
        /// Clone slice into new char* allocated by `allc`, null tolerant, returns NULL on error.
        char*           (*clone)(str_s s, IAllocator allc);
        /// Makes a copy of initial `src` slice, into `dest` buffer constrained by `destlen`. NULL tolerant,
        /// always null-terminated, overflow checked.
        Exception       (*copy)(char* dest, str_s src, usize destlen);
        /// Checks if slice ends with prefix, returns (str_s){0} on error, NULL tolerant
        bool            (*ends_with)(str_s s, str_s suffix);
        /// Compares two string slices, null tolerant
        bool            (*eq)(str_s a, str_s b);
        /// Compares two string slices, null tolerant, case insensitive
        bool            (*eqi)(str_s a, str_s b);
        /// Get index of first occurrence of `needle`, returns -1 on error.
        isize           (*index_of)(str_s s, str_s needle);
        /// iterator over slice splits:  for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {}
        str_s           (*iter_split)(str_s s, char* split_by, cex_iterator_s* iterator);
        /// Removes white spaces from the beginning of slice
        str_s           (*lstrip)(str_s s);
        /// Slice pattern matching check (see ./cex help str$ for examples)
        bool            (*match)(str_s s, char* pattern);
        /// libc `qsort()` comparator function for alphabetical sorting of str_s arrays
        int             (*qscmp)(const void* a, const void* b);
        /// libc `qsort()` comparator function for alphabetical case insensitive sorting of str_s arrays
        int             (*qscmpi)(const void* a, const void* b);
        /// Replaces slice prefix (start part), or returns the same slice if it's not found
        str_s           (*remove_prefix)(str_s s, str_s prefix);
        /// Replaces slice suffix (end part), or returns the same slice if it's not found
        str_s           (*remove_suffix)(str_s s, str_s suffix);
        /// Removes white spaces from the end of slice
        str_s           (*rstrip)(str_s s);
        /// Checks if slice starts with prefix, returns (str_s){0} on error, NULL tolerant
        bool            (*starts_with)(str_s s, str_s prefix);
        /// Removes white spaces from both ends of slice
        str_s           (*strip)(str_s s);
        /// Makes slices of `s` slice, start/end are indexes, can be negative from the end, if end=0 mean
        /// full length of the string. `s` may be not null-terminated. function is NULL tolerant, return
        /// (str_s){0} on error
        str_s           (*sub)(str_s s, isize start, isize end);
    } slice;

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__str str;
#endif



/*
*                          src/sbuf.h
*/

#if !defined(cex$enable_minimal) || defined(cex$enable_str)
#include <assert.h>

typedef char* sbuf_c;

typedef struct
{
    struct
    {
        u32 magic : 16;   // used for sanity checks
        u32 elsize : 8;   // maybe multibyte strings in the future?
        u32 nullterm : 8; // always zero to prevent usage of direct buffer
    } header;
    Exc err;
    const Allocator_i* allocator;
    usize capacity;
    usize length;
} __attribute__((packed)) sbuf_head_s;

static_assert(alignof(sbuf_head_s) == 1, "align");
static_assert(alignof(sbuf_head_s) == alignof(char), "align");
//static_assert(sizeof(sbuf_head_s) == 36, "size");
/**

Dynamic string builder class

Key features:

- Dynamically grown strings 
- Supports CEX specific formats 
- Can be backed by allocator or static buffer
- Error resilient - allows self as NULL
- `sbuf_c` - is an alias of `char*`, always null terminated, compatible with any C strings 

- Allocator driven dynamic string
```c
sbuf_c s = sbuf.create(20, mem$);

// These may fail (you may use them with e$* checks or add final check)
sbuf.appendf(&s, "%s, CEX slice: %S\n", "456", str$s("slice"));
sbuf.append(&s, "some string");

e$except(err, sbuf.validate(&s)) {
    // Error handling
}

if (!sbuf.isvalid(&s)) {
    // Error, just a boolean flag
}

// Some other stuff
s[i]   // getting i-th character of string
strlen(s); // C strings work, because sbuf_c is vanilla char*
sbuf.len(&s); // faster way of getting length (uses metadata)
sbuf.grow(&s, new_capacity); // increase capacity
sbuf.capacity(&s); // current capacity, 0 if error occurred
sbuf.clear(&s); // reset dynamic string + null term




// Frees the memory and sets s to NULL
sbuf.destroy(&s);
```

- Static buffer backed string
```c

// NOTE: `s` address is different, because `buf` will contain header and metadata, use only `s`
char buf[64];
sbuf_c s = sbuf.create_static(buf, arr$len(buf));

// You may check every operation if needed, but this more verbose
e$ret(sbuf.appendf(&s, "%s, CEX slice: %S\n", "456", str$s("slice")));
e$ret(sbuf.append(&s, "some string"));

// It's not mandatory, but will clean up buffer data at the end
sbuf.destroy(&s);
```

*/
struct __cex_namespace__sbuf {
    // Autogenerated by CEX
    // clang-format off

    /// Append string to the builder
    Exc             (*append)(sbuf_c* self, char* s);
    /// Append format (using CEX formatting engine)
    Exc             (*appendf)(sbuf_c* self, char* format,...);
    /// Append format va (using CEX formatting engine), always null-terminating
    Exc             (*appendfva)(sbuf_c* self, char* format, va_list va);
    /// Returns string capacity from its metadata
    u32             (*capacity)(sbuf_c* self);
    /// Clears string
    void            (*clear)(sbuf_c* self);
    /// Creates new dynamic string builder backed by allocator
    sbuf_c          (*create)(usize capacity, IAllocator allocator);
    /// Creates dynamic string backed by static array
    sbuf_c          (*create_static)(char* buf, usize buf_size);
    /// Destroys the string, deallocates the memory, or nullify static buffer.
    sbuf_c          (*destroy)(sbuf_c* self);
    /// Returns false if string invalid
    bool            (*isvalid)(sbuf_c* self);
    /// Returns string length from its metadata
    u32             (*len)(sbuf_c* self);
    /// Shrinks string length to new_length
    Exc             (*shrink)(sbuf_c* self, usize new_length);
    /// Validate dynamic string state, with detailed Exception
    Exception       (*validate)(sbuf_c* self);

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__sbuf sbuf;

#endif



/*
*                          src/io.h
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_io)

/// Makes string literal with ansi colored test
#define io$ansi(text, ansi_col) "\033[" ansi_col "m" text "\033[0m"

/**
Cross-platform IO namespace

- Read all file content (low level api)
```c

test$case(test_readall)
{
    // Open new file
    FILE* file;
    e$ret(io.fopen(&file, "tests/data/text_file_50b.txt", "r"));


    // get file size 
    tassert_eq(50, io.file.size(file));

    // Read all content
    str_s content;
    e$ret(io.fread_all(file, &content, mem$));
    mem$free(mem$, content.buf); // content.buf is allocated by mem$ !

    // Cleanup
    io.fclose(&file); // file will be set to NULL
    tassert(file == NULL);

    return EOK;
}

```

- File load/save (easy api)
```c

test$case(test_fload_save)
{
    tassert_eq(Error.ok, io.file.save("tests/data/text_file_write.txt", "Hello from CEX!\n"));
    char* content = io.file.load("tests/data/text_file_write.txt", mem$);
    tassert(content);
    tassert_eq(content, "Hello from CEX!\n");
    mem$free(mem$, content);
    return EOK;
}

```

- File read/write lines 

```c
test$case(test_write_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_write.txt", "w+"));

    str_s content;
    mem$scope(tmem$, _)
    {
        // Writing line by line
        tassert_eq(EOK, io.file.writeln(file, "hello"));
        tassert_eq(EOK, io.file.writeln(file, "world"));

        // Reading line by line
        io.rewind(file);

        // easy api - backed by temp allocator
        tassert_eq("hello", io.file.readln(file, _));

        // low-level api (using heap allocator, needs free!)
        tassert_er(EOK, io.fread_line(file, &content, mem$));
        tassert(str.slice.eq(content, str$s("world")));
        mem$free(mem$, content.buf);
    }

    io.fclose(&file);
    return EOK;
}
```

- File low-level write/read
```c

test$case(test_read_loop)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r+"));

    char buf[128] = {0};

    // Read bytes
    isize nread = 0;
    while((nread = io.fread(file, buf, 10))) {
        if (nread < 0) {
            // TODO: io.fread() error occured, you should handle it here
            // NOTE: you can use os.get_last_error() for Exception representation of io.fread() err
            break;
        }
        
        tassert_eq(nread, 10);
        buf[10] = '\0';
        io.printf("%s", buf);
    }

    // Write bytes
    char buf2[] = "foobar";
    tassert_ne(EOK, io.fwrite(file, buf2, arr$len(buf2)));

    io.fclose(&file);
    return EOK;
}

```

*/
struct __cex_namespace__io {
    // Autogenerated by CEX
    // clang-format off

    /// Closes file and set it to NULL.
    void            (*fclose)(FILE** file);
    /// Flush changes to file
    Exception       (*fflush)(FILE* file);
    /// Opens new file: io.fopen(&file, "file.txt", "r+")
    Exception       (*fopen)(FILE** file, char* filename, char* mode);
    /// Prints formatted string to the file. Uses CEX printf() engine with special formatting.
    Exc             (*fprintf)(FILE* stream, char* format,...);
    /// Read file contents into the buf, return nbytes read (can be < buff_len), 0 on EOF, negative on
    /// error (you may use os.get_last_error() for getting Exception for error, cross-platform )
    isize           (*fread)(FILE* file, void* buff, usize buff_len);
    /// Read all contents of the file, using allocator. You should free `s.buf` after.
    Exception       (*fread_all)(FILE* file, str_s* s, IAllocator allc);
    /// Reads line from a file into str_s buffer, allocates memory. You should free `s.buf` after.
    Exception       (*fread_line)(FILE* file, str_s* s, IAllocator allc);
    /// Seek file position
    Exception       (*fseek)(FILE* file, long offset, int whence);
    /// Returns current cursor position into `size` pointer
    Exception       (*ftell)(FILE* file, usize* size);
    /// Writes bytes to the file
    Exception       (*fwrite)(FILE* file, void* buff, usize buff_len);
    /// Check if current file supports ANSI colors and in interactive terminal mode
    bool            (*isatty)(FILE* file);
    /// Prints formatted string to stdout. Uses CEX printf() engine with special formatting.
    int             (*printf)(char* format,...);
    /// Rewind file cursor at the beginning
    void            (*rewind)(FILE* file);

    struct {
        /// Load full contents of the file at `path`, using text mode. Returns NULL on error.
        char*           (*load)(char* path, IAllocator allc);
        /// Reads line from file, allocates result. Returns NULL on error.
        char*           (*readln)(FILE* file, IAllocator allc);
        /// Saves full `contents` in the file at `path`, using text mode.
        Exception       (*save)(char* path, char* contents);
        /// Return full file size, always 0 for NULL file or atty
        usize           (*size)(FILE* file);
        /// Writes new line to the file
        Exception       (*writeln)(FILE* file, char* line);
    } file;

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__io io;

#endif



/*
*                          src/argparse.h
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_os)


struct argparse_c;
struct argparse_opt_s;

typedef Exception (*argparse_callback_f)(
    struct argparse_c* self,
    struct argparse_opt_s* option,
    void* ctx
);
typedef Exception (*argparse_convert_f)(char* s, void* out_val);
typedef Exception (*argparse_command_f)(int argc, char** argv, void* user_ctx);

/// command line options type (prefer macros)
typedef struct argparse_opt_s
{
    u8 type;
    void* value;
    char short_name;
    char* long_name;
    char* help;
    bool required;
    argparse_callback_f callback;
    void* callback_data;
    argparse_convert_f convert;
    bool is_present; // also setting in in argparse$opt* macro, allows optional parameter sugar
} argparse_opt_s;

/// command settings type (prefer macros)
typedef struct argparse_cmd_s
{
    char* name;
    argparse_command_f func;
    char* help;
    bool is_default;
} argparse_cmd_s;

enum CexArgParseType_e
{
    CexArgParseType__na,
    CexArgParseType__group,
    CexArgParseType__boolean,
    CexArgParseType__string,
    CexArgParseType__i8,
    CexArgParseType__u8,
    CexArgParseType__i16,
    CexArgParseType__u16,
    CexArgParseType__i32,
    CexArgParseType__u32,
    CexArgParseType__i64,
    CexArgParseType__u64,
    CexArgParseType__f32,
    CexArgParseType__f64,
    CexArgParseType__generic,
};

/// main argparse struct (used as options config)
typedef struct argparse_c
{
    // user supplied options
    argparse_opt_s* options;
    u32 options_len;

    argparse_cmd_s* commands;
    u32 commands_len;

    char* usage;        // usage text (can be multiline), each line prepended by program_name
    char* description;  // a description after usage
    char* epilog;       // a description at the end
    char* program_name; // program name in usage (by default: argv[0])

    int argc;    // current argument count (excludes program_name)
    char** argv; // current arguments list

    struct
    {
        // internal context
        char** out;
        int cpidx;
        char* optvalue; // current option value
        bool has_argument;
        argparse_cmd_s* current_command;
    } _ctx;
} argparse_c;


/// holder for list of  argparse$opt()
#define argparse$opt_list(...) .options = (argparse_opt_s[]) {__VA_ARGS__ {0} /* NULL TERM */}

/// command line option record (generic type of arguments)
#define argparse$opt(value, ...)                                                                   \
    ({                                                                                             \
        u32 val_type = _Generic(                                                                   \
            (value),                                                                               \
            bool*: CexArgParseType__boolean,                                                       \
            i8*: CexArgParseType__i8,                                                              \
            u8*: CexArgParseType__u8,                                                              \
            i16*: CexArgParseType__i16,                                                            \
            u16*: CexArgParseType__u16,                                                            \
            i32*: CexArgParseType__i32,                                                            \
            u32*: CexArgParseType__u32,                                                            \
            i64*: CexArgParseType__i64,                                                            \
            u64*: CexArgParseType__u64,                                                            \
            f32*: CexArgParseType__f32,                                                            \
            f64*: CexArgParseType__f64,                                                            \
            const char**: CexArgParseType__string,                                                 \
            char**: CexArgParseType__string,                                                       \
            default: CexArgParseType__generic                                                      \
        );                                                                                         \
        argparse_convert_f conv_f = _Generic(                                                      \
            (value),                                                                               \
            bool*: NULL,                                                                           \
            const char**: NULL,                                                                    \
            char**: NULL,                                                                          \
            i8*: (argparse_convert_f)str.convert.to_i8,                                            \
            u8*: (argparse_convert_f)str.convert.to_u8,                                            \
            i16*: (argparse_convert_f)str.convert.to_i16,                                          \
            u16*: (argparse_convert_f)str.convert.to_u16,                                          \
            i32*: (argparse_convert_f)str.convert.to_i32,                                          \
            u32*: (argparse_convert_f)str.convert.to_u32,                                          \
            i64*: (argparse_convert_f)str.convert.to_i64,                                          \
            u64*: (argparse_convert_f)str.convert.to_u64,                                          \
            f32*: (argparse_convert_f)str.convert.to_f32,                                          \
            f64*: (argparse_convert_f)str.convert.to_f64,                                          \
            default: NULL                                                                          \
        );                                                                                         \
        (argparse_opt_s){ val_type,                                                                \
                          (value),                                                                 \
                          __VA_ARGS__,                                                             \
                          .convert = (argparse_convert_f)conv_f,                                   \
                          .is_present = 0 };                                                       \
    })
// clang-format off


/// holder for list of 
#define argparse$cmd_list(...) .commands = (argparse_cmd_s[]) {__VA_ARGS__ {0} /* NULL TERM */}

/// options group separator
#define argparse$opt_group(h)     { CexArgParseType__group, NULL, '\0', NULL, h, false, NULL, 0, 0, .is_present=0 }

/// built-in option for -h,--help
#define argparse$opt_help()       {CexArgParseType__boolean, NULL, 'h', "help",                           \
                                        "show this help message and exit", false,    \
                                        NULL, 0, .is_present = 0}


/**

* Command line args parsing

```c
// NOTE: Command example 

Exception cmd_build_docs(int argc, char** argv, void* user_ctx);

int
main(int argc, char** argv)
{
    // clang-format off
    argparse_c args = {
        .description = "My description",
        .usage = "Usage help",
        .epilog = "Epilog text",
        argparse$cmd_list(
            { .name = "build-docs", .func = cmd_build_docs, .help = "Build CEX documentation" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }
    if (argparse.run_command(&args, NULL)) { return 1; }
    return 0;
}

Exception
cmd_build_docs(int argc, char** argv, void* user_ctx)
{
    // Command handling func
}

```

* Parsing custom arguments

```c
// Simple options example

int
main(int argc, char** argv)
{
    bool force = 0;
    bool test = 0;
    int int_num = 0;
    float flt_num = 0.f;
    char* path = NULL;

    char* usage = "basic [options] [[--] args]\n"
                  "basic [options]\n";

    argparse_c argparse = {
        argparse$opt_list(
            argparse$opt_help(),
            argparse$opt_group("Basic options"),
            argparse$opt(&force, 'f', "force", "force to do"),
            argparse$opt(&test, 't', "test", .help = "test only"),
            argparse$opt(&path, 'p', "path", "path to read", .required = true),
            argparse$opt_group("Another group"),
            argparse$opt(&int_num, 'i', "int", "selected integer"),
            argparse$opt(&flt_num, 's', "float", "selected float"),
        ),
        // NOTE: usage/description are optional 

        .usage = usage,
        .description = "\nA brief description of what the program does and how it works.",
        "\nAdditional description of the program after the description of the arguments.",
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }

    // NOTE: all args are filled and parsed after this line

    return 0;
}

```


*/
struct __cex_namespace__argparse {
    // Autogenerated by CEX
    // clang-format off

    char*           (*next)(argparse_c* self);
    Exception       (*parse)(argparse_c* self, int argc, char** argv);
    Exception       (*run_command)(argparse_c* self, void* user_ctx);
    void            (*usage)(argparse_c* self);

    // clang-format on
};
/**
* @brief Command line argument parsing module
*/
CEX_NAMESPACE struct __cex_namespace__argparse argparse;

#endif



/*
*                          src/_subprocess.h
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_os)

/*
   The latest version of this library is available on GitHub;
   https://github.com/sheredom/subprocess.h
*/

/*
   This is free and unencumbered software released into the public domain.

   Anyone is free to copy, modify, publish, use, compile, sell, or
   distribute this software, either in source code form or as a compiled
   binary, for any purpose, commercial or non-commercial, and by any
   means.

   In jurisdictions that recognize copyright laws, the author or authors
   of this software dedicate any and all copyright interest in the
   software to the public domain. We make this dedication for the benefit
   of the public at large and to the detriment of our heirs and
   successors. We intend this dedication to be an overt act of
   relinquishment in perpetuity of all present and future rights to this
   software under copyright law.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to <http://unlicense.org/>
*/

#ifndef SHEREDOM_SUBPROCESS_H_INCLUDED
#define SHEREDOM_SUBPROCESS_H_INCLUDED

#if defined(_MSC_VER)
#pragma warning(push, 1)

/* disable warning: '__cplusplus' is not defined as a preprocessor macro,
 * replacing with '0' for '#if/#elif' */
#pragma warning(disable : 4668)
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(__TINYC__)
#define SUBPROCESS_ATTRIBUTE(a) __attribute((a))
#else
#define SUBPROCESS_ATTRIBUTE(a) __attribute__((a))
#endif

#if defined(_MSC_VER)
#define subprocess_pure
#define subprocess_weak __inline
#define subprocess_tls __declspec(thread)
#elif defined(__MINGW32__)
#define subprocess_pure SUBPROCESS_ATTRIBUTE(pure)
#define subprocess_weak static SUBPROCESS_ATTRIBUTE(used)
#define subprocess_tls __thread
#elif defined(__clang__) || defined(__GNUC__) || defined(__TINYC__)
#define subprocess_pure SUBPROCESS_ATTRIBUTE(pure)
// #define subprocess_weak SUBPROCESS_ATTRIBUTE(weak)
#define subprocess_weak
#define subprocess_tls __thread
#else
#error Non clang, non gcc, non MSVC compiler found!
#endif


enum subprocess_option_e {
  // stdout and stderr are the same FILE.
  subprocess_option_combined_stdout_stderr = 0x1,

  // The child process should inherit the environment variables of the parent.
  subprocess_option_inherit_environment = 0x2,

  // Enable asynchronous reading of stdout/stderr before it has completed.
  subprocess_option_enable_async = 0x4,

  // Enable the child process to be spawned with no window visible if supported
  // by the platform.
  subprocess_option_no_window = 0x8,

  // Search for program names in the PATH variable. Always enabled on Windows.
  // Note: this will **not** search for paths in any provided custom environment
  // and instead uses the PATH of the spawning process.
  subprocess_option_search_user_path = 0x10
};

struct subprocess_s;

/// @brief Create a process.
/// @param command_line An array of strings for the command line to execute for
/// this process. The last element must be NULL to signify the end of the array.
/// The memory backing this parameter only needs to persist until this function
/// returns.
/// @param options A bit field of subprocess_option_e's to pass.
/// @param out_process The newly created process.
/// @return On success zero is returned.
subprocess_weak int subprocess_create(const char *const command_line[],
                                      int options,
                                      struct subprocess_s *const out_process);

/// @brief Create a process (extended create).
/// @param command_line An array of strings for the command line to execute for
/// this process. The last element must be NULL to signify the end of the array.
/// The memory backing this parameter only needs to persist until this function
/// returns.
/// @param options A bit field of subprocess_option_e's to pass.
/// @param environment An optional array of strings for the environment to use
/// for a child process (each element of the form FOO=BAR). The last element
/// must be NULL to signify the end of the array.
/// @param out_process The newly created process.
/// @return On success zero is returned.
///
/// If `options` contains `subprocess_option_inherit_environment`, then
/// `environment` must be NULL.
subprocess_weak int
subprocess_create_ex(const char *const command_line[], int options,
                     const char *const environment[],
                     struct subprocess_s *const out_process);

/// @brief Get the standard input file for a process.
/// @param process The process to query.
/// @return The file for standard input of the process.
///
/// The file returned can be written to by the parent process to feed data to
/// the standard input of the process.
subprocess_pure subprocess_weak FILE *
subprocess_stdin(const struct subprocess_s *const process);

/// @brief Get the standard output file for a process.
/// @param process The process to query.
/// @return The file for standard output of the process.
///
/// The file returned can be read from by the parent process to read data from
/// the standard output of the child process.
subprocess_pure subprocess_weak FILE *
subprocess_stdout(const struct subprocess_s *const process);

/// @brief Get the standard error file for a process.
/// @param process The process to query.
/// @return The file for standard error of the process.
///
/// The file returned can be read from by the parent process to read data from
/// the standard error of the child process.
///
/// If the process was created with the subprocess_option_combined_stdout_stderr
/// option bit set, this function will return NULL, and the subprocess_stdout
/// function should be used for both the standard output and error combined.
subprocess_pure subprocess_weak FILE *
subprocess_stderr(const struct subprocess_s *const process);

/// @brief Wait for a process to finish execution.
/// @param process The process to wait for.
/// @param out_return_code The return code of the returned process (can be
/// NULL).
/// @return On success zero is returned.
///
/// Joining a process will close the stdin pipe to the process.
subprocess_weak int subprocess_join(struct subprocess_s *const process,
                                    int *const out_return_code);

/// @brief Destroy a previously created process.
/// @param process The process to destroy.
/// @return On success zero is returned.
///
/// If the process to be destroyed had not finished execution, it may out live
/// the parent process.
subprocess_weak int subprocess_destroy(struct subprocess_s *const process);

/// @brief Terminate a previously created process.
/// @param process The process to terminate.
/// @return On success zero is returned.
///
/// If the process to be destroyed had not finished execution, it will be
/// terminated (i.e killed).
subprocess_weak int subprocess_terminate(struct subprocess_s *const process);

/// @brief Read the standard output from the child process.
/// @param process The process to read from.
/// @param buffer The buffer to read into.
/// @param size The maximum number of bytes to read.
/// @return The number of bytes actually read into buffer. Can only be 0 if the
/// process has complete.
///
/// The only safe way to read from the standard output of a process during it's
/// execution is to use the `subprocess_option_enable_async` option in
/// conjunction with this method.
subprocess_weak unsigned
subprocess_read_stdout(struct subprocess_s *const process, char *const buffer,
                       unsigned size);

/// @brief Read the standard error from the child process.
/// @param process The process to read from.
/// @param buffer The buffer to read into.
/// @param size The maximum number of bytes to read.
/// @return The number of bytes actually read into buffer. Can only be 0 if the
/// process has complete.
///
/// The only safe way to read from the standard error of a process during it's
/// execution is to use the `subprocess_option_enable_async` option in
/// conjunction with this method.
subprocess_weak unsigned
subprocess_read_stderr(struct subprocess_s *const process, char *const buffer,
                       unsigned size);

/// @brief Returns if the subprocess is currently still alive and executing.
/// @param process The process to check.
/// @return If the process is still alive non-zero is returned.
subprocess_weak int subprocess_alive(struct subprocess_s *const process);

#if defined(__cplusplus)
#define SUBPROCESS_CAST(type, x) static_cast<type>(x)
#define SUBPROCESS_PTR_CAST(type, x) reinterpret_cast<type>(x)
#define SUBPROCESS_CONST_CAST(type, x) const_cast<type>(x)
#define SUBPROCESS_NULL NULL
#else
#define SUBPROCESS_CAST(type, x) ((type)(x))
#define SUBPROCESS_PTR_CAST(type, x) ((type)(x))
#define SUBPROCESS_CONST_CAST(type, x) ((type)(x))
#define SUBPROCESS_NULL 0
#endif

#if !defined(_WIN32)
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#if defined(_WIN32)

#if (_MSC_VER < 1920)
#ifdef _WIN64
typedef __int64 subprocess_intptr_t;
typedef unsigned __int64 subprocess_size_t;
#else
typedef int subprocess_intptr_t;
typedef unsigned int subprocess_size_t;
#endif
#else
#include <inttypes.h>

typedef intptr_t subprocess_intptr_t;
typedef size_t subprocess_size_t;
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif

typedef struct _PROCESS_INFORMATION *LPPROCESS_INFORMATION;
typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;
typedef struct _STARTUPINFOA *LPSTARTUPINFOA;
typedef struct _OVERLAPPED *LPOVERLAPPED;

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#ifdef __MINGW32__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

struct subprocess_subprocess_information_s {
  void *hProcess;
  void *hThread;
  unsigned long dwProcessId;
  unsigned long dwThreadId;
};

struct subprocess_security_attributes_s {
  unsigned long nLength;
  void *lpSecurityDescriptor;
  int bInheritHandle;
};

struct subprocess_startup_info_s {
  unsigned long cb;
  char *lpReserved;
  char *lpDesktop;
  char *lpTitle;
  unsigned long dwX;
  unsigned long dwY;
  unsigned long dwXSize;
  unsigned long dwYSize;
  unsigned long dwXCountChars;
  unsigned long dwYCountChars;
  unsigned long dwFillAttribute;
  unsigned long dwFlags;
  unsigned short wShowWindow;
  unsigned short cbReserved2;
  unsigned char *lpReserved2;
  void *hStdInput;
  void *hStdOutput;
  void *hStdError;
};

struct subprocess_overlapped_s {
  uintptr_t Internal;
  uintptr_t InternalHigh;
  union {
    struct {
      unsigned long Offset;
      unsigned long OffsetHigh;
    } DUMMYSTRUCTNAME;
    void *Pointer;
  } DUMMYUNIONNAME;

  void *hEvent;
};

#ifdef __MINGW32__
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

__declspec(dllimport) unsigned long __stdcall GetLastError(void);
__declspec(dllimport) int __stdcall SetHandleInformation(void *, unsigned long,
                                                         unsigned long);
__declspec(dllimport) int __stdcall CreatePipe(void **, void **,
                                               LPSECURITY_ATTRIBUTES,
                                               unsigned long);
__declspec(dllimport) void *__stdcall CreateNamedPipeA(
    const char *, unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long, LPSECURITY_ATTRIBUTES);
__declspec(dllimport) int __stdcall ReadFile(void *, void *, unsigned long,
                                             unsigned long *, LPOVERLAPPED);
__declspec(dllimport) unsigned long __stdcall GetCurrentProcessId(void);
__declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);
__declspec(dllimport) void *__stdcall CreateFileA(const char *, unsigned long,
                                                  unsigned long,
                                                  LPSECURITY_ATTRIBUTES,
                                                  unsigned long, unsigned long,
                                                  void *);
__declspec(dllimport) void *__stdcall CreateEventA(LPSECURITY_ATTRIBUTES, int,
                                                   int, const char *);
__declspec(dllimport) int __stdcall CreateProcessA(
    const char *, char *, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, int,
    unsigned long, void *, const char *, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
__declspec(dllimport) int __stdcall CloseHandle(void *);
__declspec(dllimport) unsigned long __stdcall WaitForSingleObject(
    void *, unsigned long);
__declspec(dllimport) int __stdcall GetExitCodeProcess(
    void *, unsigned long *lpExitCode);
__declspec(dllimport) int __stdcall TerminateProcess(void *, unsigned int);
__declspec(dllimport) unsigned long __stdcall WaitForMultipleObjects(
    unsigned long, void *const *, int, unsigned long);
__declspec(dllimport) int __stdcall GetOverlappedResult(void *, LPOVERLAPPED,
                                                        unsigned long *, int);

#if defined(_DLL)
#define SUBPROCESS_DLLIMPORT __declspec(dllimport)
#else
#define SUBPROCESS_DLLIMPORT
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif

SUBPROCESS_DLLIMPORT int __cdecl _fileno(FILE *);
SUBPROCESS_DLLIMPORT int __cdecl _open_osfhandle(subprocess_intptr_t, int);
SUBPROCESS_DLLIMPORT subprocess_intptr_t __cdecl _get_osfhandle(int);

#ifndef __MINGW32__
void *__cdecl _alloca(subprocess_size_t);
#else
#include <malloc.h>
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#else
typedef size_t subprocess_size_t;
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
struct subprocess_s {
  FILE *stdin_file;
  FILE *stdout_file;
  FILE *stderr_file;

#if defined(_WIN32)
  void *hProcess;
  void *hStdInput;
  void *hEventOutput;
  void *hEventError;
#else
  pid_t child;
  int return_status;
#endif

  subprocess_size_t alive;
};
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif /* SHEREDOM_SUBPROCESS_H_INCLUDED */

#endif



/*
*                          src/os.h
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_os)


#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"
#    include <direct.h>
#else
#    include <dirent.h>
#    include <fcntl.h>
#    include <limits.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>
#endif

/// Additional flags for os.cmd.create()
typedef struct os_cmd_flags_s
{
    u32 combine_stdouterr : 1; // if 1 - combines output from stderr to stdout
    u32 no_inherit_env : 1;    // if 1 - disables using existing env variable of parent process
    u32 no_search_path : 1;    // if 1 - disables propagating PATH= env to command line
    u32 no_window : 1;         // if 1 - disables creation of new window if OS supported
} os_cmd_flags_s;
static_assert(sizeof(os_cmd_flags_s) == sizeof(u32), "size?");

/// Command container (current state of subprocess)
typedef struct os_cmd_c
{
    struct subprocess_s _subpr;
    os_cmd_flags_s _flags;
    bool _is_subprocess;
} os_cmd_c;

/// File stats metadata (cross-platform), returned by os.fs.stats
typedef struct os_fs_stat_s
{
    u64 is_valid : 1;
    u64 is_directory : 1;
    u64 is_symlink : 1;
    u64 is_file : 1;
    u64 is_other : 1;
    u64 size : 48;
    union
    {
        Exc error;
        time_t mtime;
    };
} os_fs_stat_s;
static_assert(sizeof(os_fs_stat_s) <= sizeof(u64) * 2, "size?");

typedef Exception os_fs_dir_walk_f(char* path, os_fs_stat_s ftype, void* user_ctx);

#define _CexOSPlatformList                                                                         \
    X(linux)                                                                                       \
    X(win)                                                                                         \
    X(macos)                                                                                       \
    X(wasm)                                                                                        \
    X(android)                                                                                     \
    X(freebsd)                                                                                     \
    X(openbsd)

#define _CexOSArchList                                                                             \
    X(x86_32)                                                                                      \
    X(x86_64)                                                                                      \
    X(arm)                                                                                         \
    X(wasm32)                                                                                      \
    X(wasm64)                                                                                      \
    X(aarch64)                                                                                     \
    X(riscv32)                                                                                     \
    X(riscv64)                                                                                     \
    X(xtensa)

#define X(name) OSPlatform__##name,
typedef enum OSPlatform_e
{
    OSPlatform__unknown,
    _CexOSPlatformList OSPlatform__count,
} OSPlatform_e;
#undef X

__attribute__((unused)) static const char* OSPlatform_str[] = {
#define X(name) #name,
    NULL,
    _CexOSPlatformList
#undef X
};

typedef enum OSArch_e
{
#define X(name) OSArch__##name,
    OSArch__unknown,
    _CexOSArchList OSArch__count,
#undef X
} OSArch_e;

__attribute__((unused)) static const char* OSArch_str[] = {
#define X(name) cex$stringize(name),
    NULL,
    _CexOSArchList
#undef X
};

#ifdef _WIN32
/// OS path separator, generally '\' for Windows, '/' otherwise
#    define os$PATH_SEP '\\'
#else
#    define os$PATH_SEP '/'
#endif

#if defined(CEX_BUILD) && CEX_LOG_LVL > 3
#    define _os$args_print(msg, args, args_len)                                                    \
        log$debug(msg "");                                                                         \
        for (u32 i = 0; i < args_len - 1; i++) {                                                   \
            char* a = args[i];                                                                     \
            io.printf(" ");                                                                        \
            if (str.find(a, " ")) {                                                                \
                io.printf("\'%s\'", a);                                                            \
            } else if (a == NULL || *a == '\0') {                                                  \
                io.printf("\'%s\'", a);                                                            \
            } else {                                                                               \
                io.printf("%s", a);                                                                \
            }                                                                                      \
        }                                                                                          \
        io.printf("\n");                                                                           \
        fflush(stdout);

#else
#    define _os$args_print(msg, args, args_len)
#endif

/// Run command by dynamic or static array (returns Exc, but error check is not mandatory). Pipes
/// all IO to stdout/err/in into current terminal, feels totally interactive.
#define os$cmda(args, args_len...)                                                                 \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        static_assert(sizeof(args) > 0, "You must pass at least one item");                        \
        usize _args_len_va[] = { args_len };                                                       \
        (void)_args_len_va;                                                                        \
        usize _args_len = (sizeof(_args_len_va) > 0) ? _args_len_va[0] : arr$len(args);            \
        uassert(_args_len < PTRDIFF_MAX && "negative length or overflow");                         \
        _os$args_print("CMD:", args, _args_len);                                                   \
        os_cmd_c _cmd = { 0 };                                                                     \
        Exc result = os.cmd.run(args, _args_len, &_cmd);                                           \
        if (result == EOK) { result = os.cmd.join(&_cmd, 0, NULL); };                              \
        result;                                                                                    \
        /* NOLINTEND */                                                                            \
    })

/// Run command by arbitrary set of arguments (returns Exc, but error check is not mandatory). Pipes
/// all IO to stdout/err/in into current terminal, feels totally interactive. 
/// Example: e$ret(os$cmd("cat", "./cex.c"))
#define os$cmd(args...)                                                                            \
    ({                                                                                             \
        char* _args[] = { args, NULL };                                                            \
        usize _args_len = arr$len(_args);                                                          \
        os$cmda(_args, _args_len);                                                                 \
    })

/// Path parts join by variable set of args: os$path_join(mem$, "foo", "bar", "cex.c")
#define os$path_join(allocator, path_parts...)                                                     \
    ({                                                                                             \
        char* _args[] = { path_parts };                                                            \
        usize _args_len = arr$len(_args);                                                          \
        os.path.join(_args, _args_len, allocator);                                                 \
    })

/**

Cross-platform OS related operations:

- `os.cmd.` - for running commands and interacting with them
- `os.fs.` - file-system related tasks
- `os.env.` - getting setting environment variable
- `os.path.` - file path operations
- `os.platform.` - information about current platform


Examples:

- Running simple commands
```c
// NOTE: there are many operation with os-related stuff in cexy build system
// try to play in example roulette:  ./cex help --example os.cmd.run

// Easy macro, run fixed number of arguments

e$ret(os$cmd(
    cexy$cc,
    "src/main.c",
    cexy$build_dir "/sqlite3.o",
    cexy$cc_include,
    "-lpthread",
    "-lm",
    "-o",
    cexy$build_dir "/hello_sqlite"
));


```

- Running dynamic arguments
```c
mem$scope(tmem$, _)
{
        arr$(char*) args = arr$new(args, _);
        arr$pushm(
            args,
            cexy$cc,
            "-Wall",
            "-Werror",
        );

        if (os.platform.current() == OSPlatform__win) {
            arr$pushm(args, "-lbcrypt");
        }

        arr$pushm(args, NULL); // NOTE: last element must be NULL
        e$ret(os$cmda(args));
    }
}
```

- Getting command output (low level api)
```c

test$case(os_cmd_create)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        char* args[] = { "./cex", NULL };
        tassert_er(EOK, os.cmd.create(&c, args, arr$len(args), NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        io.printf("%s\n", output);

        int err_code = 0;
        tassert_er(Error.runtime, os.cmd.join(&c, 0, &err_code));
        tassert_eq(err_code, 1);
    }
    return EOK;
}
```

- Working with files
```c

test$case(test_os_find_all_c_files)
{
    mem$scope(tmem$, _)
    {
        // Check if exists and remove
        if (os.path.exists("./cex")) { e$ret(os.fs.remove("./cex")); }

        // illustration of path combining
        char* pattern = os$path_join(_, "./", "*.c");

        // find all matching *.c files
        for$each (it, os.fs.find(pattern, _), false , _)) {
            log$debug("found file: %s\n", it);
        }
    }

    return EOK;
}
```


*/
struct __cex_namespace__os
{
    // Autogenerated by CEX
    // clang-format off

    /// Get last system API error as string representation (Exception compatible). Result content may be
    /// affected by OS locale settings.
    Exc             (*get_last_error)(void);
    /// Sleep for `period_millisec` duration
    void            (*sleep)(u32 period_millisec);
    /// Get high performance monotonic timer value in seconds
    f64             (*timer)(void);

    struct {
        /// Creates new os command (use os$cmd() and os$cmd() for easy cases)
        Exception       (*create)(os_cmd_c* self, char** args, usize args_len, os_cmd_flags_s* flags);
        /// Check if `cmd_exe` program name exists in PATH. cmd_exe can be absolute, or simple command name,
        /// e.g. `cat`
        bool            (*exists)(char* cmd_exe);
        /// Get running command stderr stream
        FILE*           (*fstderr)(os_cmd_c* self);
        /// Get running command stdin stream
        FILE*           (*fstdin)(os_cmd_c* self);
        /// Get running command stdout stream
        FILE*           (*fstdout)(os_cmd_c* self);
        /// Checks if process is running
        bool            (*is_alive)(os_cmd_c* self);
        /// Waits process to end, and get `out_ret_code`, if timeout_sec=0 - infinite wait, raises
        /// Error.runtime if out_ret_code != 0
        Exception       (*join)(os_cmd_c* self, u32 timeout_sec, i32* out_ret_code);
        /// Terminates the running process
        Exception       (*kill)(os_cmd_c* self);
        /// Read all output from process stdout, NULL if stdout is not available
        char*           (*read_all)(os_cmd_c* self, IAllocator allc);
        /// Read line from process stdout, NULL if stdout is not available
        char*           (*read_line)(os_cmd_c* self, IAllocator allc);
        /// Run command using arguments array and resulting os_cmd_c
        Exception       (*run)(char** args, usize args_len, os_cmd_c* out_cmd);
        /// Writes line to the process stdin
        Exception       (*write_line)(os_cmd_c* self, char* line);
    } cmd;

    struct {
        /// Get environment variable, with `deflt` if not found
        char*           (*get)(char* name, char* deflt);
        /// Set environment variable
        Exception       (*set)(char* name, char* value);
    } env;

    struct {
        /// Change current working directory
        Exception       (*chdir)(char* path);
        /// Copy file
        Exception       (*copy)(char* src_path, char* dst_path);
        /// Copy directory recursively
        Exception       (*copy_tree)(char* src_dir, char* dst_dir);
        /// Iterates over directory (can be recursive) using callback function
        Exception       (*dir_walk)(char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx);
        /// Finds files in `dir/pattern`, for example "./mydir/*.c" (all c files), if is_recursive=true, all
        /// *.c files found in sub-directories.
        arr$(char*)     (*find)(char* path_pattern, bool is_recursive, IAllocator allc);
        /// Get current working directory
        char*           (*getcwd)(IAllocator allc);
        /// Makes directory (no error if exists)
        Exception       (*mkdir)(char* path);
        /// Makes all directories in a path
        Exception       (*mkpath)(char* path);
        /// Removes file or empty directory (also see os.fs.remove_tree)
        Exception       (*remove)(char* path);
        /// Removes directory and all its contents recursively
        Exception       (*remove_tree)(char* path);
        /// Renames file or directory
        Exception       (*rename)(char* old_path, char* new_path);
        /// Returns cross-platform path stats information (see os_fs_stat_s)
        os_fs_stat_s    (*stat)(char* path);
    } fs;

    struct {
        /// Returns absolute path from relative
        char*           (*abs)(char* path, IAllocator allc);
        /// Get file name of a path
        char*           (*basename)(char* path, IAllocator allc);
        /// Get directory name of a path
        char*           (*dirname)(char* path, IAllocator allc);
        /// Check if file/directory path exists
        bool            (*exists)(char* file_path);
        /// Join path with OS specific path separator
        char*           (*join)(char** parts, u32 parts_len, IAllocator allc);
        /// Splits path by `dir` and `file` parts, when return_dir=true - returns `dir` part, otherwise
        /// `file` part
        str_s           (*split)(char* path, bool return_dir);
    } path;

    struct {
        /// Returns OSArch from string
        OSArch_e        (*arch_from_str)(char* name);
        /// Converts arch to string
        char*           (*arch_to_str)(OSArch_e platform);
        /// Returns current OS platform, returns enum of OSPlatform__*, e.g. OSPlatform__win,
        /// OSPlatform__linux, OSPlatform__macos, etc..
        OSPlatform_e    (*current)(void);
        /// Returns string name of current platform
        char*           (*current_str)(void);
        /// Converts platform name to enum
        OSPlatform_e    (*from_str)(char* name);
        /// Converts platform enum to name
        char*           (*to_str)(OSPlatform_e platform);
    } platform;

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__os os;
#endif



/*
*                          src/test.h
*/
#if !defined(cex$enable_minimal)

typedef Exception (*_cex_test_case_f)(void);

#define CEX_TEST_AMSG_MAX_LEN 512
struct _cex_test_case_s
{
    _cex_test_case_f test_fn;
    char* test_name;
    u32 test_line;
};

struct _cex_test_context_s
{
    arr$(struct _cex_test_case_s) test_cases;
    int orig_stderr_fd; // initial stdout
    int orig_stdout_fd; // initial stderr
    FILE* out_stream;   // test case captured output
    int tests_run;      // number of tests run
    int tests_failed;   // number of tests failed
    bool quiet_mode;    // quiet mode (for run all)
    char* case_name;    // current running case name
    _cex_test_case_f setup_case_fn;
    _cex_test_case_f teardown_case_fn;
    _cex_test_case_f setup_suite_fn;
    _cex_test_case_f teardown_suite_fn;
    bool has_ansi;
    bool no_stdout_capture;
    bool breakpoint;
    char* suite_file;
    char* case_filter;
    char str_buf[CEX_TEST_AMSG_MAX_LEN];
};

/**

Unit Testing engine:

- Running/building tests
```sh
./cex test create tests/test_mytest.c
./cex test run tests/test_mytest.c
./cex test run all
./cex test debug tests/test_mytest.c
./cex test clean all
./cex test --help
```

- Unit Test structure
```c
test$setup_case() {
    // Optional: runs before each test case
    return EOK;
}
test$teardown_case() {
    // Optional: runs after each test case
    return EOK;
}
test$setup_suite() {
    // Optional: runs once before full test suite initialized
    return EOK;
}
test$teardown_suite() {
    // Optional: runs once after full test suite ended
    return EOK;
}

test$case(my_test_case){
    e$ret(foo("raise")); // this test will fail if `foo()` raises Exception 
    return EOK; // Must return EOK for passed
}

test$case(my_test_another_case){
    tassert_eq(1, 0); //  tassert_ fails test, but not abort the program
    return EOK; // Must return EOK for passed
}

test$main(); // mandatory at the end of each test
```

- Test checks
```c

test$case(my_test_case){
    // Generic type assertions, fails and print values of both arguments

    tassert_eq(1, 1);
    tassert_eq(str, "foo");
    tassert_eq(num, 3.14);
    tassert_eq(str_slice, str$s("expected") );

    tassert(condition && "oops");
    tassertf(condition, "oops: %s", s);

    tassert_er(EOK, raising_exc_foo(0));
    tassert_er(Error.argument, raising_exc_foo(-1));

    tassert_eq_almost(PI, 3.14, 0.01); // 0.01 is float tolerance
    tassert_eq(3.4 * NAN, NAN); // NAN equality also works

    tassert_eq_ptr(a, b); // raw pointer comparison
    tassert_eq_mem(a, b); // raw buffer content comparison (a and b expected to be same size)

    tassert_eq_arr(a, b); // compare two arrays (static or dynamic)


    tassert_ne(1, 0); // not equal
    tassert_le(a, b); // a <= b
    tassert_lt(a, b); // a < b
    tassert_gt(a, b); // a > b
    tassert_ge(a, b); // a >= b

    return EOK;
}

```

*/
#define __test$

#if defined(__clang__)
/// Attribute for function which disables optimization for test cases or other functions
#    define test$noopt __attribute__((optnone))
#elif defined(__GNUC__) || defined(__GNUG__)
#    define test$noopt __attribute__((optimize("O0")))
#elif defined(_MSC_VER)
#    error "MSVC deprecated"
#endif

#define _test$log_err(msg) __FILE__ ":" cex$stringize(__LINE__) " -> " msg
#define _test$tassert_breakpoint()                                                                 \
    ({                                                                                             \
        if (_cex_test__mainfn_state.breakpoint) {                                                  \
            fprintf(stderr, "[BREAK] %s\n", _test$log_err("breakpoint hit"));                      \
            breakpoint();                                                                          \
        }                                                                                          \
    })


/// Unit-test test case
#define test$case(NAME)                                                                             \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                      \
    static Exception cex_test_##NAME();                                                             \
    static void cex_test_register_##NAME(void) __attribute__((constructor));                        \
    static void cex_test_register_##NAME(void)                                                      \
    {                                                                                               \
        if (_cex_test__mainfn_state.test_cases == NULL) {                                           \
            _cex_test__mainfn_state.test_cases = arr$new(_cex_test__mainfn_state.test_cases, mem$); \
            uassert(_cex_test__mainfn_state.test_cases != NULL && "memory error");                  \
        };                                                                                          \
        arr$push(                                                                                   \
            _cex_test__mainfn_state.test_cases,                                                     \
            (struct _cex_test_case_s){ .test_fn = &cex_test_##NAME,                                 \
                                       .test_name = #NAME,                                          \
                                       .test_line = __LINE__ }                                      \
        );                                                                                          \
    }                                                                                               \
    Exception test$noopt cex_test_##NAME(void)


#ifndef CEX_TEST
#    define _test$env_check()                                                                       \
        fprintf(stderr, "CEX_TEST was not defined, pass -DCEX_TEST or #define CEX_TEST");          \
        exit(1);
#else
#    define _test$env_check() (void)0
#endif

#ifdef _WIN32
#    define _cex_test_file_close$ _close
#else
#    define _cex_test_file_close$ close
#endif

/// main() function for test suite, you must place it into test file at the end 
#define test$main()                                                                                \
    _Pragma("GCC diagnostic push"); /* Mingw64:  warning: visibility attribute not supported */    \
    _Pragma("GCC diagnostic ignored \"-Wattributes\"");                                            \
    struct _cex_test_context_s _cex_test__mainfn_state = { .suite_file = __FILE__ };               \
    int main(int argc, char** argv)                                                                \
    {                                                                                              \
        _test$env_check();                                                                          \
        argv[0] = __FILE__;                                                                        \
        int ret_code = cex_test_main_fn(argc, argv);                                               \
        if (_cex_test__mainfn_state.test_cases) { arr$free(_cex_test__mainfn_state.test_cases); }  \
        if (_cex_test__mainfn_state.orig_stdout_fd) {                                              \
            _cex_test_file_close$(_cex_test__mainfn_state.orig_stdout_fd);                         \
        }                                                                                          \
        if (_cex_test__mainfn_state.orig_stderr_fd) {                                              \
            _cex_test_file_close$(_cex_test__mainfn_state.orig_stderr_fd);                         \
        }                                                                                          \
        return ret_code;                                                                           \
    }

/// Optional: initializes at test suite once at start
#define test$setup_suite()                                                                         \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                     \
    static Exception cex_test__setup_suite_fn();                                                   \
    static void cex_test__register_setup_suite_fn(void) __attribute__((constructor));              \
    static void cex_test__register_setup_suite_fn(void)                                            \
    {                                                                                              \
        uassert(_cex_test__mainfn_state.setup_suite_fn == NULL);                                   \
        _cex_test__mainfn_state.setup_suite_fn = &cex_test__setup_suite_fn;                        \
    }                                                                                              \
    Exception test$noopt cex_test__setup_suite_fn(void)

/// Optional: shut down test suite once at the end
#define test$teardown_suite()                                                                      \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                     \
    static Exception cex_test__teardown_suite_fn();                                                \
    static void cex_test__register_teardown_suite_fn(void) __attribute__((constructor));           \
    static void cex_test__register_teardown_suite_fn(void)                                         \
    {                                                                                              \
        uassert(_cex_test__mainfn_state.teardown_suite_fn == NULL);                                \
        _cex_test__mainfn_state.teardown_suite_fn = &cex_test__teardown_suite_fn;                  \
    }                                                                                              \
    Exception test$noopt cex_test__teardown_suite_fn(void)

/// Optional: called before each test$case() starts
#define test$setup_case()                                                                          \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                     \
    static Exception cex_test__setup_case_fn();                                                    \
    static void cex_test__register_setup_case_fn(void) __attribute__((constructor));               \
    static void cex_test__register_setup_case_fn(void)                                             \
    {                                                                                              \
        uassert(_cex_test__mainfn_state.setup_case_fn == NULL);                                    \
        _cex_test__mainfn_state.setup_case_fn = &cex_test__setup_case_fn;                          \
    }                                                                                              \
    Exception test$noopt cex_test__setup_case_fn(void)

/// Optional: called after each test$case() ends
#define test$teardown_case()                                                                       \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                     \
    static Exception cex_test__teardown_case_fn();                                                 \
    static void cex_test__register_teardown_case_fn(void) __attribute__((constructor));            \
    static void cex_test__register_teardown_case_fn(void)                                          \
    {                                                                                              \
        uassert(_cex_test__mainfn_state.teardown_case_fn == NULL);                                 \
        _cex_test__mainfn_state.teardown_case_fn = &cex_test__teardown_case_fn;                    \
    }                                                                                              \
    Exception test$noopt cex_test__teardown_case_fn(void)

#define _test$tassert_fn(a, b)                                                                     \
    ({                                                                                             \
        _Generic(                                                                                  \
            (a),                                                                                   \
            i32: _check_eq_int,                                                                    \
            u32: _check_eq_int,                                                                    \
            i64: _check_eq_int,                                                                    \
            u64: _check_eq_int,                                                                    \
            i16: _check_eq_int,                                                                    \
            u16: _check_eq_int,                                                                    \
            i8: _check_eq_int,                                                                     \
            u8: _check_eq_int,                                                                     \
            char: _check_eq_int,                                                                   \
            bool: _check_eq_int,                                                                   \
            char*: _check_eq_str,                                                                  \
            const char*: _check_eq_str,                                                            \
            str_s: _check_eqs_slice,                                                               \
            f32: _check_eq_f32,                                                                    \
            f64: _check_eq_f32,                                                                    \
            default: _check_eq_int                                                                 \
        );                                                                                         \
    })

/// Test assertion, fails test if A is false
#define tassert(A)                                                                                 \
    ({ /* ONLY for test$case USE */                                                                \
       if (!(A)) {                                                                                 \
           _test$tassert_breakpoint();                                                             \
           return _test$log_err(#A);                                                               \
       }                                                                                           \
    })

/// Test assertion with user formatted output, supports CEX formatting engine
#define tassertf(A, M, ...)                                                                        \
    ({ /* ONLY for test$case USE */                                                                \
       if (!(A)) {                                                                                 \
           _test$tassert_breakpoint();                                                             \
           if (str.sprintf(                                                                        \
                   _cex_test__mainfn_state.str_buf,                                                \
                   CEX_TEST_AMSG_MAX_LEN - 1,                                                      \
                   _test$log_err(M),                                                               \
                   ##__VA_ARGS__                                                                   \
               )) {}                                                                               \
           return _cex_test__mainfn_state.str_buf;                                                 \
       }                                                                                           \
    })

/// Generic type equality checks, supports Exc, char*, str_s, numbers, floats (with NAN)
#define tassert_eq(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        auto genf = _test$tassert_fn((a), (b));                                                    \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__eq))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check expected error, or EOK (if no error expected)
#define tassert_er(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        if ((cex$tmpname(err) = _check_eq_err((a), (b), __LINE__))) {                              \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check floating point values absolute difference less than delta
#define tassert_eq_almost(a, b, delta)                                                             \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        if ((cex$tmpname(err) = _check_eq_almost((a), (b), (delta), __LINE__))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check pointer address equality
#define tassert_eq_ptr(a, b)                                                                       \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        if ((cex$tmpname(err) = _check_eq_ptr((a), (b), __LINE__))) {                              \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check memory buffer contents, binary equality, a and b must be the same sizeof()
#define tassert_eq_mem(a, b...)                                                                    \
    ({                                                                                             \
        auto _a = (a);                                                                             \
        auto _b = (b);                                                                             \
        static_assert(                                                                             \
            __builtin_types_compatible_p(__typeof__(_a), __typeof__(_b)),                          \
            "incompatible"                                                                         \
        );                                                                                         \
        static_assert(sizeof(_a) == sizeof(_b), "different size");                                 \
        if (memcmp(&_a, &_b, sizeof(_a)) != 0) {                                                   \
            _test$tassert_breakpoint();                                                            \
            if (str.sprintf(                                                                       \
                    _cex_test__mainfn_state.str_buf,                                               \
                    CEX_TEST_AMSG_MAX_LEN - 1,                                                     \
                    _test$log_err("a and b are not binary equal")                                  \
                )) {}                                                                              \
            return _cex_test__mainfn_state.str_buf;                                                \
        }                                                                                          \
    })

/// Check array element-wise equality (prints at what index is difference)
#define tassert_eq_arr(a, b...)                                                                    \
    ({                                                                                             \
        auto _a = (a);                                                                             \
        auto _b = (b);                                                                             \
        static_assert(                                                                             \
            __builtin_types_compatible_p(__typeof__(*a), __typeof__(*b)),                          \
            "incompatible"                                                                         \
        );                                                                                         \
        static_assert(sizeof(*_a) == sizeof(*_b), "different size");                               \
        usize _alen = arr$len(a);                                                                  \
        usize _blen = arr$len(b);                                                                  \
        usize _itsize = sizeof(*_a);                                                               \
        if (_alen != _blen) {                                                                      \
            _test$tassert_breakpoint();                                                            \
            if (str.sprintf(                                                                       \
                    _cex_test__mainfn_state.str_buf,                                               \
                    CEX_TEST_AMSG_MAX_LEN - 1,                                                     \
                    _test$log_err("array length is different %ld != %ld"),                         \
                    _alen,                                                                         \
                    _blen                                                                          \
                )) {}                                                                              \
            return _cex_test__mainfn_state.str_buf;                                                \
        } else {                                                                                   \
            for (usize i = 0; i < _alen; i++) {                                                    \
                if (memcmp(&(_a[i]), &(_b[i]), _itsize) != 0) {                                    \
                    _test$tassert_breakpoint();                                                    \
                    if (str.sprintf(                                                               \
                            _cex_test__mainfn_state.str_buf,                                       \
                            CEX_TEST_AMSG_MAX_LEN - 1,                                             \
                            _test$log_err("array element at index [%d] is different"),             \
                            i                                                                      \
                        )) {}                                                                      \
                    return _cex_test__mainfn_state.str_buf;                                        \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    })

/// Check if a and b are not equal
#define tassert_ne(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        auto genf = _test$tassert_fn((a), (b));                                                    \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__ne))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check if a <= b
#define tassert_le(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        auto genf = _test$tassert_fn((a), (b));                                                    \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__le))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check if a < b
#define tassert_lt(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        auto genf = _test$tassert_fn((a), (b));                                                    \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__lt))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check if a >= b
#define tassert_ge(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        auto genf = _test$tassert_fn((a), (b));                                                    \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__ge))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check if a > b
#define tassert_gt(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        auto genf = _test$tassert_fn((a), (b));                                                    \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__gt))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })
#endif



/*
*                          src/test.c
*/
#if !defined(cex$enable_minimal)
#    ifdef CEX_TEST
#        include <math.h>

#        ifdef _WIN32
#            include <fcntl.h>
#            include <io.h>
#        endif

enum _cex_test_eq_op_e
{
    _cex_test_eq_op__na,
    _cex_test_eq_op__eq,
    _cex_test_eq_op__ne,
    _cex_test_eq_op__lt,
    _cex_test_eq_op__le,
    _cex_test_eq_op__gt,
    _cex_test_eq_op__ge,
};

static Exc __attribute__((noinline))
_check_eq_int(i64 a, i64 b, int line, enum _cex_test_eq_op_e op)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    bool passed = false;
    char* ops = "?";
    switch (op) {
        case _cex_test_eq_op__na:
            unreachable();
            break;
        case _cex_test_eq_op__eq:
            passed = a == b;
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = a != b;
            ops = "==";
            break;
        case _cex_test_eq_op__lt:
            passed = a < b;
            ops = ">=";
            break;
        case _cex_test_eq_op__le:
            passed = a <= b;
            ops = ">";
            break;
        case _cex_test_eq_op__gt:
            passed = a > b;
            ops = "<=";
            break;
        case _cex_test_eq_op__ge:
            passed = a >= b;
            ops = "<";
            break;
    }
    if (!passed) {
        str.sprintf(
            _cex_test__mainfn_state.str_buf,
            sizeof(_cex_test__mainfn_state.str_buf),
            "%s:%d -> %ld %s %ld",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            ops,
            b
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eq_almost(f64 a, f64 b, f64 delta, int line)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    bool passed = false;
    f64 abdelta = a - b;
    if (isnan(a) && isnan(b)) {
        passed = true;
    } else if (isinf(a) && isinf(b)) {
        passed = (signbit(a) == signbit(b));
    } else {
        passed = fabs(abdelta) <= ((delta != 0) ? delta : (f64)0.0000001);
    }
    if (!passed) {
        str.sprintf(
            _cex_test__mainfn_state.str_buf,
            sizeof(_cex_test__mainfn_state.str_buf),
            "%s:%d -> %f != %f (delta: %f, diff: %f)",
            _cex_test__mainfn_state.suite_file,
            line,
            (a),
            (b),
            delta,
            abdelta
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eq_f32(f64 a, f64 b, int line, enum _cex_test_eq_op_e op)
{
    (void)op;
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    bool passed = false;
    char* ops = "?";
    f64 delta = a - b;
    bool is_equal = false;

    if (isnan(a) && isnan(b)) {
        is_equal = true;
    } else if (isinf(a) && isinf(b)) {
        is_equal = (signbit(a) == signbit(b));
    } else {
        is_equal = fabs(delta) <= (f64)0.0000001;
    }
    switch (op) {
        case _cex_test_eq_op__na:
            unreachable();
            break;
        case _cex_test_eq_op__eq:
            passed = is_equal;
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = !is_equal;
            ops = "==";
            break;
        case _cex_test_eq_op__lt:
            passed = a < b;
            ops = ">=";
            break;
        case _cex_test_eq_op__le:
            passed = a < b || is_equal;
            ops = ">";
            break;
        case _cex_test_eq_op__gt:
            passed = a > b;
            ops = "<=";
            break;
        case _cex_test_eq_op__ge:
            passed = a >= b || is_equal;
            ops = "<";
            break;
    }
    if (!passed) {
        str.sprintf(
            _cex_test__mainfn_state.str_buf,
            sizeof(_cex_test__mainfn_state.str_buf),
            "%s:%d -> %f %s %f (delta: %f)",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            ops,
            b,
            delta
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eq_str(char* a, char* b, int line, enum _cex_test_eq_op_e op)
{
    bool passed = false;
    char* ops = "";
    switch (op) {
        case _cex_test_eq_op__eq:
            passed = str.eq(a, b);
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = !str.eq(a, b);
            ops = "==";
            break;
        default:
            unreachable();
    }
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (!passed) {
        usize a_len = str.len(a);
        usize b_len = str.len(b);

        if (a_len < 20 && b_len < 20) {
            str.sprintf(
                _cex_test__mainfn_state.str_buf,
                CEX_TEST_AMSG_MAX_LEN - 1,
                "%s:%d -> '%s' %s '%s'",
                _cex_test__mainfn_state.suite_file,
                line,
                a,
                ops,
                b
            );
        } else {
            mem$scope(tmem$, _)
            {
                u32 n_errors = 0;
                sbuf_c errors = sbuf.create(CEX_TEST_AMSG_MAX_LEN, _);
                arr$(char*) a_lines = str.split_lines(a, _);
                arr$(char*) b_lines = str.split_lines(b, _);

                for (u32 i = 0, j = 0; i < arr$len(a_lines) || j < arr$len(b_lines); i++, j++) {
                    if (n_errors > 5) { break; }
                    if (i < arr$len(a_lines) && j < arr$len(b_lines)) {
                        char* al = a_lines[i];
                        char* bl = b_lines[j];
                        usize al_len = str.len(al);
                        usize bl_len = str.len(bl);

                        if (!str.eq(al, bl)) {
                            sbuf.appendf(&errors, "\tA at line %03d: `%s`\n", i, al);
                            sbuf.appendf(&errors, "\tB at line %03d: `%s`\n", j, bl);
                            usize max_len = al_len > bl_len ? al_len : bl_len;
                            usize min_len = al_len < bl_len ? al_len : bl_len;
                            sbuf.appendf(&errors, "\t                ");
                            for(usize z = 0; z < max_len; z++) {
                                if (z < min_len && al[z] == bl[z]) {
                                    sbuf.appendf(&errors, " ");
                                } else {
                                    sbuf.appendf(&errors, "^");
                                    break;
                                }
                            }
                            sbuf.appendf(&errors, "\n");
                            n_errors++;
                        }
                    } else {
                        if (i < arr$len(a_lines)) {
                            sbuf.appendf(&errors, "\tA at line %03d: `%s`\n", i, a_lines[i]);
                            sbuf.appendf(&errors, "\tB at line %03d: (too short)\n", i);
                        } else {
                            sbuf.appendf(&errors, "\tA at line %03d: (too short)\n", j);
                            sbuf.appendf(&errors, "\tB at line %03d: `%s`\n", j, b_lines[j]);
                        }
                        n_errors++;
                    }
                }
                str.sprintf(
                    _cex_test__mainfn_state.str_buf,
                    CEX_TEST_AMSG_MAX_LEN - 1,
                    "%s:%d -> strings are not equal\n%s\n",
                    _cex_test__mainfn_state.suite_file,
                    line,
                    errors

                );
            }
        }
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eq_err(char* a, char* b, int line)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (!str.eq(a, b)) {
        char* ea = (a == EOK) ? "Error.ok" : a;
        char* eb = (b == EOK) ? "Error.ok" : b;
        str.sprintf(
            _cex_test__mainfn_state.str_buf,
            CEX_TEST_AMSG_MAX_LEN - 1,
            "%s:%d -> Exc mismatch '%s' != '%s'",
            _cex_test__mainfn_state.suite_file,
            line,
            ea,
            eb
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}


static Exc __attribute__((noinline))
_check_eq_ptr(void* a, void* b, int line)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (a != b) {
        str.sprintf(
            _cex_test__mainfn_state.str_buf,
            CEX_TEST_AMSG_MAX_LEN - 1,
            "%s:%d -> %p != %p (ptr_diff: %ld)",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            b,
            (a - b)
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eqs_slice(str_s a, str_s b, int line, enum _cex_test_eq_op_e op)
{
    bool passed = false;
    char* ops = "";
    switch (op) {
        case _cex_test_eq_op__eq:
            passed = str.slice.eq(a, b);
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = !str.slice.eq(a, b);
            ops = "==";
            break;
        default:
            unreachable();
    }
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (!passed) {
        if (str.sprintf(
                _cex_test__mainfn_state.str_buf,
                CEX_TEST_AMSG_MAX_LEN - 1,
                "%s:%d -> '%S' %s '%S'",
                _cex_test__mainfn_state.suite_file,
                line,
                a,
                ops,
                b
            )) {}
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static void __attribute__((noinline))
cex_test_mute()
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    struct _cex_test_context_s* ctx = &_cex_test__mainfn_state;
    if (ctx->out_stream) {
        fflush(stdout);
        io.rewind(ctx->out_stream);
        fflush(ctx->out_stream);

#        ifdef _WIN32
        _dup2(_fileno(ctx->out_stream), STDOUT_FILENO);
#        else
        dup2(fileno(ctx->out_stream), STDOUT_FILENO);
#        endif
    }
}
static void __attribute__((noinline))
cex_test_unmute(Exc test_result)
{
    (void)test_result;
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    struct _cex_test_context_s* ctx = &_cex_test__mainfn_state;
    if (ctx->out_stream) {
        fflush(stdout);
        putc('\0', stdout);
        fflush(stdout);
        isize flen = io.file.size(ctx->out_stream);
        io.rewind(ctx->out_stream);
#        ifdef _WIN32
        _dup2(ctx->orig_stdout_fd, STDOUT_FILENO);
#        else
        dup2(ctx->orig_stdout_fd, STDOUT_FILENO);
#        endif

        if (test_result != EOK && flen > 1) {
            fflush(stdout);
            fflush(stderr);
            fprintf(stderr, "\n============== TEST OUTPUT >>>>>>>=============\n\n");
            int c;
            while ((c = fgetc(ctx->out_stream)) != EOF && c != '\0') { putc(c, stderr); }
            fprintf(stderr, "\n============== <<<<<< TEST OUTPUT =============\n");
        }
    }
}


static int __attribute__((noinline))
cex_test_main_fn(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    extern struct _cex_test_context_s _cex_test__mainfn_state;

    struct _cex_test_context_s* ctx = &_cex_test__mainfn_state;
    if (ctx->test_cases == NULL) {
        fprintf(stderr, "No test$case() in the test file: %s\n", __FILE__);
        return 1;
    }
    u32 max_name = 0;
    for$each (t, ctx->test_cases) {
        if (max_name < strlen(t.test_name) + 2) { max_name = strlen(t.test_name) + 2; }
    }
    max_name = (max_name < 70) ? 70 : max_name;

    ctx->quiet_mode = false;
    ctx->has_ansi = io.isatty(stdout);

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt(&ctx->case_filter, 'f', "filter", .help = "execute cases with filter"),
        argparse$opt(&ctx->quiet_mode, 'q', "quiet", .help = "run test in quiet_mode"),
        argparse$opt(&ctx->breakpoint, 'b', "breakpoint", .help = "breakpoint on tassert failure"),
        argparse$opt(
            &ctx->no_stdout_capture,
            'o',
            "no-capture",
            .help = "prints all stdout as test goes"
        ),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
        .description = "Test runner program",
    };

    e$except_silent (err, argparse.parse(&args, argc, argv)) { return 1; }

    if (!ctx->no_stdout_capture) {
        ctx->out_stream = tmpfile();
        if (ctx->out_stream == NULL) {
            fprintf(stderr, "Failed opening temp output for capturing tests\n");
            uassert(false && "TODO: test this");
        }
    }

#        ifdef _WIN32
    ctx->orig_stdout_fd = _dup(_fileno(stdout));
#        else
    ctx->orig_stdout_fd = dup(fileno(stdout));
#        endif

    mem$scope(tmem$, _)
    {
        void* data = mem$malloc(_, 120);
        (void)data;
        uassert(data != NULL && "priming temp allocator failed");
    }

    if (!ctx->quiet_mode) {
        fprintf(stderr, "-------------------------------------\n");
        fprintf(stderr, "Running Tests: %s\n", argv[0]);
        fprintf(stderr, "-------------------------------------\n\n");
    }
    if (ctx->setup_suite_fn) {
        Exc err = NULL;
        if ((err = ctx->setup_suite_fn())) {
            fprintf(
                stderr,
                "[%s] test$setup_suite() failed with %s (suite %s stopped)\n",
                ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                err,
                __FILE__
            );
            return 1;
        }
    }

    if (ctx->quiet_mode) { fprintf(stderr, "%s ", ctx->suite_file); }

    for$each (t, ctx->test_cases) {
        ctx->case_name = t.test_name;
        ctx->tests_run++;
        if (ctx->case_filter && !str.find(t.test_name, ctx->case_filter)) { continue; }

        if (!ctx->quiet_mode) {
            fprintf(stderr, "%s", t.test_name);
            for (u32 i = 0; i < max_name - strlen(t.test_name) + 2; i++) { putc('.', stderr); }
            if (ctx->no_stdout_capture) { putc('\n', stderr); }
        }

#        ifndef NDEBUG
        uassert_enable(); // unconditionally enable previously disabled asserts
#        endif
        Exc err = EOK;
        AllocatorHeap_c* alloc_heap = (AllocatorHeap_c*)mem$;
        alloc_heap->stats.n_allocs = 0;
        alloc_heap->stats.n_free = 0;

        if (ctx->setup_case_fn && (err = ctx->setup_case_fn()) != EOK) {
            fflush(stdout);
            fprintf(
                stderr,
                "[%s] test$setup() failed with '%s' (suite %s stopped)\n",
                ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                err,
                __FILE__
            );
            return 1;
        }

        cex_test_mute();
        err = t.test_fn();
        if (ctx->quiet_mode && err != EOK) {
            fprintf(stdout, "[%s] %s\n", ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL", err);
            fprintf(stdout, "Test suite: %s case: %s\n", ctx->suite_file, t.test_name);
        }
        cex_test_unmute(err);

        if (err == EOK) {
            if (ctx->quiet_mode) {
                fprintf(stderr, ".");
            } else {
                fprintf(stderr, "[%s]\n", ctx->has_ansi ? io$ansi("PASS", "32") : "PASS");
            }
        } else {
            ctx->tests_failed++;
            if (!ctx->quiet_mode) {
                fprintf(
                    stderr,
                    "[%s] %s (%s)\n",
                    ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                    err,
                    t.test_name
                );
            } else {
                fprintf(stderr, "F");
            }
        }
        if (ctx->teardown_case_fn && (err = ctx->teardown_case_fn()) != EOK) {
            fflush(stdout);
            fprintf(
                stderr,
                "[%s] test$teardown() failed with %s (suite %s stopped)\n",
                ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                err,
                __FILE__
            );
            return 1;
        }
        if (err == EOK && alloc_heap->stats.n_allocs != alloc_heap->stats.n_free) {
            if (!ctx->quiet_mode) {
                fprintf(stderr, "%s", t.test_name);
                for (u32 i = 0; i < max_name - strlen(t.test_name) + 2; i++) { putc('.', stderr); }
            } else {
                putc('\n', stderr);
            }
            fprintf(
                stderr,
                "[%s] %s:%d Possible memory leak: allocated %d != free %d\n",
                ctx->has_ansi ? io$ansi("LEAK", "33") : "LEAK",
                ctx->suite_file,
                t.test_line,
                alloc_heap->stats.n_allocs,
                alloc_heap->stats.n_free
            );
            ctx->tests_failed++;
        }
    }

    if (ctx->teardown_suite_fn) {
        e$except (err, ctx->teardown_suite_fn()) {
            fprintf(
                stderr,
                "[%s] test$teardown_suite() failed with %s (suite %s stopped)\n",
                ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                err,
                __FILE__
            );
            return 1;
        }
    }

    if (!ctx->quiet_mode) {
        fprintf(stderr, "\n-------------------------------------\n");
        fprintf(
            stderr,
            "Total: %d Passed: %d Failed: %d\n",
            ctx->tests_run,
            ctx->tests_run - ctx->tests_failed,
            ctx->tests_failed
        );
        fprintf(stderr, "-------------------------------------\n");
    } else {
        fprintf(stderr, "\n");

        if (ctx->tests_failed) {
            fprintf(
                stderr,
                "\n[%s] %s %d tests failed\n",
                (ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL"),
                ctx->suite_file,
                ctx->tests_failed
            );
        }
    }

    if (ctx->out_stream) {
        fclose(ctx->out_stream);
        ctx->out_stream = NULL;
    }
    return ctx->tests_run == 0 || ctx->tests_failed > 0;
}
#    endif // ifdef CEX_TEST
#endif



/*
*                          src/cex_code_gen.h
*/
#if !defined(cex$enable_minimal)


typedef struct _cex__codegen_s
{
    sbuf_c* buf;
    u32 indent;
    Exc error;
} _cex__codegen_s;

/*
 *                  CODE GEN MACROS
 */

/**
* Code generation module 

```c

test$case(test_codegen_test)
{
    sbuf_c b = sbuf.create(1024, mem$);
    // NOTE: cg$ macros should be working within cg$init() scope or make sure cg$var is available
    cg$init(&b);

    tassert(cg$var->buf == &b);
    tassert(cg$var->indent == 0);

    cg$pn("printf(\"hello world\");");
    cg$pn("#define GOO");
    cg$pn("// this is empty scope");
    cg$scope("", "")
    {
        cg$pf("printf(\"hello world: %d\");", 2);
    }

    cg$func("void my_func(int arg_%d)", 2)
    {
        cg$scope("var my_var = (mytype)", "")
        {
            cg$pf(".arg1 = %d,", 1);
            cg$pf(".arg2 = %d,", 2);
        }
        cg$pa(";\n", "");

        cg$if("foo == %d", 312)
        {
            cg$pn("printf(\"Hello: %d\", foo);");
        }
        cg$elseif("bar == foo + %d", 7)
        {
            cg$pn("// else if scope");
        }
        cg$else()
        {
            cg$pn("// else scope");
        }

        cg$while("foo == %d", 312)
        {
            cg$pn("printf(\"Hello: %d\", foo);");
        }

        cg$for("u32 i = 0; i < %d; i++", 312)
        {
            cg$pn("printf(\"Hello: %d\", foo);");
            cg$foreach("it, my_var", "")
            {
                cg$pn("printf(\"Hello: %d\", foo);");
            }
        }

        cg$scope("do ", "")
        {
            cg$pf("// do while", 1);
        }
        cg$pa(" while(0);\n", "");
    }

    cg$switch("foo", "")
    {
        cg$case("'%c'", 'a')
        {
            cg$pn("// case scope");
        }
        cg$scope("case '%c': ", 'b')
        {
            cg$pn("fallthrough();");
        }
        cg$default()
        {
            cg$pn("// default scope");
        }
    }

    tassert(cg$is_valid());

    printf("result: \n%s\n", b);


    sbuf.destroy(&b);
    return EOK;
}

```

*/
#define __cg$

/// Common code gen buffer variable (all cg$ macros use it under the hood)
#    define cg$var __cex_code_gen

/// Initializes new code generator (uses sbuf instance as backing buffer)
#    define cg$init(out_sbuf)                                                                      \
        _cex__codegen_s cex$tmpname(code_gen) = { .buf = (out_sbuf) };                             \
        _cex__codegen_s* cg$var = &cex$tmpname(code_gen)

/// false if any cg$ operation failed, use cg$var->error to get Exception type of error
#    define cg$is_valid() (cg$var != NULL && cg$var->buf != NULL && cg$var->error == EOK)

/// increase code indent by 4
#    define cg$indent() ({ cg$var->indent += 4; })

/// decrease code indent by 4
#    define cg$dedent()                                                                            \
        ({                                                                                         \
            if (cg$var->indent >= 4) cg$var->indent -= 4;                                          \
        })

/*
 *                  CODE MACROS
 */
/// add new line of code
#    define cg$pn(text)                                                                            \
        ((text && text[0] == '\0') ? _cex__codegen_print_line(cg$var, "\n")                        \
                                   : _cex__codegen_print_line(cg$var, "%s\n", text))

/// add new line of code with formatting
#    define cg$pf(format, ...) _cex__codegen_print_line(cg$var, format "\n", __VA_ARGS__)

/// append code at the current line without "\n"
#    define cg$pa(format, ...) _cex__codegen_print(cg$var, true, format, __VA_ARGS__)

// clang-format off

/// add new code scope with indent (use for low-level stuff)
#define cg$scope(format, ...) \
    for (_cex__codegen_s* cex$tmpname(codegen_scope)  \
                __attribute__ ((__cleanup__(_cex__codegen_print_scope_exit))) =     \
                _cex__codegen_print_scope_enter(cg$var, format, __VA_ARGS__),       \
        *cex$tmpname(codegen_sentinel) = cg$var;                                    \
        cex$tmpname(codegen_sentinel) && cex$tmpname(codegen_scope) != NULL;        \
        cex$tmpname(codegen_sentinel) = NULL)
// clang-format on


/// add new function     cg$func("void my_func(int arg_%d)", 2)
#    define cg$func(format, ...) cg$scope(format, __VA_ARGS__)

/// add if statement
#    define cg$if(format, ...) cg$scope("if (" format ") ", __VA_ARGS__)

/// add else if
#    define cg$elseif(format, ...)                                                                   \
        cg$pa(" else ", "");                                                                       \
        cg$if(format, __VA_ARGS__)

/// add else 
#    define cg$else()                                                                                \
        cg$pa(" else", "");                                                                        \
        cg$scope(" ", "")

/// add while loop
#    define cg$while(format, ...) cg$scope("while (" format ") ", __VA_ARGS__)

/// add for loop
#    define cg$for(format, ...) cg$scope("for (" format ") ", __VA_ARGS__)

/// add CEX for$each loop
#    define cg$foreach(format, ...) cg$scope("for$each (" format ") ", __VA_ARGS__)

/// add switch() statement
#    define cg$switch(format, ...) cg$scope("switch (" format ") ", __VA_ARGS__)

/// add case in switch() statement
#    define cg$case(format, ...)                                                                     \
        for (_cex__codegen_s * cex$tmpname(codegen_scope)                                          \
                                   __attribute__((__cleanup__(_cex__codegen_print_case_exit))) =   \
                 _cex__codegen_print_case_enter(cg$var, "case " format, __VA_ARGS__),              \
                                   *cex$tmpname(codegen_sentinel) = cg$var;                        \
             cex$tmpname(codegen_sentinel) && cex$tmpname(codegen_scope) != NULL;                  \
             cex$tmpname(codegen_sentinel) = NULL)

/// add default in switch() statement
#    define cg$default()                                                                             \
        for (_cex__codegen_s * cex$tmpname(codegen_scope)                                          \
                                   __attribute__((__cleanup__(_cex__codegen_print_case_exit))) =   \
                 _cex__codegen_print_case_enter(cg$var, "default", NULL),                          \
                                   *cex$tmpname(codegen_sentinel) = cg$var;                        \
             cex$tmpname(codegen_sentinel) && cex$tmpname(codegen_scope) != NULL;                  \
             cex$tmpname(codegen_sentinel) = NULL)


void _cex__codegen_print_line(_cex__codegen_s* cg, char* format, ...);
void _cex__codegen_print(_cex__codegen_s* cg, bool rep_new_line, char* format, ...);
_cex__codegen_s* _cex__codegen_print_scope_enter(_cex__codegen_s* cg, char* format, ...);
void _cex__codegen_print_scope_exit(_cex__codegen_s** cgptr);
_cex__codegen_s* _cex__codegen_print_case_enter(_cex__codegen_s* cg, char* format, ...);
void _cex__codegen_print_case_exit(_cex__codegen_s** cgptr);
void _cex__codegen_indent(_cex__codegen_s* cg);

#endif



/*
*                          src/cexy.h
*/
#if !defined(cex$enable_minimal)

#if defined(CEX_BUILD) || defined(CEX_NEW)

#    ifndef cexy$cex_self_cc
#        if defined(__clang__)
/// Macro constant derived from the compiler type used to initially build ./cex app
#            define cexy$cex_self_cc "clang"
#        elif defined(__GNUC__)
#            define cexy$cex_self_cc "gcc"
#        else
#            error "Compiler type is not supported"
#        endif
#    endif // #ifndef cexy$cex_self_cc

#    ifndef cexy$cc
/// Default compiler for building tests/apps (by default inferred from ./cex tool compiler)
#        define cexy$cc cexy$cex_self_cc
#    endif // #ifndef cexy$cc


#    ifndef cexy$cc_include
/// Include path for the #include "some.h" (may be overridden by user)
#        define cexy$cc_include "-I."
#    endif

#    ifndef cexy$build_dir
/// Build dir for project executables and tests (may be overridden by user)
#        define cexy$build_dir "./build"
#    endif

#    ifndef cexy$src_dir
/// Directory for applications and code (may be overridden by user)
#        define cexy$src_dir "./src"
#    endif

#    ifndef cexy$create_compile_flags
/// If 1 creates `compile_flags.txt` in project dir at every ./cex run, 0 - ignores creation (default: 1)
#       define cexy$create_compile_flags 1
#    endif

#    ifndef cexy$cc_args_sanitizer
#        if defined(_WIN32) || defined(MACOS_X) || defined(__APPLE__)
#            if defined(__clang__)
/// Debug mode and tests sanitizer flags (may be overridden by user)
#                define cexy$cc_args_sanitizer                                                     \
                    "-fsanitize-address-use-after-scope", "-fsanitize=address",                    \
                        "-fsanitize=undefined", "-fstack-protector-strong"
#            else
#                define cexy$cc_args_sanitizer "-fstack-protector-strong"
#            endif
#        else
#            define cexy$cc_args_sanitizer                                                         \
                "-fsanitize-address-use-after-scope", "-fsanitize=address",                        \
                    "-fsanitize=undefined", "-fsanitize=leak", "-fstack-protector-strong"
#        endif
#    endif

#    ifndef cexy$cc_args
/// Common compiler flags (may be overridden by user)
#        define cexy$cc_args "-Wall", "-Wextra", "-Werror", "-g3", cexy$cc_args_sanitizer
#    endif

#    ifndef cexy$cc_args_test
/// Test runner compiler flags (may be overridden by user)
#        define cexy$cc_args_test cexy$cc_args, "-Wno-unused-function", "-Itests/"
#    endif

#    ifndef cexy$fuzzer
/// Fuzzer compilation command (supports clang libfuzzer and afl++)
#        define cexy$fuzzer "clang", "-O0", "-Wall", "-Wextra", "-Werror", "-g", "-Wno-unused-function", "-fsanitize=address,fuzzer,undefined", "-fsanitize-undefined-trap-on-error"
#    endif

#    ifndef cexy$cex_self_args
/// Compiler flags used for building ./cex.c -> ./cex (may be overridden by user)
#        define cexy$cex_self_args
#    endif

#    ifndef cexy$pkgconf_cmd
/// Dependency resolver command: pkg-config, pkgconf, etc. May be used in cross-platform
/// compilation, allowed multiple command arguments here
#        define cexy$pkgconf_cmd "pkgconf"
#    endif

/// Helper macro for running cexy.utils.pkgconf() a dependency resolver for libs
#    define cexy$pkgconf(allocator, out_cc_args, pkgconf_args...)                                  \
        ({                                                                                         \
            char* _args[] = { pkgconf_args };                                                      \
            usize _args_len = arr$len(_args);                                                      \
            cexy.utils.pkgconf(allocator, out_cc_args, _args, _args_len);                          \
        })

#    ifndef cexy$pkgconf_libs
/// list of standard system project libs (for example: "lua5.3", "libz")
#        define cexy$pkgconf_libs
#    endif

#    ifndef cexy$vcpkg_root
/// Current vcpkg root path (where ./vcpkg tool is located)
#        define cexy$vcpkg_root NULL
#    endif

#    ifndef cexy$vcpkg_triplet
/// Current build triplet (empty, NULL, or string like "x64-linux")
///   if you are using  `vcpkg install mydep`, ignored if blank or NULL, 
///   list of all supported triplets is here: `vcpkg help triplet`)
#        define cexy$vcpkg_triplet
#    endif

#    ifndef cexy$ld_args
/// Linker flags (e.g. -L./lib/path/ -lmylib -lm) (may be overridden)
#        define cexy$ld_args
#    endif

#    ifndef cexy$debug_cmd
/// Command for launching debugger for cex test/app debug (may be overridden)
#        define cexy$debug_cmd "gdb", "-q", "--args"
#    endif

#    ifndef cexy$build_ext_exe
#        ifdef _WIN32
/// Extension for executables (e.g. '.exe' for win32)
#            define cexy$build_ext_exe ".exe"
#        else
#            define cexy$build_ext_exe ""
#        endif
#    endif

#    ifndef cexy$build_ext_lib_dyn
#        ifdef _WIN32
/// Extension for dynamic linked libs (".dll" win, ".so" linux)
#            define cexy$build_ext_lib_dyn ".dll"
#        else
#            define cexy$build_ext_lib_dyn ".so"
#        endif
#    endif

#    ifndef cexy$build_ext_lib_stat
#        ifdef _WIN32
/// Extension for static libs (".lib" win, ".a" linux)
#            define cexy$build_ext_lib_stat ".lib"
#        else
#            define cexy$build_ext_lib_stat ".a"
#        endif
#    endif

#    ifndef cexy$process_ignore_kw
/**
@brief Pattern for ignoring extra macro keywords in function signatures (for cex process).
See `cex help str.match` for more information about patter syntax.
*/
#        define cexy$process_ignore_kw ""
#    endif


#    define _cexy$cmd_process                                                                      \
        { .name = "process",                                                                       \
          .func = cexy.cmd.process,                                                                \
          .help = "Create CEX namespaces from project source code" }

#    define _cexy$cmd_new { .name = "new", .func = cexy.cmd.new, .help = "Create new CEX project" }

#    define _cexy$cmd_help                                                                         \
        { .name = "help",                                                                          \
          .func = cexy.cmd.help,                                                                   \
          .help = "Search cex.h and project symbols and extract help" }

#    define _cexy$cmd_config                                                                       \
        { .name = "config",                                                                        \
          .func = cexy.cmd.config,                                                                 \
          .help = "Check project and system environment and config" }

#    define _cexy$cmd_libfetch                                                                     \
        { .name = "libfetch",                                                                      \
          .func = cexy.cmd.libfetch,                                                               \
          .help = "Get 3rd party source code via git or install CEX libs" }

#    define _cexy$cmd_stats                                                                        \
        { .name = "stats",                                                                         \
          .func = cexy.cmd.stats,                                                                  \
          .help = "Calculate project lines of code and quality stats" }

/// Simple test runner command (test runner, debugger launch, etc)
#    define cexy$cmd_test                                                                          \
        { .name = "test",                                                                          \
          .func = cexy.cmd.simple_test,                                                            \
          .help = "Generic unit test build/run/debug" }

/// Simple fuzz tests runner command
#    define cexy$cmd_fuzz                                                                          \
        { .name = "fuzz",                                                                          \
          .func = cexy.cmd.simple_fuzz,                                                            \
          .help = "Generic fuzz tester" }


/// Simple app build command (unity build, simple linking, runner, debugger launch, etc)
#    define cexy$cmd_app                                                                           \
        { .name = "app", .func = cexy.cmd.simple_app, .help = "Generic app build/run/debug" }

/// All built-in commands for ./cex tool
#    define cexy$cmd_all                                                                           \
        _cexy$cmd_help, _cexy$cmd_process, _cexy$cmd_new, _cexy$cmd_stats, _cexy$cmd_config,       \
            _cexy$cmd_libfetch

/// Initialize CEX build system (build itself)
#    define cexy$initialize() cexy.build_self(argc, argv, __FILE__)
/// ./cex --help description
#    define cexy$description "\nCEX language (cexy$) build and project management system"
/// ./cex --help usage
#    define cexy$usage " [-D] [-D<ARG1>] [-D<ARG2>] command [options] [args]"

// clang-format off
/// ./cex --help epilog
    #define cexy$epilog \
        "\nYou may try to get help for commands as well, try `cex process --help`\n"\
        "Use `cex -DFOO -DBAR config` to set project config flags\n"\
        "Use `cex -D config` to reset all project config flags to defaults\n"
// clang-format on

// clang-format off
#define _cexy$cmd_test_help (\
        "CEX built-in simple test runner\n"\
        "\nEach cexy test is self-sufficient and unity build, which allows you to test\n"\
        "static funcions, apply mocks to some selected modules and functions, have more\n"\
        "control over your code. See `cex config --help` for customization/config info.\n"\
\
        "\nCEX test runner keep checking include modified time to track changes in the\n"\
        "source files. It expects that each #include \"myfile.c\" has \"myfile.h\" in \n" \
        "the same folder. Test runner uses cexy$cc_include for searching.\n"\
\
        "\nCEX is a test-centric language, it enables additional sanity checks then in\n" \
        "test suite, all warnings are enabled -Wall -Wextra. Sanitizers are enabled by\n"\
        "default.\n"\
\
        "\nCode requirements:\n"\
        "1. You should include all your source files directly using #include \"path/src.c\"\n"\
        "2. If needed provide linker options via cexy$ld_libs / cexy$ld_args\n"\
        "3. If needed provide compiler options via cexy$cc_args_test\n"\
        "4. All tests have to be in tests/ folder, and start with `test_` prefix \n"\
        "5. Only #include with \"\" checked for modification \n"\
\
        "\nTest suite setup/teardown:\n"\
        "// setup before every case  \n"\
        "test$setup_case() {return EOK;} \n"\
        "// teardown after every case \ntest$setup_case() {return EOK;} \n"\
        "// setup before suite (only once) \ntest$setup_suite() {return EOK;} \n"\
        "// teardown after suite (only once) \ntest$setup_suite() {return EOK;} \n"\
\
        "\nTest case:\n"\
        "\ntest$case(my_test_case_name) {\n"\
        "    // run `cex help tassert_` / `cex help tassert_eq` to get more info \n" \
        "    tassert(0 == 1);\n"\
        "    tassertf(0 == 1, \"this is a failure msg: %d\", 3);\n"\
        "    tassert_eq(buf, \"foo\");\n"\
        "    tassert_eq(1, true);\n"\
        "    tassert_eq(str.sstr(\"bar\"), str$s(\"bar\"));\n"\
        "    tassert_ne(1, 0);\n"\
        "    tassert_le(0, 1);\n"\
        "    tassert_lt(0, 1);\n"\
        "    return EOK;\n"\
        "}\n"\
        \
        "\nIf you need more control you can build your own test runner. Just use cex help\n"\
        "and get source code `./cex help --source cexy.cmd.simple_test`\n")
        
#define _cexy$cmd_test_epilog \
        "\nTest running examples: \n"\
        "cex test create tests/test_file.c        - creates new test file from template\n"\
        "cex test build all                       - build all tests\n"\
        "cex test run all                         - build and run all tests\n"\
        "cex test run tests/test_file.c           - run test by path\n"\
        "cex test debug tests/test_file.c         - run test via `cexy$debug_cmd` program\n"\
        "cex test clean all                       - delete all test executables in `cexy$build_dir`\n"\
        "cex test clean test/test_file.c          - delete specific test executable\n"\
        "cex test run tests/test_file.c [--help]  - run test with passing arguments to the test runner program\n"


// clang-format on
struct __cex_namespace__cexy {
    // Autogenerated by CEX
    // clang-format off

    void            (*build_self)(int argc, char** argv, char* cex_source);
    bool            (*src_changed)(char* target_path, char** src_array, usize src_array_len);
    bool            (*src_include_changed)(char* target_path, char* src_path, arr$(char*) alt_include_path);
    char*           (*target_make)(char* src_path, char* build_dir, char* name_or_extension, IAllocator allocator);

    struct {
        Exception       (*clean)(char* target);
        Exception       (*create)(char* target);
        Exception       (*find_app_target_src)(IAllocator allc, char* target, char** out_result);
        Exception       (*run)(char* target, bool is_debug, int argc, char** argv);
    } app;

    struct {
        Exception       (*config)(int argc, char** argv, void* user_ctx);
        Exception       (*help)(int argc, char** argv, void* user_ctx);
        Exception       (*libfetch)(int argc, char** argv, void* user_ctx);
        Exception       (*new)(int argc, char** argv, void* user_ctx);
        Exception       (*process)(int argc, char** argv, void* user_ctx);
        Exception       (*simple_app)(int argc, char** argv, void* user_ctx);
        Exception       (*simple_fuzz)(int argc, char** argv, void* user_ctx);
        Exception       (*simple_test)(int argc, char** argv, void* user_ctx);
        Exception       (*stats)(int argc, char** argv, void* user_ctx);
    } cmd;

    struct {
        Exception       (*create)(char* target);
    } fuzz;

    struct {
        Exception       (*clean)(char* target);
        Exception       (*create)(char* target, bool include_sample);
        Exception       (*make_target_pattern)(char** target);
        Exception       (*run)(char* target, bool is_debug, int argc, char** argv);
    } test;

    struct {
        char*           (*git_hash)(IAllocator allc);
        Exception       (*git_lib_fetch)(char* git_url, char* git_label, char* out_dir, bool update_existing, bool preserve_dirs, char** repo_paths, usize repo_paths_len);
        Exception       (*make_compile_flags)(char* flags_file, bool include_cexy_flags, arr$(char*) cc_flags_or_null);
        Exception       (*make_new_project)(char* proj_dir);
        Exception       (*pkgconf)(IAllocator allc, arr$(char*)* out_cc_args, char** pkgconf_args, usize pkgconf_args_len);
    } utils;

    // clang-format on
};
#endif // #if defined(CEX_BUILD)
CEX_NAMESPACE struct __cex_namespace__cexy cexy;

#endif



/*
*                          src/CexParser.h
*/

#if !defined(cex$enable_minimal)

#define _CexTknList                                                                                \
    X(eof)                                                                                         \
    X(unk)                                                                                         \
    X(error)                                                                                       \
    X(ident)                                                                                       \
    X(number)                                                                                      \
    X(string)                                                                                      \
    X(char)                                                                                        \
    X(comment_single)                                                                              \
    X(comment_multi)                                                                               \
    X(preproc)                                                                                     \
    X(lparen)                                                                                      \
    X(rparen)                                                                                      \
    X(lbrace)                                                                                      \
    X(rbrace)                                                                                      \
    X(lbracket)                                                                                    \
    X(rbracket)                                                                                    \
    X(bracket_block)                                                                               \
    X(brace_block)                                                                                 \
    X(paren_block)                                                                                 \
    X(star)                                                                                        \
    X(minus)                                                                                       \
    X(plus)                                                                                        \
    X(dot)                                                                                         \
    X(comma)                                                                                       \
    X(eq)                                                                                          \
    X(colon)                                                                                       \
    X(question)                                                                                    \
    X(eos)                                                                                         \
    X(typedef)                                                                                     \
    X(func_decl)                                                                                   \
    X(func_def)                                                                                    \
    X(macro_const)                                                                                 \
    X(macro_func)                                                                                  \
    X(var_decl)                                                                                    \
    X(var_def)                                                                                     \
    X(cex_module_struct)                                                                           \
    X(cex_module_decl)                                                                             \
    X(cex_module_def)                                                                              \
    X(global_misc)                                                                                 \
    X(count)

typedef enum CexTkn_e
{
#define X(name) CexTkn__##name,
    _CexTknList
#undef X
} CexTkn_e;

extern const char* CexTkn_str[];

typedef struct cex_token_s
{
    CexTkn_e type;
    str_s value;
} cex_token_s;

typedef struct CexParser_c
{
    char* content;     // full content
    char* cur;         // current cursor in the source
    char* content_end; // last pointer of the source
    u32 line;          // current cursor line relative to content beginning
    u32 col;           // current cursor column relative to start of the line
    bool fold_scopes;  // count all {} / () / [] as a single token CexTkn_*_block
} CexParser_c;

typedef struct cex_decl_s
{
    str_s name;       // function/macro/const/var name
    str_s docs;       // reference to closest /// or /** block
    str_s body;       // reference to code {} if applicable
    sbuf_c ret_type;  // refined return type
    sbuf_c args;      // refined argument list
    const char* file; // decl file (must be set externally)
    u32 line;         // decl line in the source code
    CexTkn_e type;    // decl type (typedef, func, macro, etc)
    bool is_static;   // decl is a static func
    bool is_inline;   // decl is a inline func
} cex_decl_s;


CEX_NAMESPACE struct __cex_namespace__CexParser CexParser;

struct __cex_namespace__CexParser {
    // Autogenerated by CEX
    // clang-format off

    CexParser_c     (*create)(char* content, u32 content_len, bool fold_scopes);
    void            (*decl_free)(cex_decl_s* decl, IAllocator alloc);
    cex_decl_s*     (*decl_parse)(CexParser_c* lx, cex_token_s decl_token, arr$(cex_token_s) children, char* ignore_keywords_pattern, IAllocator alloc);
    cex_token_s     (*next_entity)(CexParser_c* lx, arr$(cex_token_s)* children);
    cex_token_s     (*next_token)(CexParser_c* lx);
    void            (*reset)(CexParser_c* lx);

    // clang-format on
};

#endif



/*
*                          src/cex_maker.h
*/


/*
*                          src/fuzz.h
*/
#if !defined(cex$enable_minimal)

/// Fuzz case: ``int fuzz$case(const u8* data, usize size) { return 0;}
#define fuzz$case test$noopt LLVMFuzzerTestOneInput

/// Fuzz test constructor (for building corpus seeds programmatically)
#define fuzz$setup __attribute__((constructor)) void _cex_fuzzer_setup_constructor

/// Special fuzz variable used by all fuzz$ macros
#define fuzz$dvar _cex_fuz_dvar

/// Initialize fuzz$ helper macros
#define fuzz$dnew(data, size) cex_fuzz_s fuzz$dvar = fuzz.create((data), (size))

/// Load random data into variable by pointer from random fuzz data
#define fuzz$dget(out_result_ptr) fuzz.dget(&(fuzz$dvar), (out_result_ptr), sizeof(*(out_result_ptr)))

/// Get deterministic probability based on fuzz data
#define fuzz$dprob(prob_threshold) fuzz.dprob(&(fuzz$dvar), prob_threshold)

/// Current fuzz_ file corpus directory relative to calling source file
#define fuzz$corpus_dir fuzz.corpus_dir(__FILE_NAME__)                                                                          \

/// Fuzz data fetcher (makes C-type payloads from random fuzz$case data)
typedef struct cex_fuzz_s
{
    const u8* cur;
    const u8* end;
} cex_fuzz_s;

#ifndef CEX_FUZZ_MAX_BUF
#    define CEX_FUZZ_MAX_BUF 1024000
#endif

#ifndef CEX_FUZZ_AFL
/// Fuzz main function
#    define fuzz$main()
#else
#    ifndef __AFL_INIT
void __AFL_INIT(void);
#    endif
#    ifndef __AFL_LOOP
int __AFL_LOOP(unsigned int N);
#    endif
#    define fuzz$main()                                                                            \
        test$noopt int main(int argc, char** argv)                                                 \
        {                                                                                          \
            (void)argc;                                                                            \
            (void)argv;                                                                            \
            isize len = 0;                                                                         \
            char buf[CEX_FUZZ_MAX_BUF];                                                            \
            __AFL_INIT();                                                                          \
            while (__AFL_LOOP(UINT_MAX)) {                                                         \
                len = read(0, buf, sizeof(buf));                                                   \
                if (len < 0) {                                                                     \
                    log$error("ERROR: reading AFL input: %s\n", strerror(errno));                  \
                    return errno;                                                                  \
                }                                                                                  \
                if (len == 0) {                                                                    \
                    log$info("Nothing to read, breaking?\n");                                      \
                    break;                                                                         \
                }                                                                                  \
                u8* test_data = malloc(len);                                                       \
                uassert(test_data != NULL && "memory error");                                      \
                memcpy(test_data, buf, len);                                                       \
                LLVMFuzzerTestOneInput(test_data, (usize)len);                                     \
                free(test_data);                                                                   \
            }                                                                                      \
            return 0;                                                                              \
        }

#endif

/**
- Fuzz runner commands

`./cex fuzz create fuzz/myapp/fuzz_bar.c`

`./cex fuzz run fuzz/myapp/fuzz_bar.c`

- Fuzz testing tools using `fuzz` namespace

```c

int
fuzz$case(const u8* data, usize size)
{
    cex_fuzz_s fz = fuzz.create(data, size);
    u16 random_val = 0;
    some_struct random_struct = {0}; 

    while(fuzz.dget(&fz, &random_val, sizeof(random_val))) {
        // testing function with random data
        my_func(random_val);

        // checking probability based on fuzz data
        if (fuzz.dprob(&fz, 0.2)) {
            my_func(random_val * 10);
        }

        if (fuzz.dget(&fz, &random_struct, sizeof(random_struct))){
            my_func_struct(&random_struct);
        }
    }
}

```
- Fuzz testing tools using `fuzz$` macros (shortcuts)

```c

int
fuzz$case(const u8* data, usize size)
{
    fuzz$dnew(data, size);

    u16 random_val = 0;
    some_struct random_struct = {0}; 

    while(fuzz$dget(&random_val)) {
        // testing function with random data
        my_func(random_val);

        // checking probability based on fuzz data
        if (fuzz$dprob(0.2)) {
            my_func(random_val * 10);
        }

        // it's possible to fill whole structs with data
        if (fuzz$dget(&random_struct)){
            my_func_struct(&random_struct);
        }
    }
}

```

- Fuzz corpus priming (it's optional step, but useful)

```c

typedef struct fuzz_match_s
{
    char pattern[100];
    char null_term;
    char text[300];
    char null_term2;
} fuzz_match_s;

Exception
match_make(char* out_file, char* text, char* pattern)
{
    fuzz_match_s f = { 0 };
    e$ret(str.copy(f.text, text, sizeof(f.text)));
    e$ret(str.copy(f.pattern, pattern, sizeof(f.pattern)));

    FILE* fh;
    e$ret(io.fopen(&fh, out_file, "wb"));
    e$ret(io.fwrite(fh, &f, sizeof(f)));
    io.fclose(&fh);

    return EOK;
}

fuzz$setup()
{
    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    struct
    {
        char* text;
        char* pattern;
    } match_tuple[] = {
        { "test", "*" },
        { "", "*" },
        { ".txt", "*.txt" },
        { "test.txt", "" },
        { "test.txt", "*txt" },
    };
    mem$scope(tmem$, _)
    {
        for (u32 i = 0; i < arr$len(match_tuple); i++) {
            char* fn = str.fmt(_, "%s/%05d", fuzz$corpus_dir, i);
            e$except (err, match_make(fn, match_tuple[i].text, match_tuple[i].pattern)) {
                uassertf(false, "Error writing file: %s", fn);
            }
        }
    }
}

```

*/
struct __cex_namespace__fuzz {
    // Autogenerated by CEX
    // clang-format off

    /// Get current corpus dir relative tho the `this_file_name`
    char*           (*corpus_dir)(char* this_file_name);
    /// Creates new fuzz data generator, for fuzz-driven randomization
    cex_fuzz_s      (*create)(const u8* data, usize size);
    /// Get result from random data into buffer (returns false if not enough data)
    bool            (*dget)(cex_fuzz_s* fz, void* out_result, usize result_size);
    /// Get deterministic probability using fuzz data, based on threshold
    bool            (*dprob)(cex_fuzz_s* fz, double threshold);

    // clang-format on
};
CEX_NAMESPACE struct __cex_namespace__fuzz fuzz;

#endif



/*
*                          src/cex_footer.h
*/


/*
*                   CEX IMPLEMENTATION 
*/



#if defined(CEX_IMPLEMENTATION) || defined(CEX_NEW)



#define _cex_main_boilerplate \
"#if !defined(cex$enable_minimal)\n"\
"\n"\
"#if __has_include(\"cex_config.h\")\n"\
"// Custom config file\n"\
"#    include \"cex_config.h\"\n"\
"#else\n"\
"// Overriding config values\n"\
"#    define cexy$cc_include \"-I.\", \"-I./lib\"\n"\
"#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */\n"\
"#endif\n"\
"\n"\
"#define CEX_IMPLEMENTATION\n"\
"#define CEX_BUILD\n"\
"#include \"cex.h\"\n"\
"\n"\
"Exception cmd_build_lib(int argc, char** argv, void* user_ctx);\n"\
"\n"\
"int\n"\
"main(int argc, char** argv)\n"\
"{\n"\
"\n"\
"    cexy$initialize(); // cex self rebuild and init\n"\
"    argparse_c args = {\n"\
"        .description = cexy$description,\n"\
"        .epilog = cexy$epilog,\n"\
"        .usage = cexy$usage,\n"\
"        argparse$cmd_list(\n"\
"            cexy$cmd_all,\n"\
"            cexy$cmd_fuzz, /* feel free to make your own if needed */\n"\
"            cexy$cmd_test, /* feel free to make your own if needed */\n"\
"            cexy$cmd_app,  /* feel free to make your own if needed */\n"\
"            { .name = \"build-lib\", .func = cmd_build_lib, .help = \"Custom build command\" },\n"\
"        ),\n"\
"    };\n"\
"    if (argparse.parse(&args, argc, argv)) { return 1; }\n"\
"    void* my_user_ctx = NULL; // passed as `user_ctx` to command\n"\
"    if (argparse.run_command(&args, my_user_ctx)) { return 1; }\n"\
"    return 0;\n"\
"}\n"\
"\n"\
"/// Custom build command for building static lib\n"\
"Exception\n"\
"cmd_build_lib(int argc, char** argv, void* user_ctx)\n"\
"{\n"\
"    (void)argc;\n"\
"    (void)argv;\n"\
"    (void)user_ctx;\n"\
"    log$info(\"Launching custom command\\n\");\n"\
"    return EOK;\n"\
"}\n"\
"\n"\
"#endif\n"\
""

/*
*                          src/cex_platform.c
*/



/*
*                          src/cex_base.c
*/


const struct _CEX_Error_struct Error = {
    .ok = EOK,                       // Success
    .memory = "MemoryError",         // memory allocation error
    .io = "IOError",                 // IO error
    .overflow = "OverflowError",     // buffer overflow
    .argument = "ArgumentError",     // function argument error
    .integrity = "IntegrityError",   // data integrity error
    .exists = "ExistsError",         // entity or key already exists
    .not_found = "NotFoundError",    // entity or key already exists
    .skip = "ShouldBeSkipped",       // NOT an error, function result must be skipped
    .empty = "EmptyError",           // resource is empty
    .eof = "EOF",                    // end of file reached
    .argsparse = "ProgramArgsError", // program arguments empty or incorrect
    .runtime = "RuntimeError",       // generic runtime error
    .assert = "AssertError",         // generic runtime check
    .os = "OSError",                 // generic OS check
    .timeout = "TimeoutError",       // await interval timeout
    .permission = "PermissionError", // Permission denied
    .try_again = "TryAgainError",    // EAGAIN / EWOULDBLOCK errno analog for async operations
};

#ifdef _cex$platform_panic_builtin

void
__cex__panic(void)
{
    fflush(stdout);
    fflush(stderr);
    sanitizer_stack_trace();

#ifdef CEX_TEST
    breakpoint();
#else
    abort();
#endif
    return;
}

#endif



/*
*                          src/mem.c
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_mem)


void
_cex_allocator_memscope_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    (*allc)->scope_exit(*allc);
}

void
_cex_allocator_arena_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    AllocatorArena.destroy(*allc);
}

// NOTE: destructor(101) - 101 lowest priority for destructors
__attribute__((destructor(101))) void
_cex_global_allocators_destructor()
{
    AllocatorArena_c* allc = (AllocatorArena_c*)tmem$;
    allocator_arena_page_s* page = allc->last_page;
    while (page) {
        auto tpage = page->prev_page;
        mem$free(mem$, page);
        page = tpage;
    }
}

#endif



/*
*                          src/AllocatorHeap.c
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_mem)


// clang-format off
static void* _cex_allocator_heap__malloc(IAllocator self,usize size, usize alignment);
static void* _cex_allocator_heap__calloc(IAllocator self,usize nmemb, usize size, usize alignment);
static void* _cex_allocator_heap__realloc(IAllocator self,void* ptr, usize size, usize alignment);
static void* _cex_allocator_heap__free(IAllocator self,void* ptr);
static IAllocator  _cex_allocator_heap__scope_enter(IAllocator self);
static void _cex_allocator_heap__scope_exit(IAllocator self);
static u32 _cex_allocator_heap__scope_depth(IAllocator self);

AllocatorHeap_c _cex__default_global__allocator_heap = {
    .alloc = {
        .malloc = _cex_allocator_heap__malloc,
        .realloc = _cex_allocator_heap__realloc,
        .calloc = _cex_allocator_heap__calloc,
        .free = _cex_allocator_heap__free,
        .scope_enter = _cex_allocator_heap__scope_enter,
        .scope_exit = _cex_allocator_heap__scope_exit,
        .scope_depth = _cex_allocator_heap__scope_depth,
        .meta = {
            .magic_id =CEX_ALLOCATOR_HEAP_MAGIC,
            .is_arena = false,
            .is_temp = false, 
        }
    },
};
IAllocator const _cex__default_global__allocator_heap__allc = &_cex__default_global__allocator_heap.alloc;
// clang-format on


static void
_cex_allocator_heap__validate(IAllocator self)
{
    (void)self;
#ifndef NDEBUG
    uassert(self != NULL);
    uassert(
        self->meta.magic_id == CEX_ALLOCATOR_HEAP_MAGIC && "bad allocator pointer or mem corruption"
    );
#endif
}

static inline u64
_cex_allocator_heap__hdr_set(u64 size, u8 ptr_offset, u8 alignment)
{
    size &= 0xFFFFFFFFFFFFULL; // Mask to 48 bits
    return size | ((u64)ptr_offset << 48) | ((u64)alignment << 56);
}

static inline usize
_cex_allocator_heap__hdr_get_size(u64 alloc_hdr)
{
    return alloc_hdr & 0xFFFFFFFFFFFFULL;
}

static inline u8
_cex_allocator_heap__hdr_get_offset(u64 alloc_hdr)
{
    return (u8)((alloc_hdr >> 48) & 0xFF);
}

static inline u8
_cex_allocator_heap__hdr_get_alignment(u64 alloc_hdr)
{
    return (u8)(alloc_hdr >> 56);
}

static u64
_cex_allocator_heap__hdr_make(usize alloc_size, usize alignment)
{

    usize size = alloc_size;

    if (unlikely(alloc_size == 0 || alloc_size > PTRDIFF_MAX || alignment > 64)) {
        uassert(alloc_size > 0 && "zero size");
        uassert(alloc_size > PTRDIFF_MAX && "size is too high");
        uassert(alignment <= 64);
        return 0;
    }

#if UINTPTR_MAX > 0xFFFFFFFFU
    // Only 64 bit
    if (unlikely((u64)alloc_size > (u64)0xFFFFFFFFFFFFULL)) {
        uassert(
            (u64)alloc_size < (u64)0xFFFFFFFFFFFFULL && "size is too high, or negative overflow"
        );
        return 0;
    }
#endif

    if (alignment < 8) {
        static_assert(alignof(void*) <= 8, "unexpected ptr alignment");
        // We don't need to reserve extra room for base alignment
        alignment = 8;
    } else {
        uassert(mem$is_power_of2(alignment) && "must be pow2");

        if ((alloc_size & (alignment - 1)) != 0) {
            uassert(alloc_size % alignment == 0 && "requested size is not aligned");
            return 0;
        }
        // Adding extra alignment chunk because system malloc() may return misaligned pointer
        //   and we will need to offset it in order to return truly aligned address 
        size += alignment;
    }
    // room for header
    size += sizeof(u64);

    // extra area for poisoning
    size += sizeof(u64);

    size = mem$aligned_round(size, alignment);

    return _cex_allocator_heap__hdr_set(size, 0, alignment);
}

static void*
_cex_allocator_heap__alloc(IAllocator self, u8 fill_val, usize size, usize alignment)
{
    _cex_allocator_heap__validate(self);
    AllocatorHeap_c* a = (AllocatorHeap_c*)self;
    (void)a;

    u64 hdr = _cex_allocator_heap__hdr_make(size, alignment);
    if (unlikely(hdr == 0)) { return NULL; }

    usize full_size = _cex_allocator_heap__hdr_get_size(hdr);
    alignment = _cex_allocator_heap__hdr_get_alignment(hdr);

    // Memory alignment
    // |                 <hdr>|<poisn>|---<data>---
    // ^---malloc()
    u8* raw_result = NULL;
    if (fill_val != 0) {
        raw_result = cex$platform_malloc(full_size);
    } else {
        raw_result = cex$platform_calloc(1, full_size);
    }
    u8* result = raw_result;

    if (raw_result) {
        result = mem$aligned_pointer(raw_result + sizeof(u64) * 2, alignment);
        uassert(mem$aligned_pointer(result, 8) == result);
        uassert(mem$aligned_pointer(result, alignment) == result);

#ifdef CEX_TEST
        a->stats.n_allocs++;
        // intentionally set malloc to 0xf7 pattern to mark uninitialized data
        if (fill_val != 0) { memset(result, 0xf7, size); }
#endif
        usize ptr_offset = result - raw_result;
        uassert(ptr_offset >= sizeof(u64) * 2);
        uassert(ptr_offset <= 64 + 16);
        uassert(ptr_offset <= alignment + sizeof(u64) * 2);
        uassert(result + size <= raw_result + full_size);

        // poison area after header and before allocated pointer
        mem$asan_poison(result - sizeof(u64), sizeof(u64));
        ((u64*)result)[-2] = _cex_allocator_heap__hdr_set(size, ptr_offset, alignment);

        if (ptr_offset + size < full_size) {
            // Adding padding poison for non 8-byte aligned data
            mem$asan_poison(result + size, full_size - size - ptr_offset);
        }
    }

    return result;
}

static void*
_cex_allocator_heap__malloc(IAllocator self, usize size, usize alignment)
{
    return _cex_allocator_heap__alloc(self, 1, size, alignment);
}
static void*
_cex_allocator_heap__calloc(IAllocator self, usize nmemb, usize size, usize alignment)
{
    if (unlikely(nmemb == 0 || nmemb >= PTRDIFF_MAX)) {
        uassert(nmemb > 0 && "nmemb is zero");
        uassert(nmemb < PTRDIFF_MAX && "nmemb is too high or negative overflow");
        return NULL;
    }
    if (unlikely(size == 0 || size >= PTRDIFF_MAX)) {
        uassert(size > 0 && "size is zero");
        uassert(size < PTRDIFF_MAX && "size is too high or negative overflow");
        return NULL;
    }

    return _cex_allocator_heap__alloc(self, 0, size * nmemb, alignment);
}

static void*
_cex_allocator_heap__realloc(IAllocator self, void* ptr, usize size, usize alignment)
{
    _cex_allocator_heap__validate(self);
    if (unlikely(ptr == NULL)) {
        uassert(ptr != NULL);
        return NULL;
    }
    AllocatorHeap_c* a = (AllocatorHeap_c*)self;
    (void)a;

    // Memory alignment
    // |                 <hdr>|<poisn>|---<data>---
    // ^---realloc()
    char* p = ptr;
    uassert(
        mem$asan_poison_check(p - sizeof(u64), sizeof(u64)) &&
        "corrupted pointer or unallocated by mem$"
    );
    u64 old_hdr = *(u64*)(p - sizeof(u64) * 2);
    uassert(old_hdr > 0 && "bad poitner or corrupted malloced header?");
    u64 old_size = _cex_allocator_heap__hdr_get_size(old_hdr);
    (void)old_size;
    u8 old_offset = _cex_allocator_heap__hdr_get_offset(old_hdr);
    u8 old_alignment = _cex_allocator_heap__hdr_get_alignment(old_hdr);
    uassert((old_alignment >= 8 && old_alignment <= 64) && "corrupted header?");
    uassert(old_offset <= 64 + sizeof(u64) * 2 && "corrupted header?");
    if (unlikely(
            (alignment <= 8 && old_alignment != 8) || (alignment > 8 && alignment != old_alignment)
        )) {
        uassert(alignment == old_alignment && "given alignment doesn't match to old one");
        goto fail;
    }

    u64 new_hdr = _cex_allocator_heap__hdr_make(size, alignment);
    if (unlikely(new_hdr == 0)) { goto fail; }

    u8* raw_result = NULL;
    u8* result = NULL;
    usize new_full_size = _cex_allocator_heap__hdr_get_size(new_hdr);

    if (alignment <= _Alignof(max_align_t)) {
        uassert(new_full_size > size);
        raw_result = cex$platform_realloc(p - old_offset, new_full_size);
        if (unlikely(raw_result == NULL)) { goto fail; }
        result = mem$aligned_pointer(raw_result + sizeof(u64) * 2, old_alignment);
    } else {
        // fallback to malloc + memcpy because realloc doesn't guarantee alignment
        raw_result = cex$platform_malloc(new_full_size);
        if (unlikely(raw_result == NULL)) { goto fail; }
        result = mem$aligned_pointer(raw_result + sizeof(u64) * 2, old_alignment);
        memcpy(result, ptr, size > old_size ? old_size : size);
        cex$platform_free(ptr - old_offset);
    }
    uassert(mem$aligned_pointer(result, 8) == result);
    uassert(mem$aligned_pointer(result, old_alignment) == result);

    usize ptr_offset = result - raw_result;
    uassert(ptr_offset <= 64 + 16);
    uassert(ptr_offset <= old_alignment + sizeof(u64) * 2);
    // uassert(ptr_offset + size <= new_full_size);

#ifdef CEX_TEST
    a->stats.n_reallocs++;
    if (old_size < size) {
        // intentionally set unallocated to 0xf7 pattern to mark uninitialized data
        memset(result + old_size, 0xf7, size - old_size);
    }
#endif
    mem$asan_poison(result - sizeof(u64), sizeof(u64));
    ((u64*)result)[-2] = _cex_allocator_heap__hdr_set(size, ptr_offset, old_alignment);

    if (ptr_offset + size < new_full_size) {
        // Adding padding poison for non 8-byte aligned data
        mem$asan_poison(result + size, new_full_size - size - ptr_offset);
    }

    return result;

fail:
    _cex_allocator_heap__free(self, ptr);
    return NULL;
}

static void*
_cex_allocator_heap__free(IAllocator self, void* ptr)
{
    _cex_allocator_heap__validate(self);
    if (ptr != NULL) {
        AllocatorHeap_c* a = (AllocatorHeap_c*)self;
        (void)a;

        char* p = ptr;
        uassert(
            mem$asan_poison_check(p - sizeof(u64), sizeof(u64)) &&
            "corrupted pointer or unallocated by mem$"
        );
        u64 hdr = *(u64*)(p - sizeof(u64) * 2);
        uassert(hdr > 0 && "bad pointner or corrupted malloced header?");
        u8 offset = _cex_allocator_heap__hdr_get_offset(hdr);
        u8 alignment = _cex_allocator_heap__hdr_get_alignment(hdr);
        uassert(alignment >= 8 && "corrupted header?");
        uassert(alignment <= 64 && "corrupted header?");
        uassert(offset >= 16 && "corrupted header?");
        uassert(offset <= 64+16 && "corrupted header?");

#ifdef CEX_TEST
        a->stats.n_free++;
        u64 size = _cex_allocator_heap__hdr_get_size(hdr);
        u32 padding = mem$aligned_round(size + offset, alignment) - size - offset;
        if (padding > 0) {
            uassert(
                mem$asan_poison_check(p + size, padding) &&
                "corrupted area after unallocated size by mem$"
            );
        }
#endif

        cex$platform_free(p - offset);
    }
    return NULL;
}

static const struct Allocator_i*
_cex_allocator_heap__scope_enter(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    uassert(false && "this only supported by arenas");
    abort();
}

static void
_cex_allocator_heap__scope_exit(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    uassert(false && "this only supported by arenas");
    abort();
}

static u32
_cex_allocator_heap__scope_depth(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    return 1; // always 1
}

#endif



/*
*                          src/AllocatorArena.c
*/

#if !defined(cex$enable_minimal) || defined(cex$enable_mem)


#define CEX_ARENA_MAX_ALLOC UINT32_MAX - 1000
#define CEX_ARENA_MAX_ALIGN 64


static void
_cex_allocator_arena__validate(IAllocator self)
{
    (void)self;
#ifndef NDEBUG
    uassert(self != NULL);
    uassert(
        (self->meta.magic_id == CEX_ALLOCATOR_ARENA_MAGIC ||
         self->meta.magic_id == CEX_ALLOCATOR_TEMP_MAGIC) &&
        "bad allocator pointer or mem corruption"
    );
#endif
}


static inline usize
_cex_alloc_estimate_page_size(usize page_size, usize alloc_size)
{
    uassert(alloc_size < CEX_ARENA_MAX_ALLOC && "allocation is to big");
    usize base_page_size = mem$aligned_round(
        page_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN,
        alignof(allocator_arena_page_s)
    );
    uassert(base_page_size % alignof(allocator_arena_page_s) == 0 && "expected to be 64 aligned");

    if (alloc_size > 0.7 * base_page_size) {
        if (alloc_size > 1024 * 1024) {
            alloc_size *= 1.1;
            alloc_size += sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN;
        } else {
            alloc_size *= 2;
        }

        usize result = mem$aligned_round(
            alloc_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN,
            alignof(allocator_arena_page_s)
        );
        uassert(result % alignof(allocator_arena_page_s) == 0 && "expected to be 64 aligned");
        return result;
    } else {
        return base_page_size;
    }
}
static allocator_arena_rec_s
_cex_alloc_estimate_alloc_size(usize alloc_size, usize alignment)
{
    if (alloc_size == 0 || alloc_size > CEX_ARENA_MAX_ALLOC || alignment > CEX_ARENA_MAX_ALIGN) {
        uassert(alloc_size > 0);
        uassert(alloc_size <= CEX_ARENA_MAX_ALLOC && "allocation size is too high");
        uassert(alignment <= CEX_ARENA_MAX_ALIGN);
        return (allocator_arena_rec_s){ 0 };
    }
    usize size = alloc_size;

    if (alignment < 8) {
        static_assert(sizeof(allocator_arena_rec_s) == 8, "unexpected size");
        static_assert(alignof(void*) <= 8, "unexpected ptr alignment");
        alignment = 8;
        size = mem$aligned_round(alloc_size, 8);
    } else {
        uassert(mem$is_power_of2(alignment) && "must be pow2");
        if ((alloc_size & (alignment - 1)) != 0) {
            uassert(alloc_size % alignment == 0 && "requested size is not aligned");
            return (allocator_arena_rec_s){ 0 };
        }
    }

    size += sizeof(allocator_arena_rec_s);

    if (size - alloc_size == sizeof(allocator_arena_rec_s)) {
        size += sizeof(allocator_arena_rec_s); // adding extra bytes for ASAN poison
    }
    uassert(size - alloc_size >= sizeof(allocator_arena_rec_s));
    uassert(size - alloc_size <= 255 - sizeof(allocator_arena_rec_s) && "ptr_offset oveflow");
    uassert(size < alloc_size + 128 && "weird overflow");

    return (allocator_arena_rec_s){
        .size = alloc_size, // original size of allocation
        .ptr_offset = 0,    // offset from allocator_arena_rec_s to pointer (will be set later!)
        .ptr_alignment = alignment, // expected pointer alignment
        .ptr_padding = size - alloc_size - sizeof(allocator_arena_rec_s), // from last data to next
    };
}

static inline allocator_arena_rec_s*
_cex_alloc_arena__get_rec(void* alloc_pointer)
{
    uassert(alloc_pointer != NULL);
    u8 offset = *((u8*)alloc_pointer - 1);
    uassert(offset <= CEX_ARENA_MAX_ALIGN);
    return (allocator_arena_rec_s*)((char*)alloc_pointer - offset);
}

static bool
_cex_allocator_arena__check_pointer_valid(AllocatorArena_c* self, void* old_ptr)
{
    uassert(self->scope_depth > 0);
    allocator_arena_page_s* page = self->last_page;
    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(old_ptr);
    while (page) {
        auto tpage = page->prev_page;
        if ((char*)rec > (char*)page &&
            (char*)rec < (((char*)page) + sizeof(allocator_arena_page_s) + page->capacity)) {
            uassert((char*)rec >= (char*)page + sizeof(allocator_arena_page_s));

            u32 ptr_scope_mark =
                (((char*)rec) - ((char*)page) - sizeof(allocator_arena_page_s) + page->used_start);

            if (self->scope_depth < sizeof(self->scope_stack) / sizeof((self->scope_stack)[0])) {
                if (ptr_scope_mark < self->scope_stack[self->scope_depth - 1]) {
                    uassert(
                        ptr_scope_mark >= self->scope_stack[self->scope_depth - 1] &&
                        "trying to operate on pointer from different mem$scope() it will lead to use-after-free / ASAN poison issues"
                    );
                    return false; // using pointer out of scope of previous page
                }
            }
            return true;
        }
        page = tpage;
    }
    return false;
}

static allocator_arena_page_s*
_cex_allocator_arena__request_page_size(
    AllocatorArena_c* self,
    allocator_arena_rec_s new_rec,
    bool* out_is_allocated
)
{
    usize req_size = new_rec.size + new_rec.ptr_alignment + new_rec.ptr_padding;
    if (out_is_allocated) { *out_is_allocated = false; }

    if (self->last_page == NULL ||
        // self->last_page->capacity < req_size + mem$aligned_round(self->last_page->cursor, 8)) {
        self->last_page->capacity < req_size + self->last_page->cursor) {
        usize page_size = _cex_alloc_estimate_page_size(self->page_size, req_size);

        if (page_size == 0 || page_size > CEX_ARENA_MAX_ALLOC) {
            uassert(page_size > 0 && "page_size is zero");
            uassert(page_size <= CEX_ARENA_MAX_ALLOC && "page_size is to big");
            return NULL;
        }
        allocator_arena_page_s*
            page = mem$calloc(mem$, 1, page_size, alignof(allocator_arena_page_s));
        if (page == NULL) {
            return NULL; // memory error
        }

        uassert(mem$aligned_pointer(page, 64) == page);

        page->prev_page = self->last_page;
        page->used_start = self->used;
        page->capacity = page_size - sizeof(allocator_arena_page_s);
        mem$asan_poison(page->__poison_area, sizeof(page->__poison_area));
        mem$asan_poison(&page->data, page->capacity);

        self->last_page = page;
        self->stats.pages_created++;

        if (out_is_allocated) { *out_is_allocated = true; }
    }

    return self->last_page;
}

static void*
_cex_allocator_arena__malloc(IAllocator allc, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    uassert(self->scope_depth > 0 && "arena allocation must be performed in mem$scope() block!");

    allocator_arena_rec_s rec = _cex_alloc_estimate_alloc_size(size, alignment);
    if (rec.size == 0) { return NULL; }

    allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(self, rec, NULL);
    if (page == NULL) { return NULL; }
    uassert(page->capacity - page->cursor >= rec.size + rec.ptr_padding + rec.ptr_alignment);
    uassert(page->cursor % 8 == 0);
    uassert(rec.ptr_padding <= 8);
    uassertf((usize)page->data % 8 == 0, "page.data offset: %zi\n", (page->data - (char*)page));

    allocator_arena_rec_s* page_rec = (allocator_arena_rec_s*)&page->data[page->cursor];
    uassert((((usize)(page_rec) & ((8) - 1)) == 0) && "unaligned pointer");
    static_assert(sizeof(allocator_arena_rec_s) == 8, "unexpected size");
    static_assert(alignof(allocator_arena_rec_s) <= 8, "unexpected alignment");

    mem$asan_unpoison(page_rec, sizeof(allocator_arena_rec_s));
    *page_rec = rec;

    void* result = mem$aligned_pointer(
        (char*)page_rec + sizeof(allocator_arena_rec_s),
        rec.ptr_alignment
    );

    uassert((char*)result >= ((char*)page_rec) + sizeof(allocator_arena_rec_s));
    rec.ptr_offset = (char*)result - (char*)page_rec;
    uassert(rec.ptr_offset <= rec.ptr_alignment);

    page_rec->ptr_offset = rec.ptr_offset;
    uassert(rec.ptr_alignment <= CEX_ARENA_MAX_ALIGN);

    mem$asan_unpoison(((char*)result) - 1, rec.size + 1);
    *(((char*)result) - 1) = rec.ptr_offset;

    usize bytes_alloc = rec.ptr_offset + rec.size + rec.ptr_padding;
    self->used += bytes_alloc;
    self->stats.bytes_alloc += bytes_alloc;
    page->cursor += bytes_alloc;
    page->last_alloc = result;
    uassert(page->cursor % 8 == 0);
    uassert(self->used % 8 == 0);
    uassert(((usize)(result) & ((rec.ptr_alignment) - 1)) == 0);


#ifdef CEX_TEST
    // intentionally set malloc to 0xf7 pattern to mark uninitialized data
    memset(result, 0xf7, rec.size);
#endif

    return result;
}
static void*
_cex_allocator_arena__calloc(IAllocator allc, usize nmemb, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    if (nmemb > CEX_ARENA_MAX_ALLOC) {
        uassert(nmemb < CEX_ARENA_MAX_ALLOC);
        return NULL;
    }
    if (size > CEX_ARENA_MAX_ALLOC) {
        uassert(size < CEX_ARENA_MAX_ALLOC);
        return NULL;
    }
    usize alloc_size = nmemb * size;
    void* result = _cex_allocator_arena__malloc(allc, alloc_size, alignment);
    if (result != NULL) { memset(result, 0, alloc_size); }

    return result;
}

static void*
_cex_allocator_arena__free(IAllocator allc, void* ptr)
{
    (void)ptr;
    // NOTE: this intentionally does nothing, all memory releasing in scope_exit()
    _cex_allocator_arena__validate(allc);

    if (ptr == NULL) { return NULL; }

    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    (void)self;
    uassert(
        _cex_allocator_arena__check_pointer_valid(self, ptr) && "pointer doesn't belong to arena"
    );
    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(ptr);
    rec->is_free = true;
    mem$asan_poison(ptr, rec->size);

    return NULL;
}

static void*
_cex_allocator_arena__realloc(IAllocator allc, void* old_ptr, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    uassert(old_ptr != NULL);
    uassert(size > 0);
    uassert(size < CEX_ARENA_MAX_ALLOC);

    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    uassert(self->scope_depth > 0 && "arena allocation must be performed in mem$scope() block!");

    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(old_ptr);
    uassert(!rec->is_free && "trying to realloc() already freed pointer");
    if (alignment < 8) {
        uassert(rec->ptr_alignment == 8);
    } else {
        uassert(alignment == rec->ptr_alignment && "realloc alignment mismatch with old_ptr");
        uassert(((usize)(old_ptr) & ((alignment)-1)) == 0 && "weird old_ptr not aligned");
        uassert(((usize)(size) & ((alignment)-1)) == 0 && "size is not aligned as expected");
    }

    uassert(
        _cex_allocator_arena__check_pointer_valid(self, old_ptr) &&
        "pointer doesn't belong to arena"
    );

    if (unlikely(size <= rec->size)) {
        if (size == rec->size) { return old_ptr; }
        // we can't change size/padding of this allocation, because this will break iterating
        // ptr_padding is only u8 size, we cant store size, change, so we currently poison new size
        mem$asan_poison((char*)old_ptr + size, rec->size - size);
        return old_ptr;
    }

    if (unlikely(self->last_page && self->last_page->last_alloc == old_ptr)) {
        allocator_arena_rec_s nrec = _cex_alloc_estimate_alloc_size(size, alignment);
        if (nrec.size == 0) { goto fail; }
        bool is_created = false;
        allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(
            self,
            nrec,
            &is_created
        );
        if (page == NULL) { goto fail; }
        if (!is_created) {
            // If new page was created, fall back to malloc/copy/free method
            //   but currently we have spare capacity for growth
            u32 extra_bytes = size - rec->size;
            mem$asan_unpoison((char*)old_ptr + rec->size, extra_bytes);
#ifdef CEX_TEST
            memset((char*)old_ptr + rec->size, 0xf7, extra_bytes);
#endif
            extra_bytes += (nrec.ptr_padding - rec->ptr_padding);
            page->cursor += extra_bytes;
            self->used += extra_bytes;
            self->stats.bytes_alloc += extra_bytes;
            rec->size = size;
            rec->ptr_padding = nrec.ptr_padding;

            uassert(
                (char*)rec + rec->size + rec->ptr_padding + rec->ptr_offset ==
                &page->data[page->cursor]
            );
            uassert(page->cursor % 8 == 0);
            uassert(self->used % 8 == 0);
            mem$asan_poison((char*)old_ptr + size, rec->ptr_padding);
            return old_ptr;
        }
        // NOTE: fall through to default way
    }

    void* new_ptr = _cex_allocator_arena__malloc(allc, size, alignment);
    if (new_ptr == NULL) { goto fail; }
    memcpy(new_ptr, old_ptr, rec->size);
    _cex_allocator_arena__free(allc, old_ptr);
    return new_ptr;
fail:
    _cex_allocator_arena__free(allc, old_ptr);
    return NULL;
}


static const struct Allocator_i*
_cex_allocator_arena__scope_enter(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    // NOTE: If scope_depth is higher CEX_ALLOCATOR_MAX_SCOPE_STACK, we stop marking
    //  all memory will be released after exiting scope_depth == CEX_ALLOCATOR_MAX_SCOPE_STACK
    if (self->scope_depth < sizeof(self->scope_stack) / sizeof((self->scope_stack)[0])) {
        self->scope_stack[self->scope_depth] = self->used;
    }
    self->scope_depth++;
    return allc;
}
static void
_cex_allocator_arena__scope_exit(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    uassert(self->scope_depth > 0);

#ifdef CEX_TEST
    bool AllocatorArena_sanitize(IAllocator allc);
    uassert(AllocatorArena_sanitize(allc));
#endif
    self->scope_depth--;
    if (self->scope_depth >= sizeof(self->scope_stack) / sizeof((self->scope_stack)[0])) {
        // Scope overflow, wait until we reach CEX_ALLOCATOR_MAX_SCOPE_STACK
        return;
    }

    usize used_mark = self->scope_stack[self->scope_depth];

    allocator_arena_page_s* page = self->last_page;
    while (page) {
        auto tpage = page->prev_page;
        if (page->used_start == 0 || page->used_start < used_mark) {
            // last page, just set mark and poison
            usize free_offset = (used_mark - page->used_start);
            uassert(page->cursor >= free_offset);

            usize free_len = page->cursor - free_offset;
            page->cursor = free_offset;
            mem$asan_poison(&page->data[free_offset], free_len);

            uassert(self->used >= free_len);
            self->used -= free_len;
            self->stats.bytes_free += free_len;
            break; // we are done
        } else {
            usize free_len = page->cursor;
            uassert(self->used >= free_len);

            self->used -= free_len;
            self->stats.bytes_free += free_len;
            self->last_page = page->prev_page;
            self->stats.pages_free++;
            mem$free(mem$, page);
        }
        page = tpage;
    }
}
static u32
_cex_allocator_arena__scope_depth(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    return self->scope_depth;
}

IAllocator
AllocatorArena_create(usize page_size)
{
    if (page_size < 1024 || page_size >= UINT32_MAX) {
        uassert(page_size >= 1024 && "page size is too small");
        uassert(page_size < UINT32_MAX && "page size is too big");
        return NULL;
    }

    AllocatorArena_c template = {
        .alloc = {
            .malloc = _cex_allocator_arena__malloc,
            .realloc = _cex_allocator_arena__realloc,
            .calloc = _cex_allocator_arena__calloc,
            .free = _cex_allocator_arena__free,
            .scope_enter = _cex_allocator_arena__scope_enter,
            .scope_exit = _cex_allocator_arena__scope_exit,
            .scope_depth = _cex_allocator_arena__scope_depth,
            .meta = {
                .magic_id = CEX_ALLOCATOR_ARENA_MAGIC,
                .is_arena = true, 
                .is_temp = false, 
            }
        },
        .page_size = page_size,
    };

    AllocatorArena_c* self = mem$new(mem$, AllocatorArena_c);
    if (self == NULL) {
        return NULL; // memory error
    }

    memcpy(self, &template, sizeof(AllocatorArena_c));
    uassert(self->alloc.meta.magic_id == CEX_ALLOCATOR_ARENA_MAGIC);
    uassert(self->alloc.malloc == _cex_allocator_arena__malloc);

    _cex_allocator_arena__scope_enter(&self->alloc);

    return &self->alloc;
}

bool
AllocatorArena_sanitize(IAllocator allc)
{
    (void)allc;
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    if (self->scope_depth == 0) {
        uassert(self->stats.bytes_alloc == self->stats.bytes_free && "memory leaks?");
    }
    allocator_arena_page_s* page = self->last_page;
    while (page) {
        uassert(page->cursor <= page->capacity);
        uassert(mem$asan_poison_check(page->__poison_area, sizeof(page->__poison_area)));

        u32 i = 0;
        while (i < page->cursor) {
            allocator_arena_rec_s* rec = (allocator_arena_rec_s*)&page->data[i];
            uassert(rec->size <= page->capacity);
            uassert(rec->size <= page->cursor);
            uassert(rec->ptr_offset <= CEX_ARENA_MAX_ALIGN);
            uassert(rec->ptr_padding <= 16);
            uassert(rec->ptr_alignment <= CEX_ARENA_MAX_ALIGN);
            uassert(rec->is_free <= 1);
            uassert(mem$is_power_of2(rec->ptr_alignment));

            char* alloc_p = ((char*)rec) + rec->ptr_offset;
            u8 poffset = alloc_p[-1];
            (void)poffset;
            uassert(poffset == rec->ptr_offset && "near pointer offset mismatch to rec.ptr_offset");

            if (rec->ptr_padding) {
                uassert(
                    mem$asan_poison_check(alloc_p + rec->size, rec->ptr_padding) &&
                    "poison data overwrite past allocated item"
                );
            }

            if (rec->is_free) {
                uassert(
                    mem$asan_poison_check(alloc_p, rec->size) &&
                    "poison data corruction in freed item area"
                );
            }
            i += rec->ptr_padding + rec->ptr_offset + rec->size;
        }
        if (page->cursor < page->capacity) {
            // unallocated page must be poisoned
            uassert(
                mem$asan_poison_check(&page->data[page->cursor], page->capacity - page->cursor) &&
                "poison data overwrite in unallocated area"
            );
        }

        page = page->prev_page;
    }

    return true;
}

void
AllocatorArena_destroy(IAllocator self)
{
    _cex_allocator_arena__validate(self);
    AllocatorArena_c* allc = (AllocatorArena_c*)self;

    uassert(allc->scope_depth == 1 && "trying to destroy in mem$scope?");
    _cex_allocator_arena__scope_exit(self);

#ifdef CEX_TEST
    uassert(AllocatorArena_sanitize(self));
#endif

    allocator_arena_page_s* page = allc->last_page;
    while (page) {
        auto tpage = page->prev_page;
        mem$free(mem$, page);
        page = tpage;
    }
    mem$free(mem$, allc);
}

#if !cex$is_freestanding
_Thread_local 
#endif
AllocatorArena_c _cex__default_global__allocator_temp = {
    .alloc = {
        .malloc = _cex_allocator_arena__malloc,
        .realloc = _cex_allocator_arena__realloc,
        .calloc = _cex_allocator_arena__calloc,
        .free = _cex_allocator_arena__free,
        .scope_enter = _cex_allocator_arena__scope_enter,
        .scope_exit = _cex_allocator_arena__scope_exit,
        .scope_depth = _cex_allocator_arena__scope_depth,
        .meta = {
            .magic_id = CEX_ALLOCATOR_TEMP_MAGIC,
            .is_arena = true,  // coming... soon
            .is_temp = true, 
        }, 
    },
    .page_size = CEX_ALLOCATOR_TEMP_PAGE_SIZE,
};

const struct __cex_namespace__AllocatorArena AllocatorArena = {
    // Autogenerated by CEX
    // clang-format off

    .create = AllocatorArena_create,
    .destroy = AllocatorArena_destroy,
    .sanitize = AllocatorArena_sanitize,

    // clang-format on
};

#endif



/*
*                          src/ds.c
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_ds)



#ifdef _CEXDS_STATISTICS
#    define _CEXDS_STATS(x) x
usize _cexds__array_grow;
usize _cexds__hash_grow;
usize _cexds__hash_shrink;
usize _cexds__hash_rebuild;
usize _cexds__hash_probes;
usize _cexds__hash_alloc;
usize _cexds__rehash_probes;
usize _cexds__rehash_items;
#else
#    define _CEXDS_STATS(x)
#endif

//
// _cexds__arr implementation
//

#define _cexds__item_ptr(t, i, elemsize) ((char*)a + elemsize * i)

static inline void*
_cexds__base(_cexds__array_header* hdr)
{
    if (hdr->el_align <= sizeof(_cexds__array_header)) {
        return hdr;
    } else {
        char* arr_ptr = (char*)hdr + sizeof(_cexds__array_header);
        return arr_ptr - hdr->el_align;
    }
}

bool
_cexds__arr_integrity(const void* arr, usize magic_num)
{
    (void)magic_num;
    (void)arr;

#ifndef NDEBUG
    _cexds__array_header* hdr = _cexds__header(arr);
    (void)hdr;

    uassert(arr != NULL && "array uninitialized or out-of-mem");
    // WARNING: next can trigger sanitizer with "stack/heap-buffer-underflow on address"
    //          when arr pointer is invalid arr$ / hm$ type pointer
    if (magic_num == 0) {
        uassert(((hdr->magic_num == _CEXDS_ARR_MAGIC) || hdr->magic_num == _CEXDS_HM_MAGIC));
    } else {
        uassert(hdr->magic_num == magic_num);
    }

    uassert(mem$asan_poison_check(hdr->__poison_area, sizeof(hdr->__poison_area)));

#endif


    return true;
}

inline usize
_cexds__arr_len(const void* arr)
{
    return (arr) ? _cexds__header(arr)->length : 0;
}

void*
_cexds__arrgrowf(
    void* arr,
    usize elemsize,
    usize addlen,
    usize min_cap,
    u16 el_align,
    IAllocator allc
)
{
    uassert(addlen < PTRDIFF_MAX && "negative or overflow");
    uassert(min_cap < PTRDIFF_MAX && "negative or overflow");
    uassert(el_align <= 64 && "alignment is too high");

    if (arr == NULL) {
        if (allc == NULL) {
            uassert(allc != NULL && "using uninitialized arr/hm or out-of-mem error");
            // unconditionally abort even in production
            abort();
        }
    } else {
        _cexds__arr_integrity(arr, 0);
    }
    usize min_len = (arr ? _cexds__header(arr)->length : 0) + addlen;

    // compute the minimum capacity needed
    if (min_len > min_cap) { min_cap = min_len; }

    // increase needed capacity to guarantee O(1) amortized
    if (min_cap < 2 * arr$cap(arr)) {
        min_cap = 2 * arr$cap(arr);
    } else if (min_cap < 16) {
        min_cap = 16;
    }
    uassert(min_cap < PTRDIFF_MAX && "negative or overflow after processing");
    uassert(addlen > 0 || min_cap > 0);

    if (min_cap <= arr$cap(arr)) { return arr; }

    // General types with alignment <= usize use generic realloc (less mem overhead + realloc faster)
    el_align = (el_align <= alignof(_cexds__array_header)) ? alignof(_cexds__array_header) : 64;

    void* new_arr;
    if (arr == NULL) {
        new_arr = mem$malloc(
            allc,
            mem$aligned_round(elemsize * min_cap + sizeof(_cexds__array_header), el_align),
            el_align
        );
    } else {
        _cexds__array_header* hdr = _cexds__header(arr);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        // NOTE: we must unpoison to prevent false ASAN use-after-poison check if data is copied
        mem$asan_unpoison(hdr->__poison_area, sizeof(hdr->__poison_area));
        new_arr = mem$realloc(
            _cexds__header(arr)->allocator,
            _cexds__base(hdr),
            mem$aligned_round(elemsize * min_cap + sizeof(_cexds__array_header), el_align),
            el_align
        );
    }

    if (new_arr == NULL) {
        // oops memory error
        return NULL;
    }

    new_arr = mem$aligned_pointer(new_arr + sizeof(_cexds__array_header), el_align);
    _cexds__array_header* hdr = _cexds__header(new_arr);
    if (arr == NULL) {
        hdr->length = 0;
        hdr->_hash_table = NULL;
        hdr->allocator = allc;
        hdr->magic_num = _CEXDS_ARR_MAGIC;
        hdr->allocator_scope_depth = allc->scope_depth(allc);
        hdr->el_align = el_align;
        mem$asan_poison(hdr->__poison_area, sizeof(hdr->__poison_area));
    } else {
        uassert(
            (hdr->magic_num == _CEXDS_ARR_MAGIC || hdr->magic_num == _CEXDS_HM_MAGIC) &&
            "bad magic after realloc"
        );
        mem$asan_poison(hdr->__poison_area, sizeof(hdr->__poison_area));
        _CEXDS_STATS(++_cexds__array_grow);
    }
    hdr->capacity = min_cap;

    return new_arr;
}

void
_cexds__arrfreef(void* a)
{
    if (a != NULL) {
        uassert(_cexds__header(a)->allocator != NULL);
        _cexds__array_header* h = _cexds__header(a);
        h->allocator->free(h->allocator, _cexds__base(h));
    }
}

//
// _cexds__hm hash table implementation
//

#define _CEXDS_BUCKET_LENGTH 8
#define _CEXDS_BUCKET_SHIFT (_CEXDS_BUCKET_LENGTH == 8 ? 3 : 2)
#define _CEXDS_BUCKET_MASK (_CEXDS_BUCKET_LENGTH - 1)
#define _CEXDS_CACHE_LINE_SIZE 64

#define _cexds__hash_table(a) ((_cexds__hash_index*)_cexds__header(a)->_hash_table)


typedef struct
{
    usize hash[_CEXDS_BUCKET_LENGTH];
    ptrdiff_t index[_CEXDS_BUCKET_LENGTH];
} _cexds__hash_bucket;
static_assert(sizeof(_cexds__hash_bucket) % 64 == 0, "cacheline aligned");

typedef struct _cexds__hash_index
{
    usize slot_count;
    usize used_count;
    usize used_count_threshold;
    usize used_count_shrink_threshold;
    usize tombstone_count;
    usize tombstone_count_threshold;
    usize seed;
    usize slot_count_log2;
    enum _CexDsKeyType_e key_type;
    IAllocator key_arena;
    bool copy_keys;

    // not a separate allocation, just 64-byte aligned storage after this struct
    _cexds__hash_bucket* storage;
} _cexds__hash_index;

#define _CEXDS_INDEX_EMPTY -1
#define _CEXDS_INDEX_DELETED -2
#define _CEXDS_INDEX_IN_USE(x) ((x) >= 0)

#define _CEXDS_HASH_EMPTY 0
#define _CEXDS_HASH_DELETED 1

#define _CEXDS_usize_BITS ((sizeof(usize)) * 8)

static inline usize
_cexds__probe_position(usize hash, usize slot_count, usize slot_log2)
{
    usize pos;
    (void)(slot_log2);
    pos = hash & (slot_count - 1);
#ifdef _CEXDS_INTERNAL_BUCKET_START
    pos &= ~_CEXDS_BUCKET_MASK;
#endif
    return pos;
}

static inline usize
_cexds__log2(usize slot_count)
{
    usize n = 0;
    while (slot_count > 1) {
        slot_count >>= 1;
        ++n;
    }
    return n;
}

void
_cexds__hmclear_func(struct _cexds__hash_index* t, _cexds__hash_index* old_table)
{
    if (t == NULL) {
        // typically external call of uninitialized table
        return;
    }

    uassert(t->slot_count > 0);
    t->tombstone_count = 0;
    t->used_count = 0;

    if (old_table) {
        t->key_arena = old_table->key_arena;
        t->seed = old_table->seed;
        t->key_arena = old_table->key_arena;
        t->copy_keys = old_table->copy_keys;
    } else {
        uassert(t->seed != 0);
    }

    usize i, j;
    for (i = 0; i < t->slot_count >> _CEXDS_BUCKET_SHIFT; ++i) {
        _cexds__hash_bucket* b = &t->storage[i];
        for (j = 0; j < _CEXDS_BUCKET_LENGTH; ++j) { b->hash[j] = _CEXDS_HASH_EMPTY; }
        for (j = 0; j < _CEXDS_BUCKET_LENGTH; ++j) { b->index[j] = _CEXDS_INDEX_EMPTY; }
    }
}

static _cexds__hash_index*
_cexds__make_hash_index(
    usize slot_count,
    _cexds__hash_index* old_table,
    IAllocator allc,
    usize hash_seed,
    enum _CexDsKeyType_e key_type
)
{
    _cexds__hash_index* t = mem$calloc(
        allc,
        1,
        (slot_count >> _CEXDS_BUCKET_SHIFT) * sizeof(_cexds__hash_bucket) +
            sizeof(_cexds__hash_index) + _CEXDS_CACHE_LINE_SIZE - 1
    );
    if (t == NULL) {
        return NULL; // memory error
    }
    t->storage = (_cexds__hash_bucket*)mem$aligned_pointer((usize)(t + 1), _CEXDS_CACHE_LINE_SIZE);
    t->slot_count = slot_count;
    t->slot_count_log2 = _cexds__log2(slot_count);
    t->tombstone_count = 0;
    t->used_count = 0;
    t->key_type = key_type;
    t->seed = hash_seed;

    // compute without overflowing
    t->used_count_threshold = slot_count - (slot_count >> 2);
    t->tombstone_count_threshold = (slot_count >> 3) + (slot_count >> 4);
    t->used_count_shrink_threshold = slot_count >> 2;

    if (slot_count <= _CEXDS_BUCKET_LENGTH) { t->used_count_shrink_threshold = 0; }
    // to avoid infinite loop, we need to guarantee that at least one slot is empty and will
    // terminate probes
    uassert(t->used_count_threshold + t->tombstone_count_threshold < t->slot_count);
    _CEXDS_STATS(++_cexds__hash_alloc);

    _cexds__hmclear_func(t, old_table);

    // copy out the old data, if any
    if (old_table) {
        usize i, j;
        t->used_count = old_table->used_count;
        for (i = 0; i < old_table->slot_count >> _CEXDS_BUCKET_SHIFT; ++i) {
            _cexds__hash_bucket* ob = &old_table->storage[i];
            for (j = 0; j < _CEXDS_BUCKET_LENGTH; ++j) {
                if (_CEXDS_INDEX_IN_USE(ob->index[j])) {
                    usize hash = ob->hash[j];
                    usize pos = _cexds__probe_position(hash, t->slot_count, t->slot_count_log2);
                    usize step = _CEXDS_BUCKET_LENGTH;
                    _CEXDS_STATS(++_cexds__rehash_items);
                    for (;;) {
                        usize limit, z;
                        _cexds__hash_bucket* bucket;
                        bucket = &t->storage[pos >> _CEXDS_BUCKET_SHIFT];
                        _CEXDS_STATS(++_cexds__rehash_probes);

                        for (z = pos & _CEXDS_BUCKET_MASK; z < _CEXDS_BUCKET_LENGTH; ++z) {
                            if (bucket->hash[z] == 0) {
                                bucket->hash[z] = hash;
                                bucket->index[z] = ob->index[j];
                                goto done;
                            }
                        }

                        limit = pos & _CEXDS_BUCKET_MASK;
                        for (z = 0; z < limit; ++z) {
                            if (bucket->hash[z] == 0) {
                                bucket->hash[z] = hash;
                                bucket->index[z] = ob->index[j];
                                goto done;
                            }
                        }

                        pos += step; // quadratic probing
                        step += _CEXDS_BUCKET_LENGTH;
                        pos &= (t->slot_count - 1);
                    }
                }
            done:;
            }
        }
    }

    return t;
}

#define _CEXDS_ROTATE_LEFT(val, n) (((val) << (n)) | ((val) >> (_CEXDS_usize_BITS - (n))))
#define _CEXDS_ROTATE_RIGHT(val, n) (((val) >> (n)) | ((val) << (_CEXDS_usize_BITS - (n))))


#ifdef _CEXDS_SIPHASH_2_4
#    define _CEXDS_SIPHASH_C_ROUNDS 2
#    define _CEXDS_SIPHASH_D_ROUNDS 4
typedef int _CEXDS_SIPHASH_2_4_can_only_be_used_in_64_bit_builds[sizeof(usize) == 8 ? 1 : -1];
#endif

#ifndef _CEXDS_SIPHASH_C_ROUNDS
#    define _CEXDS_SIPHASH_C_ROUNDS 1
#endif
#ifndef _CEXDS_SIPHASH_D_ROUNDS
#    define _CEXDS_SIPHASH_D_ROUNDS 1
#endif

static inline usize
_cexds__hash_string(const char* str, usize str_cap, usize seed)
{
    usize hash = seed;
    // NOTE: using max buffer capacity capping, this allows using hash
    //       on char buf[N] - without stack overflowing
    for (usize i = 0; i < str_cap && *str; i++) {
        hash = _CEXDS_ROTATE_LEFT(hash, 9) + (unsigned char)*str++;
    }

    // Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
    hash ^= seed;
    hash = (~hash) + (hash << 18);
    hash ^= hash ^ _CEXDS_ROTATE_RIGHT(hash, 31);
    hash = hash * 21;
    hash ^= hash ^ _CEXDS_ROTATE_RIGHT(hash, 11);
    hash += (hash << 6);
    hash ^= _CEXDS_ROTATE_RIGHT(hash, 22);
    return hash + seed;
}

static inline usize
_cexds__siphash_bytes(const void* p, usize len, usize seed)
{
    // hash that works on 32- or 64-bit registers without knowing which we have
    // (computes different results on 32-bit and 64-bit platform)
    // derived from siphash, but on 32-bit platforms very different as it uses 4 32-bit state not 4
    // 64-bit
    usize v0 = ((((usize)0x736f6d65 << 16) << 16) + 0x70736575) ^ seed;
    usize v1 = ((((usize)0x646f7261 << 16) << 16) + 0x6e646f6d) ^ ~seed;
    usize v2 = ((((usize)0x6c796765 << 16) << 16) + 0x6e657261) ^ seed;
    usize v3 = ((((usize)0x74656462 << 16) << 16) + 0x79746573) ^ ~seed;

#ifdef _CEXDS_TEST_SIPHASH_2_4
    // hardcoded with key material in the siphash test vectors
    v0 ^= 0x0706050403020100ull ^ seed;
    v1 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
    v2 ^= 0x0706050403020100ull ^ seed;
    v3 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
#endif

#define _CEXDS_SIPROUND()                                                                          \
    do {                                                                                           \
        v0 += v1;                                                                                  \
        v1 = _CEXDS_ROTATE_LEFT(v1, 13);                                                           \
        v1 ^= v0;                                                                                  \
        v0 = _CEXDS_ROTATE_LEFT(v0, _CEXDS_usize_BITS / 2);                                        \
        v2 += v3;                                                                                  \
        v3 = _CEXDS_ROTATE_LEFT(v3, 16);                                                           \
        v3 ^= v2;                                                                                  \
        v2 += v1;                                                                                  \
        v1 = _CEXDS_ROTATE_LEFT(v1, 17);                                                           \
        v1 ^= v2;                                                                                  \
        v2 = _CEXDS_ROTATE_LEFT(v2, _CEXDS_usize_BITS / 2);                                        \
        v0 += v3;                                                                                  \
        v3 = _CEXDS_ROTATE_LEFT(v3, 21);                                                           \
        v3 ^= v0;                                                                                  \
    } while (0)

    unsigned char* d = (unsigned char*)p;
    usize data = 0;
    usize i = 0;
    usize j = 0;
    for (i = 0; i + sizeof(usize) <= len; i += sizeof(usize), d += sizeof(usize)) {
        data = (usize)d[0] | ((usize)d[1] << 8) | ((usize)d[2] << 16) | ((usize)d[3] << 24);

#if UINTPTR_MAX > 0xFFFFFFFFU
        // 64 bits only
        data |= (usize)(d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24)) << 16 << 16;
#endif

        v3 ^= data;
        for (j = 0; j < _CEXDS_SIPHASH_C_ROUNDS; ++j) { _CEXDS_SIPROUND(); }
        v0 ^= data;
    }
    data = len << (_CEXDS_usize_BITS - 8);
    switch (len - i) {
        case 7:
            data |= ((usize)d[6] << 24) << 24; // fall through
        case 6:
            data |= ((usize)d[5] << 20) << 20; // fall through
        case 5:
            data |= ((usize)d[4] << 16) << 16; // fall through
        case 4:
            data |= (d[3] << 24); // fall through
        case 3:
            data |= (d[2] << 16); // fall through
        case 2:
            data |= (d[1] << 8); // fall through
        case 1:
            data |= d[0]; // fall through
        case 0:
            break;
    }
    v3 ^= data;
    for (j = 0; j < _CEXDS_SIPHASH_C_ROUNDS; ++j) { _CEXDS_SIPROUND(); }
    v0 ^= data;
    v2 ^= 0xff;
    for (j = 0; j < _CEXDS_SIPHASH_D_ROUNDS; ++j) { _CEXDS_SIPROUND(); }

#ifdef _CEXDS_SIPHASH_2_4
    return v0 ^ v1 ^ v2 ^ v3;
#else
    return v1 ^ v2 ^ v3;
#endif
}

static inline usize
_cexds__hash_bytes(const void* p, usize len, usize seed)
{
#ifdef _CEXDS_SIPHASH_2_4
    return _cexds__siphash_bytes(p, len, seed);
#else
    unsigned char* d = (unsigned char*)p;

    if (len == 4) {
        u32 hash = (u32)d[0] | ((u32)d[1] << 8) | ((u32)d[2] << 16) | ((u32)d[3] << 24);
        // HASH32-BB  Bob Jenkin's presumably-accidental version of Thomas Wang hash with rotates
        // turned into shifts. Note that converting these back to rotates makes it run a lot slower,
        // presumably due to collisions, so I'm not really sure what's going on.
        hash ^= seed;
        hash = (hash ^ 61) ^ (hash >> 16);
        hash = hash + (hash << 3);
        hash = hash ^ (hash >> 4);
        hash = hash * 0x27d4eb2d;
        hash ^= seed;
        hash = hash ^ (hash >> 15);
        return (((usize)hash << 16 << 16) | hash) ^ seed;
    } else if (len == 8 && sizeof(usize) == 8) {
        usize hash = (usize)d[0] | ((usize)d[1] << 8) | ((usize)d[2] << 16) | ((usize)d[3] << 24);

        hash |= (usize)((usize)d[4] | ((usize)d[5] << 8) | ((usize)d[6] << 16) | ((usize)d[7] << 24))
             << 16 << 16;
        hash ^= seed;
        hash = (~hash) + (hash << 21);
        hash ^= _CEXDS_ROTATE_RIGHT(hash, 24);
        hash *= 265;
        hash ^= _CEXDS_ROTATE_RIGHT(hash, 14);
        hash ^= seed;
        hash *= 21;
        hash ^= _CEXDS_ROTATE_RIGHT(hash, 28);
        hash += (hash << 31);
        hash = (~hash) + (hash << 18);
        return hash;
    } else {
        return _cexds__siphash_bytes(p, len, seed);
    }
#endif
}

static inline usize
_cexds__hash(enum _CexDsKeyType_e key_type, const void* key, usize key_size, usize seed)
{
    switch (key_type) {
        case _CexDsKeyType__generic:
            return _cexds__hash_bytes(key, key_size, seed);

        case _CexDsKeyType__charptr:
            // NOTE: we can't know char* length without touching it,
            // 65536 is a max key length in case of char was not null term
            return _cexds__hash_string(*(char**)key, 65536, seed);

        case _CexDsKeyType__charbuf:
            return _cexds__hash_string(key, key_size, seed);

        case _CexDsKeyType__cexstr: {
            str_s* s = (str_s*)key;
            return _cexds__hash_string(s->buf, s->len, seed);
        }
    }
    uassert(false && "unexpected key type");
    abort();
}

static bool
_cexds__is_key_equal(
    void* a,
    usize elemsize,
    void* key,
    usize keysize,
    usize keyoffset,
    enum _CexDsKeyType_e key_type,
    usize i
)
{
    void* hm_key = _cexds__item_ptr(a, i, elemsize) + keyoffset;

    switch (key_type) {
        case _CexDsKeyType__generic:
            return 0 == memcmp(key, hm_key, keysize);

        case _CexDsKeyType__charptr:
            return 0 == strcmp(*(char**)key, *(char**)hm_key);
        case _CexDsKeyType__charbuf:
            return 0 == strcmp((char*)key, (char*)hm_key);

        case _CexDsKeyType__cexstr: {
            str_s* _k = (str_s*)key;
            str_s* _hm = (str_s*)hm_key;
            if (_k->len != _hm->len) { return false; }
            return 0 == memcmp(_k->buf, _hm->buf, _k->len);
        }
    }
    uassert(false && "unexpected key type");
    abort();
}

static inline void*
_cexds__hmkey_ptr(void* a, usize elemsize, usize index, usize keyoffset)
{
    _cexds__hash_index* table = (_cexds__hash_index*)_cexds__header(a)->_hash_table;
    void* key_data_p = NULL;
    switch (table->key_type) {
        case _CexDsKeyType__generic:
            key_data_p = (char*)a + elemsize * index + keyoffset;
            break;

        case _CexDsKeyType__charbuf:
            key_data_p = (char*)((char*)a + elemsize * index + keyoffset);
            break;

        case _CexDsKeyType__charptr:
            key_data_p = *(char**)((char*)a + elemsize * index + keyoffset);
            break;

        case _CexDsKeyType__cexstr: {
            str_s* s = (str_s*)((char*)a + elemsize * index + keyoffset);
            key_data_p = s;
            break;
        }
        default:
            unreachable();
    }
    return key_data_p;
}

void
_cexds__hmfree_keys_func(void* a, usize elemsize, usize keyoffset)
{
    _cexds__array_header* h = _cexds__header(a);
    uassert(h->allocator != NULL);
    _cexds__hash_index* table = h->_hash_table;

    if (table != NULL) {
        if (table->copy_keys) {
            if (table->key_arena == NULL) {
                for (usize i = 0; i < h->length; ++i) {
                    h->allocator->free(h->allocator, *(char**)((char*)a + elemsize * i + keyoffset));
                }
            } else {
                uassert(table->key_arena->scope_depth(table->key_arena) > 0);
                // Exit + enter for arena is equivalent of cleaning up and poisoning
                table->key_arena->scope_exit(table->key_arena);
                table->key_arena->scope_enter(table->key_arena);
            }
        }
    }
}
void
_cexds__hmfree_func(void* a, usize elemsize, usize keyoffset)
{
    if (a == NULL) { return; }
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);

    _cexds__array_header* h = _cexds__header(a);
    _cexds__hmfree_keys_func(a, elemsize, keyoffset);
    if (h->_hash_table->key_arena) { AllocatorArena.destroy(h->_hash_table->key_arena); }
    h->allocator->free(h->allocator, h->_hash_table);
    h->allocator->free(h->allocator, _cexds__base(h));
}

static ptrdiff_t
_cexds__hm_find_slot(void* a, usize elemsize, void* key, usize keysize, usize keyoffset)
{
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);
    _cexds__hash_index* table = _cexds__hash_table(a);
    enum _CexDsKeyType_e key_type = table->key_type;
    usize hash = _cexds__hash(key_type, key, keysize, table->seed);
    usize step = _CEXDS_BUCKET_LENGTH;

    if (hash < 2) {
        hash += 2; // stored hash values are forbidden from being 0, so we can detect empty slots
    }

    usize pos = _cexds__probe_position(hash, table->slot_count, table->slot_count_log2);

    // TODO: check when this could be infinite loop (due to overflow or something)?
    for (;;) {
        _CEXDS_STATS(++_cexds__hash_probes);
        _cexds__hash_bucket* bucket = &table->storage[pos >> _CEXDS_BUCKET_SHIFT];

        // start searching from pos to end of bucket, this should help performance on small hash
        // tables that fit in cache
        for (usize i = pos & _CEXDS_BUCKET_MASK; i < _CEXDS_BUCKET_LENGTH; ++i) {
            if (bucket->hash[i] == hash) {
                if (_cexds__is_key_equal(
                        a,
                        elemsize,
                        key,
                        keysize,
                        keyoffset,
                        key_type,
                        bucket->index[i]
                    )) {
                    return (pos & ~_CEXDS_BUCKET_MASK) + i;
                }
            } else if (bucket->hash[i] == _CEXDS_HASH_EMPTY) {
                return -1;
            }
        }

        // search from beginning of bucket to pos
        usize limit = pos & _CEXDS_BUCKET_MASK;
        for (usize i = 0; i < limit; ++i) {
            if (bucket->hash[i] == hash) {
                if (_cexds__is_key_equal(
                        a,
                        elemsize,
                        key,
                        keysize,
                        keyoffset,
                        key_type,
                        bucket->index[i]
                    )) {
                    return (pos & ~_CEXDS_BUCKET_MASK) + i;
                }
            } else if (bucket->hash[i] == _CEXDS_HASH_EMPTY) {
                return -1;
            }
        }

        // quadratic probing
        pos += step;
        step += _CEXDS_BUCKET_LENGTH;
        pos &= (table->slot_count - 1);
    }
}

void*
_cexds__hmget_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset)
{
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);

    _cexds__hash_index* table = (_cexds__hash_index*)_cexds__header(a)->_hash_table;
    if (table != NULL) {
        ptrdiff_t slot = _cexds__hm_find_slot(a, elemsize, key, keysize, keyoffset);
        if (slot >= 0) {
            _cexds__hash_bucket* b = &table->storage[slot >> _CEXDS_BUCKET_SHIFT];
            usize idx = b->index[slot & _CEXDS_BUCKET_MASK];
            return ((char*)a + elemsize * idx);
        }
    }
    return NULL;
}

void*
_cexds__hminit(
    usize elemsize,
    IAllocator allc,
    enum _CexDsKeyType_e key_type,
    u16 el_align,
    struct _cexds__hm_new_kwargs_s* kwargs
)
{
    usize capacity = 0;
    usize hm_seed = 0xBadB0dee;
    bool copy_keys = false;

    if (kwargs) {
        capacity = kwargs->capacity;
        hm_seed = kwargs->seed ? kwargs->seed : hm_seed;
        copy_keys = kwargs->copy_keys;
    }
    void* a = _cexds__arrgrowf(NULL, elemsize, 0, capacity, el_align, allc);
    if (a == NULL) {
        return NULL; // memory error
    }

    uassert(_cexds__header(a)->magic_num == _CEXDS_ARR_MAGIC);
    uassert(_cexds__header(a)->_hash_table == NULL);

    _cexds__header(a)->magic_num = _CEXDS_HM_MAGIC;

    _cexds__hash_index* table = _cexds__header(a)->_hash_table;

    // ensure slot counts must be pow of 2
    uassert(mem$is_power_of2(arr$cap(a)));
    table = _cexds__header(a)->_hash_table = _cexds__make_hash_index(
        arr$cap(a),
        NULL,
        _cexds__header(a)->allocator,
        hm_seed,
        key_type
    );

    if (table) {
        // NEW Table initialization here
        if (copy_keys) {
            uassert(table->key_type == _CexDsKeyType__charptr && "Only char* keys supported");
        }
        table->copy_keys = copy_keys;
        if (kwargs && kwargs->copy_keys_arena_pgsize > 0) {
            table->key_arena = AllocatorArena.create(kwargs->copy_keys_arena_pgsize);
        }
    }

    return a;
}


void*
_cexds__hmput_key(
    void* a,
    usize elemsize,
    void* key,
    usize keysize,
    usize keyoffset,
    void* full_elem,
    void* result
)
{
    uassert(result != NULL);
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);

    void** out_result = (void**)result;
    _cexds__hash_index* table = (_cexds__hash_index*)_cexds__header(a)->_hash_table;
    enum _CexDsKeyType_e key_type = table->key_type;
    *out_result = NULL;

    uassert(table != NULL);
    if (table->used_count >= table->used_count_threshold) {

        usize slot_count = (table == NULL) ? _CEXDS_BUCKET_LENGTH : table->slot_count * 2;
        _cexds__array_header* hdr = _cexds__header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        _cexds__hash_index* nt = _cexds__make_hash_index(
            slot_count,
            table,
            _cexds__header(a)->allocator,
            table->seed,
            table->key_type
        );

        if (nt == NULL) {
            uassert(nt != NULL && "new hash table memory error");
            *out_result = NULL;
            goto end;
        }

        if (table) {
            _cexds__header(a)->allocator->free(_cexds__header(a)->allocator, table);
        } else {
            // NEW Table initialization here
            nt->copy_keys = table->copy_keys;
            nt->key_arena = table->key_arena;
        }
        _cexds__header(a)->_hash_table = table = nt;
        _CEXDS_STATS(++_cexds__hash_grow);
    }

    // we iterate hash table explicitly because we want to track if we saw a tombstone
    {
        usize hash = _cexds__hash(key_type, key, keysize, table->seed);
        usize step = _CEXDS_BUCKET_LENGTH;
        usize pos;
        ptrdiff_t tombstone = -1;
        _cexds__hash_bucket* bucket;

        // stored hash values are forbidden from being 0, so we can detect empty slots to early out
        // quickly
        if (hash < 2) { hash += 2; }

        pos = _cexds__probe_position(hash, table->slot_count, table->slot_count_log2);

        for (;;) {
            usize limit, i;
            _CEXDS_STATS(++_cexds__hash_probes);
            bucket = &table->storage[pos >> _CEXDS_BUCKET_SHIFT];

            // start searching from pos to end of bucket
            for (i = pos & _CEXDS_BUCKET_MASK; i < _CEXDS_BUCKET_LENGTH; ++i) {
                if (bucket->hash[i] == hash) {
                    if (_cexds__is_key_equal(
                            a,
                            elemsize,
                            key,
                            keysize,
                            keyoffset,
                            key_type,
                            bucket->index[i]
                        )) {

                        *out_result = _cexds__item_ptr(a, bucket->index[i], elemsize);
                        goto process_key;
                    }
                } else if (bucket->hash[i] == 0) {
                    pos = (pos & ~_CEXDS_BUCKET_MASK) + i;
                    goto found_empty_slot;
                } else if (tombstone < 0) {
                    if (bucket->index[i] == _CEXDS_INDEX_DELETED) {
                        tombstone = (ptrdiff_t)((pos & ~_CEXDS_BUCKET_MASK) + i);
                    }
                }
            }

            // search from beginning of bucket to pos
            limit = pos & _CEXDS_BUCKET_MASK;
            for (i = 0; i < limit; ++i) {
                if (bucket->hash[i] == hash) {
                    if (_cexds__is_key_equal(
                            a,
                            elemsize,
                            key,
                            keysize,
                            keyoffset,
                            key_type,
                            bucket->index[i]
                        )) {
                        *out_result = _cexds__item_ptr(a, bucket->index[i], elemsize);
                        goto process_key;
                    }
                } else if (bucket->hash[i] == 0) {
                    pos = (pos & ~_CEXDS_BUCKET_MASK) + i;
                    goto found_empty_slot;
                } else if (tombstone < 0) {
                    if (bucket->index[i] == _CEXDS_INDEX_DELETED) {
                        tombstone = (ptrdiff_t)((pos & ~_CEXDS_BUCKET_MASK) + i);
                    }
                }
            }

            // quadratic probing
            pos += step;
            step += _CEXDS_BUCKET_LENGTH;
            pos &= (table->slot_count - 1);
        }
    found_empty_slot:
        if (tombstone >= 0) {
            pos = tombstone;
            --table->tombstone_count;
        }
        ++table->used_count;

        {
            ptrdiff_t i = (ptrdiff_t)_cexds__header(a)->length;
            // we want to do _cexds__arraddn(1), but we can't use the macros since we don't have
            // something of the right type
            if ((usize)i + 1 > arr$cap(a)) {
                *(void**)&a = _cexds__arrgrowf(a, elemsize, 1, 0, _cexds__header(a)->el_align, NULL);
                if (a == NULL) {
                    uassert(a != NULL && "new array for table memory error");
                    *out_result = NULL;
                    goto end;
                }
            }

            uassert((usize)i + 1 <= arr$cap(a));
            _cexds__header(a)->length = i + 1;
            bucket = &table->storage[pos >> _CEXDS_BUCKET_SHIFT];
            bucket->hash[pos & _CEXDS_BUCKET_MASK] = hash;
            bucket->index[pos & _CEXDS_BUCKET_MASK] = i;
            *out_result = _cexds__item_ptr(a, i, elemsize);
        }
        goto process_key;
    }

process_key:
    uassert(*out_result != NULL);
    if (full_elem) {
        memcpy(((char*)*out_result), full_elem, elemsize);
    } else {
        memcpy(((char*)*out_result) + keyoffset, key, keysize);
    }
    if (table->copy_keys) {
        uassert(key_type == _CexDsKeyType__charptr);
        uassert(keysize == sizeof(usize) && "expected pointer size");
        char** key_data_p = (char**)(*out_result + keyoffset);

        // Naive reimplementation of str.close() for optional isolation
        if (*key_data_p) {
            usize slen = strlen(*key_data_p);
            uassert(slen < PTRDIFF_MAX);
            IAllocator _allc = (table->key_arena) ? table->key_arena : _cexds__header(a)->allocator;

            char* result = mem$malloc(_allc, slen + 1);
            if (result) {
                memcpy(result, *key_data_p, slen);
                result[slen] = '\0';
            }
            *key_data_p = result;
        }
        

    }

end:
    return a;
}


bool
_cexds__hmdel_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset)
{
    _cexds__arr_integrity(a, _CEXDS_HM_MAGIC);

    _cexds__hash_index* table = (_cexds__hash_index*)_cexds__header(a)->_hash_table;
    uassert(_cexds__header(a)->allocator != NULL);
    if (table == NULL) { return false; }

    ptrdiff_t slot = _cexds__hm_find_slot(a, elemsize, key, keysize, keyoffset);
    if (slot < 0) { return false; }

    _cexds__hash_bucket* b = &table->storage[slot >> _CEXDS_BUCKET_SHIFT];
    int i = slot & _CEXDS_BUCKET_MASK;
    ptrdiff_t old_index = b->index[i];
    ptrdiff_t final_index = (ptrdiff_t)_cexds__header(a)->length - 1;
    uassert(slot < (ptrdiff_t)table->slot_count);
    uassert(table->used_count > 0);
    --table->used_count;
    ++table->tombstone_count;
    // uassert(table->tombstone_count < table->slot_count/4);
    b->hash[i] = _CEXDS_HASH_DELETED;
    b->index[i] = _CEXDS_INDEX_DELETED;

    if (table->copy_keys) {
        if (table->key_arena == NULL) {
            IAllocator _allc = _cexds__header(a)->allocator;
            _allc->free(_allc, *(char**)((char*)a + elemsize * old_index + keyoffset));
        }
    }

    // if indices are the same, memcpy is a no-op, but back-pointer-fixup will fail, so
    // skip
    if (old_index != final_index) {
        // Replacing deleted element by last one, and update hashmap buckets for last element
        memmove((char*)a + elemsize * old_index, (char*)a + elemsize * final_index, elemsize);

        void* key_data_p = _cexds__hmkey_ptr(a, elemsize, old_index, keyoffset);
        uassert(key_data_p != NULL);
        slot = _cexds__hm_find_slot(a, elemsize, key_data_p, keysize, keyoffset);
        uassert(slot >= 0);
        b = &table->storage[slot >> _CEXDS_BUCKET_SHIFT];
        i = slot & _CEXDS_BUCKET_MASK;
        uassert(b->index[i] == final_index);
        b->index[i] = old_index;
    }
    _cexds__header(a)->length -= 1;

    if (table->used_count < table->used_count_shrink_threshold &&
        table->slot_count > _CEXDS_BUCKET_LENGTH) {
        _cexds__array_header* hdr = _cexds__header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        _cexds__header(a)->_hash_table = _cexds__make_hash_index(
            table->slot_count >> 1,
            table,
            _cexds__header(a)->allocator,
            table->seed,
            table->key_type
        );
        _cexds__header(a)->allocator->free(_cexds__header(a)->allocator, table);
        _CEXDS_STATS(++_cexds__hash_shrink);
    } else if (table->tombstone_count > table->tombstone_count_threshold) {
        _cexds__array_header* hdr = _cexds__header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        _cexds__header(a)->_hash_table = _cexds__make_hash_index(
            table->slot_count,
            table,
            _cexds__header(a)->allocator,
            table->seed,
            table->key_type
        );
        _cexds__header(a)->allocator->free(_cexds__header(a)->allocator, table);
        _CEXDS_STATS(++_cexds__hash_rebuild);
    }

    return a;
}

#endif



/*
*                          src/_sprintf.c
*/
/*
This code is based on refactored stb_sprintf.h

Original code
https://github.com/nothings/stb/tree/master
ALTERNATIVE A - MIT License
stb_sprintf - v1.10 - public domain snprintf() implementation
Copyright (c) 2017 Sean Barrett
*/

// #include <assert.h> // NOTE: using LibC asserts here to avoid infinite callback stack overflow
#include <ctype.h>


#ifndef CEX_SPRINTF_NOFLOAT
// internal float utility functions
static i32 cexsp__real_to_str(
    char const** start,
    u32* len,
    char* out,
    i32* decimal_pos,
    double value,
    u32 frac_digits
);
static i32 cexsp__real_to_parts(i64* bits, i32* expo, double value);
#    define CEXSP__SPECIAL 0x7000
#endif

static char cexsp__period = '.';
static char cexsp__comma = ',';
static struct
{
    short temp; // force next field to be 2-byte aligned
    char pair[201];
} cexsp__digitpair = { 0,
                       "00010203040506070809101112131415161718192021222324"
                       "25262728293031323334353637383940414243444546474849"
                       "50515253545556575859606162636465666768697071727374"
                       "75767778798081828384858687888990919293949596979899" };

CEXSP__PUBLICDEF void
cexsp__set_separators(char pcomma, char pperiod)
{
    cexsp__period = pperiod;
    cexsp__comma = pcomma;
}

#define CEXSP__LEFTJUST 1
#define CEXSP__LEADINGPLUS 2
#define CEXSP__LEADINGSPACE 4
#define CEXSP__LEADING_0X 8
#define CEXSP__LEADINGZERO 16
#define CEXSP__INTMAX 32
#define CEXSP__TRIPLET_COMMA 64
#define CEXSP__NEGATIVE 128
#define CEXSP__METRIC_SUFFIX 256
#define CEXSP__HALFWIDTH 512
#define CEXSP__METRIC_NOSPACE 1024
#define CEXSP__METRIC_1024 2048
#define CEXSP__METRIC_JEDEC 4096

static void
cexsp__lead_sign(u32 fl, char* sign)
{
    sign[0] = 0;
    if (fl & CEXSP__NEGATIVE) {
        sign[0] = 1;
        sign[1] = '-';
    } else if (fl & CEXSP__LEADINGSPACE) {
        sign[0] = 1;
        sign[1] = ' ';
    } else if (fl & CEXSP__LEADINGPLUS) {
        sign[0] = 1;
        sign[1] = '+';
    }
}

static u32
cexsp__format_s_check_va_item_string_len(char const* s, u32 limit)
{
    char const* sn = s;
    while (limit && *sn) { // WARNING: if getting segfault here, typically %s format messes with int
        ++sn;
        --limit;
    }

    return (u32)(sn - s);
}

CEXSP__PUBLICDEF int
cexsp__vsprintfcb(cexsp_callback_f* callback, void* user, char* buf, char const* fmt, va_list va)
{
    static char hex[] = "0123456789abcdefxp";
    static char hexu[] = "0123456789ABCDEFXP";
    char* bf = buf;
    char const* f;
    int tlen = 0;

    f = fmt;
    for (;;) {
        i32 fw, pr, tz;
        u32 fl;

// macros for the callback buffer stuff
#define cexsp__chk_cb_bufL(bytes)                                                                  \
    {                                                                                              \
        int len = (int)(bf - buf);                                                                 \
        if ((len + (bytes)) >= CEX_SPRINTF_MIN) {                                                  \
            tlen += len;                                                                           \
            if (0 == (bf = buf = callback(buf, user, len))) goto done;                             \
        }                                                                                          \
    }
#define cexsp__chk_cb_buf(bytes)                                                                   \
    {                                                                                              \
        if (callback) { cexsp__chk_cb_bufL(bytes); }                                               \
    }
#define cexsp__flush_cb()                                                                          \
    {                                                                                              \
        cexsp__chk_cb_bufL(CEX_SPRINTF_MIN - 1);                                                   \
    } // flush if there is even one byte in the buffer
#define cexsp__cb_buf_clamp(cl, v)                                                                 \
    cl = v;                                                                                        \
    if (callback) {                                                                                \
        int lg = CEX_SPRINTF_MIN - (int)(bf - buf);                                                \
        if (cl > lg) cl = lg;                                                                      \
    }

        // fast copy everything up to the next % (or end of string)
        for (;;) {
            if (f[0] == '%') { goto scandd; }
            if (f[0] == 0) { goto endfmt; }
            cexsp__chk_cb_buf(1);
            *bf++ = f[0];
            ++f;
        }
    scandd:

        ++f;

        // ok, we have a percent, read the modifiers first
        fw = 0;
        pr = -1;
        fl = 0;
        tz = 0;

        // flags
        for (;;) {
            switch (f[0]) {
                // if we have left justify
                case '-':
                    fl |= CEXSP__LEFTJUST;
                    ++f;
                    continue;
                // if we have leading plus
                case '+':
                    fl |= CEXSP__LEADINGPLUS;
                    ++f;
                    continue;
                // if we have leading space
                case ' ':
                    fl |= CEXSP__LEADINGSPACE;
                    ++f;
                    continue;
                // if we have leading 0x
                case '#':
                    fl |= CEXSP__LEADING_0X;
                    ++f;
                    continue;
                // if we have thousand commas
                case '\'':
                    fl |= CEXSP__TRIPLET_COMMA;
                    ++f;
                    continue;
                // if we have kilo marker (none->kilo->kibi->jedec)
                case '$':
                    if (fl & CEXSP__METRIC_SUFFIX) {
                        if (fl & CEXSP__METRIC_1024) {
                            fl |= CEXSP__METRIC_JEDEC;
                        } else {
                            fl |= CEXSP__METRIC_1024;
                        }
                    } else {
                        fl |= CEXSP__METRIC_SUFFIX;
                    }
                    ++f;
                    continue;
                // if we don't want space between metric suffix and number
                case '_':
                    fl |= CEXSP__METRIC_NOSPACE;
                    ++f;
                    continue;
                // if we have leading zero
                case '0':
                    fl |= CEXSP__LEADINGZERO;
                    ++f;
                    goto flags_done;
                default:
                    goto flags_done;
            }
        }
    flags_done:

        // get the field width
        if (f[0] == '*') {
            fw = va_arg(va, u32);
            ++f;
        } else {
            while ((f[0] >= '0') && (f[0] <= '9')) {
                fw = fw * 10 + f[0] - '0';
                f++;
            }
        }
        // get the precision
        if (f[0] == '.') {
            ++f;
            if (f[0] == '*') {
                pr = va_arg(va, u32);
                ++f;
            } else {
                pr = 0;
                while ((f[0] >= '0') && (f[0] <= '9')) {
                    pr = pr * 10 + f[0] - '0';
                    f++;
                }
            }
        }

        // handle integer size overrides
        switch (f[0]) {
            // are we halfwidth?
            case 'h':
                fl |= CEXSP__HALFWIDTH;
                ++f;
                if (f[0] == 'h') {
                    ++f; // QUARTERWIDTH
                }
                break;
            // are we 64-bit (unix style)
            case 'l':
                // %ld/%lld - is always 64 bits
                fl |= CEXSP__INTMAX;
                ++f;
                if (f[0] == 'l') { ++f; }
                break;
            // are we 64-bit on intmax? (c99)
            case 'j':
                fl |= (sizeof(intmax_t) == 8) ? CEXSP__INTMAX : 0;
                ++f;
                break;
            // are we 64-bit on size_t or ptrdiff_t? (c99)
            case 'z':
                fl |= (sizeof(ptrdiff_t) == 8) ? CEXSP__INTMAX : 0;
                ++f;
                break;
            case 't':
                fl |= (sizeof(ptrdiff_t) == 8) ? CEXSP__INTMAX : 0;
                ++f;
                break;
            // are we 64-bit (msft style)
            case 'I':
                if ((f[1] == '6') && (f[2] == '4')) {
                    fl |= CEXSP__INTMAX;
                    f += 3;
                } else if ((f[1] == '3') && (f[2] == '2')) {
                    f += 3;
                } else {
                    fl |= ((sizeof(void*) == 8) ? CEXSP__INTMAX : 0);
                    ++f;
                }
                break;
            default:
                break;
        }

        // handle each replacement
        switch (f[0]) {
#define CEXSP__NUMSZ 512 // big enough for e308 (with commas) or e-307
            char num[CEXSP__NUMSZ];
            char lead[8];
            char tail[8];
            char* s;
            char const* h;
            u32 l, n, cs;
            u64 n64;
#ifndef CEX_SPRINTF_NOFLOAT
            double fv;
#endif
            i32 dp;
            char const* sn;

            case 's':
                // get the string
                s = va_arg(va, char*);
                if (unlikely((void*)s <= (void*)(UINT16_MAX))) {
                    if (s == 0) {
                        s = "(null)";
                    } else {
#if !defined(__EMSCRIPTEN__)
                        // NOTE: cex is str_s passed as %s, s will be length
                        // try to double check sensible value of pointer (only on OS-like platforms)
                        s = "(%s-bad)";
#endif
                    }
                }
#if defined(CEX_TEST) && defined(_WIN32)
                if (IsBadReadPtr(s, 1)) { s = (char*)"(%s-bad)"; }
#endif
                // get the length, limited to desired precision
                // always limit to ~0u chars since our counts are 32b
                l = cexsp__format_s_check_va_item_string_len(s, (pr >= 0) ? (unsigned)pr : ~0u);
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                // copy the string in
                goto scopy;
            case 'S': {
                // NOTE: CEX extra (support of str_s)
                str_s sv = va_arg(va, str_s);
                s = sv.buf;
                if (s == 0) {
                    s = "(null)";
                    l = cexsp__format_s_check_va_item_string_len(s, ~0u);
                } else {
                    if (pr == -1 && sv.len > UINT16_MAX) {
                        s = "(%S-bad/overflow)";
                        l = cexsp__format_s_check_va_item_string_len(s, ~0u);
                    } else {
                        l = (pr >= 0 && (u32)pr < sv.len) ? (u32)pr : sv.len;
                    }
                }
#if defined(CEX_TEST) && defined(_WIN32)
                if (IsBadReadPtr(s, 1)) {
                    s = "(%S-bad)";
                    l = cexsp__format_s_check_va_item_string_len(s, ~0u);
                }
#endif
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                // copy the string in
                goto scopy;
            }
            case 'c': // char
                // get the character
                s = num + CEXSP__NUMSZ - 1;
                *s = (char)va_arg(va, int);
                l = 1;
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                goto scopy;

            case 'n': // weird write-bytes specifier
            {
                int* d = va_arg(va, int*);
                *d = tlen + (int)(bf - buf);
            } break;

#ifdef CEX_SPRINTF_NOFLOAT
            case 'A':               // float
            case 'a':               // hex float
            case 'G':               // float
            case 'g':               // float
            case 'E':               // float
            case 'e':               // float
            case 'f':               // float
                va_arg(va, double); // eat it
                s = (char*)"No float";
                l = 8;
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                cs = 0;
                (void)(dp);
                goto scopy;
#else
            case 'A': // hex float
            case 'a': // hex float
                h = (f[0] == 'A') ? hexu : hex;
                fv = va_arg(va, double);
                if (pr == -1) {
                    pr = 6; // default is 6
                }
                // read the double into a string
                if (cexsp__real_to_parts((i64*)&n64, &dp, fv)) { fl |= CEXSP__NEGATIVE; }

                s = num + 64;

                cexsp__lead_sign(fl, lead);

                if (dp == -1023) {
                    dp = (n64) ? -1022 : 0;
                } else {
                    n64 |= (((u64)1) << 52);
                }
                n64 <<= (64 - 56);
                if (pr < 15) { n64 += ((((u64)8) << 56) >> (pr * 4)); }
                // add leading chars

                lead[1 + lead[0]] = '0';
                lead[2 + lead[0]] = 'x';
                lead[0] += 2;
                *s++ = h[(n64 >> 60) & 15];
                n64 <<= 4;
                if (pr) { *s++ = cexsp__period; }
                sn = s;

                // print the bits
                n = pr;
                if (n > 13) { n = 13; }
                if (pr > (i32)n) { tz = pr - n; }
                pr = 0;
                while (n--) {
                    *s++ = h[(n64 >> 60) & 15];
                    n64 <<= 4;
                }

                // print the expo
                tail[1] = h[17];
                if (dp < 0) {
                    tail[2] = '-';
                    dp = -dp;
                } else {
                    tail[2] = '+';
                }
                n = (dp >= 1000) ? 6 : ((dp >= 100) ? 5 : ((dp >= 10) ? 4 : 3));
                tail[0] = (char)n;
                for (;;) {
                    tail[n] = '0' + dp % 10;
                    if (n <= 3) { break; }
                    --n;
                    dp /= 10;
                }

                dp = (int)(s - sn);
                l = (int)(s - (num + 64));
                s = num + 64;
                cs = 1 + (3 << 24);
                goto scopy;

            case 'G': // float
            case 'g': // float
                h = (f[0] == 'G') ? hexu : hex;
                fv = va_arg(va, double);
                if (pr == -1) {
                    pr = 6;
                } else if (pr == 0) {
                    pr = 1; // default is 6
                }
                // read the double into a string
                if (cexsp__real_to_str(&sn, &l, num, &dp, fv, (pr - 1) | 0x80000000)) {
                    fl |= CEXSP__NEGATIVE;
                }

                // clamp the precision and delete extra zeros after clamp
                n = pr;
                if (l > (u32)pr) { l = pr; }
                while ((l > 1) && (pr) && (sn[l - 1] == '0')) {
                    --pr;
                    --l;
                }

                // should we use %e
                if ((dp <= -4) || (dp > (i32)n)) {
                    if (pr > (i32)l) {
                        pr = l - 1;
                    } else if (pr) {
                        --pr; // when using %e, there is one digit before the decimal
                    }
                    goto doexpfromg;
                }
                // this is the insane action to get the pr to match %g semantics for %f
                if (dp > 0) {
                    pr = (dp < (i32)l) ? l - dp : 0;
                } else {
                    pr = -dp + ((pr > (i32)l) ? (i32)l : pr);
                }
                goto dofloatfromg;

            case 'E': // float
            case 'e': // float
                h = (f[0] == 'E') ? hexu : hex;
                fv = va_arg(va, double);
                if (pr == -1) {
                    pr = 6; // default is 6
                }
                // read the double into a string
                if (cexsp__real_to_str(&sn, &l, num, &dp, fv, pr | 0x80000000)) {
                    fl |= CEXSP__NEGATIVE;
                }
            doexpfromg:
                tail[0] = 0;
                cexsp__lead_sign(fl, lead);
                if (dp == CEXSP__SPECIAL) {
                    s = (char*)sn;
                    cs = 0;
                    pr = 0;
                    goto scopy;
                }
                s = num + 64;
                // handle leading chars
                *s++ = sn[0];

                if (pr) { *s++ = cexsp__period; }

                // handle after decimal
                if ((l - 1) > (u32)pr) { l = pr + 1; }
                for (n = 1; n < l; n++) { *s++ = sn[n]; }
                // trailing zeros
                tz = pr - (l - 1);
                pr = 0;
                // dump expo
                tail[1] = h[0xe];
                dp -= 1;
                if (dp < 0) {
                    tail[2] = '-';
                    dp = -dp;
                } else {
                    tail[2] = '+';
                }
                n = (dp >= 100) ? 5 : 4;
                tail[0] = (char)n;
                for (;;) {
                    tail[n] = '0' + dp % 10;
                    if (n <= 3) { break; }
                    --n;
                    dp /= 10;
                }
                cs = 1 + (3 << 24); // how many tens
                goto flt_lead;

            case 'f': // float
                fv = va_arg(va, double);
            doafloat:
                // do kilos
                if (fl & CEXSP__METRIC_SUFFIX) {
                    double divisor;
                    divisor = 1000.0f;
                    if (fl & CEXSP__METRIC_1024) { divisor = 1024.0; }
                    while (fl < 0x4000000) {
                        if ((fv < divisor) && (fv > -divisor)) { break; }
                        fv /= divisor;
                        fl += 0x1000000;
                    }
                }
                if (pr == -1) {
                    pr = 6; // default is 6
                }
                // read the double into a string
                if (cexsp__real_to_str(&sn, &l, num, &dp, fv, pr)) { fl |= CEXSP__NEGATIVE; }
            dofloatfromg:
                tail[0] = 0;
                cexsp__lead_sign(fl, lead);
                if (dp == CEXSP__SPECIAL) {
                    s = (char*)sn;
                    cs = 0;
                    pr = 0;
                    goto scopy;
                }
                s = num + 64;

                // handle the three decimal varieties
                if (dp <= 0) {
                    i32 i;
                    // handle 0.000*000xxxx
                    *s++ = '0';
                    if (pr) { *s++ = cexsp__period; }
                    n = -dp;
                    if ((i32)n > pr) { n = pr; }
                    i = n;
                    while (i) {
                        if ((((usize)s) & 3) == 0) { break; }
                        *s++ = '0';
                        --i;
                    }
                    while (i >= 4) {
                        *(u32*)s = 0x30303030;
                        s += 4;
                        i -= 4;
                    }
                    while (i) {
                        *s++ = '0';
                        --i;
                    }
                    if ((i32)(l + n) > pr) { l = pr - n; }
                    i = l;
                    while (i) {
                        *s++ = *sn++;
                        --i;
                    }
                    tz = pr - (n + l);
                    cs = 1 + (3 << 24); // how many tens did we write (for commas below)
                } else {
                    cs = (fl & CEXSP__TRIPLET_COMMA) ? ((600 - (u32)dp) % 3) : 0;
                    if ((u32)dp >= l) {
                        // handle xxxx000*000.0
                        n = 0;
                        for (;;) {
                            if ((fl & CEXSP__TRIPLET_COMMA) && (++cs == 4)) {
                                cs = 0;
                                *s++ = cexsp__comma;
                            } else {
                                *s++ = sn[n];
                                ++n;
                                if (n >= l) { break; }
                            }
                        }
                        if (n < (u32)dp) {
                            n = dp - n;
                            if ((fl & CEXSP__TRIPLET_COMMA) == 0) {
                                while (n) {
                                    if ((((usize)s) & 3) == 0) { break; }
                                    *s++ = '0';
                                    --n;
                                }
                                while (n >= 4) {
                                    *(u32*)s = 0x30303030;
                                    s += 4;
                                    n -= 4;
                                }
                            }
                            while (n) {
                                if ((fl & CEXSP__TRIPLET_COMMA) && (++cs == 4)) {
                                    cs = 0;
                                    *s++ = cexsp__comma;
                                } else {
                                    *s++ = '0';
                                    --n;
                                }
                            }
                        }
                        cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
                        if (pr) {
                            *s++ = cexsp__period;
                            tz = pr;
                        }
                    } else {
                        // handle xxxxx.xxxx000*000
                        n = 0;
                        for (;;) {
                            if ((fl & CEXSP__TRIPLET_COMMA) && (++cs == 4)) {
                                cs = 0;
                                *s++ = cexsp__comma;
                            } else {
                                *s++ = sn[n];
                                ++n;
                                if (n >= (u32)dp) { break; }
                            }
                        }
                        cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
                        if (pr) { *s++ = cexsp__period; }
                        if ((l - dp) > (u32)pr) { l = pr + dp; }
                        while (n < l) {
                            *s++ = sn[n];
                            ++n;
                        }
                        tz = pr - (l - dp);
                    }
                }
                pr = 0;

                // handle k,m,g,t
                if (fl & CEXSP__METRIC_SUFFIX) {
                    char idx;
                    idx = 1;
                    if (fl & CEXSP__METRIC_NOSPACE) { idx = 0; }
                    tail[0] = idx;
                    tail[1] = ' ';
                    {
                        if (fl >> 24) { // SI kilo is 'k', JEDEC and SI kibits are 'K'.
                            if (fl & CEXSP__METRIC_1024) {
                                tail[idx + 1] = "_KMGT"[fl >> 24];
                            } else {
                                tail[idx + 1] = "_kMGT"[fl >> 24];
                            }
                            idx++;
                            // If printing kibits and not in jedec, add the 'i'.
                            if (fl & CEXSP__METRIC_1024 && !(fl & CEXSP__METRIC_JEDEC)) {
                                tail[idx + 1] = 'i';
                                idx++;
                            }
                            tail[0] = idx;
                        }
                    }
                };

            flt_lead:
                // get the length that we copied
                l = (u32)(s - (num + 64));
                s = num + 64;
                goto scopy;
#endif

            case 'B': // upper binary
            case 'b': // lower binary
                h = (f[0] == 'B') ? hexu : hex;
                lead[0] = 0;
                if (fl & CEXSP__LEADING_0X) {
                    lead[0] = 2;
                    lead[1] = '0';
                    lead[2] = h[0xb];
                }
                l = (8 << 4) | (1 << 8);
                goto radixnum;

            case 'o': // octal
                h = hexu;
                lead[0] = 0;
                if (fl & CEXSP__LEADING_0X) {
                    lead[0] = 1;
                    lead[1] = '0';
                }
                l = (3 << 4) | (3 << 8);
                goto radixnum;

            case 'p': // pointer
                fl |= (sizeof(void*) == 8) ? CEXSP__INTMAX : 0;
                // pr = sizeof(void*) * 2;
                // fl &= ~CEXSP__LEADINGZERO; // 'p' only prints the pointer with zeros
                fl |= CEXSP__LEADING_0X; // 'p' only prints the pointer with zeros
                fallthrough();

            case 'X': // upper hex
            case 'x': // lower hex
                h = (f[0] == 'X') ? hexu : hex;
                l = (4 << 4) | (4 << 8);
                lead[0] = 0;
                if (fl & CEXSP__LEADING_0X) {
                    lead[0] = 2;
                    lead[1] = '0';
                    lead[2] = h[16];
                }
            radixnum:
                // get the number
                if (fl & CEXSP__INTMAX) {
                    n64 = va_arg(va, u64);
                } else {
                    n64 = va_arg(va, u32);
                }

                s = num + CEXSP__NUMSZ;
                dp = 0;
                // clear tail, and clear leading if value is zero
                tail[0] = 0;
                if (n64 == 0) {
                    // lead[0] = 0;
                    if (pr == 0) {
                        l = 0;
                        cs = 0;
                        goto scopy;
                    }
                }
                // convert to string
                for (;;) {
                    *--s = h[n64 & ((1 << (l >> 8)) - 1)];
                    n64 >>= (l >> 8);
                    if (!((n64) || ((i32)((num + CEXSP__NUMSZ) - s) < pr))) { break; }
                    if (fl & CEXSP__TRIPLET_COMMA) {
                        ++l;
                        if ((l & 15) == ((l >> 4) & 15)) {
                            l &= ~15;
                            *--s = cexsp__comma;
                        }
                    }
                };
                // get the tens and the comma pos
                cs = (u32)((num + CEXSP__NUMSZ) - s) + ((((l >> 4) & 15)) << 24);
                // get the length that we copied
                l = (u32)((num + CEXSP__NUMSZ) - s);
                // copy it
                goto scopy;

            case 'u': // unsigned
            case 'i':
            case 'd': // integer
                // get the integer and abs it
                if (fl & CEXSP__INTMAX) {
                    i64 _i64 = va_arg(va, i64);
                    n64 = (u64)_i64;
                    if ((f[0] != 'u') && (_i64 < 0)) {
                        n64 = (_i64 != INT64_MIN) ? (u64)-_i64 : INT64_MIN;
                        fl |= CEXSP__NEGATIVE;
                    }
                } else {
                    i32 i = va_arg(va, i32);
                    n64 = (u32)i;
                    if ((f[0] != 'u') && (i < 0)) {
                        n64 = (i != INT32_MIN) ? (u32)-i : INT32_MIN;
                        fl |= CEXSP__NEGATIVE;
                    }
                }

#ifndef CEX_SPRINTF_NOFLOAT
                if (fl & CEXSP__METRIC_SUFFIX) {
                    if (n64 < 1024) {
                        pr = 0;
                    } else if (pr == -1) {
                        pr = 1;
                    }
                    fv = (double)(i64)n64;
                    goto doafloat;
                }
#endif

                // convert to string
                s = num + CEXSP__NUMSZ;
                l = 0;

                for (;;) {
                    // do in 32-bit chunks (avoid lots of 64-bit divides even with constant
                    // denominators)
                    char* o = s - 8;
                    if (n64 >= 100000000) {
                        n = (u32)(n64 % 100000000);
                        n64 /= 100000000;
                    } else {
                        n = (u32)n64;
                        n64 = 0;
                    }
                    if ((fl & CEXSP__TRIPLET_COMMA) == 0) {
                        do {
                            s -= 2;
                            *(u16*)s = *(u16*)&cexsp__digitpair.pair[(n % 100) * 2];
                            n /= 100;
                        } while (n);
                    }
                    while (n) {
                        if ((fl & CEXSP__TRIPLET_COMMA) && (l++ == 3)) {
                            l = 0;
                            *--s = cexsp__comma;
                            --o;
                        } else {
                            *--s = (char)(n % 10) + '0';
                            n /= 10;
                        }
                    }
                    if (n64 == 0) {
                        if ((s[0] == '0') && (s != (num + CEXSP__NUMSZ))) { ++s; }
                        break;
                    }
                    while (s != o) {
                        if ((fl & CEXSP__TRIPLET_COMMA) && (l++ == 3)) {
                            l = 0;
                            *--s = cexsp__comma;
                            --o;
                        } else {
                            *--s = '0';
                        }
                    }
                }

                tail[0] = 0;
                cexsp__lead_sign(fl, lead);

                // get the length that we copied
                l = (u32)((num + CEXSP__NUMSZ) - s);
                if (l == 0) {
                    *--s = '0';
                    l = 1;
                }
                cs = l + (3 << 24);
                if (pr < 0) { pr = 0; }

            scopy:
                // get fw=leading/trailing space, pr=leading zeros
                if (pr < (i32)l) { pr = l; }
                n = pr + lead[0] + tail[0] + tz;
                if (fw < (i32)n) { fw = n; }
                fw -= n;
                pr -= l;

                // handle right justify and leading zeros
                if ((fl & CEXSP__LEFTJUST) == 0) {
                    if (fl & CEXSP__LEADINGZERO) // if leading zeros, everything is in pr
                    {
                        pr = (fw > pr) ? fw : pr;
                        fw = 0;
                    } else {
                        fl &= ~CEXSP__TRIPLET_COMMA; // if no leading zeros, then no commas
                    }
                }

                // copy the spaces and/or zeros
                if (fw + pr) {
                    i32 i;
                    u32 c;

                    // copy leading spaces (or when doing %8.4d stuff)
                    if ((fl & CEXSP__LEFTJUST) == 0) {
                        while (fw > 0) {
                            cexsp__cb_buf_clamp(i, fw);
                            fw -= i;
                            while (i) {
                                if ((((usize)bf) & 3) == 0) { break; }
                                *bf++ = ' ';
                                --i;
                            }
                            while (i >= 4) {
                                *(u32*)bf = 0x20202020;
                                bf += 4;
                                i -= 4;
                            }
                            while (i) {
                                *bf++ = ' ';
                                --i;
                            }
                            cexsp__chk_cb_buf(1);
                        }
                    }

                    // copy leader
                    sn = lead + 1;
                    while (lead[0]) {
                        cexsp__cb_buf_clamp(i, lead[0]);
                        lead[0] -= (char)i;
                        while (i) {
                            *bf++ = *sn++; // NOLINT
                            --i;
                        }
                        cexsp__chk_cb_buf(1);
                    }

                    // copy leading zeros
                    c = cs >> 24;
                    cs &= 0xffffff;
                    cs = (fl & CEXSP__TRIPLET_COMMA) ? ((u32)(c - ((pr + cs) % (c + 1)))) : 0;
                    while (pr > 0) {
                        cexsp__cb_buf_clamp(i, pr);
                        pr -= i;
                        if ((fl & CEXSP__TRIPLET_COMMA) == 0) {
                            while (i) {
                                if ((((usize)bf) & 3) == 0) { break; }
                                *bf++ = '0';
                                --i;
                            }
                            while (i >= 4) {
                                *(u32*)bf = 0x30303030;
                                bf += 4;
                                i -= 4;
                            }
                        }
                        while (i) {
                            if ((fl & CEXSP__TRIPLET_COMMA) && (cs++ == c)) {
                                cs = 0;
                                *bf++ = cexsp__comma;
                            } else {
                                *bf++ = '0';
                            }
                            --i;
                        }
                        cexsp__chk_cb_buf(1);
                    }
                }

                // copy leader if there is still one
                sn = lead + 1;
                while (lead[0]) {
                    i32 i;
                    cexsp__cb_buf_clamp(i, lead[0]);
                    lead[0] -= (char)i;
                    while (i) {
                        *bf++ = *sn++;
                        --i;
                    }
                    cexsp__chk_cb_buf(1);
                }

                // copy the string
                n = l;
                while (n) {
                    i32 i;
                    cexsp__cb_buf_clamp(i, n);
                    n -= i;
                    while (i) {
                        *bf++ = *s++; // NOLINT
                        --i;
                    }
                    cexsp__chk_cb_buf(1);
                }

                // copy trailing zeros
                while (tz) {
                    i32 i;
                    cexsp__cb_buf_clamp(i, tz);
                    tz -= i;
                    while (i) {
                        if ((((usize)bf) & 3) == 0) { break; }
                        *bf++ = '0';
                        --i;
                    }
                    while (i >= 4) {
                        *(u32*)bf = 0x30303030;
                        bf += 4;
                        i -= 4;
                    }
                    while (i) {
                        *bf++ = '0';
                        --i;
                    }
                    cexsp__chk_cb_buf(1);
                }

                // copy tail if there is one
                sn = tail + 1;
                while (tail[0]) {
                    i32 i;
                    cexsp__cb_buf_clamp(i, tail[0]);
                    tail[0] -= (char)i;
                    while (i) {
                        *bf++ = *sn++;
                        --i;
                    }
                    cexsp__chk_cb_buf(1);
                }

                // handle the left justify
                if (fl & CEXSP__LEFTJUST) {
                    if (fw > 0) {
                        while (fw) {
                            i32 i;
                            cexsp__cb_buf_clamp(i, fw);
                            fw -= i;
                            while (i) {
                                if ((((usize)bf) & 3) == 0) { break; }
                                *bf++ = ' ';
                                --i;
                            }
                            while (i >= 4) {
                                *(u32*)bf = 0x20202020;
                                bf += 4;
                                i -= 4;
                            }
                            while (i--) { *bf++ = ' '; }
                            cexsp__chk_cb_buf(1);
                        }
                    }
                }
                break;

            default: // unknown, just copy code
                s = num + CEXSP__NUMSZ - 1;
                *s = f[0];
                l = 1;
                fw = fl = 0;
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                goto scopy;
        }
        ++f;
    }
endfmt:

    if (!callback) {
        *bf = 0;
    } else {
        cexsp__flush_cb();
    }

done:
    return tlen + (int)(bf - buf);
}

// cleanup
#undef CEXSP__LEFTJUST
#undef CEXSP__LEADINGPLUS
#undef CEXSP__LEADINGSPACE
#undef CEXSP__LEADING_0X
#undef CEXSP__LEADINGZERO
#undef CEXSP__INTMAX
#undef CEXSP__TRIPLET_COMMA
#undef CEXSP__NEGATIVE
#undef CEXSP__METRIC_SUFFIX
#undef CEXSP__NUMSZ
#undef cexsp__chk_cb_bufL
#undef cexsp__chk_cb_buf
#undef cexsp__flush_cb
#undef cexsp__cb_buf_clamp

// ============================================================================
//   wrapper functions

static char*
cexsp__clamp_callback(char* buf, void* user, u32 len)
{
    cexsp__context* c = (cexsp__context*)user;
    c->length += len;

    if (len > c->capacity) { len = c->capacity; }

    if (len) {
        if (buf != c->buf) {
            const char *s, *se;
            char* d;
            d = c->buf;
            s = buf;
            se = buf + len;
            do {
                *d++ = *s++;
            } while (s < se);
        }
        c->buf += len;
        c->capacity -= len;
    }

    if (c->capacity <= 0) { return c->tmp; }
    return (c->capacity >= CEX_SPRINTF_MIN) ? c->buf : c->tmp; // go direct into buffer if you can
}


CEXSP__PUBLICDEF int
cexsp__vsnprintf(char* buf, int count, char const* fmt, va_list va)
{
    cexsp__context c;

    if (!buf || count <= 0) {
        return -1;
    } else {
        int l;

        c.buf = buf;
        c.capacity = count;
        c.length = 0;

        cexsp__vsprintfcb(cexsp__clamp_callback, &c, cexsp__clamp_callback(0, &c, 0), fmt, va);

        // zero-terminate
        l = (int)(c.buf - buf);
        if (l >= count) { // should never be greater, only equal (or less) than count
            c.length = l;
            l = count - 1;
        }
        buf[l] = 0;
        // assert(c.length <= INT32_MAX);
    }

    return c.length;
}

CEXSP__PUBLICDEF int
cexsp__snprintf(char* buf, int count, char const* fmt, ...)
{
    int result;
    va_list va;
    va_start(va, fmt);

    result = cexsp__vsnprintf(buf, count, fmt, va);
    va_end(va);

    return result;
}

static char*
cexsp__fprintf_callback(char* buf, void* user, u32 len)
{
    cexsp__context* c = (cexsp__context*)user;
    c->length += len;
    if (len) {
        if (fwrite(buf, sizeof(char), len, c->file) != (size_t)len) { c->has_error = 1; }
    }
    return c->tmp;
}

CEXSP__PUBLICDEF int
cexsp__vfprintf(FILE* stream, const char* format, va_list va)
{
    cexsp__context c = { .file = stream, .length = 0 };

    cexsp__vsprintfcb(cexsp__fprintf_callback, &c, cexsp__fprintf_callback(0, &c, 0), format, va);
    // assert(c.length <= INT32_MAX);

    return c.has_error == 0 ? (i32)c.length : -1;
}

CEXSP__PUBLICDEF int
cexsp__fprintf(FILE* stream, const char* format, ...)
{
    int result;
    va_list va;
    va_start(va, format);
    result = cexsp__vfprintf(stream, format, va);
    va_end(va);
    return result;
}

// =======================================================================
//   low level float utility functions

#ifndef CEX_SPRINTF_NOFLOAT

// copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
#    define CEXSP__COPYFP(dest, src)                                                               \
        {                                                                                          \
            int cn;                                                                                \
            for (cn = 0; cn < 8; cn++) ((char*)&dest)[cn] = ((char*)&src)[cn];                     \
        }

// get float info
static i32
cexsp__real_to_parts(i64* bits, i32* expo, double value)
{
    double d;
    i64 b = 0;

    // load value and round at the frac_digits
    d = value;

    CEXSP__COPYFP(b, d);

    *bits = b & ((((u64)1) << 52) - 1);
    *expo = (i32)(((b >> 52) & 2047) - 1023);

    return (i32)((u64)b >> 63);
}

static double const cexsp__bot[23] = { 1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005,
                                       1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
                                       1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017,
                                       1e+018, 1e+019, 1e+020, 1e+021, 1e+022 };
static double const cexsp__negbot[22] = { 1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006,
                                          1e-007, 1e-008, 1e-009, 1e-010, 1e-011, 1e-012,
                                          1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018,
                                          1e-019, 1e-020, 1e-021, 1e-022 };
static double const cexsp__negboterr[22] = {
    -5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020,
    -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
    4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026,
    -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
    -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032,
    2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
    2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,
    -4.8596774326570872e-039
};
static double const cexsp__top[13] = { 1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161,
                                       1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299 };
static double const cexsp__negtop[13] = { 1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161,
                                          1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299 };
static double const cexsp__toperr[13] = { 8388608,
                                          6.8601809640529717e+028,
                                          -7.253143638152921e+052,
                                          -4.3377296974619174e+075,
                                          -1.5559416129466825e+098,
                                          -3.2841562489204913e+121,
                                          -3.7745893248228135e+144,
                                          -1.7356668416969134e+167,
                                          -3.8893577551088374e+190,
                                          -9.9566444326005119e+213,
                                          6.3641293062232429e+236,
                                          -5.2069140800249813e+259,
                                          -5.2504760255204387e+282 };
static double const cexsp__negtoperr[13] = { 3.9565301985100693e-040,  -2.299904345391321e-063,
                                             3.6506201437945798e-086,  1.1875228833981544e-109,
                                             -5.0644902316928607e-132, -6.7156837247865426e-155,
                                             -2.812077463003139e-178,  -5.7778912386589953e-201,
                                             7.4997100559334532e-224,  -4.6439668915134491e-247,
                                             -6.3691100762962136e-270, -9.436808465446358e-293,
                                             8.0970921678014997e-317 };

#    if defined(_MSC_VER) && (_MSC_VER <= 1200)
static u64 const cexsp__powten[20] = { 1,
                                       10,
                                       100,
                                       1000,
                                       10000,
                                       100000,
                                       1000000,
                                       10000000,
                                       100000000,
                                       1000000000,
                                       10000000000,
                                       100000000000,
                                       1000000000000,
                                       10000000000000,
                                       100000000000000,
                                       1000000000000000,
                                       10000000000000000,
                                       100000000000000000,
                                       1000000000000000000,
                                       10000000000000000000U };
#        define cexsp__tento19th ((u64)1000000000000000000)
#    else
static u64 const cexsp__powten[20] = { 1,
                                       10,
                                       100,
                                       1000,
                                       10000,
                                       100000,
                                       1000000,
                                       10000000,
                                       100000000,
                                       1000000000,
                                       10000000000ULL,
                                       100000000000ULL,
                                       1000000000000ULL,
                                       10000000000000ULL,
                                       100000000000000ULL,
                                       1000000000000000ULL,
                                       10000000000000000ULL,
                                       100000000000000000ULL,
                                       1000000000000000000ULL,
                                       10000000000000000000ULL };
#        define cexsp__tento19th (1000000000000000000ULL)
#    endif

#    define cexsp__ddmulthi(oh, ol, xh, yh)                                                        \
        {                                                                                          \
            double ahi = 0, alo, bhi = 0, blo;                                                     \
            i64 bt;                                                                                \
            oh = xh * yh;                                                                          \
            CEXSP__COPYFP(bt, xh);                                                                 \
            bt &= ((~(u64)0) << 27);                                                               \
            CEXSP__COPYFP(ahi, bt);                                                                \
            alo = xh - ahi;                                                                        \
            CEXSP__COPYFP(bt, yh);                                                                 \
            bt &= ((~(u64)0) << 27);                                                               \
            CEXSP__COPYFP(bhi, bt);                                                                \
            blo = yh - bhi;                                                                        \
            ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo;                           \
        }

#    define cexsp__ddtoS64(ob, xh, xl)                                                             \
        {                                                                                          \
            double ahi = 0, alo, vh, t;                                                            \
            ob = (i64)xh;                                                                          \
            vh = (double)ob;                                                                       \
            ahi = (xh - vh);                                                                       \
            t = (ahi - xh);                                                                        \
            alo = (xh - (ahi - t)) - (vh + t);                                                     \
            ob += (i64)(ahi + alo + xl);                                                           \
        }

#    define cexsp__ddrenorm(oh, ol)                                                                \
        {                                                                                          \
            double s;                                                                              \
            s = oh + ol;                                                                           \
            ol = ol - (s - oh);                                                                    \
            oh = s;                                                                                \
        }

#    define cexsp__ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#    define cexsp__ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);

static void
cexsp__raise_to_power10(double* ohi, double* olo, double d, i32 power) // power can be -323
                                                                       // to +350
{
    double ph, pl;
    if ((power >= 0) && (power <= 22)) {
        cexsp__ddmulthi(ph, pl, d, cexsp__bot[power]);
    } else {
        i32 e, et, eb;
        double p2h, p2l;

        e = power;
        if (power < 0) { e = -e; }
        et = (e * 0x2c9) >> 14; /* %23 */
        if (et > 13) { et = 13; }
        eb = e - (et * 23);

        ph = d;
        pl = 0.0;
        if (power < 0) {
            if (eb) {
                --eb;
                cexsp__ddmulthi(ph, pl, d, cexsp__negbot[eb]);
                cexsp__ddmultlos(ph, pl, d, cexsp__negboterr[eb]);
            }
            if (et) {
                cexsp__ddrenorm(ph, pl);
                --et;
                cexsp__ddmulthi(p2h, p2l, ph, cexsp__negtop[et]);
                cexsp__ddmultlo(p2h, p2l, ph, pl, cexsp__negtop[et], cexsp__negtoperr[et]);
                ph = p2h;
                pl = p2l;
            }
        } else {
            if (eb) {
                e = eb;
                if (eb > 22) { eb = 22; }
                e -= eb;
                cexsp__ddmulthi(ph, pl, d, cexsp__bot[eb]);
                if (e) {
                    cexsp__ddrenorm(ph, pl);
                    cexsp__ddmulthi(p2h, p2l, ph, cexsp__bot[e]);
                    cexsp__ddmultlos(p2h, p2l, cexsp__bot[e], pl);
                    ph = p2h;
                    pl = p2l;
                }
            }
            if (et) {
                cexsp__ddrenorm(ph, pl);
                --et;
                cexsp__ddmulthi(p2h, p2l, ph, cexsp__top[et]);
                cexsp__ddmultlo(p2h, p2l, ph, pl, cexsp__top[et], cexsp__toperr[et]);
                ph = p2h;
                pl = p2l;
            }
        }
    }
    cexsp__ddrenorm(ph, pl);
    *ohi = ph;
    *olo = pl;
}

// given a float value, returns the significant bits in bits, and the position of the
//   decimal point in decimal_pos.  +/-INF and NAN are specified by special values
//   returned in the decimal_pos parameter.
// frac_digits is absolute normally, but if you want from first significant digits (got %g and %e),
// or in 0x80000000
static i32
cexsp__real_to_str(
    char const** start,
    u32* len,
    char* out,
    i32* decimal_pos,
    double value,
    u32 frac_digits
)
{
    double d;
    i64 bits = 0;
    i32 expo, e, ng, tens;

    d = value;
    CEXSP__COPYFP(bits, d);
    expo = (i32)((bits >> 52) & 2047);
    ng = (i32)((u64)bits >> 63);
    if (ng) { d = -d; }

    if (expo == 2047) // is nan or inf?
    {
        // CEX: lower case nan/inf
        *start = (bits & ((((u64)1) << 52) - 1)) ? "nan" : "inf";
        *decimal_pos = CEXSP__SPECIAL;
        *len = 3;
        return ng;
    }

    if (expo == 0) // is zero or denormal
    {
        if (((u64)bits << 1) == 0) // do zero
        {
            *decimal_pos = 1;
            *start = out;
            out[0] = '0';
            *len = 1;
            return ng;
        }
        // find the right expo for denormals
        {
            i64 v = ((u64)1) << 51;
            while ((bits & v) == 0) {
                --expo;
                v >>= 1;
            }
        }
    }

    // find the decimal exponent as well as the decimal bits of the value
    {
        double ph, pl;

        // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of
        // log10 of all expos 1..2046
        tens = expo - 1023;
        tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

        // move the significant bits into position and stick them into an int
        cexsp__raise_to_power10(&ph, &pl, d, 18 - tens);

        // get full as much precision from double-double as possible
        cexsp__ddtoS64(bits, ph, pl);

        // check if we undershot
        if (((u64)bits) >= cexsp__tento19th) { ++tens; }
    }

    // now do the rounding in integer land
    frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1)
                                             : (tens + frac_digits);
    if ((frac_digits < 24)) {
        u32 dg = 1;
        if ((u64)bits >= cexsp__powten[9]) { dg = 10; }
        while ((u64)bits >= cexsp__powten[dg]) {
            ++dg;
            if (dg == 20) { goto noround; }
        }
        if (frac_digits < dg) {
            u64 r;
            // add 0.5 at the right position and round
            e = dg - frac_digits;
            if ((u32)e >= 24) { goto noround; }
            r = cexsp__powten[e];
            bits = bits + (r / 2);
            if ((u64)bits >= cexsp__powten[dg]) { ++tens; }
            bits /= r;
        }
    noround:;
    }

    // kill long trailing runs of zeros
    if (bits) {
        u32 n;
        for (;;) {
            if (bits <= 0xffffffff) { break; }
            if (bits % 1000) { goto donez; }
            bits /= 1000;
        }
        n = (u32)bits;
        while ((n % 1000) == 0) { n /= 1000; }
        bits = n;
    donez:;
    }

    // convert to string
    out += 64;
    e = 0;
    for (;;) {
        u32 n;
        char* o = out - 8;
        // do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant
        // denomiators be damned)
        if (bits >= 100000000) {
            n = (u32)(bits % 100000000);
            bits /= 100000000;
        } else {
            n = (u32)bits;
            bits = 0;
        }
        while (n) {
            out -= 2;
            *(u16*)out = *(u16*)&cexsp__digitpair.pair[(n % 100) * 2];
            n /= 100;
            e += 2;
        }
        if (bits == 0) {
            if ((e) && (out[0] == '0')) {
                ++out;
                --e;
            }
            break;
        }
        while (out != o) {
            *--out = '0';
            ++e;
        }
    }

    *decimal_pos = tens;
    *start = out;
    *len = e;
    return ng;
}

#    undef cexsp__ddmulthi
#    undef cexsp__ddrenorm
#    undef cexsp__ddmultlo
#    undef cexsp__ddmultlos
#    undef CEXSP__SPECIAL
#    undef CEXSP__COPYFP

#endif // CEX_SPRINTF_NOFLOAT



/*
*                          src/str.c
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_str)

static inline int _cex_str__toupper(int c) {
    if ((unsigned)c - 'a' < 26) return c & 0x5f;
    return c;
}

static inline int _cex_str__tolower(int c) {
    if ((unsigned)c - 'A' < 26) return c | 0x20;
    return c;
}

static inline bool
_cex_str__isvalid(str_s* s)
{
    return s->buf != NULL;
}

static inline isize
_cex_str__index(str_s* s, char* c, u8 clen)
{
    isize result = -1;

    if (!_cex_str__isvalid(s)) { return -1; }

    u8 split_by_idx[UINT8_MAX] = { 0 };
    for (u8 i = 0; i < clen; i++) { split_by_idx[(u8)c[i]] = 1; }

    for (usize i = 0; i < s->len; i++) {
        if (split_by_idx[(u8)s->buf[i]]) {
            result = i;
            break;
        }
    }

    return result;
}

/// Creates string slice of input C string (NULL tolerant, (str_s){0} on error)
static str_s
cex_str_sstr(char* ccharptr)
{
    if (unlikely(ccharptr == NULL)) { return (str_s){ 0 }; }

    return (str_s){
        .buf = (char*)ccharptr,
        .len = strlen(ccharptr),
    };
}

/// Creates string slice from a buf+len
static str_s
cex_str_sbuf(char* s, usize length)
{
    if (unlikely(s == NULL)) { return (str_s){ 0 }; }

    return (str_s){
        .buf = s,
        .len = strnlen(s, length),
    };
}

/// Compares two null-terminated strings (null tolerant)
static bool
cex_str_eq(char* a, char* b)
{
    if (unlikely(a == NULL || b == NULL)) { return a == b; }
    return strcmp(a, b) == 0;
}

/// Compares two strings, case insensitive, null tolerant
bool
cex_str_eqi(char* a, char* b)
{
    if (unlikely(a == NULL || b == NULL)) { return a == b; }
    while (*a && *b) {
        if (_cex_str__tolower((u8)*a) != _cex_str__tolower((u8)*b)) { return false; }
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

/// Compares two string slices, null tolerant
static bool
cex_str__slice__eq(str_s a, str_s b)
{
    if (a.len != b.len) { return false; }
    return str.slice.qscmp(&a, &b) == 0;
}

/// Compares two string slices, null tolerant, case insensitive
static bool
cex_str__slice__eqi(str_s a, str_s b)
{
    if (a.len != b.len) { return false; }
    return str.slice.qscmpi(&a, &b) == 0;
}

/// Makes slices of `s` slice, start/end are indexes, can be negative from the end, if end=0 mean
/// full length of the string. `s` may be not null-terminated. function is NULL tolerant, return
/// (str_s){0} on error
static str_s
cex_str__slice__sub(str_s s, isize start, isize end)
{
    str_s result = { 0 };

    if (s.buf != NULL) {
        isize _len = s.len;
        if (unlikely(start < 0)) { start += _len; }
        if (end == 0) { /* _end=0 equivalent infinity */
            end = _len;
        } else if (unlikely(end < 0)) {
            end += _len;
        }
        end = end < _len ? end : _len;
        start = start > 0 ? start : 0;

        if (start < end) {
            result.buf = &(s.buf[start]);
            result.len = (usize)(end - start);
        }
    }

    return result;
}

/// Makes slices of `s` char* string, start/end are indexes, can be negative from the end, if end=0
/// mean full length of the string. `s` may be not null-terminated. function is NULL tolerant,
/// return (str_s){0} on error
static str_s
cex_str_sub(char* s, isize start, isize end)
{
    str_s slice = cex_str_sstr(s);
    return cex_str__slice__sub(slice, start, end);
}

/// Makes a copy of initial `src`, into `dest` buffer constrained by `destlen`. NULL tolerant,
/// always null-terminated, overflow checked.
static Exception
cex_str_copy(char* dest, char* src, usize destlen)
{
    uassert(dest != src && "buffers overlap");
    if (unlikely(dest == NULL || destlen == 0)) { return Error.argument; }
    dest[0] = '\0'; // If we fail next, it still writes empty string
    if (unlikely(src == NULL)) { return Error.argument; }

    char* d = dest;
    char* s = src;
    size_t n = destlen;

    while (--n != 0) {
        if (unlikely((*d = *s) == '\0')) { break; }
        d++;
        s++;
    }
    *d = '\0'; // always terminate

    if (unlikely(*s != '\0')) { return Error.overflow; }

    return Error.ok;
}

/// Replaces substring occurrence in a string
char*
cex_str_replace(char* s, char* old_sub, char* new_sub, IAllocator allc)
{
    if (s == NULL || old_sub == NULL || new_sub == NULL || old_sub[0] == '\0') { return NULL; }
    size_t str_len = strlen(s);
    size_t old_sub_len = strlen(old_sub);
    size_t new_sub_len = strlen(new_sub);

    size_t count = 0;
    char* pos = s;
    while ((pos = strstr(pos, old_sub)) != NULL) {
        count++;
        pos += old_sub_len;
    }
    size_t new_str_len = str_len + count * (new_sub_len - old_sub_len);
    char* new_str = (char*)mem$malloc(allc, new_str_len + 1); // +1 for the null terminator
    if (new_str == NULL) {
        return NULL; // Memory allocation failed
    }
    new_str[0] = '\0';

    char* current_pos = new_str;
    char* start = s;
    while (count--) {
        char* found = strstr(start, old_sub);
        size_t segment_len = found - start;
        memcpy(current_pos, start, segment_len);
        current_pos += segment_len;
        memcpy(current_pos, new_sub, new_sub_len);
        current_pos += new_sub_len;
        start = found + old_sub_len;
    }
    // NOLINTNEXTLINE
    strcpy(current_pos, start);
    new_str[new_str_len] = '\0';
    return new_str;
}

/// Makes a copy of initial `src` slice, into `dest` buffer constrained by `destlen`. NULL tolerant,
/// always null-terminated, overflow checked.
static Exception
cex_str__slice__copy(char* dest, str_s src, usize destlen)
{
    uassert(dest != src.buf && "buffers overlap");
    if (unlikely(dest == NULL || destlen == 0)) { return Error.argument; }
    dest[0] = '\0';
    if (unlikely(src.buf == NULL)) { return Error.argument; }
    if (src.len >= destlen) { return Error.overflow; }
    memcpy(dest, src.buf, src.len);
    dest[src.len] = '\0';
    dest[destlen - 1] = '\0';
    return Error.ok;
}

/// Analog of vsprintf() uses CEX sprintf engine. NULL tolerant, overflow safe.
static Exception
cex_str_vsprintf(char* dest, usize dest_len, char* format, va_list va)
{
    if (unlikely(dest == NULL)) { return Error.argument; }
    if (unlikely(dest_len == 0)) { return Error.argument; }
    uassert(format != NULL);

    dest[dest_len - 1] = '\0'; // always null term at capacity

    int result = cexsp__vsnprintf(dest, dest_len, format, va);

    if (result < 0 || (usize)result >= dest_len) {
        // NOTE: even with overflow, data is truncated and written to the dest + null term
        return Error.overflow;
    }

    return EOK;
}

/// Analog of sprintf() uses CEX sprintf engine. NULL tolerant, overflow safe.
static Exc
cex_str_sprintf(char* dest, usize dest_len, char* format, ...)
{
    va_list va;
    va_start(va, format);
    Exc result = cex_str_vsprintf(dest, dest_len, format, va);
    va_end(va);
    return result;
}


/// Calculates string length, NULL tolerant.
static usize
cex_str_len(char* s)
{
    if (s == NULL) { return 0; }
    return strlen(s);
}

/// Find a substring in a string, returns pointer to first element. NULL tolerant, and NULL on err.
static char*
cex_str_find(char* haystack, char* needle)
{
    if (unlikely(haystack == NULL || needle == NULL || needle[0] == '\0')) { return NULL; }
    return strstr(haystack, needle);
}

/// Find substring from the end , NULL tolerant, returns NULL on error.
char*
cex_str_findr(char* haystack, char* needle)
{
    if (unlikely(haystack == NULL || needle == NULL || needle[0] == '\0')) { return NULL; }
    usize haystack_len = strlen(haystack);
    usize needle_len = strlen(needle);
    if (unlikely(needle_len > haystack_len)) { return NULL; }
    for (char* ptr = haystack + haystack_len - needle_len; ptr >= haystack; ptr--) {
        if (unlikely(strncmp(ptr, needle, needle_len) == 0)) {
            uassert(ptr >= haystack);
            uassert(ptr <= haystack + haystack_len);
            return (char*)ptr;
        }
    }
    return NULL;
}

/// Get index of first occurrence of `needle`, returns -1 on error.
static isize
cex_str__slice__index_of(str_s s, str_s needle)
{
    if (unlikely(!s.buf || !needle.buf || needle.len == 0 || needle.len > s.len)) { return -1; }
    if (unlikely(needle.len == 1)) {
        char n = needle.buf[0];
        for (usize i = 0; i < s.len; i++) {
            if (s.buf[i] == n) { return i; }
        }
    } else {
        for (usize i = 0; i <= s.len - needle.len; i++) {
            if (memcmp(&s.buf[i], needle.buf, needle.len) == 0) { return i; }
        }
    }
    return -1;
}

/// Checks if slice starts with prefix, returns (str_s){0} on error, NULL tolerant
static bool
cex_str__slice__starts_with(str_s s, str_s prefix)
{
    if (unlikely(!s.buf || !prefix.buf || prefix.len == 0 || prefix.len > s.len)) { return false; }
    return memcmp(s.buf, prefix.buf, prefix.len) == 0;
}

/// Checks if slice ends with prefix, returns (str_s){0} on error, NULL tolerant
static bool
cex_str__slice__ends_with(str_s s, str_s suffix)
{
    if (unlikely(!s.buf || !suffix.buf || suffix.len == 0 || suffix.len > s.len)) { return false; }
    return s.len >= suffix.len && !memcmp(s.buf + s.len - suffix.len, suffix.buf, suffix.len);
}

/// Checks if string starts with prefix, returns false on error, NULL tolerant
static bool
cex_str_starts_with(char* s, char* prefix)
{
    if (s == NULL || prefix == NULL || prefix[0] == '\0') { return false; }

    while (*prefix && *s == *prefix) { ++s, ++prefix; }
    return *prefix == 0;
}

/// Checks if string ends with prefix, returns false on error, NULL tolerant
static bool
cex_str_ends_with(char* s, char* suffix)
{
    if (s == NULL || suffix == NULL || suffix[0] == '\0') { return false; }
    size_t slen = strlen(s);
    size_t sufflen = strlen(suffix);

    return slen >= sufflen && !memcmp(s + slen - sufflen, suffix, sufflen);
}

/// Replaces slice prefix (start part), or returns the same slice if it's not found
static str_s
cex_str__slice__remove_prefix(str_s s, str_s prefix)
{
    if (!cex_str__slice__starts_with(s, prefix)) { return s; }

    return (str_s){
        .buf = s.buf + prefix.len,
        .len = s.len - prefix.len,
    };
}

/// Replaces slice suffix (end part), or returns the same slice if it's not found
static str_s
cex_str__slice__remove_suffix(str_s s, str_s suffix)
{
    if (!cex_str__slice__ends_with(s, suffix)) { return s; }
    return (str_s){
        .buf = s.buf,
        .len = s.len - suffix.len,
    };
}

static inline void
cex_str__strip_left(str_s* s)
{
    char* cend = s->buf + s->len;

    while (s->buf < cend) {
        switch (*s->buf) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->buf++;
                s->len--;
                break;
            default:
                return;
        }
    }
}

static inline void
cex_str__strip_right(str_s* s)
{
    while (s->len > 0) {
        switch (s->buf[s->len - 1]) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->len--;
                break;
            default:
                return;
        }
    }
}


/// Removes white spaces from the beginning of slice
static str_s
cex_str__slice__lstrip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    cex_str__strip_left(&result);
    return result;
}

/// Removes white spaces from the end of slice
static str_s
cex_str__slice__rstrip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    cex_str__strip_right(&result);
    return result;
}


/// Removes white spaces from both ends of slice
static str_s
cex_str__slice__strip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    cex_str__strip_left(&result);
    cex_str__strip_right(&result);
    return result;
}

/// libc `qsort()` comparator function for alphabetical sorting of str_s arrays
static int
cex_str__slice__qscmp(const void* a, const void* b)
{
    str_s self = *(str_s*)a;
    str_s other = *(str_s*)b;
    if (unlikely(self.buf == NULL || other.buf == NULL)) {
        return (self.buf < other.buf) - (self.buf > other.buf);
    }

    usize min_len = self.len < other.len ? self.len : other.len;
    int cmp = memcmp(self.buf, other.buf, min_len);

    if (unlikely(cmp == 0 && self.len != other.len)) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

/// libc `qsort()` comparator function for alphabetical case insensitive sorting of str_s arrays
static int
cex_str__slice__qscmpi(const void* a, const void* b)
{
    str_s self = *(str_s*)a;
    str_s other = *(str_s*)b;

    if (unlikely(self.buf == NULL || other.buf == NULL)) {
        return (self.buf < other.buf) - (self.buf > other.buf);
    }

    usize min_len = self.len < other.len ? self.len : other.len;

    int cmp = 0;
    char* s = self.buf;
    char* o = other.buf;
    for (usize i = 0; i < min_len; i++) {
        cmp = _cex_str__tolower(*s) - _cex_str__tolower(*o);
        if (cmp != 0) { return cmp; }
        s++;
        o++;
    }

    if (cmp == 0 && self.len != other.len) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

/// iterator over slice splits:  for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {}
static str_s
cex_str__slice__iter_split(str_s s, char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        usize cursor;
        usize split_by_len;
        usize str_len;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    static_assert(alignof(struct iter_ctx) <= alignof(usize), "cex_iterator_s _ctx misalign");

    // NOLINTNEXTLINE
    if (unlikely(!iterator->initialized)) {
        iterator->initialized = 1;
        // First run handling
        if (unlikely(!_cex_str__isvalid(&s) || s.len == 0)) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }
        ctx->split_by_len = strlen(split_by);
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        if (ctx->split_by_len == 0) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }

        isize idx = _cex_str__index(&s, split_by, ctx->split_by_len);
        if (idx < 0) { idx = s.len; }
        ctx->cursor = idx;
        ctx->str_len = s.len; // this prevents s being changed in a loop
        iterator->idx.i = 0;
        if (idx == 0) {
            // first line is \n
            return (str_s){ .buf = "", .len = 0 };
        } else {
            return str.slice.sub(s, 0, idx);
        }
    } else {
        if (unlikely(ctx->cursor >= ctx->str_len)) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == ctx->str_len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            iterator->idx.i++;
            return (str_s){ .buf = "", .len = 0 };
        }

        // Get remaining string after prev split_by char
        str_s tok = str.slice.sub(s, ctx->cursor, 0);
        isize idx = _cex_str__index(&tok, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->cursor = s.len;
            // iterator->stopped = 1;
            return tok;
        } else if (idx == 0) {
            return (str_s){ .buf = "", .len = 0 };
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->cursor += idx;
            return str.slice.sub(tok, 0, idx);
        }
    }
}


static Exception
cex_str__to_signed_num(char* self, usize len, i64* num, i64 num_min, i64 num_max)
{
    static_assert(sizeof(i64) == 8, "unexpected u64 size");
    uassert(num_min < num_max);
    uassert(num_min <= 0);
    uassert(num_max > 0);
    uassert(num_max > 64);
    uassert(num_min == 0 || num_min < -64);
    uassert(num_min >= INT64_MIN + 1 && "try num_min+1, negation overflow");

    if (unlikely(self == NULL)) { return Error.argument; }
    if (unlikely(len > 32)) { return Error.argument; }

    char* s = self;
    if (len == 0) { len = strlen(self); }
    usize i = 0;

    for (; i < len && s[i] == ' '; i++) {}
    if (unlikely(i >= len)) { return Error.argument; }

    u64 neg = 1;
    if (s[i] == '-') {
        neg = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) { return Error.argument; }

    if ((len - i) >= 2 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        i += 2;
        base = 16;
        if (unlikely(i >= len)) {
            // edge case when '0x'
            return Error.argument;
        }
    }

    u64 cutoff = (u64)(neg == 1 ? (u64)num_max : (u64)-num_min);
    u64 cutlim = cutoff % (u64)base;
    cutoff /= (u64)base;

    u64 acc = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'F') {
            c = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 10;
        } else {
            break;
        }

        if (unlikely(c >= base)) { return Error.argument; }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') { return Error.argument; }
    }

    *num = (i64)acc * neg;

    return Error.ok;
}

static Exception
cex_str__to_unsigned_num(char* s, usize len, u64* num, u64 num_max)
{
    static_assert(sizeof(u64) == 8, "unexpected u64 size");
    uassert(num_max > 0);
    uassert(num_max > 64);

    if (unlikely(s == NULL)) { return Error.argument; }

    if (len == 0) { len = strlen(s); }
    if (unlikely(len > 32)) { return Error.argument; }
    usize i = 0;

    for (; i < len && s[i] == ' '; i++) {}
    if (unlikely(i >= len)) { return Error.argument; }


    if (s[i] == '-') {
        return Error.argument;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) { return Error.argument; }

    if ((len - i) >= 2 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        i += 2;
        base = 16;

        if (unlikely(i >= len)) {
            // edge case when '0x'
            return Error.argument;
        }
    }

    u64 cutoff = num_max;
    u64 cutlim = cutoff % (u64)base;
    cutoff /= (u64)base;

    u64 acc = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'F') {
            c = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 10;
        } else {
            break;
        }

        if (unlikely(c >= base)) { return Error.argument; }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') { return Error.argument; }
    }

    *num = (i64)acc;

    return Error.ok;
}

static Exception
cex_str__to_double(char* self, usize len, double* num, i32 exp_min, i32 exp_max)
{
    static_assert(sizeof(double) == 8, "unexpected double precision");
    if (unlikely(self == NULL)) { return Error.argument; }

    char* s = self;
    if (len == 0) { len = strlen(s); }
    if (unlikely(len > 64)) { return Error.argument; }

    usize i = 0;
    double number = 0.0;

    for (; i < len && s[i] == ' '; i++) {}
    if (unlikely(i >= len)) { return Error.argument; }

    double sign = 1;
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }

    if (unlikely(i >= len)) { return Error.argument; }

    if (unlikely(s[i] == 'n' || s[i] == 'i' || s[i] == 'N' || s[i] == 'I')) {
        if (unlikely(len - i < 3)) { return Error.argument; }
        if (s[i] == 'n' || s[i] == 'N') {
            if ((s[i + 1] == 'a' || s[i + 1] == 'A') && (s[i + 2] == 'n' || s[i + 2] == 'N')) {
                number = NAN;
                i += 3;
            }
        } else {
            // s[i] = 'i'
            if ((s[i + 1] == 'n' || s[i + 1] == 'N') && (s[i + 2] == 'f' || s[i + 2] == 'F')) {
                number = (double)INFINITY * sign;
                i += 3;
            }
            // INF 'INITY' part (optional but still valid)
            // clang-format off
            if (unlikely(len - i >= 5)) {
                if ((s[i + 0] == 'i' || s[i + 0] == 'I') && 
                    (s[i + 1] == 'n' || s[i + 1] == 'N') &&
                    (s[i + 2] == 'i' || s[i + 2] == 'I') &&
                    (s[i + 3] == 't' || s[i + 3] == 'T') &&
                    (s[i + 4] == 'y' || s[i + 4] == 'Y')) {
                    i += 5;
                }
            }
            // clang-format on
        }

        // Allow trailing spaces, but no other character allowed
        for (; i < len; i++) {
            if (s[i] != ' ') { return Error.argument; }
        }

        *num = number;
        return Error.ok;
    }

    i32 exponent = 0;
    u32 num_decimals = 0;
    u32 num_digits = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c == 'e' || c == 'E' || c == '.') {
            break;
        } else {
            return Error.argument;
        }

        number = number * 10. + c;
        num_digits++;
    }
    // Process decimal part
    if (i < len && s[i] == '.') {
        i++;

        for (; i < len; i++) {
            u8 c = (u8)s[i];

            if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
                break;
            }
            number = number * 10. + c;
            num_decimals++;
            num_digits++;
        }
        exponent -= num_decimals;
    }


    number *= sign;

    if (i < len - 1 && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        sign = 1;
        if (s[i] == '-') {
            sign = -1;
            i++;
        } else if (s[i] == '+') {
            i++;
        }

        u32 n = 0;
        for (; i < len; i++) {
            u8 c = (u8)s[i];

            if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
                break;
            }

            n = n * 10 + c;
        }
        if (unlikely(n > INT32_MAX)) { return Error.overflow; }

        exponent += n * sign;
    }

    if (num_digits == 0) { return Error.argument; }

    if (unlikely(exponent < exp_min || exponent > exp_max)) { return Error.overflow; }

    // Scale the result
    double p10 = 10.;
    i32 n = exponent;
    if (n < 0) { n = -n; }
    while (n) {
        if (n & 1) {
            if (exponent < 0) {
                number /= p10;
            } else {
                number *= p10;
            }
        }
        n >>= 1;
        p10 *= p10;
    }

    if (number == HUGE_VAL) { return Error.overflow; }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') { return Error.argument; }
    }

    *num = number;

    return Error.ok;
}

static Exception
cex_str__convert__to_f32s(str_s s, f32* num)
{
    uassert(num != NULL);
    f64 res = 0;
    Exc r = cex_str__to_double(s.buf, s.len, &res, -37, 38);
    *num = (f32)res;
    return r;
}

static Exception
cex_str__convert__to_f64s(str_s s, f64* num)
{
    uassert(num != NULL);
    return cex_str__to_double(s.buf, s.len, num, -307, 308);
}

static Exception
cex_str__convert__to_i8s(str_s s, i8* num)
{
    uassert(num != NULL);
    i64 res = 0;
    Exc r = cex_str__to_signed_num(s.buf, s.len, &res, INT8_MIN, INT8_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_i16s(str_s s, i16* num)
{
    uassert(num != NULL);
    i64 res = 0;
    auto r = cex_str__to_signed_num(s.buf, s.len, &res, INT16_MIN, INT16_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_i32s(str_s s, i32* num)
{
    uassert(num != NULL);
    i64 res = 0;
    auto r = cex_str__to_signed_num(s.buf, s.len, &res, INT32_MIN, INT32_MAX);
    *num = res;
    return r;
}


static Exception
cex_str__convert__to_i64s(str_s s, i64* num)
{
    uassert(num != NULL);
    i64 res = 0;
    // NOTE:INT64_MIN+1 because negating of INT64_MIN leads to UB!
    auto r = cex_str__to_signed_num(s.buf, s.len, &res, INT64_MIN + 1, INT64_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_u8s(str_s s, u8* num)
{
    uassert(num != NULL);
    u64 res = 0;
    Exc r = cex_str__to_unsigned_num(s.buf, s.len, &res, UINT8_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_u16s(str_s s, u16* num)
{
    uassert(num != NULL);
    u64 res = 0;
    Exc r = cex_str__to_unsigned_num(s.buf, s.len, &res, UINT16_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_u32s(str_s s, u32* num)
{
    uassert(num != NULL);
    u64 res = 0;
    Exc r = cex_str__to_unsigned_num(s.buf, s.len, &res, UINT32_MAX);
    *num = res;
    return r;
}

static Exception
cex_str__convert__to_u64s(str_s s, u64* num)
{
    uassert(num != NULL);
    u64 res = 0;
    Exc r = cex_str__to_unsigned_num(s.buf, s.len, &res, UINT64_MAX);
    *num = res;

    return r;
}

static Exception
cex_str__convert__to_f32(char* s, f32* num)
{
    return cex_str__convert__to_f32s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_f64(char* s, f64* num)
{
    return cex_str__convert__to_f64s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_i8(char* s, i8* num)
{
    return cex_str__convert__to_i8s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_i16(char* s, i16* num)
{
    return cex_str__convert__to_i16s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_i32(char* s, i32* num)
{
    return cex_str__convert__to_i32s(str.sstr(s), num);
}


static Exception
cex_str__convert__to_i64(char* s, i64* num)
{
    return cex_str__convert__to_i64s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_u8(char* s, u8* num)
{
    return cex_str__convert__to_u8s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_u16(char* s, u16* num)
{
    return cex_str__convert__to_u16s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_u32(char* s, u32* num)
{
    return cex_str__convert__to_u32s(str.sstr(s), num);
}

static Exception
cex_str__convert__to_u64(char* s, u64* num)
{
    return cex_str__convert__to_u64s(str.sstr(s), num);
}


static char*
_cex_str__fmt_callback(char* buf, void* user, u32 len)
{
    (void)buf;
    cexsp__context* ctx = user;
    if (unlikely(ctx->has_error)) { return NULL; }

    if (unlikely(
            len >= CEX_SPRINTF_MIN && (ctx->buf == NULL || ctx->length + len >= ctx->capacity)
        )) {

        if (len > INT32_MAX || ctx->length + len > INT32_MAX) {
            ctx->has_error = true;
            return NULL;
        }

        uassert(ctx->allc != NULL);

        if (ctx->buf == NULL) {
            ctx->buf = mem$calloc(ctx->allc, 1, CEX_SPRINTF_MIN * 2);
            if (ctx->buf == NULL) {
                ctx->has_error = true;
                return NULL;
            }
            ctx->capacity = CEX_SPRINTF_MIN * 2;
        } else {
            ctx->buf = mem$realloc(ctx->allc, ctx->buf, ctx->capacity + CEX_SPRINTF_MIN * 2);
            if (ctx->buf == NULL) {
                ctx->has_error = true;
                return NULL;
            }
            ctx->capacity += CEX_SPRINTF_MIN * 2;
        }
    }
    ctx->length += len;

    // fprintf(stderr, "len: %d, total_len: %d capacity: %d\n", len, ctx->length, ctx->capacity);
    if (len > 0) {
        if (ctx->buf) {
            if (buf == ctx->tmp) {
                uassert(ctx->length <= CEX_SPRINTF_MIN);
                memcpy(ctx->buf, buf, len);
            }
        }
    }


    return (ctx->buf != NULL) ? &ctx->buf[ctx->length] : ctx->tmp;
}

/// Formats string and allocates it dynamically using allocator, supports CEX format engine
static char*
cex_str_fmt(IAllocator allc, char* format, ...)
{
    va_list va;
    va_start(va, format);

    cexsp__context ctx = {
        .allc = allc,
    };
    // TODO: add optional flag, to check if any va is null?
    cexsp__vsprintfcb(_cex_str__fmt_callback, &ctx, ctx.tmp, format, va);
    va_end(va);

    if (unlikely(ctx.has_error)) {
        mem$free(allc, ctx.buf);
        return NULL;
    }

    if (ctx.buf) {
        uassert(ctx.length < ctx.capacity);
        ctx.buf[ctx.length] = '\0';
        ctx.buf[ctx.capacity - 1] = '\0';
    } else {
        uassert(ctx.length <= arr$len(ctx.tmp) - 1);
        ctx.buf = mem$malloc(allc, ctx.length + 1);
        if (ctx.buf == NULL) { return NULL; }
        memcpy(ctx.buf, ctx.tmp, ctx.length);
        ctx.buf[ctx.length] = '\0';
    }
    return ctx.buf;
}

/// Clone slice into new char* allocated by `allc`, null tolerant, returns NULL on error.
static char*
cex_str__slice__clone(str_s s, IAllocator allc)
{
    if (s.buf == NULL) { return NULL; }
    char* result = mem$malloc(allc, s.len + 1);
    if (result) {
        memcpy(result, s.buf, s.len);
        result[s.len] = '\0';
    }
    return result;
}

/// Clones string using allocator, null tolerant, returns NULL on error.
static char*
cex_str_clone(char* s, IAllocator allc)
{
    if (s == NULL) { return NULL; }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(allc, slen + 1);
    if (result) {
        memcpy(result, s, slen);
        result[slen] = '\0';
    }
    return result;
}

/// Returns new lower case string, returns NULL on error, null tolerant
static char*
cex_str_lower(char* s, IAllocator allc)
{
    if (s == NULL) { return NULL; }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(allc, slen + 1);
    if (result) {
        for (usize i = 0; i < slen; i++) { result[i] = _cex_str__tolower(s[i]); }
        result[slen] = '\0';
    }
    return result;
}

/// Returns new upper case string, returns NULL on error, null tolerant
static char*
cex_str_upper(char* s, IAllocator allc)
{
    if (s == NULL) { return NULL; }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(allc, slen + 1);
    if (result) {
        for (usize i = 0; i < slen; i++) { result[i] = _cex_str__toupper(s[i]); }
        result[slen] = '\0';
    }
    return result;
}

/// Splits string using split_by (allows many) chars, returns new dynamic array of split char*
/// tokens, allocates memory with allc, returns NULL on error. NULL tolerant. Items of array are
/// cloned, so you need free them independently or better use arena or tmem$.
static arr$(char*) cex_str_split(char* s, char* split_by, IAllocator allc)
{
    str_s src = cex_str_sstr(s);
    if (src.buf == NULL || split_by == NULL) { return NULL; }
    arr$(char*) result = arr$new(result, allc);
    if (result == NULL) { return NULL; }

    for$iter (str_s, it, cex_str__slice__iter_split(src, split_by, &it.iterator)) {
        char* tok = cex_str__slice__clone(it.val, allc);
        arr$push(result, tok);
    }

    return result;
}

/// Splits string by lines, result allocated by allc, as dynamic array of cloned lines, Returns NULL
/// on error, NULL tolerant. Items of array are cloned, so you need free them independently or
/// better use arena or tmem$. Supports \n or \r\n.
static arr$(char*) cex_str_split_lines(char* s, IAllocator allc)
{
    uassert(allc != NULL);
    if (s == NULL) { return NULL; }
    arr$(char*) result = arr$new(result, allc);
    if (result == NULL) { return NULL; }
    char c;
    char* line_start = s;
    char* cur = s;
    while ((c = *cur)) {
        switch (c) {
            case '\r':
                if (cur[1] == '\n') { goto default_next; }
                fallthrough();
            case '\n':
            case '\v':
            case '\f': {
                str_s line = { .buf = (char*)line_start, .len = cur - line_start };
                if (line.len > 0 && line.buf[line.len - 1] == '\r') { line.len--; }
                char* tok = cex_str__slice__clone(line, allc);
                arr$push(result, tok);
                line_start = cur + 1;
            }
                fallthrough();
            default:
            default_next:
                cur++;
        }
    }
    if (line_start <= cur) {
        str_s line = { .buf = (char*)line_start, .len = cur - line_start };
        if (line.len > 0){
        char* tok = cex_str__slice__clone(line, allc);
        arr$push(result, tok);
        }
    }
    return result;
}

/// Joins string using a separator (join_by), NULL tolerant, returns NULL on error.
static char*
cex_str_join(char** str_arr, usize str_arr_len, char* join_by, IAllocator allc)
{
    if (str_arr == NULL || join_by == NULL) { return NULL; }

    usize jlen = strlen(join_by);
    if (jlen == 0) { return NULL; }

    char* result = NULL;
    usize cursor = 0;
    for$each (s, str_arr, str_arr_len) {
        if (s == NULL) {
            if (result != NULL) { mem$free(allc, result); }
            return NULL;
        }
        usize slen = strlen(s);
        if (result == NULL) {
            result = mem$malloc(allc, slen + jlen + 1);
        } else {
            result = mem$realloc(allc, result, cursor + slen + jlen + 1);
        }
        if (result == NULL) {
            return NULL; // memory error
        }

        if (cursor > 0) {
            memcpy(&result[cursor], join_by, jlen);
            cursor += jlen;
        }

        memcpy(&result[cursor], s, slen);
        cursor += slen;
    }
    if (result) { result[cursor] = '\0'; }

    return result;
}


static bool
_cex_str_match(char* str, isize str_len, char* pattern)
{
    if (unlikely(str == NULL || str_len <= 0)) { return false; }
    uassert(pattern && "null pattern");

    while (*pattern != '\0') {
        switch (*pattern) {
            case '*':
                while (*pattern == '*' || *pattern == '?') {
                    if (unlikely(str_len > 0 && *pattern == '?')) {
                        str++;
                        str_len--;
                    }
                    pattern++;
                }

                if (!*pattern) { return true; }

                if (*pattern != '?' && *pattern != '[' && *pattern != '(' && *pattern != '\\') {
                    while (str_len > 0 && *pattern != *str) {
                        str++;
                        str_len--;
                    }
                }

                while (str_len > 0) {
                    if (_cex_str_match(str, str_len, pattern)) { return true; }
                    str++;
                    str_len--;
                }
                return false;

            case '?':
                if (str_len == 0) {
                    return false; // '?' requires a character
                }
                str++;
                str_len--;
                pattern++;
                break;

            case '(': {
                char* strstart = str;
                isize str_len_start = str_len;
                if (unlikely(*(pattern + 1) == ')')) {
                    uassert(false && "Empty '()' group");
                    return false;
                }
                if (unlikely(str_len_start) == 0) { return false; }

                while (str_len_start > 0) {
                    pattern++;
                    str = strstart;
                    str_len = str_len_start;
                    bool matched = false;
                    while (*pattern != '\0') {
                        if (unlikely(*pattern == '\\')) {
                            pattern++;
                            if (unlikely(*pattern == '\0')) {
                                uassert(false && "Unterminated \\ sequence inside '()' group");
                                return false;
                            }
                        }
                        if (str_len > 0 && *pattern == *str) {
                            matched = true;
                        } else {
                            while (*pattern != '|' && *pattern != ')' && *pattern != '\0') {
                                matched = false;
                                pattern++;
                            }
                            break;
                        }
                        pattern++;
                        str++;
                        str_len--;
                    }
                    if (*pattern == '|') {
                        if (!matched) { continue; }
                        // we have found the match, just skip to the end of group
                        while (*pattern != ')' && *pattern != '\0') { pattern++; }
                    }

                    if (unlikely(*pattern != ')')) {
                        uassert(false && "Invalid pattern - no closing ')'");
                        return false;
                    }

                    pattern++;
                    if (!matched) {
                        return false;
                    } else {
                        // All good find next pattern
                        break; // while (*str != '\0') {
                    }
                }
                break;
            }
            case '[': {
                char* pstart = pattern;
                bool has_previous_match = false;
                while (str_len > 0) {
                    bool negate = false;
                    bool repeating = false;
                    pattern = pstart + 1;

                    if (unlikely(*pattern == '!')) {
                        uassert(*(pattern + 1) != ']' && "expected some chars after [!..]");
                        negate = true;
                        pattern++;
                    }

                    bool matched = false;
                    while (*pattern != ']' && *pattern != '\0') {
                        if (*(pattern + 1) == '-' && *(pattern + 2) != ']' &&
                            *(pattern + 2) != '\0') {
                            // Handle character ranges like a-zA-Z0-9
                            uassertf(
                                *pattern < *(pattern + 2),
                                "pattern [n-m] sequence, n must be less than m: [%c-%c]",
                                *pattern,
                                *(pattern + 2)
                            );
                            if (*str >= *pattern && *str <= *(pattern + 2)) { matched = true; }
                            pattern += 3;
                        } else if (*pattern == '\\') {
                            // Escape sequence
                            pattern++;
                            if (*pattern != '\0') {
                                if (*pattern == *str) { matched = true; }
                                pattern++;
                            }
                        } else {
                            if (unlikely(*pattern == '+' && *(pattern + 1) == ']')) {
                                // repeating group [a-z+]@, match all cases until @
                                repeating = true;
                            } else {
                                if (*pattern == *str) { matched = true; }
                            }
                            pattern++;
                        }
                    }

                    if (unlikely(*pattern != ']')) {
                        uassert(false && "Invalid pattern - no closing ']'");
                        return false;
                    } else {
                        pattern++;

                        if (matched == negate) {
                            if (repeating && has_previous_match) {
                                // We have not matched char, but it may match to next pattern
                                break;
                            }
                            return false;
                        }

                        str++;
                        str_len--;
                        has_previous_match = true;
                        if (!repeating) {
                            break; // while (*str != '\0') {
                        }
                    }
                }

                if (str_len == 0) {
                    // str end reached, pattern also must be at end (null-term)
                    return *pattern == '\0';
                }
                break;
            }

            case '\\':
                // Escape next character
                pattern++;
                if (*pattern == '\0') { return false; }
                fallthrough();

            default:
                if (*pattern && str_len == 0) { return false; }
                if (*pattern != *str) { return false; }
                str++;
                str_len--;
                pattern++;
        }
    }

    return str_len == 0;
}

/// Slice pattern matching check (see ./cex help str$ for examples)
static bool
cex_str__slice__match(str_s s, char* pattern)
{
    return _cex_str_match(s.buf, s.len, pattern);
}

/// String pattern matching check (see ./cex help str$ for examples)
static bool
cex_str_match(char* s, char* pattern)
{
    return _cex_str_match(s, str.len(s), pattern);
}

/// libc `qsort()` comparator functions, for arrays of `char*`, sorting alphabetical
static int
cex_str_qscmp(const void* a, const void* b)
{
    const char* _a = *(const char**)a;
    const char* _b = *(const char**)b;

    if (_a == NULL || _b == NULL) { return (_a < _b) - (_a > _b); }
    return strcmp(_a, _b);
}

/// libc `qsort()` comparator functions, for arrays of `char*`, sorting alphabetical case insensitive
static int
cex_str_qscmpi(const void* a, const void* b)
{
    const char* _a = *(const char**)a;
    const char* _b = *(const char**)b;

    if (_a == NULL || _b == NULL) { return (_a < _b) - (_a > _b); }

    while (*_a && *_b) {
        int diff = _cex_str__tolower((unsigned char)*_a) - _cex_str__tolower((unsigned char)*_b);
        if (diff != 0) { return diff; }
        _a++;
        _b++;
    }
    return _cex_str__tolower((unsigned char)*_a) - _cex_str__tolower((unsigned char)*_b);
}

const struct __cex_namespace__str str = {
    // Autogenerated by CEX
    // clang-format off

    .clone = cex_str_clone,
    .copy = cex_str_copy,
    .ends_with = cex_str_ends_with,
    .eq = cex_str_eq,
    .eqi = cex_str_eqi,
    .find = cex_str_find,
    .findr = cex_str_findr,
    .fmt = cex_str_fmt,
    .join = cex_str_join,
    .len = cex_str_len,
    .lower = cex_str_lower,
    .match = cex_str_match,
    .qscmp = cex_str_qscmp,
    .qscmpi = cex_str_qscmpi,
    .replace = cex_str_replace,
    .sbuf = cex_str_sbuf,
    .split = cex_str_split,
    .split_lines = cex_str_split_lines,
    .sprintf = cex_str_sprintf,
    .sstr = cex_str_sstr,
    .starts_with = cex_str_starts_with,
    .sub = cex_str_sub,
    .upper = cex_str_upper,
    .vsprintf = cex_str_vsprintf,

    .convert = {
        .to_f32 = cex_str__convert__to_f32,
        .to_f32s = cex_str__convert__to_f32s,
        .to_f64 = cex_str__convert__to_f64,
        .to_f64s = cex_str__convert__to_f64s,
        .to_i16 = cex_str__convert__to_i16,
        .to_i16s = cex_str__convert__to_i16s,
        .to_i32 = cex_str__convert__to_i32,
        .to_i32s = cex_str__convert__to_i32s,
        .to_i64 = cex_str__convert__to_i64,
        .to_i64s = cex_str__convert__to_i64s,
        .to_i8 = cex_str__convert__to_i8,
        .to_i8s = cex_str__convert__to_i8s,
        .to_u16 = cex_str__convert__to_u16,
        .to_u16s = cex_str__convert__to_u16s,
        .to_u32 = cex_str__convert__to_u32,
        .to_u32s = cex_str__convert__to_u32s,
        .to_u64 = cex_str__convert__to_u64,
        .to_u64s = cex_str__convert__to_u64s,
        .to_u8 = cex_str__convert__to_u8,
        .to_u8s = cex_str__convert__to_u8s,
    },

    .slice = {
        .clone = cex_str__slice__clone,
        .copy = cex_str__slice__copy,
        .ends_with = cex_str__slice__ends_with,
        .eq = cex_str__slice__eq,
        .eqi = cex_str__slice__eqi,
        .index_of = cex_str__slice__index_of,
        .iter_split = cex_str__slice__iter_split,
        .lstrip = cex_str__slice__lstrip,
        .match = cex_str__slice__match,
        .qscmp = cex_str__slice__qscmp,
        .qscmpi = cex_str__slice__qscmpi,
        .remove_prefix = cex_str__slice__remove_prefix,
        .remove_suffix = cex_str__slice__remove_suffix,
        .rstrip = cex_str__slice__rstrip,
        .starts_with = cex_str__slice__starts_with,
        .strip = cex_str__slice__strip,
        .sub = cex_str__slice__sub,
    },

    // clang-format on
};
#endif



/*
*                          src/sbuf.c
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_str)
#include <stdarg.h>

struct _sbuf__sprintf_ctx
{
    sbuf_head_s* head;
    char* buf;
    Exc err;
    usize count;
    usize length;
    char tmp[CEX_SPRINTF_MIN];
};

static inline sbuf_head_s*
_sbuf__head(sbuf_c self)
{
    if (unlikely(!self)) { return NULL; }
    sbuf_head_s* head = (sbuf_head_s*)(self - sizeof(sbuf_head_s));

    uassert(head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");
    uassert(head->length <= head->capacity && "count > capacity");
    uassert(head->header.nullterm == 0 && "nullterm != 0");

    return head;
}

static inline void
_sbuf__set_error(sbuf_head_s* head, Exc err)
{
    if (!head) { return; }
    if (head->err) { return; }

    head->err = err;
    head->length = 0;
    head->capacity = 0;

    // nullterm string contents
    *((char*)head + sizeof(sbuf_head_s)) = '\0';
}

static inline usize
_sbuf__alloc_capacity(usize capacity)
{
    uassert(capacity < INT32_MAX && "requested capacity exceeds 2gb, maybe overflow?");

    capacity += sizeof(sbuf_head_s) + 1; // also +1 for nullterm

    if (capacity >= 512) {
        return capacity * 1.2;
    } else {
        // Round up to closest pow*2 int
        u64 p = 64;
        while (p < capacity) { p *= 2; }
        return p;
    }
}
static inline Exception
_sbuf__grow_buffer(sbuf_c* self, u32 length)
{
    sbuf_head_s* head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));

    if (unlikely(head->allocator == NULL)) {
        // sbuf is static, bad luck, overflow
        _sbuf__set_error(head, Error.overflow);
        return Error.overflow;
    }

    u32 new_capacity = _sbuf__alloc_capacity(length);
    head = mem$realloc(head->allocator, head, new_capacity);
    if (unlikely(head == NULL)) {
        *self = NULL;
        return Error.memory;
    }

    head->capacity = new_capacity - sizeof(sbuf_head_s) - 1,
    *self = (char*)head + sizeof(sbuf_head_s);
    (*self)[head->capacity] = '\0';
    return Error.ok;
}

/// Creates new dynamic string builder backed by allocator
static sbuf_c
cex_sbuf_create(usize capacity, IAllocator allocator)
{
    if (unlikely(allocator == NULL)) {
        uassert(allocator != NULL);
        return NULL;
    }

    if (capacity < 512) { capacity = _sbuf__alloc_capacity(capacity); }

    char* buf = mem$malloc(allocator, capacity);
    if (unlikely(buf == NULL)) { return NULL; }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = capacity - sizeof(sbuf_head_s) - 1,
        .allocator = allocator,
    };

    sbuf_c self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (self)[0] = '\0';
    (self)[head->capacity] = '\0';

    return self;
}


/// Creates dynamic string backed by static array
static sbuf_c
cex_sbuf_create_static(char* buf, usize buf_size)
{
    if (unlikely(buf == NULL)) {
        uassert(buf != NULL);
        return NULL;
    }
    if (unlikely(buf_size == 0)) {
        uassert(buf_size > 0);
        return NULL;
    }
    if (unlikely(buf_size <= sizeof(sbuf_head_s) + 1)) {
        uassert(buf_size > sizeof(sbuf_head_s) + 1);
        return NULL;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = buf_size - sizeof(sbuf_head_s) - 1,
        .allocator = NULL,
    };
    sbuf_c self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (self)[0] = '\0';
    (self)[head->capacity] = '\0';

    return self;
}



/// Shrinks string length to new_length (fails when new_length > existing length)
static Exc
cex_sbuf_shrink(sbuf_c* self, usize new_length)
{
    uassert(self != NULL);
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(!head)) { return Error.runtime; }
    if (unlikely(head->err)) { return head->err; }

    if (unlikely(new_length > head->length)) {
        _sbuf__set_error(head, Error.argument);
        return Error.argument;
    }

    head->length = new_length;
    (*self)[head->length] = '\0';
    return EOK;
}

/// Clears string
static void
cex_sbuf_clear(sbuf_c* self)
{
    cex_sbuf_shrink(self, 0);
}

/// Returns string length from its metadata
static u32
cex_sbuf_len(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(head == NULL)) { return 0; }
    return head->length;
}


/// Returns string capacity from its metadata
static u32
cex_sbuf_capacity(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(head == NULL)) { return 0; }
    return head->capacity;
}

/// Destroys the string, deallocates the memory, or nullify static buffer.
static sbuf_c
cex_sbuf_destroy(sbuf_c* self)
{
    uassert(self != NULL);

    sbuf_head_s* head = _sbuf__head(*self);
    if (head != NULL) {

        // NOTE: null-terminate string to avoid future usage,
        // it will appear as empty string if references anywhere else
        ((char*)head)[0] = '\0';
        (*self)[0] = '\0';
        *self = NULL;

        if (head->allocator != NULL) {
            // allocator is NULL for static sbuf
            mem$free(head->allocator, head);
        } else {
            // static buffer
            memset(self, 0, sizeof(*self));
        }
    }
    return NULL;
}

static char*
_cex_sbuf_sprintf_callback(char* buf, void* user, u32 len)
{
    struct _sbuf__sprintf_ctx* ctx = (struct _sbuf__sprintf_ctx*)user;
    sbuf_c sbuf = ((char*)ctx->head + sizeof(sbuf_head_s));

    uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");
    if (unlikely(ctx->err != EOK)) { return NULL; }
    uassert((buf != ctx->buf) || (sbuf + ctx->length + len <= sbuf + ctx->count && "out of bounds"));

    if (unlikely(ctx->length + len > ctx->count)) {
        bool buf_is_tmp = buf != ctx->buf;

        if (len > INT32_MAX || ctx->length + len > (u32)INT32_MAX) {
            ctx->err = Error.integrity;
            return NULL;
        }

        // sbuf likely changed after realloc
        e$except_silent (err, _sbuf__grow_buffer(&sbuf, ctx->length + len + 1)) {
            ctx->err = err;
            return NULL;
        }
        // re-fetch head in case of realloc
        ctx->head = (sbuf_head_s*)(sbuf - sizeof(sbuf_head_s));
        uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");

        ctx->buf = sbuf + ctx->head->length;
        ctx->count = ctx->head->capacity;
        uassert(ctx->count >= ctx->length);
        if (!buf_is_tmp) { buf = ctx->buf; }
    }

    ctx->length += len;
    ctx->head->length += len;
    if (len > 0) {
        if (buf != ctx->buf) {
            memcpy(ctx->buf, buf, len); // copy data only if previously tmp buffer used
        }
        ctx->buf += len;
    }
    return ((ctx->count - ctx->length) >= CEX_SPRINTF_MIN) ? ctx->buf : ctx->tmp;
}

/// Append format va (using CEX formatting engine), always null-terminating
static Exc
cex_sbuf_appendfva(sbuf_c* self, char* format, va_list va)
{
    if (unlikely(self == NULL)) { return Error.argument; }
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(head == NULL)) { return Error.runtime; }
    if (unlikely(head->err)) { return head->err; }

    struct _sbuf__sprintf_ctx ctx = {
        .head = head,
        .err = EOK,
        .buf = *self + head->length,
        .length = head->length,
        .count = head->capacity,
    };

    cexsp__vsprintfcb(
        _cex_sbuf_sprintf_callback,
        &ctx,
        _cex_sbuf_sprintf_callback(NULL, &ctx, 0),
        format,
        va
    );

    // re-fetch self in case of realloc in sbuf__sprintf_callback
    *self = ((char*)ctx.head + sizeof(sbuf_head_s));

    // always null terminate
    (*self)[ctx.head->length] = '\0';
    (*self)[ctx.head->capacity] = '\0';

    return ctx.err;
}


/// Append format (using CEX formatting engine)
static Exc
cex_sbuf_appendf(sbuf_c* self, char* format, ...)
{

    va_list va;
    va_start(va, format);
    Exc result = cex_sbuf_appendfva(self, format, va);
    va_end(va);
    return result;
}

/// Append string to the builder
static Exc
cex_sbuf_append(sbuf_c* self, char* s)
{
    uassert(self != NULL);
    sbuf_head_s* head = _sbuf__head(*self);
    if (unlikely(head == NULL)) { return Error.runtime; }

    if (unlikely(s == NULL)) {
        _sbuf__set_error(head, "sbuf.append s=NULL");
        return Error.argument;
    }
    if (head->err) { return head->err; }

    u32 length = head->length;
    u32 capacity = head->capacity;
    u32 slen = strlen(s);

    uassert(*self != s && "buffer overlap");

    // Try resize
    if (length + slen > capacity - 1) {
        e$except_silent (err, _sbuf__grow_buffer(self, length + slen)) { return err; }
    }
    // NOLINTNEXTLINE
    memcpy((*self + length), s, slen);
    length += slen;

    // always null terminate
    (*self)[length] = '\0';

    // re-fetch head in case of realloc
    head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
    head->length = length;

    return Error.ok;
}

/// Validate dynamic string state, with detailed Exception 
static Exception
cex_sbuf_validate(sbuf_c* self)
{
    if (unlikely(self == NULL)) { return "NULL argument"; }
    if (unlikely(*self == NULL)) { return "Memory error or already free'd"; }

    sbuf_head_s* head = (sbuf_head_s*)((char*)(*self) - sizeof(sbuf_head_s));

    if (unlikely(head->err)) { return head->err; }
    if (unlikely(head->header.magic != 0xf00e)) { return "Bad magic or non sbuf_c* pointer type"; }
    if (unlikely(head->capacity == 0)) { return "Zero capacity"; }
    if (head->length > head->capacity) { return "Length > capacity"; }
    if (head->header.nullterm != 0) { return "Missing null term in header"; }

    return EOK;
}


/// Returns false if string invalid
static bool
cex_sbuf_isvalid(sbuf_c* self)
{
    if (cex_sbuf_validate(self)) {
        return false;
    } else {
        return true;
    }
}

const struct __cex_namespace__sbuf sbuf = {
    // Autogenerated by CEX
    // clang-format off

    .append = cex_sbuf_append,
    .appendf = cex_sbuf_appendf,
    .appendfva = cex_sbuf_appendfva,
    .capacity = cex_sbuf_capacity,
    .clear = cex_sbuf_clear,
    .create = cex_sbuf_create,
    .create_static = cex_sbuf_create_static,
    .destroy = cex_sbuf_destroy,
    .isvalid = cex_sbuf_isvalid,
    .len = cex_sbuf_len,
    .shrink = cex_sbuf_shrink,
    .validate = cex_sbuf_validate,

    // clang-format on
};
#endif



/*
*                          src/io.c
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_io)
#include <errno.h>
#include <stdio.h>

#ifdef _WIN32
#    include <io.h>
#    include <sys/stat.h>
#    include <windows.h>
#elif cex$is_freestanding
// does not support unistd.h
#else
#    include <sys/stat.h>
#    include <unistd.h>
#endif

/// Opens new file: io.fopen(&file, "file.txt", "r+")
Exception
cex_io_fopen(FILE** file, char* filename, char* mode)
{
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }

    *file = NULL;

    if (filename == NULL) { return Error.argument; }
    if (mode == NULL) {
        uassert(mode != NULL);
        return Error.argument;
    }

    *file = fopen(filename, mode);
    if (*file == NULL) {
        switch (errno) {
            case ENOENT:
                return Error.not_found;
            default:
                return strerror(errno);
        }
    }
    return Error.ok;
}


/// Check if current file supports ANSI colors and in interactive terminal mode
bool
cex_io_isatty(FILE* file)
{
    uassert(file != NULL);

    if (unlikely(file == NULL)) { return false; }
#ifdef _WIN32
    return _isatty(_fileno(file));
#elif cex$is_freestanding
    return false;
#else
    return isatty(fileno(file));
#endif
}

/// Flush changes to file
Exception
cex_io_fflush(FILE* file)
{
    uassert(file != NULL);

    int ret = fflush(file);
    if (unlikely(ret == -1)) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

/// Seek file position
Exception
cex_io_fseek(FILE* file, long offset, int whence)
{
    uassert(file != NULL);

    int ret = fseek(file, offset, whence);
    if (unlikely(ret == -1)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
    } else {
        return Error.ok;
    }
}

/// Rewind file cursor at the beginning
void
cex_io_rewind(FILE* file)
{
    uassert(file != NULL);
    rewind(file);
}

/// Returns current cursor position into `size` pointer
Exception
cex_io_ftell(FILE* file, usize* size)
{
    uassert(file != NULL);

    long ret = ftell(file);
    if (unlikely(ret < 0)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
        *size = 0;
    } else {
        *size = ret;
        return Error.ok;
    }
}

/// Return full file size, always 0 for NULL file or atty
usize
cex_io__file__size(FILE* file)
{
    if (unlikely(file == NULL)) { return 0; }
    if (unlikely(io.isatty(file))) { return 0; }
#ifdef _WIN32
    struct _stat win_stat;
    int ret = _fstat(fileno(file), &win_stat);
    if (ret == 0) {
        return win_stat.st_size;
    } else {
        return 0;
    }
#elif cex$is_freestanding
    return 0;
#else
    struct stat stat;
    int ret = fstat(fileno(file), &stat);
    if (ret == 0) {
        return stat.st_size;
    } else {
        return 0;
    }
#endif
}

/// Read file contents into the buf, return nbytes read (can be < buff_len), 0 on EOF, negative on
/// error (you may use os.get_last_error() for getting Exception for error, cross-platform )
isize
cex_io_fread(FILE* file, void* buff, usize buff_len)
{
    uassert(file != NULL);
    uassert(buff != NULL);
    uassert(buff_len < PTRDIFF_MAX && "Must fit to isize max");

    if (unlikely(buff_len >= PTRDIFF_MAX)) {
        return -1; // hard protecting even in production
    }

    usize ret_count = fread(buff, 1, buff_len, file);

    if (ret_count != buff_len) {
        if (ferror(file)) {
            // NOTE: use os.get_last_error() to get platform specific error
            return -1;
        }
    }

    uassert(ret_count < PTRDIFF_MAX);
    return ret_count;
}

/// Read all contents of the file, using allocator. You should free `s.buf` after.
Exception
cex_io_fread_all(FILE* file, str_s* s, IAllocator allc)
{
    Exc result = Error.runtime;
    usize buf_size = 0;
    char* buf = NULL;


    if (file == NULL) {
        result = Error.argument;
        goto fail;
    }
    uassert(s != NULL);
    uassert(allc != NULL);

    // Forbid console and stdin
    if (unlikely(cex_io_isatty(file))) {
        result = "io.fread_all() not allowed for pipe/socket/std[in/out/err]";
        goto fail;
    }

    usize _fsize = cex_io__file__size(file);
    if (unlikely(_fsize > INT32_MAX)) {
        result = "io.fread_all() file is too big.";
        goto fail;
    }
    bool is_stream = false;
    if (unlikely(_fsize == 0)) {
        if (feof(file)) {
            result = EOK; // return just empty data
            goto fail;
        } else {
            is_stream = true;
        }
    }
    buf_size = (is_stream ? 4096 : _fsize) + 1 + 15;
    buf = mem$malloc(allc, buf_size);

    if (unlikely(buf == NULL)) {
        buf_size = 0;
        result = Error.memory;
        goto fail;
    }

    size_t total_read = 0;
    while (1) {
        if (total_read == buf_size) {
            if (unlikely(total_read > INT32_MAX)) {
                result = "io.fread_all() stream output is too big.";
                goto fail;
            }
            if (buf_size > 100 * 1024 * 1024) {
                buf_size *= 1.2;
            } else {
                buf_size *= 2;
            }
            char* new_buf = mem$realloc(allc, buf, buf_size);
            if (!new_buf) {
                result = Error.memory;
                goto fail;
            }
            buf = new_buf;
        }

        // Read data into the buf
        size_t bytes_read = fread(buf + total_read, 1, buf_size - total_read, file);
        if (bytes_read == 0) {
            if (feof(file)) {
                break;
            } else if (ferror(file)) {
                result = Error.io;
                goto fail;
            }
        }
        total_read += bytes_read;
    }
    char* final_buffer = mem$realloc(allc, buf, total_read + 1);
    if (!final_buffer) {
        result = Error.memory;
        goto fail;
    }
    buf = final_buffer;

    *s = (str_s){
        .buf = buf,
        .len = total_read,
    };
    buf[total_read] = '\0';
    return EOK;

fail:
    *s = (str_s){ .buf = NULL, .len = 0 };
    if (cex_io_fseek(file, 0, SEEK_END)) {}
    if (buf) { mem$free(allc, buf); }
    return result;
}

/// Reads line from a file into str_s buffer, allocates memory. You should free `s.buf` after.
Exception
cex_io_fread_line(FILE* file, str_s* s, IAllocator allc)
{
    Exc result = Error.runtime;
    usize cursor = 0;
    char* buf = NULL;
    usize buf_size = 0;

    if (unlikely(file == NULL)) {
        result = Error.argument;
        goto fail;
    }
    uassert(s != NULL);

    int c = EOF;
    while ((c = fgetc(file)) != EOF) {
        if (unlikely(c == '\n')) {
            // Handle windows \r\n new lines also
            if (cursor > 0 && buf[cursor - 1] == '\r') { cursor--; }
            break;
        }
        if (unlikely(c == '\0')) {
            // plain text file should not have any zero bytes in there
            result = Error.integrity;
            goto fail;
        }

        if (unlikely(cursor >= buf_size)) {
            if (buf == NULL) {
                uassert(cursor == 0 && "no buf, cursor expected 0");

                buf = mem$malloc(allc, 256);
                if (buf == NULL) {
                    result = Error.memory;
                    goto fail;
                }
                buf_size = 256 - 1; // keep extra for null
                buf[buf_size] = '\0';
                buf[cursor] = '\0';
            } else {
                uassert(cursor > 0 && "no buf, cursor expected 0");
                uassert(buf_size > 0 && "empty buffer, weird");

                if (buf_size + 1 < 256) {
                    // Cap minimal buf size
                    buf_size = 256 - 1;
                }

                // Grow initial size by factor of 2
                buf = mem$realloc(allc, buf, (buf_size + 1) * 2);
                if (buf == NULL) {
                    buf_size = 0;
                    result = Error.memory;
                    goto fail;
                }
                buf_size = (buf_size + 1) * 2 - 1;
                buf[buf_size] = '\0';
            }
        }
        buf[cursor] = c;
        cursor++;
    }


    if (unlikely(ferror(file))) {
        result = Error.io;
        goto fail;
    }

    if (unlikely(cursor == 0 && feof(file))) {
        result = Error.eof;
        goto fail;
    }

    if (buf != NULL) {
        buf[cursor] = '\0';
    } else {
        buf = mem$malloc(allc, 1);
        buf[0] = '\0';
        cursor = 0;
    }
    *s = (str_s){
        .buf = buf,
        .len = cursor,
    };
    return Error.ok;

fail:
    if (buf) { mem$free(allc, buf); }
    *s = (str_s){
        .buf = NULL,
        .len = 0,
    };
    return result;
}

/// Prints formatted string to the file. Uses CEX printf() engine with special formatting.
Exc
cex_io_fprintf(FILE* stream, char* format, ...)
{
    uassert(stream != NULL);

    va_list va;
    va_start(va, format);
    int result = cexsp__vfprintf(stream, format, va);
    va_end(va);

    if (result == -1) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

/// Prints formatted string to stdout. Uses CEX printf() engine with special formatting.
int
cex_io_printf(char* format, ...)
{
    va_list va;
    va_start(va, format);
    int result = cexsp__vfprintf(stdout, format, va);
    va_end(va);
    return result;
}

/// Writes bytes to the file
Exception
cex_io_fwrite(FILE* file, void* buff, usize buff_len)
{
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }

    if (buff == NULL) { return Error.argument; }
    if (buff_len == 0) { return Error.argument; }

    usize ret_count = fwrite(buff, 1, buff_len, file);

    if (ret_count != buff_len) {
        // return os.get_last_error();
        switch (errno) {
            case 0:
                uassert(errno != 0 && "errno is ok");
                return "Error, but errno is not set";
            case ENOENT:
                return Error.not_found;
            case EPERM:
                return Error.permission;
            case EIO:
                return Error.io;
            case EAGAIN:
                return Error.try_again;
            default:
                return strerror(errno);
        }
    } else {
        return Error.ok;
    }
}

/// Writes new line to the file
Exception
cex_io__file__writeln(FILE* file, char* line)
{
    errno = 0;
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }
    if (line == NULL) { return Error.argument; }
    usize line_len = strlen(line);
    usize ret_count = fwrite(line, 1, line_len, file);
    if (ret_count != line_len) { return Error.io; }

    char new_line[] = { '\n' };
    ret_count = fwrite(new_line, 1, sizeof(new_line), file);

    if (ret_count != sizeof(new_line)) { return Error.io; }
    return Error.ok;
}

/// Closes file and set it to NULL.
void
cex_io_fclose(FILE** file)
{
    uassert(file != NULL);

    if (*file != NULL) { fclose(*file); }
    *file = NULL;
}


/// Saves full `contents` in the file at `path`, using text mode. 
Exception
cex_io__file__save(char* path, char* contents)
{
    if (path == NULL) { return Error.argument; }

    if (contents == NULL) { return Error.argument; }

    FILE* file;
    // NOLINTNEXTLINE
    e$except_silent (err, cex_io_fopen(&file, path, "w")) { return err; }

    usize contents_len = strlen(contents);
    if (contents_len > 0) {
        e$except_silent (err, cex_io_fwrite(file, contents, contents_len)) {
            cex_io_fclose(&file);
            return err;
        }
    }

    cex_io_fclose(&file);
    return EOK;
}

/// Load full contents of the file at `path`, using text mode. Returns NULL on error.
char*
cex_io__file__load(char* path, IAllocator allc)
{
    errno = 0;
    uassert(allc != NULL);
    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    FILE* file;
    // NOLINTNEXTLINE
    e$except_silent (err, cex_io_fopen(&file, path, "r")) { return NULL; }

    str_s out_content = (str_s){ 0 };
    e$except_silent (err, cex_io_fread_all(file, &out_content, allc)) {
        if (err == Error.eof) {
            uassert(out_content.buf == NULL);
            out_content.buf = mem$malloc(allc, 1);
            if (out_content.buf) { out_content.buf[0] = '\0'; }
            goto end;
        }
        if (out_content.buf) { mem$free(allc, out_content.buf); }
        goto end;
    }
end:
    cex_io_fclose(&file);

    return out_content.buf;
}

/// Reads line from file, allocates result. Returns NULL on error.
char*
cex_io__file__readln(FILE* file, IAllocator allc)
{
    errno = 0;
    uassert(allc != NULL);
    if (file == NULL) {
        errno = EINVAL;
        return NULL;
    }

    str_s out_content = (str_s){ 0 };
    Exc err = cex_io_fread_line(file, &out_content, allc);
    if (err) {
        if (err == Error.eof) { return NULL; }
        if (out_content.buf) { mem$free(allc, out_content.buf); }
        return NULL;
    }
    return out_content.buf;
}

const struct __cex_namespace__io io = {
    // Autogenerated by CEX
    // clang-format off

    .fclose = cex_io_fclose,
    .fflush = cex_io_fflush,
    .fopen = cex_io_fopen,
    .fprintf = cex_io_fprintf,
    .fread = cex_io_fread,
    .fread_all = cex_io_fread_all,
    .fread_line = cex_io_fread_line,
    .fseek = cex_io_fseek,
    .ftell = cex_io_ftell,
    .fwrite = cex_io_fwrite,
    .isatty = cex_io_isatty,
    .printf = cex_io_printf,
    .rewind = cex_io_rewind,

    .file = {
        .load = cex_io__file__load,
        .readln = cex_io__file__readln,
        .save = cex_io__file__save,
        .size = cex_io__file__size,
        .writeln = cex_io__file__writeln,
    },

    // clang-format on
};

#endif



/*
*                          src/argparse.c
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_os)
#include <math.h>

static char*
_cex_argparse__prefix_skip(char* str, char* prefix)
{
    usize len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static Exception
_cex_argparse__error(argparse_c* self, argparse_opt_s* opt, char* reason, bool is_long)
{
    (void)self;
    if (is_long) {
        cexsp__fprintf(stdout, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        cexsp__fprintf(stdout, "error: option `-%c` %s\n", opt->short_name, reason);
    }

    return Error.argument;
}

static void
cex_argparse_usage(argparse_c* self)
{
    uassert(self->argv != NULL && "usage before parse!");

    io.printf("Usage:\n");
    if (self->usage) {

        for$iter (str_s, it, str.slice.iter_split(str.sstr(self->usage), "\n", &it.iterator)) {
            if (it.val.len == 0) { break; }

            char* fn = str.findr(self->program_name, "/");

            if (fn != NULL) {
                io.printf("%s ", fn + 1);
            } else {
                io.printf("%s ", self->program_name);
            }

            if (io.fwrite(stdout, it.val.buf, it.val.len)) { /* error ignored */ }

            io.printf("\n");
        }
    } else {
        if (self->commands) {
            io.printf("%s {", self->program_name);
            for$eachp(c, self->commands, self->commands_len)
            {
                isize i = c - self->commands;
                if (i == 0) {
                    io.printf("%s", c->name);
                } else {
                    io.printf(",%s", c->name);
                }
            }
            io.printf("} [cmd_options] [cmd_args]\n");

        } else {
            io.printf("%s [options] [--] [arg1 argN]\n", self->program_name);
        }
    }

    // print description
    if (self->description) { io.printf("%s\n", self->description); }

    io.printf("\n");


    // figure out best width
    usize usage_opts_width = 0;
    usize len = 0;
    for$eachp(opt, self->options, self->options_len)
    {
        len = 0;
        if (opt->short_name) { len += 2; }
        if (opt->short_name && opt->long_name) {
            len += 2; // separator ", "
        }
        if (opt->long_name) { len += strlen(opt->long_name) + 2; }
        switch (opt->type) {
            case CexArgParseType__boolean:
                break;
            case CexArgParseType__string:
                break;
            case CexArgParseType__i8:
            case CexArgParseType__u8:
            case CexArgParseType__i16:
            case CexArgParseType__u16:
            case CexArgParseType__i32:
            case CexArgParseType__u32:
            case CexArgParseType__i64:
            case CexArgParseType__u64:
            case CexArgParseType__f32:
            case CexArgParseType__f64:
                len += strlen("=<xxx>");
                break;

            default:
                break;
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) { usage_opts_width = len; }
    }
    usage_opts_width += 4; // 4 spaces prefix

    for$eachp(opt, self->options, self->options_len)
    {
        usize pos = 0;
        usize pad = 0;
        if (opt->type == CexArgParseType__group) {
            io.printf("\n%s\n", opt->help);
            continue;
        }
        pos = io.printf("    ");
        if (opt->short_name) { pos += io.printf("-%c", opt->short_name); }
        if (opt->long_name && opt->short_name) { pos += io.printf(", "); }
        if (opt->long_name) { pos += io.printf("--%s", opt->long_name); }

        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            io.printf("\n");
            pad = usage_opts_width;
        }
        if (!str.find(opt->help, "\n")) {
            io.printf("%*s%s", (int)pad + 2, "", opt->help);
        } else {
            u32 i = 0;
            for$iter (str_s, it, str.slice.iter_split(str.sstr(opt->help), "\n", &it.iterator)) {
                str_s clean = str.slice.strip(it.val);
                if (clean.len == 0) { continue; }
                if (i > 0) { io.printf("\n"); }
                io.printf("%*s%S", (i == 0) ? pad + 2 : usage_opts_width + 2, "", clean);
                i++;
            }
        }

        if (!opt->required && opt->value) {
            io.printf(" (default: ");
            switch (opt->type) {
                case CexArgParseType__boolean:
                    io.printf("%c", *(bool*)opt->value ? 'Y' : 'N');
                    break;
                case CexArgParseType__string:
                    if (*(char**)opt->value != NULL) {
                        io.printf("'%s'", *(char**)opt->value);
                    } else {
                        io.printf("''");
                    }
                    break;
                case CexArgParseType__i8:
                    io.printf("%d", *(i8*)opt->value);
                    break;
                case CexArgParseType__i16:
                    io.printf("%d", *(i16*)opt->value);
                    break;
                case CexArgParseType__i32:
                    io.printf("%d", *(i32*)opt->value);
                    break;
                case CexArgParseType__u8:
                    io.printf("%u", *(u8*)opt->value);
                    break;
                case CexArgParseType__u16:
                    io.printf("%u", *(u16*)opt->value);
                    break;
                case CexArgParseType__u32:
                    io.printf("%u", *(u32*)opt->value);
                    break;
                case CexArgParseType__i64:
                    io.printf("%ld", *(i64*)opt->value);
                    break;
                case CexArgParseType__u64:
                    io.printf("%lu", *(u64*)opt->value);
                    break;
                case CexArgParseType__f32:
                    io.printf("%g", *(f32*)opt->value);
                    break;
                case CexArgParseType__f64:
                    io.printf("%g", *(f64*)opt->value);
                    break;
                default:
                    break;
            }
            io.printf(")");
        }
        io.printf("\n");
    }

    for$eachp(cmd, self->commands, self->commands_len)
    {
        io.printf("%-20s%s%s\n", cmd->name, cmd->help, cmd->is_default ? " (default)" : "");
    }

    // print epilog
    if (self->epilog) { io.printf("%s\n", self->epilog); }
}
__attribute__((no_sanitize("undefined"))) static inline Exception
_cex_argparse__convert(char* s, argparse_opt_s* opt)
{
    // NOTE: this hits UBSAN because we casting convert function of
    // (char*, void*) into str.convert.to_u32(char*, u32*)
    // however we do explicit type checking and tagging so it should be good!
    return opt->convert(s, opt->value);
}

static Exception
_cex_argparse__getvalue(argparse_c* self, argparse_opt_s* opt, bool is_long)
{
    if (!opt->value) { goto skipped; }

    switch (opt->type) {
        case CexArgParseType__boolean:
            *(bool*)opt->value = !(*(bool*)opt->value);
            opt->is_present = true;
            break;
        case CexArgParseType__string:
            if (self->_ctx.optvalue) {
                *(char**)opt->value = self->_ctx.optvalue;
                self->_ctx.optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                self->_ctx.cpidx++;
                *(char**)opt->value = *++self->argv;
            } else {
                return _cex_argparse__error(self, opt, "requires a value", is_long);
            }
            opt->is_present = true;
            break;
        case CexArgParseType__i8:
        case CexArgParseType__u8:
        case CexArgParseType__i16:
        case CexArgParseType__u16:
        case CexArgParseType__i32:
        case CexArgParseType__u32:
        case CexArgParseType__i64:
        case CexArgParseType__u64:
        case CexArgParseType__f32:
        case CexArgParseType__f64:
            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return _cex_argparse__error(self, opt, "requires a value", is_long);
                }
                uassert(opt->convert != NULL);
                e$except_silent (err, _cex_argparse__convert(self->_ctx.optvalue, opt)) {
                    return _cex_argparse__error(self, opt, "argument parsing error", is_long);
                }
                self->_ctx.optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                self->_ctx.cpidx++;
                self->argv++;
                e$except_silent (err, _cex_argparse__convert(*self->argv, opt)) {
                    return _cex_argparse__error(self, opt, "argument parsing error", is_long);
                }
            } else {
                return _cex_argparse__error(self, opt, "requires a value", is_long);
            }
            if (opt->type == CexArgParseType__f32) {
                f32 res = *(f32*)opt->value;
                if (__builtin_isnan(res) || res == INFINITY || res == -INFINITY) {
                    return _cex_argparse__error(
                        self,
                        opt,
                        "argument parsing error (float out of range)",
                        is_long
                    );
                }
            } else if (opt->type == CexArgParseType__f64) {
                f64 res = *(f64*)opt->value;
                if (__builtin_isnan(res) || res == INFINITY || res == -INFINITY) {
                    return _cex_argparse__error(
                        self,
                        opt,
                        "argument parsing error (float out of range)",
                        is_long
                    );
                }
            }
            opt->is_present = true;
            break;
        default:
            uassert(false && "unhandled");
            return Error.runtime;
    }

skipped:
    if (opt->callback) {
        opt->is_present = true;
        return opt->callback(self, opt, opt->callback_data);
    } else {
        if (opt->short_name == 'h') {
            cex_argparse_usage(self);
            return Error.argsparse;
        }
    }

    return Error.ok;
}

static Exception
_cex_argparse__options_check(argparse_c* self, bool reset)
{
    for$eachp(opt, self->options, self->options_len)
    {
        if (opt->type != CexArgParseType__group) {
            if (reset) {
                opt->is_present = 0;
                if (!(opt->short_name || opt->long_name)) {
                    uassert(
                        (opt->short_name || opt->long_name) && "options both long/short_name NULL"
                    );
                    return Error.argument;
                }
                if (opt->value == NULL && opt->short_name != 'h') {
                    uassertf(
                        opt->value != NULL,
                        "option value [%c/%s] is null\n",
                        opt->short_name,
                        opt->long_name
                    );
                    return Error.argument;
                }
            } else {
                if (opt->required && !opt->is_present) {
                    cexsp__fprintf(
                        stdout,
                        "Error: missing required option: -%c/--%s\n",
                        opt->short_name,
                        opt->long_name
                    );
                    return Error.argsparse;
                }
            }
        }

        switch (opt->type) {
            case CexArgParseType__group:
                continue;
            case CexArgParseType__boolean:
            case CexArgParseType__string: {
                uassert(opt->convert == NULL && "unexpected to be set for strings/bools");
                uassert(opt->callback == NULL && "unexpected to be set for strings/bools");
                break;
            }
            case CexArgParseType__i8:
            case CexArgParseType__u8:
            case CexArgParseType__i16:
            case CexArgParseType__u16:
            case CexArgParseType__i32:
            case CexArgParseType__u32:
            case CexArgParseType__i64:
            case CexArgParseType__u64:
            case CexArgParseType__f32:
            case CexArgParseType__f64:
                uassert(opt->convert != NULL && "expected to be set for standard args");
                uassert(opt->callback == NULL && "unexpected to be set for standard args");
                continue;
            case CexArgParseType__generic:
                uassert(opt->convert == NULL && "unexpected to be set for generic args");
                uassert(opt->callback != NULL && "expected to be set for generic args");
                continue;
            default:
                uassertf(false, "wrong option type: %d", opt->type);
        }
    }

    return Error.ok;
}

static Exception
_cex_argparse__short_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        if (options->short_name == *self->_ctx.optvalue) {
            self->_ctx.optvalue = self->_ctx.optvalue[1] ? self->_ctx.optvalue + 1 : NULL;
            return _cex_argparse__getvalue(self, options, false);
        }
    }
    return Error.not_found;
}

static Exception
_cex_argparse__long_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        char* rest;
        if (!options->long_name) { continue; }
        rest = _cex_argparse__prefix_skip(self->argv[0] + 2, options->long_name);
        if (!rest) {
            if (options->type != CexArgParseType__boolean) { continue; }
            rest = _cex_argparse__prefix_skip(self->argv[0] + 2 + 3, options->long_name);
            if (!rest) { continue; }
        }
        if (*rest) {
            if (*rest != '=') { continue; }
            self->_ctx.optvalue = rest + 1;
        }
        return _cex_argparse__getvalue(self, options, true);
    }
    return Error.not_found;
}


static Exception
_cex_argparse__report_error(argparse_c* self, Exc err)
{
    // invalidate argc
    self->argc = 0;

    if (err == Error.not_found) {
        if (self->options != NULL) {
            io.printf("error: unknown option `%s`\n", self->argv[0]);
        } else {
            cexsp__fprintf(
                stdout,
                "error: command name expected, got option `%s`, try --help\n",
                self->argv[0]
            );
        }
    } else if (err == Error.integrity) {
        io.printf("error: option `%s` follows argument\n", self->argv[0]);
    }
    return Error.argsparse;
}

static Exception
_cex_argparse__parse_commands(argparse_c* self)
{
    uassert(self->_ctx.current_command == NULL);
    if (self->commands_len == 0) {
        argparse_cmd_s* _cmd = self->commands;
        while (_cmd != NULL) {
            if (_cmd->name == NULL) { break; }
            self->commands_len++;
            _cmd++;
        }
    }

    argparse_cmd_s* cmd = NULL;
    char* cmd_arg = (self->argc > 0) ? self->argv[0] : NULL;

    if (str.eq(cmd_arg, "-h") || str.eq(cmd_arg, "--help")) {
        cex_argparse_usage(self);
        return Error.argsparse;
    }

    for$eachp(c, self->commands, self->commands_len)
    {
        uassert(c->func != NULL && "missing command funcion");
        uassert(c->help != NULL && "missing command help message");
        if (cmd_arg != NULL) {
            if (str.eq(cmd_arg, c->name)) {
                cmd = c;
                break;
            }
        } else {
            if (c->is_default) {
                uassert(cmd == NULL && "multiple default commands in argparse_c");
                cmd = c;
            }
        }
    }
    if (cmd == NULL) {
        cex_argparse_usage(self);
        io.printf("error: unknown command name '%s', try --help\n", (cmd_arg) ? cmd_arg : "");
        return Error.argsparse;
    }
    self->_ctx.current_command = cmd;
    self->_ctx.cpidx = 0;

    return EOK;
}

static Exception
_cex_argparse__parse_options(argparse_c* self)
{
    if (self->options_len == 0) {
        argparse_opt_s* _opt = self->options;
        while (_opt != NULL) {
            if (_opt->type == CexArgParseType__na) { break; }
            self->options_len++;
            _opt++;
        }
    }
    int initial_argc = self->argc + 1;
    e$except_silent (err, _cex_argparse__options_check(self, true)) { return err; }

    for (; self->argc; self->argc--, self->argv++) {
        char* arg = self->argv[0];
        if (arg[0] != '-' || !arg[1]) {
            self->_ctx.has_argument = true;

            if (self->commands != 0) {
                self->argc--;
                self->argv++;
                break;
            }
            continue;
        }
        // short option
        if (arg[1] != '-') {
            if (self->_ctx.has_argument) {
                // Breaking when first argument appears (more flexible support of subcommands)
                break;
            }

            self->_ctx.optvalue = arg + 1;
            self->_ctx.cpidx++;
            e$except_silent (err, _cex_argparse__short_opt(self, self->options)) {
                return _cex_argparse__report_error(self, err);
            }
            while (self->_ctx.optvalue) {
                e$except_silent (err, _cex_argparse__short_opt(self, self->options)) {
                    return _cex_argparse__report_error(self, err);
                }
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            self->argc--;
            self->argv++;
            self->_ctx.cpidx++;
            break;
        }
        // long option
        if (self->_ctx.has_argument) {
            // Breaking when first argument appears (more flexible support of subcommands)
            break;
        }
        e$except_silent (err, _cex_argparse__long_opt(self, self->options)) {
            return _cex_argparse__report_error(self, err);
        }
        self->_ctx.cpidx++;
        continue;
    }

    e$except_silent (err, _cex_argparse__options_check(self, false)) { return err; }

    self->argv = self->_ctx.out + self->_ctx.cpidx + 1; // excludes 1st argv[0], program_name
    self->argc = initial_argc - self->_ctx.cpidx - 1;
    self->_ctx.cpidx = 0;

    return EOK;
}

static Exception
cex_argparse_parse(argparse_c* self, int argc, char** argv)
{
    if (self->options != NULL && self->commands != NULL) {
        uassert(false && "options and commands are mutually exclusive");
        return Error.integrity;
    }
    uassert(argc > 0);
    uassert(argv != NULL);
    uassert(argv[0] != NULL);

    if (self->program_name == NULL) { self->program_name = argv[0]; }

    // reset if we have several runs
    memset(&self->_ctx, 0, sizeof(self->_ctx));

    self->argc = argc - 1;
    self->argv = argv + 1;
    self->_ctx.out = argv;

    if (self->commands) {
        return _cex_argparse__parse_commands(self);
    } else if (self->options) {
        return _cex_argparse__parse_options(self);
    }
    return Error.ok;
}

static char*
cex_argparse_next(argparse_c* self)
{
    uassert(self != NULL);
    uassert(self->argv != NULL && "forgot argparse.parse() call?");

    auto result = self->argv[0];
    if (self->argc > 0) {

        if (self->_ctx.cpidx > 0) {
            // we have --opt=foo, return 'foo' part
            result = self->argv[0] + self->_ctx.cpidx + 1;
            self->_ctx.cpidx = 0;
        } else {
            if (str.starts_with(result, "--")) {
                char* eq = str.find(result, "=");
                if (eq) {
                    static char part_arg[128]; // temp buffer sustained after scope exit
                    self->_ctx.cpidx = eq - result;
                    if ((usize)self->_ctx.cpidx + 1 >= sizeof(part_arg)) { return NULL; }
                    if (str.copy(part_arg, result, sizeof(part_arg))) { return NULL; }
                    part_arg[self->_ctx.cpidx] = '\0';
                    return part_arg;
                }
            }
        }
        self->argc--;
        self->argv++;
    }

    if (unlikely(self->argc == 0)) {
        // After reaching argc=0, argv getting stack-overflowed (ASAN issues), we set to fake NULL
        static char* null_argv[] = { NULL };
        // reset NULL every call, because static null_argv may be overwritten in user code maybe
        null_argv[0] = NULL;
        self->argv = null_argv;
    }
    return result;
}

static Exception
cex_argparse_run_command(argparse_c* self, void* user_ctx)
{
    uassert(self->_ctx.current_command != NULL && "not parsed/parse error?");
    if (self->argc == 0) {
        // seems default command (with no args)
        char* dummy_args[] = { self->_ctx.current_command->name };
        return self->_ctx.current_command->func(1, (char**)dummy_args, user_ctx);
    } else {
        return self->_ctx.current_command->func(self->argc, (char**)self->argv, user_ctx);
    }
}


const struct __cex_namespace__argparse argparse = {
    // Autogenerated by CEX
    // clang-format off

    .next = cex_argparse_next,
    .parse = cex_argparse_parse,
    .run_command = cex_argparse_run_command,
    .usage = cex_argparse_usage,

    // clang-format on
};

#endif



/*
*                          src/_subprocess.c
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_os)


#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
#endif

#if defined(_WIN32)
subprocess_weak int subprocess_create_named_pipe_helper(void **rd, void **wr);
int subprocess_create_named_pipe_helper(void **rd, void **wr) {
  const unsigned long pipeAccessInbound = 0x00000001;
  const unsigned long fileFlagOverlapped = 0x40000000;
  const unsigned long pipeTypeByte = 0x00000000;
  const unsigned long pipeWait = 0x00000000;
  const unsigned long genericWrite = 0x40000000;
  const unsigned long openExisting = 3;
  const unsigned long fileAttributeNormal = 0x00000080;
  const void *const invalidHandleValue =
      SUBPROCESS_PTR_CAST(void *, ~(SUBPROCESS_CAST(subprocess_intptr_t, 0)));
  struct subprocess_security_attributes_s saAttr = {sizeof(saAttr),
                                                    SUBPROCESS_NULL, 1};
  char name[256] = {0};
  static subprocess_tls long index = 0;
  const long unique = index++;

#if defined(_MSC_VER) && _MSC_VER < 1900
#pragma warning(push, 1)
#pragma warning(disable : 4996)
  _snprintf(name, sizeof(name) - 1,
            "\\\\.\\pipe\\sheredom_subprocess_h.%08lx.%08lx.%ld",
            GetCurrentProcessId(), GetCurrentThreadId(), unique);
#pragma warning(pop)
#else
  snprintf(name, sizeof(name) - 1,
           "\\\\.\\pipe\\sheredom_subprocess_h.%08lx.%08lx.%ld",
           GetCurrentProcessId(), GetCurrentThreadId(), unique);
#endif

  *rd =
      CreateNamedPipeA(name, pipeAccessInbound | fileFlagOverlapped,
                       pipeTypeByte | pipeWait, 1, 4096, 4096, SUBPROCESS_NULL,
                       SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr));

  if (invalidHandleValue == *rd) {
    return -1;
  }

  *wr = CreateFileA(name, genericWrite, SUBPROCESS_NULL,
                    SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr),
                    openExisting, fileAttributeNormal, SUBPROCESS_NULL);

  if (invalidHandleValue == *wr) {
    return -1;
  }

  return 0;
}
#endif

int subprocess_create(const char *const commandLine[], int options,
                      struct subprocess_s *const out_process) {
  return subprocess_create_ex(commandLine, options, SUBPROCESS_NULL,
                              out_process);
}

int subprocess_create_ex(const char *const commandLine[], int options,
                         const char *const environment[],
                         struct subprocess_s *const out_process) {
#if defined(_WIN32)
  int fd;
  void *rd, *wr;
  char *commandLineCombined;
  subprocess_size_t len;
  int i, j;
  int need_quoting;
  unsigned long flags = 0;
  const unsigned long startFUseStdHandles = 0x00000100;
  const unsigned long handleFlagInherit = 0x00000001;
  const unsigned long createNoWindow = 0x08000000;
  struct subprocess_subprocess_information_s processInfo;
  struct subprocess_security_attributes_s saAttr = {sizeof(saAttr),
                                                    SUBPROCESS_NULL, 1};
  char *used_environment = SUBPROCESS_NULL;
  struct subprocess_startup_info_s startInfo = {0,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL};

  startInfo.cb = sizeof(startInfo);
  startInfo.dwFlags = startFUseStdHandles;

  if (subprocess_option_no_window == (options & subprocess_option_no_window)) {
    flags |= createNoWindow;
  }

  if (subprocess_option_inherit_environment !=
      (options & subprocess_option_inherit_environment)) {
    if (SUBPROCESS_NULL == environment) {
      used_environment = SUBPROCESS_CONST_CAST(char *, "\0\0");
    } else {
      // We always end with two null terminators.
      len = 2;

      for (i = 0; environment[i]; i++) {
        for (j = 0; '\0' != environment[i][j]; j++) {
          len++;
        }

        // For the null terminator too.
        len++;
      }

      used_environment = SUBPROCESS_CAST(char *, _alloca(len));

      // Re-use len for the insertion position
      len = 0;

      for (i = 0; environment[i]; i++) {
        for (j = 0; '\0' != environment[i][j]; j++) {
          used_environment[len++] = environment[i][j];
        }

        used_environment[len++] = '\0';
      }

      // End with the two null terminators.
      used_environment[len++] = '\0';
      used_environment[len++] = '\0';
    }
  } else {
    if (SUBPROCESS_NULL != environment) {
      return -1;
    }
  }

  if (!CreatePipe(&rd, &wr, SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr),
                  0)) {
    return -1;
  }

  if (!SetHandleInformation(wr, handleFlagInherit, 0)) {
    return -1;
  }

  fd = _open_osfhandle(SUBPROCESS_PTR_CAST(subprocess_intptr_t, wr), 0);

  if (-1 != fd) {
    out_process->stdin_file = _fdopen(fd, "wb");

    if (SUBPROCESS_NULL == out_process->stdin_file) {
      return -1;
    }
  }

  startInfo.hStdInput = rd;

  if (options & subprocess_option_enable_async) {
    if (subprocess_create_named_pipe_helper(&rd, &wr)) {
      return -1;
    }
  } else {
    if (!CreatePipe(&rd, &wr,
                    SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 0)) {
      return -1;
    }
  }

  if (!SetHandleInformation(rd, handleFlagInherit, 0)) {
    return -1;
  }

  fd = _open_osfhandle(SUBPROCESS_PTR_CAST(subprocess_intptr_t, rd), 0);

  if (-1 != fd) {
    out_process->stdout_file = _fdopen(fd, "rb");

    if (SUBPROCESS_NULL == out_process->stdout_file) {
      return -1;
    }
  }

  startInfo.hStdOutput = wr;

  if (subprocess_option_combined_stdout_stderr ==
      (options & subprocess_option_combined_stdout_stderr)) {
    out_process->stderr_file = out_process->stdout_file;
    startInfo.hStdError = startInfo.hStdOutput;
  } else {
    if (options & subprocess_option_enable_async) {
      if (subprocess_create_named_pipe_helper(&rd, &wr)) {
        return -1;
      }
    } else {
      if (!CreatePipe(&rd, &wr,
                      SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 0)) {
        return -1;
      }
    }

    if (!SetHandleInformation(rd, handleFlagInherit, 0)) {
      return -1;
    }

    fd = _open_osfhandle(SUBPROCESS_PTR_CAST(subprocess_intptr_t, rd), 0);

    if (-1 != fd) {
      out_process->stderr_file = _fdopen(fd, "rb");

      if (SUBPROCESS_NULL == out_process->stderr_file) {
        return -1;
      }
    }

    startInfo.hStdError = wr;
  }

  if (options & subprocess_option_enable_async) {
    out_process->hEventOutput =
        CreateEventA(SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 1, 1,
                     SUBPROCESS_NULL);
    out_process->hEventError =
        CreateEventA(SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 1, 1,
                     SUBPROCESS_NULL);
  } else {
    out_process->hEventOutput = SUBPROCESS_NULL;
    out_process->hEventError = SUBPROCESS_NULL;
  }

  // Combine commandLine together into a single string
  len = 0;
  for (i = 0; commandLine[i]; i++) {
    // for the trailing \0
    len++;

    // Quote the argument if it has a space in it
    if (strpbrk(commandLine[i], "\t\v ") != SUBPROCESS_NULL ||
        commandLine[i][0] == SUBPROCESS_NULL)
      len += 2;

    for (j = 0; '\0' != commandLine[i][j]; j++) {
      switch (commandLine[i][j]) {
      default:
        break;
      case '\\':
        if (commandLine[i][j + 1] == '"') {
          len++;
        }

        break;
      case '"':
        len++;
        break;
      }
      len++;
    }
  }

  commandLineCombined = SUBPROCESS_CAST(char *, _alloca(len));

  if (!commandLineCombined) {
    return -1;
  }

  // Gonna re-use len to store the write index into commandLineCombined
  len = 0;

  for (i = 0; commandLine[i]; i++) {
    if (0 != i) {
      commandLineCombined[len++] = ' ';
    }

    need_quoting = strpbrk(commandLine[i], "\t\v ") != SUBPROCESS_NULL ||
                   commandLine[i][0] == SUBPROCESS_NULL;
    if (need_quoting) {
      commandLineCombined[len++] = '"';
    }

    for (j = 0; '\0' != commandLine[i][j]; j++) {
      switch (commandLine[i][j]) {
      default:
        break;
      case '\\':
        if (commandLine[i][j + 1] == '"') {
          commandLineCombined[len++] = '\\';
        }

        break;
      case '"':
        commandLineCombined[len++] = '\\';
        break;
      }

      commandLineCombined[len++] = commandLine[i][j];
    }
    if (need_quoting) {
      commandLineCombined[len++] = '"';
    }
  }

  commandLineCombined[len] = '\0';

  if (!CreateProcessA(
          SUBPROCESS_NULL,
          commandLineCombined, // command line
          SUBPROCESS_NULL,     // process security attributes
          SUBPROCESS_NULL,     // primary thread security attributes
          1,                   // handles are inherited
          flags,               // creation flags
          used_environment,    // used environment
          SUBPROCESS_NULL,     // use parent's current directory
          SUBPROCESS_PTR_CAST(LPSTARTUPINFOA,
                              &startInfo), // STARTUPINFO pointer
          SUBPROCESS_PTR_CAST(LPPROCESS_INFORMATION, &processInfo))) {
    return -1;
  }

  out_process->hProcess = processInfo.hProcess;

  out_process->hStdInput = startInfo.hStdInput;

  // We don't need the handle of the primary thread in the called process.
  CloseHandle(processInfo.hThread);

  if (SUBPROCESS_NULL != startInfo.hStdOutput) {
    CloseHandle(startInfo.hStdOutput);

    if (startInfo.hStdError != startInfo.hStdOutput) {
      CloseHandle(startInfo.hStdError);
    }
  }

  out_process->alive = 1;

  return 0;
#    elif defined(__EMSCRIPTEN__)
  (void)commandLine;
  (void)environment;
  (void)out_process;
  (void)options;
  uassert(false && "This functionality is not supported by WASM" );
  return -1;
#else
  int stdinfd[2];
  int stdoutfd[2];
  int stderrfd[2];
  pid_t child;
  extern char **environ;
  char *const empty_environment[1] = {SUBPROCESS_NULL};
  posix_spawn_file_actions_t actions;
  char *const *used_environment;

  if (subprocess_option_inherit_environment ==
      (options & subprocess_option_inherit_environment)) {
    if (SUBPROCESS_NULL != environment) {
      return -1;
    }
  }

  if (0 != pipe(stdinfd)) {
    return -1;
  }

  if (0 != pipe(stdoutfd)) {
    return -1;
  }

  if (subprocess_option_combined_stdout_stderr !=
      (options & subprocess_option_combined_stdout_stderr)) {
    if (0 != pipe(stderrfd)) {
      return -1;
    }
  }

  if (environment) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
    used_environment = SUBPROCESS_CONST_CAST(char *const *, environment);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  } else if (subprocess_option_inherit_environment ==
             (options & subprocess_option_inherit_environment)) {
    used_environment = environ;
  } else {
    used_environment = empty_environment;
  }

  if (0 != posix_spawn_file_actions_init(&actions)) {
    return -1;
  }

  // Close the stdin write end
  if (0 != posix_spawn_file_actions_addclose(&actions, stdinfd[1])) {
    posix_spawn_file_actions_destroy(&actions);
    return -1;
  }

  // Map the read end to stdin
  if (0 !=
      posix_spawn_file_actions_adddup2(&actions, stdinfd[0], STDIN_FILENO)) {
    posix_spawn_file_actions_destroy(&actions);
    return -1;
  }

  // Close the stdout read end
  if (0 != posix_spawn_file_actions_addclose(&actions, stdoutfd[0])) {
    posix_spawn_file_actions_destroy(&actions);
    return -1;
  }

  // Map the write end to stdout
  if (0 !=
      posix_spawn_file_actions_adddup2(&actions, stdoutfd[1], STDOUT_FILENO)) {
    posix_spawn_file_actions_destroy(&actions);
    return -1;
  }

  if (subprocess_option_combined_stdout_stderr ==
      (options & subprocess_option_combined_stdout_stderr)) {
    if (0 != posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO,
                                              STDERR_FILENO)) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
  } else {
    // Close the stderr read end
    if (0 != posix_spawn_file_actions_addclose(&actions, stderrfd[0])) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
    // Map the write end to stdout
    if (0 != posix_spawn_file_actions_adddup2(&actions, stderrfd[1],
                                              STDERR_FILENO)) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
  }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
  if (subprocess_option_search_user_path ==
      (options & subprocess_option_search_user_path)) {
    if (0 != posix_spawnp(&child, commandLine[0], &actions, SUBPROCESS_NULL,
                          SUBPROCESS_CONST_CAST(char *const *, commandLine),
                          used_environment)) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
  } else {
    if (0 != posix_spawn(&child, commandLine[0], &actions, SUBPROCESS_NULL,
                         SUBPROCESS_CONST_CAST(char *const *, commandLine),
                         used_environment)) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
  }
#ifdef __clang__
#pragma clang diagnostic pop
#endif

  // Close the stdin read end
  close(stdinfd[0]);
  // Store the stdin write end
  out_process->stdin_file = fdopen(stdinfd[1], "wb");

  // Close the stdout write end
  close(stdoutfd[1]);
  // Store the stdout read end
  out_process->stdout_file = fdopen(stdoutfd[0], "rb");

  if (subprocess_option_combined_stdout_stderr ==
      (options & subprocess_option_combined_stdout_stderr)) {
    out_process->stderr_file = out_process->stdout_file;
  } else {
    // Close the stderr write end
    close(stderrfd[1]);
    // Store the stderr read end
    out_process->stderr_file = fdopen(stderrfd[0], "rb");
  }

  // Store the child's pid
  out_process->child = child;

  out_process->alive = 1;

  posix_spawn_file_actions_destroy(&actions);
  return 0;
#endif
}

FILE *subprocess_stdin(const struct subprocess_s *const process) {
  return process->stdin_file;
}

FILE *subprocess_stdout(const struct subprocess_s *const process) {
  return process->stdout_file;
}

FILE *subprocess_stderr(const struct subprocess_s *const process) {
  if (process->stdout_file != process->stderr_file) {
    return process->stderr_file;
  } else {
    return SUBPROCESS_NULL;
  }
}

int subprocess_join(struct subprocess_s *const process,
                    int *const out_return_code) {
#if defined(_WIN32)
  const unsigned long infinite = 0xFFFFFFFF;

  if (process->stdin_file) {
    fclose(process->stdin_file);
    process->stdin_file = SUBPROCESS_NULL;
  }

  if (process->hStdInput) {
    CloseHandle(process->hStdInput);
    process->hStdInput = SUBPROCESS_NULL;
  }

  WaitForSingleObject(process->hProcess, infinite);

  if (out_return_code) {
    if (!GetExitCodeProcess(
            process->hProcess,
            SUBPROCESS_PTR_CAST(unsigned long *, out_return_code))) {
      return -1;
    }
  }

  process->alive = 0;

  return 0;
#else
  int status;

  if (process->stdin_file) {
    fclose(process->stdin_file);
    process->stdin_file = SUBPROCESS_NULL;
  }

  if (process->child) {
    if (process->child != waitpid(process->child, &status, 0)) {
      return -1;
    }

    process->child = 0;

    if (WIFEXITED(status)) {
      process->return_status = WEXITSTATUS(status);
    } else {
      process->return_status = EXIT_FAILURE;
    }

    process->alive = 0;
  }

  if (out_return_code) {
    *out_return_code = process->return_status;
  }

  return 0;
#endif
}

int subprocess_destroy(struct subprocess_s *const process) {
  if (process->stdin_file) {
    fclose(process->stdin_file);
    process->stdin_file = SUBPROCESS_NULL;
  }

  if (process->stdout_file) {
    fclose(process->stdout_file);

    if (process->stdout_file != process->stderr_file) {
      fclose(process->stderr_file);
    }

    process->stdout_file = SUBPROCESS_NULL;
    process->stderr_file = SUBPROCESS_NULL;
  }

#if defined(_WIN32)
  if (process->hProcess) {
    CloseHandle(process->hProcess);
    process->hProcess = SUBPROCESS_NULL;

    if (process->hStdInput) {
      CloseHandle(process->hStdInput);
    }

    if (process->hEventOutput) {
      CloseHandle(process->hEventOutput);
    }

    if (process->hEventError) {
      CloseHandle(process->hEventError);
    }
  }
#endif

  return 0;
}

int subprocess_terminate(struct subprocess_s *const process) {
#if defined(_WIN32)
  unsigned int killed_process_exit_code;
  int success_terminate;
  int windows_call_result;

  killed_process_exit_code = 99;
  windows_call_result =
      TerminateProcess(process->hProcess, killed_process_exit_code);
  success_terminate = (windows_call_result == 0) ? 1 : 0;
  return success_terminate;
#else
  int result;
  result = kill(process->child, 9);
  return result;
#endif
}

unsigned subprocess_read_stdout(struct subprocess_s *const process,
                                char *const buffer, unsigned size) {
#if defined(_WIN32)
  void *handle;
  unsigned long bytes_read = 0;
  struct subprocess_overlapped_s overlapped = {0, 0, {{0, 0}}, SUBPROCESS_NULL};
  overlapped.hEvent = process->hEventOutput;

  handle = SUBPROCESS_PTR_CAST(void *,
                               _get_osfhandle(_fileno(process->stdout_file)));

  if (!ReadFile(handle, buffer, size, &bytes_read,
                SUBPROCESS_PTR_CAST(LPOVERLAPPED, &overlapped))) {
    const unsigned long errorIoPending = 997;
    unsigned long error = GetLastError();

    // Means we've got an async read!
    if (error == errorIoPending) {
      if (!GetOverlappedResult(handle,
                               SUBPROCESS_PTR_CAST(LPOVERLAPPED, &overlapped),
                               &bytes_read, 1)) {
        const unsigned long errorIoIncomplete = 996;
        const unsigned long errorHandleEOF = 38;
        error = GetLastError();

        if ((error != errorIoIncomplete) && (error != errorHandleEOF)) {
          return 0;
        }
      }
    }
  }

  return SUBPROCESS_CAST(unsigned, bytes_read);
#else
  const int fd = fileno(process->stdout_file);
  const ssize_t bytes_read = read(fd, buffer, size);

  if (bytes_read < 0) {
    return 0;
  }

  return SUBPROCESS_CAST(unsigned, bytes_read);
#endif
}

unsigned subprocess_read_stderr(struct subprocess_s *const process,
                                char *const buffer, unsigned size) {
#if defined(_WIN32)
  void *handle;
  unsigned long bytes_read = 0;
  struct subprocess_overlapped_s overlapped = {0, 0, {{0, 0}}, SUBPROCESS_NULL};
  overlapped.hEvent = process->hEventError;

  handle = SUBPROCESS_PTR_CAST(void *,
                               _get_osfhandle(_fileno(process->stderr_file)));

  if (!ReadFile(handle, buffer, size, &bytes_read,
                SUBPROCESS_PTR_CAST(LPOVERLAPPED, &overlapped))) {
    const unsigned long errorIoPending = 997;
    unsigned long error = GetLastError();

    // Means we've got an async read!
    if (error == errorIoPending) {
      if (!GetOverlappedResult(handle,
                               SUBPROCESS_PTR_CAST(LPOVERLAPPED, &overlapped),
                               &bytes_read, 1)) {
        const unsigned long errorIoIncomplete = 996;
        const unsigned long errorHandleEOF = 38;
        error = GetLastError();

        if ((error != errorIoIncomplete) && (error != errorHandleEOF)) {
          return 0;
        }
      }
    }
  }

  return SUBPROCESS_CAST(unsigned, bytes_read);
#else
  const int fd = fileno(process->stderr_file);
  const ssize_t bytes_read = read(fd, buffer, size);

  if (bytes_read < 0) {
    return 0;
  }

  return SUBPROCESS_CAST(unsigned, bytes_read);
#endif
}

int subprocess_alive(struct subprocess_s *const process) {
  int is_alive = SUBPROCESS_CAST(int, process->alive);

  if (!is_alive) {
    return 0;
  }
#if defined(_WIN32)
  {
    const unsigned long zero = 0x0;
    const unsigned long wait_object_0 = 0x00000000L;

    is_alive = wait_object_0 != WaitForSingleObject(process->hProcess, zero);
  }
#else
  {
    int status;
    is_alive = 0 == waitpid(process->child, &status, WNOHANG);

    // If the process was successfully waited on we need to cleanup now.
    if (!is_alive) {
      if (WIFEXITED(status)) {
        process->return_status = WEXITSTATUS(status);
      } else {
        process->return_status = EXIT_FAILURE;
      }

      // Since we've already successfully waited on the process, we need to wipe
      // the child now.
      process->child = 0;

      if (subprocess_join(process, SUBPROCESS_NULL)) {
        return -1;
      }
    }
  }
#endif

  if (!is_alive) {
    process->alive = 0;
  }

  return is_alive;
}

#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic pop
#endif
#endif

#endif



/*
*                          src/os.c
*/
#if !defined(cex$enable_minimal) || defined(cex$enable_os)

#ifndef _WIN32
#    include <dirent.h>
#else // _WIN32
// minirent.h HEADER BEGIN
// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// ============================================================

#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"

struct dirent
{
    char d_name[MAX_PATH + 1];
};

typedef struct DIR DIR;

static DIR* opendir(char* dirpath);
static struct dirent* readdir(DIR* dirp);
static int closedir(DIR* dirp);

struct DIR
{
    HANDLE hFind;
    WIN32_FIND_DATA data;
    struct dirent* dirent;
};

DIR*
opendir(char* dirpath)
{
    char buffer[MAX_PATH + 10];
    snprintf(buffer, sizeof(buffer), "%s\\*", dirpath);

    DIR* dir = (DIR*)realloc(NULL, sizeof(DIR));
    if (dir == NULL) {
        errno = ENOMEM;
        goto fail;
    }
    memset(dir, 0, sizeof(DIR));

    dir->hFind = FindFirstFile(buffer, &dir->data);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        // TODO: opendir should set errno accordingly on FindFirstFile fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        goto fail;
    }

    return dir;

fail:
    if (dir) { free(dir); }

    return NULL;
}

struct dirent*
readdir(DIR* dirp)
{
    if (dirp->dirent == NULL) {
        dirp->dirent = (struct dirent*)realloc(NULL, sizeof(struct dirent));
        if (dirp->dirent == NULL) {
            errno = ENOMEM;
            return NULL;
        }
        memset(dirp->dirent, 0, sizeof(struct dirent));
    } else {
        if (!FindNextFile(dirp->hFind, &dirp->data)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                // TODO: readdir should set errno accordingly on FindNextFile fail
                // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
                errno = ENOSYS;
            }

            return NULL;
        }
    }

    memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

    strncpy(dirp->dirent->d_name, dirp->data.cFileName, sizeof(dirp->dirent->d_name) - 1);

    return dirp->dirent;
}

int
closedir(DIR* dirp)
{
    if (!FindClose(dirp->hFind)) {
        // TODO: closedir should set errno accordingly on FindClose fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        return -1;
    }

    if (dirp->dirent) { free(dirp->dirent); }
    free(dirp);

    return 0;
}
#endif // _WIN32

/// Sleep for `period_millisec` duration
static void
cex_os_sleep(u32 period_millisec)
{
#ifdef _WIN32
    Sleep(period_millisec);
#else
    usleep(period_millisec * 1000);
#endif
}

/// Get high performance monotonic timer value in seconds
static f64
cex_os_timer(void)
{
#ifdef _WIN32
    static LARGE_INTEGER frequency = { 0 };
    if (unlikely(frequency.QuadPart == 0)) { QueryPerformanceFrequency(&frequency); }
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);
    return (f64)(start.QuadPart) / (f64)frequency.QuadPart;
#else
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    return (f64)start.tv_sec + (f64)start.tv_nsec / 1e9;
#endif
}

/// Get last system API error as string representation (Exception compatible). Result content may be
/// affected by OS locale settings.
static Exc
cex_os_get_last_error(void)
{
#ifdef _WIN32
    DWORD err = GetLastError();
    switch (err) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return Error.not_found;
        case ERROR_ACCESS_DENIED:
            return Error.permission;
        default:
            break;
    }

    _Thread_local static char win32_err_buf[256] = { 0 };

    DWORD err_msg_size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        LANG_USER_DEFAULT,
        win32_err_buf,
        arr$len(win32_err_buf),
        NULL
    );

    if (unlikely(err_msg_size == 0)) {
        if (GetLastError() != ERROR_MR_MID_NOT_FOUND) {
            if (sprintf(win32_err_buf, "Generic windows error code: 0x%lX", err) > 0) {
                return (char*)&win32_err_buf;
            } else {
                return NULL;
            }
        } else {
            if (sprintf(win32_err_buf, "Invalid Windows Error code (0x%lX)", err) > 0) {
                return (char*)&win32_err_buf;
            } else {
                return NULL;
            }
        }
    }

    while (err_msg_size > 1 && isspace(win32_err_buf[err_msg_size - 1])) {
        win32_err_buf[--err_msg_size] = '\0';
    }

    return win32_err_buf;
#else
    switch (errno) {
        case 0:
            uassert(errno != 0 && "errno is ok");
            return "Error, but errno is not set";
        case ENOENT:
            return Error.not_found;
        case EPERM:
            return Error.permission;
        case EIO:
            return Error.io;
        case EAGAIN:
            return Error.try_again;
        default:
            return strerror(errno);
    }
#endif
}

/// Renames file or directory
static Exception
cex_os__fs__rename(char* old_path, char* new_path)
{
    if (old_path == NULL || old_path[0] == '\0') { return Error.argument; }
    if (new_path == NULL || new_path[0] == '\0') { return Error.argument; }
    if (os.path.exists(new_path)) { return Error.exists; }
#ifdef _WIN32
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) { return os.get_last_error(); }
    return EOK;
#else
    if (rename(old_path, new_path) < 0) { return os.get_last_error(); }
    return EOK;
#endif // _WIN32
}

/// Makes directory (no error if exists)
static Exception
cex_os__fs__mkdir(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.argument; }
#ifdef _WIN32
    int result = mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif
    if (result < 0) {
        uassert(errno != 0);
        if (errno == EEXIST) { return EOK; }
        return os.get_last_error();
    }
    return EOK;
}

/// Makes all directories in a path
static Exception
cex_os__fs__mkpath(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.argument; }
    str_s dir = os.path.split(path, true);
    char dir_path[PATH_MAX] = { 0 };
    e$ret(str.slice.copy(dir_path, dir, sizeof(dir_path)));
    if (os.path.exists(dir_path)) { return EOK; }
    usize dir_path_len = 0;

    for$iter (str_s, it, str.slice.iter_split(dir, "\\/", &it.iterator)) {
        if (dir_path_len > 0) {
            uassert(dir_path_len < sizeof(dir_path) - 2);
            dir_path[dir_path_len] = os$PATH_SEP;
            dir_path_len++;
            dir_path[dir_path_len] = '\0';
        }
        e$ret(str.slice.copy(dir_path + dir_path_len, it.val, sizeof(dir_path) - dir_path_len));
        dir_path_len += it.val.len;
        e$ret(os.fs.mkdir(dir_path));
    }
    return EOK;
}

/// Returns cross-platform path stats information (see os_fs_stat_s)
static os_fs_stat_s
cex_os__fs__stat(char* path)
{
    os_fs_stat_s result = { .error = Error.argument };
    if (path == NULL || path[0] == '\0') { return result; }

#ifdef _WIN32
    // NOTE: for mingw64 _stat() doesn't do well when path has trailing /
    usize plen = strlen(path);
    if (unlikely(plen >= PATH_MAX)) {
        result.error = Error.overflow;
        return result;
    }

    char clean_path[PATH_MAX + 10];
    memcpy(clean_path, path, plen + 1); // including \0
    if (unlikely(clean_path[plen - 1] == '/' || clean_path[plen - 1] == '\\')) {
        for (u32 i = plen; --i > 0;) {
            if (clean_path[i] == '/' || clean_path[i] == '\\') {
                clean_path[i] = '\0';
            } else {
                break;
            }
        }
    }

    struct _stat statbuf;
    if (unlikely(_stat(clean_path, &statbuf) < 0)) {
        result.error = os.get_last_error();
        return result;
    }

    result.error = EOK;
    result.is_valid = true;
    result.mtime = statbuf.st_mtime;
    result.is_other = true;
    result.is_symlink = false;

    if (statbuf.st_mode & _S_IFDIR) {
        result.is_directory = true;
        result.is_other = false;
    }

    if (statbuf.st_mode & _S_IFREG) {
        result.is_directory = false;
        result.is_other = false;
        result.is_file = true;
    }

    result.size = statbuf.st_size;

    return result;
#else // _WIN32
    struct stat statbuf;
    if (unlikely(lstat(path, &statbuf) < 0)) {
        result.error = os.get_last_error();
        return result;
    }
    result.is_valid = true;
    result.error = EOK;
    result.is_other = true;
    result.mtime = statbuf.st_mtime;

    if (!S_ISLNK(statbuf.st_mode)) {
        if (S_ISREG(statbuf.st_mode)) {
            result.is_file = true;
            result.is_other = false;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            result.is_directory = true;
            result.is_other = false;
        }
    } else {
        result.is_symlink = true;
        if (unlikely(stat(path, &statbuf) < 0)) {
            result.error = os.get_last_error();
            return result;
        }
        if (S_ISREG(statbuf.st_mode)) {
            result.is_file = true;
            result.is_other = false;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            result.is_directory = true;
            result.is_other = false;
        }
    }
    result.size = statbuf.st_size;
    return result;
#endif
}

/// Removes file or empty directory (also see os.fs.remove_tree)
static Exception
cex_os__fs__remove(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.argument; }

    os_fs_stat_s stat = os.fs.stat(path);
    if (!stat.is_valid) { return stat.error; }
#ifdef _WIN32
    if (stat.is_file || stat.is_symlink) {
        if (!DeleteFileA(path)) { return os.get_last_error(); }
    } else if (stat.is_directory) {
        if (!RemoveDirectoryA(path)) { return os.get_last_error(); }
    } else {
        return "Unsupported path type";
    }
    return EOK;
#else
    if (remove(path) < 0) { return os.get_last_error(); }
    return EOK;
#endif
}

/// Iterates over directory (can be recursive) using callback function
Exception
cex_os__fs__dir_walk(char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx)
{
    (void)user_ctx;
    if (path == NULL || path[0] == '\0') { return Error.argument; }
    Exc result = Error.os;
    uassert(callback_fn != NULL && "you must provide callback_fn");

    DIR* dp = opendir(path);

    if (unlikely(dp == NULL)) {
        result = os.get_last_error();
        goto end;
    }

    u32 path_len = strlen(path);
    if (path_len > PATH_MAX - 2) {
        result = Error.overflow;
        goto end;
    }

    char path_buf[PATH_MAX];

    struct dirent* ep;
    while ((ep = readdir(dp)) != NULL) {
        errno = 0;
        if (str.eq(ep->d_name, ".")) { continue; }
        if (str.eq(ep->d_name, "..")) { continue; }
        memcpy(path_buf, path, path_len);
        u32 path_offset = 0;
        if (path_buf[path_len - 1] != '/' && path_buf[path_len - 1] != '\\') {
            path_buf[path_len] = os$PATH_SEP;
            path_offset = 1;
        }

        e$except_silent (
            err,
            str.copy(
                path_buf + path_len + path_offset,
                ep->d_name,
                sizeof(path_buf) - path_len - 1 - path_offset
            )
        ) {
            result = err;
            goto end;
        }

        auto ftype = os.fs.stat(path_buf);
        if (!ftype.is_valid) {
            result = ftype.error;
            goto end;
        }

        if (is_recursive && ftype.is_directory && !ftype.is_symlink) {
            e$except_silent (
                err,
                cex_os__fs__dir_walk(path_buf, is_recursive, callback_fn, user_ctx)
            ) {
                result = err;
                goto end;
            }
        }
        // After recursive call make a callback on a directory itself
        e$except_silent (err, callback_fn(path_buf, ftype, user_ctx)) {
            result = err;
            goto end;
        }
    }

    result = EOK;
end:
    if (dp != NULL) { (void)closedir(dp); }
    return result;
}

struct _os_fs_find_ctx_s
{
    char* pattern;
    arr$(char*) result;
    IAllocator allc;
};

static Exception
_os__fs__remove_tree_walker(char* path, os_fs_stat_s ftype, void* user_ctx)
{
    (void)user_ctx;
    (void)ftype;
    e$except_silent (err, cex_os__fs__remove(path)) {
        log$trace("Error removing: %s\n", path);
        return err;
    }
    return EOK;
}

/// Removes directory and all its contents recursively
static Exception
cex_os__fs__remove_tree(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.argument; }
    if (!os.path.exists(path)) { return Error.not_found; }
    e$except_silent (err, cex_os__fs__dir_walk(path, true, _os__fs__remove_tree_walker, NULL)) {
        return err;
    }
    e$except_silent (err, cex_os__fs__remove(path)) {
        log$trace("Error removing: %s\n", path);
        return err;
    }
    return EOK;
}

struct _os_fs_copy_tree_ctx_s
{
    str_s src_dir;
    str_s dest_dir;
};

static Exception
_os__fs__copy_tree_walker(char* path, os_fs_stat_s ftype, void* user_ctx)
{
    struct _os_fs_copy_tree_ctx_s* ctx = user_ctx;
    mem$scope(tmem$, _)
    {
        uassert(str.starts_with(path, ctx->src_dir.buf) && "file must start with source dir");
        char* out_file = str.fmt(_, "%S/%S", ctx->dest_dir, str.sub(path, ctx->src_dir.len, 0));

        if (ftype.is_file) {
            e$ret(os.fs.mkpath(out_file));
            e$ret(os.fs.copy(path, out_file));
        } else {
            // Making empty directory if necessary
            char* out_dir = str.fmt(_, "%s/", out_file);
            e$ret(os.fs.mkpath(out_dir));
        }
    }

    return EOK;
}


/// Copy directory recursively
static Exception
cex_os__fs__copy_tree(char* src_dir, char* dst_dir)
{
    if (src_dir == NULL || src_dir[0] == '\0') { return Error.argument; }
    os_fs_stat_s s = os.fs.stat(src_dir);
    if (!s.is_valid) { return s.error; }
    if (!s.is_directory) { return Error.argument; }

    if (dst_dir == NULL || dst_dir[0] == '\0') { return Error.argument; }
    if (os.path.exists(dst_dir)) { return Error.exists; }

    // TODO: add absolute path overlap check

    struct _os_fs_copy_tree_ctx_s ctx = {
        .src_dir = str.sstr(src_dir),
        .dest_dir = str.sstr(dst_dir),
    };
    e$except_silent (err, cex_os__fs__dir_walk(src_dir, true, _os__fs__copy_tree_walker, &ctx)) {
        return err;
    }

    return EOK;
}

static Exception
_os__fs__find_walker(char* path, os_fs_stat_s ftype, void* user_ctx)
{
    (void)ftype;
    struct _os_fs_find_ctx_s* ctx = (struct _os_fs_find_ctx_s*)user_ctx;
    uassert(ctx->result != NULL);
    if (ftype.is_directory) {
        return EOK; // skip, only supports finding files!
    }
    if (ftype.is_symlink) {
        return EOK; // does not follow symlinks!
    }

    str_s file_part = os.path.split(path, false);
    if (!str.match(file_part.buf, ctx->pattern)) {
        return EOK; // just skip when patten not matched
    }

    // allocate new string because path is stack allocated buffer in os__fs__dir_walk()
    char* new_path = str.clone(path, ctx->allc);
    if (new_path == NULL) { return Error.memory; }

    // Doing graceful memory check, otherwise arr$push will assert
    if (!arr$grow_check(ctx->result, 1)) { return Error.memory; }
    arr$push(ctx->result, new_path);
    return EOK;
}

/// Finds files in `dir/pattern`, for example "./mydir/*.c" (all c files), if is_recursive=true, all
/// *.c files found in sub-directories.
static arr$(char*) cex_os__fs__find(char* path_pattern, bool is_recursive, IAllocator allc)
{

    if (unlikely(allc == NULL)) { return NULL; }

    str_s dir_part = os.path.split(path_pattern, true);
    if (dir_part.buf == NULL) {
#if defined(CEX_TEST) || defined(CEX_BUILD)
        (void)e$raise(Error.argument, "Bad path: os.fn.find('%s')", path_pattern);
#endif
        return NULL;
    }

    if (!is_recursive) {
        auto f = os.fs.stat(path_pattern);
        if (f.is_valid && f.is_file) {
            // Find used with exact file path, we still have to return array + allocated path copy
            arr$(char*) result = arr$new(result, allc);
            char* it = str.clone(path_pattern, allc);
            arr$push(result, it);
            return result;
        }
    }

    char path_buf[PATH_MAX + 2] = { 0 };
    if (dir_part.len > 0 && str.slice.copy(path_buf, dir_part, sizeof(path_buf))) {
        uassert(dir_part.len < PATH_MAX);
        return NULL;
    }
    if (str.copy(
            path_buf + dir_part.len + 1,
            path_pattern + dir_part.len,
            sizeof(path_buf) - dir_part.len - 1
        )) {
        uassert(dir_part.len < PATH_MAX);
        return NULL;
    }
    char* dir_name = (dir_part.len > 0) ? path_buf : ".";
    char* pattern = path_buf + dir_part.len + 1;
    if (*pattern == '/' || *pattern == '\\') { pattern++; }
    if (*pattern == '\0') { pattern = "*"; }

    struct _os_fs_find_ctx_s ctx = { .result = arr$new(ctx.result, allc),
                                     .pattern = pattern,
                                     .allc = allc };
    if (unlikely(ctx.result == NULL)) { return NULL; }

    e$except_silent (err, cex_os__fs__dir_walk(dir_name, is_recursive, _os__fs__find_walker, &ctx)) {
        for$each (it, ctx.result) {
            mem$free(allc, it); // each individual item was allocated too
        }
        if (ctx.result != NULL) { arr$free(ctx.result); }
        ctx.result = NULL;
    }
    return ctx.result;
}


/// Get current working directory
static char*
cex_os__fs__getcwd(IAllocator allc)
{
    char* buf = mem$malloc(allc, PATH_MAX);

    char* result = NULL;
#ifdef _WIN32
    result = _getcwd(buf, PATH_MAX);
#else
    result = getcwd(buf, PATH_MAX);
#endif
    if (result == NULL) { mem$free(allc, buf); }

    return result;
}

/// Change current working directory
static Exception
cex_os__fs__chdir(char* path)
{
    if (path == NULL || path[0] == '\0') { return Error.exists; }

    int result;
#ifdef _WIN32
    result = _chdir(path);
#else
    result = chdir(path);
#endif

    if (result == -1) {
        if (errno == ENOENT) {
            return Error.not_found;
        } else {
            return strerror(errno);
        }
    }

    return EOK;
}


/// Copy file
static Exception
cex_os__fs__copy(char* src_path, char* dst_path)
{
    if (src_path == NULL || src_path[0] == '\0' || dst_path == NULL || dst_path[0] == '\0') {
        return Error.argument;
    }
    log$trace("copying %s -> %s\n", src_path, dst_path);

    if (os.path.exists(dst_path)) { return Error.exists; }

#ifdef _WIN32
    if (!CopyFile(src_path, dst_path, FALSE)) { return os.get_last_error(); }
    return EOK;
#else
    int src_fd = -1;
    int dst_fd = -1;
    size_t buf_size = 32 * 1024;
    char* buf = mem$malloc(mem$, buf_size);
    if (buf == NULL) { return Error.memory; }
    Exc result = Error.runtime;

    if ((src_fd = open(src_path, O_RDONLY)) == -1) {
        result = os.get_last_error();
        goto defer;
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) {
        result = os.get_last_error();
        goto defer;
    }

    dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dst_fd < 0) {
        result = strerror(errno);
        goto defer;
    }

    for (;;) {
        ssize_t n = read(src_fd, buf, buf_size);
        if (n == 0) { break; }
        if (n < 0) {
            result = os.get_last_error();
            goto defer;
        }
        char* buf2 = buf;
        while (n > 0) {
            ssize_t m = write(dst_fd, buf2, n);
            if (m < 0) {
                result = os.get_last_error();
                goto defer;
            }
            n -= m;
            buf2 += m;
        }
    }
    result = EOK;

defer:
    mem$free(mem$, buf);
    if (src_fd >= 0) { close(src_fd); }
    if (dst_fd >= 0) { close(dst_fd); }
    return result;
#endif
}

/// Get environment variable, with `deflt` if not found
static char*
cex_os__env__get(char* name, char* deflt)
{
    char* result = getenv(name);

    if (result == NULL) { result = deflt; }

    return result;
}

/// Set environment variable
static Exception
cex_os__env__set(char* name, char* value)
{
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, true);
#endif
    // TODO: add error reporting
    return EOK;
}

/// Check if file/directory path exists
static bool
cex_os__path__exists(char* file_path)
{
    auto ftype = os.fs.stat(file_path);
    return ftype.is_valid;
}

/// Returns absolute path from relative
static char*
cex_os__path__abs(char* path, IAllocator allc)
{
    uassert(allc != NULL);
    if (path == NULL || path[0] == '\0') { return NULL; }

    char buffer[PATH_MAX];

#ifdef _WIN32
    DWORD result = GetFullPathNameA(path, sizeof(buffer), buffer, NULL);
    if (result == 0 || result > sizeof(buffer) - 1) { return NULL; }
#else
    if (realpath(path, buffer) == NULL) { return NULL; }
#endif

    return str.clone(buffer, allc);
}

/// Join path with OS specific path separator
static char*
cex_os__path__join(char** parts, u32 parts_len, IAllocator allc)
{
    char sep[2] = { os$PATH_SEP, '\0' };
    return str.join(parts, parts_len, sep, allc);
}

/// Splits path by `dir` and `file` parts, when return_dir=true - returns `dir` part, otherwise
/// `file` part
static str_s
cex_os__path__split(char* path, bool return_dir)
{
    if (path == NULL) { return (str_s){ 0 }; }
    usize pathlen = strlen(path);
    if (pathlen == 0) { return str$s(""); }

    isize last_slash_idx = -1;

    for (usize i = pathlen; i-- > 0;) {
        if (path[i] == '/' || path[i] == '\\') {
            last_slash_idx = i;
            break;
        }
    }
    if (last_slash_idx != -1) {
        if (return_dir) {
            return str.sub(path, 0, last_slash_idx == 0 ? 1 : last_slash_idx);
        } else {
            if ((usize)last_slash_idx == pathlen - 1) {
                return str$s("");
            } else {
                return str.sub(path, last_slash_idx + 1, 0);
            }
        }

    } else {
        if (return_dir) {
            return str$s("");
        } else {
            return str.sstr(path);
        }
    }
}

/// Get file name of a path
static char*
cex_os__path__basename(char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') { return NULL; }
    str_s fname = cex_os__path__split(path, false);
    return str.slice.clone(fname, allc);
}

/// Get directory name of a path
static char*
cex_os__path__dirname(char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') { return NULL; }
    str_s fname = cex_os__path__split(path, true);
    return str.slice.clone(fname, allc);
}


/// Creates new os command (use os$cmd() and os$cmd() for easy cases). flags can be NULL.
static Exception
cex_os__cmd__create(os_cmd_c* self, char** args, usize args_len, os_cmd_flags_s* flags)
{
    uassert(self != NULL);
    if (args == NULL || args_len == 0) { return "`args` is empty or null"; }
    if (args_len == 1 || args[args_len - 1] != NULL) { return "`args` last item must be a NULL"; }
    for (u32 i = 0; i < args_len - 1; i++) {
        if (args[i] == NULL) {
            return "one of `args` items is NULL, which may indicate string operation failure";
        }
    }

    *self = (os_cmd_c){
        ._is_subprocess = true,
        ._flags = (flags) ? *flags : (os_cmd_flags_s){ 0 },
    };

    int sub_flags = 0;
    if (!self->_flags.no_inherit_env) { sub_flags |= subprocess_option_inherit_environment; }
    if (self->_flags.no_window) { sub_flags |= subprocess_option_no_window; }
    if (!self->_flags.no_search_path) { sub_flags |= subprocess_option_search_user_path; }
    if (self->_flags.combine_stdouterr) { sub_flags |= subprocess_option_combined_stdout_stderr; }

    int result = subprocess_create((const char* const*)args, sub_flags, &self->_subpr);
    if (result != 0) { return os.get_last_error(); }

    return EOK;
}

/// Checks if process is running
static bool
cex_os__cmd__is_alive(os_cmd_c* self)
{
    return subprocess_alive(&self->_subpr);
}

/// Terminates the running process
static Exception
cex_os__cmd__kill(os_cmd_c* self)
{
    if (subprocess_alive(&self->_subpr)) {
        if (subprocess_terminate(&self->_subpr) != 0) { return Error.os; }
    }
    return EOK;
}

/// Waits process to end, and get `out_ret_code`, if timeout_sec=0 - infinite wait, raises
/// Error.runtime if out_ret_code != 0
static Exception
cex_os__cmd__join(os_cmd_c* self, u32 timeout_sec, i32* out_ret_code)
{
    uassert(self != NULL);
    Exc result = Error.os;
    int ret_code = 1;

    if (timeout_sec == 0) {
        // timeout_sec == 0 -> infinite wait
        int join_result = subprocess_join(&self->_subpr, &ret_code);
        if (join_result != 0) {
            ret_code = -1;
            result = Error.os;
            goto end;
        }
    } else {
        uassert(timeout_sec < INT32_MAX && "timeout is negative or too high");
        u64 timeout_elapsed_ms = 0;
        u64 timeout_ms = timeout_sec * 1000;
        do {
            if (cex_os__cmd__is_alive(self)) {
                cex_os_sleep(100); // 100 ms sleep
            } else {
                subprocess_join(&self->_subpr, &ret_code);
                break;
            }
            timeout_elapsed_ms += 100;
        } while (timeout_elapsed_ms < timeout_ms);

        if (timeout_elapsed_ms >= timeout_ms) {
            result = Error.timeout;
            if (cex_os__cmd__kill(self)) { // discard
            }
            subprocess_join(&self->_subpr, NULL);
            ret_code = -1;
            goto end;
        }
    }

    if (ret_code != 0) {
        result = Error.runtime;
        goto end;
    }

    result = Error.ok;

end:
    if (out_ret_code) { *out_ret_code = ret_code; }
    subprocess_destroy(&self->_subpr);
    memset(self, 0, sizeof(os_cmd_c));
    return result;
}

/// Get running command stdout stream
static FILE*
cex_os__cmd__fstdout(os_cmd_c* self)
{
    return self->_subpr.stdout_file;
}

/// Get running command stderr stream
static FILE*
cex_os__cmd__fstderr(os_cmd_c* self)
{
    return self->_subpr.stderr_file;
}

/// Get running command stdin stream
static FILE*
cex_os__cmd__fstdin(os_cmd_c* self)
{
    return self->_subpr.stdin_file;
}

/// Read all output from process stdout, NULL if stdout is not available
static char*
cex_os__cmd__read_all(os_cmd_c* self, IAllocator allc)
{
    uassert(self != NULL);
    uassert(allc != NULL);
    if (self->_subpr.stdout_file) {
        str_s out = { 0 };
        if (io.fread_all(self->_subpr.stdout_file, &out, allc)) { return NULL; }
        return out.buf;
    }
    return NULL;
}

/// Read line from process stdout, NULL if stdout is not available
static char*
cex_os__cmd__read_line(os_cmd_c* self, IAllocator allc)
{
    uassert(self != NULL);
    uassert(allc != NULL);
    if (self->_subpr.stdout_file) {
        str_s out = { 0 };
        if (io.fread_line(self->_subpr.stdout_file, &out, allc)) { return NULL; }
        return out.buf;
    }
    return NULL;
}

/// Writes line to the process stdin
static Exception
cex_os__cmd__write_line(os_cmd_c* self, char* line)
{
    uassert(self != NULL);
    if (line == NULL) { return Error.argument; }

    if (self->_subpr.stdin_file == NULL) { return Error.not_found; }

    e$except_silent (err, io.file.writeln(self->_subpr.stdin_file, line)) { return err; }
    fflush(self->_subpr.stdin_file);

    return EOK;
}

/// Check if `cmd_exe` program name exists in PATH. cmd_exe can be absolute, or simple command name,
/// e.g. `cat`
static bool
cex_os__cmd__exists(char* cmd_exe)
{
    if (cmd_exe == NULL || cmd_exe[0] == '\0') { return false; }
    mem$scope(tmem$, _)
    {
#ifdef _WIN32
        char* extensions[] = { ".exe", ".cmd", ".bat" };
        bool has_ext = str.find(os.path.basename(cmd_exe, _), ".") != NULL;

        if (str.find(cmd_exe, "/") != NULL || str.find(cmd_exe, "\\") != NULL) {
            if (has_ext) {
                return os.path.exists(cmd_exe);
            } else {
                for$each (ext, extensions) {
                    char* exe = str.fmt(_, "%s%s", cmd_exe, ext);
                    os_fs_stat_s stat = os.fs.stat(exe);
                    if (stat.is_valid && stat.is_file) { return true; }
                }
                return false;
            }
        }

        str_s path_env = str.sstr(os.env.get("PATH", NULL));
        if (path_env.buf == NULL) { return false; }

        for$iter (str_s, it, str.slice.iter_split(path_env, ";", &it.iterator)) {
            if (has_ext) {
                char* exe = str.fmt(_, "%S/%s", it.val, cmd_exe);
                os_fs_stat_s stat = os.fs.stat(exe);
                if (stat.is_valid && stat.is_file) { return true; }
            } else {
                for$each (ext, extensions) {
                    char* exe = str.fmt(_, "%S/%s%s", it.val, cmd_exe, ext);
                    os_fs_stat_s stat = os.fs.stat(exe);
                    if (stat.is_valid && stat.is_file) { return true; }
                }
            }
        }
#else
        if (str.find(cmd_exe, "/") != NULL) {
            os_fs_stat_s stat = os.fs.stat(cmd_exe);
            if (stat.is_valid && stat.is_file && access(cmd_exe, X_OK) == 0) {
                return true; // check if executable
            }
            return false;
        }

        str_s path_env = str.sstr(os.env.get("PATH", NULL));
        if (path_env.buf == NULL) { return false; }

        for$iter (str_s, it, str.slice.iter_split(path_env, ":", &it.iterator)) {
            char* exe = str.fmt(_, "%S/%s", it.val, cmd_exe);
            os_fs_stat_s stat = os.fs.stat(exe);
            if (stat.is_valid && stat.is_file && access(exe, X_OK) == 0) { return true; }
        }
#endif
    }
    return false;
}

/// Run command using arguments array and resulting os_cmd_c
static Exception
cex_os__cmd__run(char** args, usize args_len, os_cmd_c* out_cmd)
{
    uassert(out_cmd != NULL);
    memset(out_cmd, 0, sizeof(os_cmd_c));

    if (args == NULL || args_len == 0) {
        return e$raise(Error.argument, "`args` argument is empty or null");
    }
    if (args_len == 1 || args[args_len - 1] != NULL) {
        return e$raise(Error.argument, "`args` last item must be a NULL");
    }

    for (u32 i = 0; i < args_len - 1; i++) {
        if (args[i] == NULL || args[i][0] == '\0') {
            return e$raise(
                Error.argument,
                "`args` item[%d] is NULL/empty, which may indicate string operation failure",
                i
            );
        }
    }


#ifdef _WIN32
    Exc result = Error.runtime;

    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(STARTUPINFO);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    mem$scope(tmem$, _)
    {
        sbuf_c cmd = sbuf.create(1024, _);
        for (u32 i = 0; i < args_len - 1; i++) {
            if (str.find(args[i], " ") || str.find(args[i], "\"")) {
                char* escaped_arg = str.replace(args[i], "\"", "\\\"", _);
                e$except_silent (err, sbuf.appendf(&cmd, "\"%s\" ", escaped_arg)) {
                    result = err;
                    goto end;
                }
            } else {
                e$except_silent (err, sbuf.appendf(&cmd, "%s ", args[i])) {
                    result = err;
                    goto end;
                }
            }
        }

        if (!CreateProcessA(
                NULL, // Application name (use command line)
                cmd,  // Command line
                NULL, // Process security attributes
                NULL, // Thread security attributes
                TRUE, // Inherit handles
                0,    // Creation flags
                NULL, // Environment
                NULL, // Current directory
                &si,  // Startup info
                &pi   // Process information
            )) {
            result = os.get_last_error();
            goto end;
        }
    }

    *out_cmd = (os_cmd_c){ ._is_subprocess = false,
                           ._subpr = {
                               .hProcess = pi.hProcess,
                               .alive = 1,
                           } };
    CloseHandle(pi.hThread);
    result = EOK;
end:
    return result;
#else
    pid_t cpid = fork();
    if (cpid < 0) { return e$raise(Error.os, "Could not fork child process: %s", strerror(errno)); }

    if (cpid == 0) {
        if (execvp(args[0], (char* const*)args) < 0) {
            log$error("Could not exec child process: %s\n", strerror(errno));
            exit(1);
        }
        uassert(false && "unreachable");
    }

    *out_cmd = (os_cmd_c){ ._is_subprocess = false,
                           ._subpr = {
                               .child = cpid,
                               .alive = 1,
                           } };
    return EOK;

#endif
}

/// Returns current OS platform, returns enum of OSPlatform__*, e.g. OSPlatform__win,
/// OSPlatform__linux, OSPlatform__macos, etc..
static OSPlatform_e
cex_os__platform__current(void)
{
#if defined(_WIN32)
    return OSPlatform__win;
#elif defined(__linux__)
    return OSPlatform__linux;
#elif defined(__APPLE__) || defined(__MACH__)
    return OSPlatform__macos;
#elif defined(__unix__)
#    if defined(__FreeBSD__)
    return OSPlatform__freebsd;
#    elif defined(__NetBSD__)
    return OSPlatform__netbsd;
#    elif defined(__OpenBSD__)
    return OSPlatform__openbsd;
#    elif defined(__ANDROID__)
    return OSPlatform__android;
#    elif defined(__EMSCRIPTEN__)
    return OSPlatform__wasm;
#    else
#        error "Untested platform. Need more?"
#    endif
#elif defined(__wasm__)
    return OSPlatform__wasm;
#elif defined(__EMSCRIPTEN__)
    return OSPlatform__wasm;
#else
#    error "Untested platform. Need more?"
#endif
}

/// Returns string name of current platform
static char*
cex_os__platform__current_str(void)
{
    return os.platform.to_str(os.platform.current());
}

/// Converts platform name to enum
static OSPlatform_e
cex_os__platform__from_str(char* name)
{
    if (name == NULL || name[0] == '\0') { return OSPlatform__unknown; }
    for (u32 i = 1; i < OSPlatform__count; i++) {
        if (str.eq((char*)OSPlatform_str[i], name)) { return (OSPlatform_e)i; }
    }
    return OSPlatform__unknown;
    ;
}

/// Converts platform enum to name
static char*
cex_os__platform__to_str(OSPlatform_e platform)
{
    if (unlikely(platform <= OSPlatform__unknown || platform >= OSPlatform__count)) { return NULL; }
    return (char*)OSPlatform_str[platform];
}

/// Returns OSArch from string
static OSArch_e
cex_os__platform__arch_from_str(char* name)
{
    if (name == NULL || name[0] == '\0') { return OSArch__unknown; }
    for (u32 i = 1; i < OSArch__count; i++) {
        if (str.eq((char*)OSArch_str[i], name)) { return (OSArch_e)i; }
    }
    return OSArch__unknown;
    ;
}

/// Converts arch to string
static char*
cex_os__platform__arch_to_str(OSArch_e platform)
{
    if (unlikely(platform <= OSArch__unknown || platform >= OSArch__count)) { return NULL; }
    return (char*)OSArch_str[platform];
}

const struct __cex_namespace__os os = {
    // Autogenerated by CEX
    // clang-format off

    .get_last_error = cex_os_get_last_error,
    .sleep = cex_os_sleep,
    .timer = cex_os_timer,

    .cmd = {
        .create = cex_os__cmd__create,
        .exists = cex_os__cmd__exists,
        .fstderr = cex_os__cmd__fstderr,
        .fstdin = cex_os__cmd__fstdin,
        .fstdout = cex_os__cmd__fstdout,
        .is_alive = cex_os__cmd__is_alive,
        .join = cex_os__cmd__join,
        .kill = cex_os__cmd__kill,
        .read_all = cex_os__cmd__read_all,
        .read_line = cex_os__cmd__read_line,
        .run = cex_os__cmd__run,
        .write_line = cex_os__cmd__write_line,
    },

    .env = {
        .get = cex_os__env__get,
        .set = cex_os__env__set,
    },

    .fs = {
        .chdir = cex_os__fs__chdir,
        .copy = cex_os__fs__copy,
        .copy_tree = cex_os__fs__copy_tree,
        .dir_walk = cex_os__fs__dir_walk,
        .find = cex_os__fs__find,
        .getcwd = cex_os__fs__getcwd,
        .mkdir = cex_os__fs__mkdir,
        .mkpath = cex_os__fs__mkpath,
        .remove = cex_os__fs__remove,
        .remove_tree = cex_os__fs__remove_tree,
        .rename = cex_os__fs__rename,
        .stat = cex_os__fs__stat,
    },

    .path = {
        .abs = cex_os__path__abs,
        .basename = cex_os__path__basename,
        .dirname = cex_os__path__dirname,
        .exists = cex_os__path__exists,
        .join = cex_os__path__join,
        .split = cex_os__path__split,
    },

    .platform = {
        .arch_from_str = cex_os__platform__arch_from_str,
        .arch_to_str = cex_os__platform__arch_to_str,
        .current = cex_os__platform__current,
        .current_str = cex_os__platform__current_str,
        .from_str = cex_os__platform__from_str,
        .to_str = cex_os__platform__to_str,
    },

    // clang-format on
};
#endif



/*
*                          src/cex_code_gen.c
*/
#if !defined(cex$enable_minimal)


void
_cex__codegen_indent(_cex__codegen_s* cg)
{
    if (unlikely(cg->error != EOK)) { return; }
    for (u32 i = 0; i < cg->indent; i++) {
        Exc err = sbuf.append(cg->buf, " ");
        if (unlikely(err != EOK && cg->error != EOK)) { cg->error = err; }
    }
}

#    define cg$printva(cg) /* temp macro! */                                                       \
        do {                                                                                       \
            va_list va;                                                                            \
            va_start(va, format);                                                                  \
            Exc err = sbuf.appendfva(cg->buf, format, va);                                         \
            if (unlikely(err != EOK && cg->error != EOK)) { cg->error = err; }                     \
            va_end(va);                                                                            \
        } while (0)

void
_cex__codegen_print(_cex__codegen_s* cg, bool rep_new_line, char* format, ...)
{
    if (unlikely(cg->error != EOK)) { return; }
    if (rep_new_line) {
        usize slen = sbuf.len(cg->buf);
        if (slen && cg->buf[0][slen - 1] == '\n') { sbuf.shrink(cg->buf, slen - 1); }
    }
    cg$printva(cg);
}

void
_cex__codegen_print_line(_cex__codegen_s* cg, char* format, ...)
{
    if (unlikely(cg->error != EOK)) { return; }
    if (format[0] != '\n') { _cex__codegen_indent(cg); }
    cg$printva(cg);
}

_cex__codegen_s*
_cex__codegen_print_scope_enter(_cex__codegen_s* cg, char* format, ...)
{
    usize slen = sbuf.len(cg->buf);
    if (slen && cg->buf[0][slen - 1] == '\n') { _cex__codegen_indent(cg); }
    cg$printva(cg);
    _cex__codegen_print(cg, false, "%c\n", '{');
    cg->indent += 4;
    return cg;
}

void
_cex__codegen_print_scope_exit(_cex__codegen_s** cgptr)
{
    uassert(*cgptr != NULL);
    _cex__codegen_s* cg = *cgptr;

    if (cg->indent >= 4) { cg->indent -= 4; }
    _cex__codegen_indent(cg);
    _cex__codegen_print(cg, false, "%c\n", '}');
}


_cex__codegen_s*
_cex__codegen_print_case_enter(_cex__codegen_s* cg, char* format, ...)
{
    _cex__codegen_indent(cg);
    cg$printva(cg);
    _cex__codegen_print(cg, false, ": %c\n", '{');
    cg->indent += 4;
    return cg;
}

void
_cex__codegen_print_case_exit(_cex__codegen_s** cgptr)
{
    uassert(*cgptr != NULL);
    _cex__codegen_s* cg = *cgptr;

    if (cg->indent >= 4) { cg->indent -= 4; }
    _cex__codegen_indent(cg);
    _cex__codegen_print_line(cg, "break;\n", '}');
    _cex__codegen_indent(cg);
    _cex__codegen_print(cg, false, "%c\n", '}');
}

#    undef cg$printva

#endif



/*
*                          src/cexy.c
*/
#if !defined(cex$enable_minimal)

#include <stdbool.h>
#include <stdio.h>
#if defined(CEX_BUILD) || defined(CEX_NEW)

#    include <ctype.h>
#    include <time.h>

static void
cexy_build_self(int argc, char** argv, char* cex_source)
{
    mem$scope(tmem$, _)
    {
        uassert(str.ends_with(argv[0], "cex") || str.ends_with(argv[0], "cex.exe"));
        char* bin_path = argv[0];
        bool has_darg_before_cmd = (argc > 1 && str.starts_with(argv[1], "-D"));
        (void)has_darg_before_cmd;

#    if cexy$create_compile_flags
        e$except (err, cexy.utils.make_compile_flags("compile_flags.txt", true, NULL)) {}
#    endif

#    ifdef _CEX_SELF_BUILD
        if (!has_darg_before_cmd) {
            log$trace("Checking self build for executable: %s\n", bin_path);
            if (!cexy.src_include_changed(bin_path, cex_source, NULL)) {
                log$trace("%s unchanged, skipping self build\n", cex_source);
                // cex.c/cex.h are up to date no rebuild needed
                return;
            }
        } else {
            log$trace("Passed extra -Dflags to cex command, build now\n");
        }
#    endif

        log$info("Rebuilding self: %s -> %s\n", cex_source, bin_path);
        char* old_name = str.fmt(_, "%s.old", bin_path);
        if (os.path.exists(bin_path)) {
            if (os.fs.remove(old_name)) {}
            e$except (err, os.fs.rename(bin_path, old_name)) { goto fail_recovery; }
        }
        arr$(char*) args = arr$new(args, _, .capacity = 64);
        sbuf_c dargs_sbuf = sbuf.create(256, _);
        arr$pushm(args, cexy$cex_self_cc, "-D_CEX_SELF_BUILD", "-g");
        e$goto(sbuf.append(&dargs_sbuf, "-D_CEX_SELF_DARGS=\""), err);

        i32 dflag_idx = 1;
        for$each (darg, argv + 1, argc - 1) {
            if (str.starts_with(darg, "-D")) {
                if (!str.eq(darg, "-D")) {
                    arr$push(args, darg);
                    e$goto(sbuf.appendf(&dargs_sbuf, "%s ", darg), fail_recovery);
                }
                dflag_idx++;
            } else {
                break; // stop at first non -D<flag>
            }
        }
        if (dflag_idx > 1 && (dflag_idx >= argc || !str.eq(argv[dflag_idx], "config"))) {
            log$error("Expected config command after passing -D<ARG> to ./cex\n");
            goto fail_recovery;
        }
#    ifdef _CEX_SELF_DARGS
        if (dflag_idx == 1) {
            // new compilation no flags (maybe due to source changes, we need to maintain old flags)
            log$trace("Preserving CEX_SELF_DARGS: %s\n", _CEX_SELF_DARGS);
            for$iter (
                str_s,
                it,
                str.slice.iter_split(str.sstr(_CEX_SELF_DARGS), " ", &it.iterator)
            ) {
                str_s clean_it = str.slice.strip(it.val);
                if (clean_it.len == 0) { continue; }
                if (!str.slice.starts_with(clean_it, str$s("-D"))) { continue; }
                if (str.slice.eq(clean_it, str$s("-D"))) { continue; }
                auto _darg = str.fmt(_, "%S", clean_it);
                arr$push(args, _darg);
                e$goto(sbuf.appendf(&dargs_sbuf, "%s ", _darg), err);
            }
        }
#    endif
        e$goto(sbuf.append(&dargs_sbuf, "\""), err);

        arr$push(args, dargs_sbuf);

        char* custom_args[] = { cexy$cex_self_args };
        arr$pusha(args, custom_args);

        arr$pushm(args, "-o", bin_path, cex_source, NULL);
        _os$args_print("CMD:", args, arr$len(args));
        os_cmd_c _cmd = { 0 };
        e$except (err, os.cmd.run(args, arr$len(args), &_cmd)) { goto fail_recovery; }
        e$except (err, os.cmd.join(&_cmd, 0, NULL)) { goto fail_recovery; }

        // All good new build successful, remove old binary
        if (os.fs.remove(old_name)) {}

        // run rebuilt cex executable again
        arr$clear(args);
        arr$pushm(args, bin_path);
        for$each (darg, argv + dflag_idx, argc - dflag_idx) { arr$push(args, darg); }
        arr$pushm(args, NULL);
        _os$args_print("CMD:", args, arr$len(args));
        e$except (err, os.cmd.run(args, arr$len(args), &_cmd)) { goto err; }
        if (os.cmd.join(&_cmd, 0, NULL)) { goto err; }
        exit(0); // unconditionally exit after build was successful
    fail_recovery:
        if (os.path.exists(old_name)) {
            e$except (err, os.fs.rename(old_name, bin_path)) { goto err; }
        }
        goto err;
    err:
        exit(1);
    }
}

static bool
cexy_src_include_changed(char* target_path, char* src_path, arr$(char*) alt_include_path)
{
    if (unlikely(target_path == NULL)) {
        log$error("target_path is NULL\n");
        return false;
    }
    if (unlikely(src_path == NULL)) {
        log$error("src_path is NULL\n");
        return false;
    }

    auto src_meta = os.fs.stat(src_path);
    if (!src_meta.is_valid) {
        (void)e$raise(src_meta.error, "Error src: %s", src_path);
        return false;
    } else if (!src_meta.is_file || src_meta.is_symlink) {
        (void)e$raise("Bad type", "src is not a file: %s", src_path);
        return false;
    }

    auto target_meta = os.fs.stat(target_path);
    if (!target_meta.is_valid) {
        if (target_meta.error == Error.not_found) {
            return true;
        } else {
            (void)e$raise(target_meta.error, "target_path: '%s'", target_path);
            return false;
        }
    } else if (!target_meta.is_file || target_meta.is_symlink) {
        (void)e$raise("Bad type", "target_path is not a file: %s", target_path);
        return false;
    }

    if (src_meta.mtime > target_meta.mtime) {
        // File itself changed
        log$debug("Src changed: %s\n", src_path);
        return true;
    }

    if (!str.ends_with(src_path, ".c") && !str.ends_with(src_path, ".h")) {
        // We only parse includes for appropriate .c/.h files
        return false;
    }

    mem$scope(tmem$, _)
    {
        arr$(char*) incl_path = arr$new(incl_path, _);
        if (arr$len(alt_include_path) > 0) {
            for$each (p, alt_include_path) {
                arr$push(incl_path, p);
                if (!os.path.exists(p)) { log$warn("alt_include_path not exists: %s\n", p); }
            }
        } else {
            char* def_incl_path[] = { cexy$cc_include };
            for$each (p, def_incl_path) {
                char* clean_path = p;
                if (str.starts_with(p, "-I")) {
                    clean_path = p + 2;
                } else if (str.starts_with(p, "-iquote=")) {
                    clean_path = p + strlen("-iquote=");
                }
                if (!os.path.exists(clean_path)) {
                    log$trace("cexy$cc_include not exists: %s\n", clean_path);
                    continue;
                }
                arr$push(incl_path, clean_path);
            }
            // Adding relative to src_path directory as a search target
            arr$push(incl_path, os.path.dirname(src_path, _));
        }

        char* code = io.file.load(src_path, _);
        if (code == NULL) {
            (void)e$raise("IOError", "src is not a file: '%s'", src_path);
            return false;
        }

        (void)CexTkn_str;
        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        while ((t = CexParser.next_token(&lx)).type) {
            if (t.type != CexTkn__preproc) { continue; }
            if (str.slice.starts_with(t.value, str$s("include"))) {
                str_s incf = str.slice.sub(t.value, strlen("include"), 0);
                incf = str.slice.strip(incf);
                if (incf.len <= 4) { // <.h>
                    log$warn(
                        "Bad include in: %s (item: %S at line: %d)\n",
                        src_path,
                        t.value,
                        lx.line
                    );
                    continue;
                }
                log$trace("Processing include: '%S'\n", incf);
                if (!str.slice.match(incf, "\"*.[hc]\"*")) {
                    // system includes skipped
                    log$trace("Skipping include: '%S'\n", incf);
                    continue;
                }
                incf = str.slice.sub(incf, 1, 0);
                incf = str.slice.sub(incf, 0, str.slice.index_of(incf, str$s("\"")) - 1);

                mem$scope(tmem$, _)
                {
                    char extensions[] = { 'c', 'h' };
                    for$each (ext, extensions) {
                        char* inc_fn = str.fmt(_, "%S%c", incf, ext);
                        uassert(inc_fn != NULL);
                        for$each (inc_dir, incl_path) {
                            char* try_path = os$path_join(_, inc_dir, inc_fn);
                            uassert(try_path != NULL);
                            auto src_meta = os.fs.stat(try_path);
                            log$trace("Probing include: %s\n", try_path);
                            if (src_meta.is_valid && src_meta.mtime > target_meta.mtime) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

static bool
cexy_src_changed(char* target_path, char** src_array, usize src_array_len)
{
    if (unlikely(src_array == NULL || src_array_len == 0)) {
        if (src_array == NULL) {
            log$error("src_array is NULL, which may indicate error\n");
        } else {
            log$error("src_array is empty\n");
        }
        return false;
    }
    if (unlikely(target_path == NULL)) {
        log$error("target_path is NULL\n");
        return false;
    }
    auto target_ftype = os.fs.stat(target_path);
    if (!target_ftype.is_valid) {
        if (target_ftype.error == Error.not_found) {
            log$trace("Target '%s' not exists, needs build.\n", target_path);
            return true;
        } else {
            (void)e$raise(target_ftype.error, "target_path: '%s'", target_path);
            return false;
        }
    } else if (!target_ftype.is_file || target_ftype.is_symlink) {
        (void)e$raise("Bad type", "target_path is not a file: %s", target_path);
        return false;
    }

    bool has_error = false;
    for$each (p, src_array, src_array_len) {
        auto ftype = os.fs.stat(p);
        if (!ftype.is_valid) {
            (void)e$raise(ftype.error, "Error src: %s", p);
            has_error = true;
        } else if (!ftype.is_file || ftype.is_symlink) {
            (void)e$raise("Bad type", "src is not a file: %s", p);
            has_error = true;
        } else {
            if (has_error) { continue; }
            if (ftype.mtime > target_ftype.mtime) {
                log$trace("Source changed '%s'\n", p);
                return true;
            }
        }
    }

    return false;
}

static char*
cexy_target_make(char* src_path, char* build_dir, char* name_or_extension, IAllocator allocator)
{
    uassert(allocator != NULL);

    if (src_path == NULL || src_path[0] == '\0') {
        log$error("src_path is NULL or empty\n");
        return NULL;
    }
    if (build_dir == NULL) {
        log$error("build_dir is NULL\n");
        return NULL;
    }
    if (name_or_extension == NULL || name_or_extension[0] == '\0') {
        log$error("name_or_extension is NULL or empty\n");
        return NULL;
    }
    if (!os.path.exists(src_path)) {
        log$error("src_path does not exist: '%s'\n", src_path);
        return NULL;
    }

    char* result = NULL;
    if (name_or_extension[0] == '.') {
        // starts_with .ext, make full path as following: build_dir/src_path/src_file.ext[.exe]
        result = str.fmt(
            allocator,
            "%s%c%s%s%s",
            build_dir,
            os$PATH_SEP,
            src_path,
            name_or_extension,
            cexy$build_ext_exe
        );
        uassert(result != NULL && "memory error");
    } else {
        // probably a program name, make full path: build_dir/name_or_extension[.exe]
        result = str.fmt(
            allocator,
            "%s%c%s%s",
            build_dir,
            os$PATH_SEP,
            name_or_extension,
            cexy$build_ext_exe
        );
        uassert(result != NULL && "memory error");
    }
    e$except (err, os.fs.mkpath(result)) { mem$free(allocator, result); }

    return result;
}

Exception
cexy__fuzz__create(char* target)
{
    if (!str.slice.starts_with(os.path.split(target, false), str$s("fuzz_"))) {
        return e$raise(Error.argument, "Fuzz file must start with `fuzz_` prefix, got: %s", target);
    }
    if (os.path.exists(target)) {
        return e$raise(Error.exists, "Fuzz file already exists: %s", target);
    }
    e$ret(os.fs.mkpath(target));

    mem$scope(tmem$, _)
    {
        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        cg$pn("#define CEX_IMPLEMENTATION");
        cg$pn("#include \"cex.h\"");

        cg$pn("");
        cg$pn("/*");
        cg$func("fuzz$setup(void)", "")
        {
            cg$pn("// This function allows programmatically seed new corpus for fuzzer");
            cg$pn("io.printf(\"CORPUS: %s\\n\", fuzz$corpus_dir);");
            cg$scope("memcg$scope(tmem$, _)", "")
            {
                cg$pn("char* fn = str.fmt(_, \"%s/my_case\", fuzz$corpus_dir);");
                cg$pn("(void)fn;");
                cg$pn("// io.file.save(fn, \"my seed data\");");
            }
        }
        cg$pn("*/");

        cg$pn("");
        cg$func("int\nfuzz$case(const u8* data, usize size)", "")
        {
            cg$pn("// TODO: do your stuff based on input data and size");
            cg$if("size > 2 && data[0] == 'C' && data[1] == 'E' && data[2] == 'X'", "")
            {
                cg$pn("__builtin_trap();");
            }
            cg$pn("return 0;");
        }

        cg$pn("");
        cg$pn("fuzz$main();");

        e$ret(io.file.save(target, buf));
    }
    return EOK;
}

Exception
cexy__test__create(char* target, bool include_sample)
{
    if (os.path.exists(target)) {
        return e$raise(Error.exists, "Test file already exists: %s", target);
    }
    if (str.eq(target, "all") || str.find(target, "*")) {
        return e$raise(
            Error.argument,
            "You must pass exact file path, not pattern, got: %s",
            target
        );
    }
    e$ret(os.fs.mkpath(target));

    mem$scope(tmem$, _)
    {
        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        cg$pn("#define CEX_IMPLEMENTATION");
        cg$pn("#define CEX_TEST");
        cg$pn("#include \"cex.h\"");
        if (include_sample) { cg$pn("#include \"lib/mylib.c\""); }
        cg$pn("");
        cg$pn("//test$setup_case() {return EOK;}");
        cg$pn("//test$teardown_case() {return EOK;}");
        cg$pn("//test$setup_suite() {return EOK;}");
        cg$pn("//test$teardown_suite() {return EOK;}");
        cg$pn("");
        cg$scope("test$case(%s)", (include_sample) ? "mylib_test_case" : "my_test_case")
        {
            if (include_sample) {
                cg$pn("tassert_eq(mylib_add(1, 2), 3);");
                cg$pn("// Next will be available after calling `cex process lib/mylib.c`");
                cg$pn("// tassert_eq(mylib.add(1, 2), 3);");
            } else {
                cg$pn("tassert_eq(1, 0);");
            }
            cg$pn("return EOK;");
        }
        cg$pn("");
        cg$pn("test$main();");

        e$ret(io.file.save(target, buf));
    }
    return EOK;
}

Exception
cexy__test__clean(char* target)
{

    if (str.eq(target, "all")) {
        log$info("Cleaning all tests\n");
        e$ret(os.fs.remove_tree(cexy$build_dir "/tests/"));
    } else {
        log$info("Cleaning target: %s\n", target);
        if (!os.path.exists(target)) {
            return e$raise(Error.exists, "Test target not exists: %s", target);
        }

        mem$scope(tmem$, _)
        {
            char* test_target = cexy.target_make(target, cexy$build_dir, ".test", _);
            e$ret(os.fs.remove(test_target));
        }
    }
    return EOK;
}

Exception
cexy__test__make_target_pattern(char** target)
{
    if (target == NULL) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '(null)', expected all or tests/test_some_file.c"
        );
    }
    if (str.eq(*target, "all")) { *target = "tests/test_*.c"; }

    if (!str.match(*target, "*test*.c")) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            *target
        );
    }
    return EOK;
}

Exception
cexy__test__run(char* target, bool is_debug, int argc, char** argv)
{
    Exc result = EOK;
    u32 n_tests = 0;
    u32 n_failed = 0;
    mem$scope(tmem$, _)
    {
        if (str.ends_with(target, "test_*.c")) {
            // quiet mode
            io.printf("-------------------------------------\n");
            io.printf("Running Tests: %s\n", target);
            io.printf("-------------------------------------\n\n");
        } else {
            if (!os.path.exists(target)) {
                return e$raise(Error.not_found, "Test file not found: %s", target);
            }
        }

        for$each (test_src, os.fs.find(target, true, _)) {
            n_tests++;
            char* test_target = cexy.target_make(test_src, cexy$build_dir, ".test", _);
            arr$(char*) args = arr$new(args, _);
            if (is_debug) { arr$pushm(args, cexy$debug_cmd); }
            arr$pushm(args, test_target, );
            if (str.ends_with(target, "test_*.c")) { arr$push(args, "--quiet"); }
            arr$pusha(args, argv, argc);
            arr$push(args, NULL);
            if (os$cmda(args)) {
                log$error("<<<<<<<<<<<<<<<<<< Test failed: %s\n", test_target);
                n_failed++;
                result = Error.runtime;
            }
        }
    }
    if (str.ends_with(target, "test_*.c")) {
        io.printf("\n-------------------------------------\n");
        io.printf("Total: %d Passed: %d Failed: %d\n", n_tests, n_tests - n_failed, n_failed);
        io.printf("-------------------------------------\n\n");
    }
    return result;
}

static int
_cexy__decl_comparator(const void* a, const void* b)
{
    cex_decl_s** _a = (cex_decl_s**)a;
    cex_decl_s** _b = (cex_decl_s**)b;

    // Makes str__sub__ to be placed after
    isize isub_a = str.slice.index_of(_a[0]->name, str$s("__"));
    isize isub_b = str.slice.index_of(_b[0]->name, str$s("__"));
    if (unlikely(isub_a != isub_b)) {
        if (isub_a != -1) {
            return 1;
        } else {
            return -1;
        }
    }

    return str.slice.qscmp(&_a[0]->name, &_b[0]->name);
}

static str_s
_cexy__process_make_brief_docs(cex_decl_s* decl)
{
    str_s brief_str = { 0 };
    if (!decl->docs.buf) { return brief_str; }

    if (str.slice.starts_with(decl->docs, str$s("///"))) {
        // doxygen style ///
        brief_str = str.slice.strip(str.slice.sub(decl->docs, 3, 0));
    } else if (str.slice.match(decl->docs, "/\\*[\\*!]*")) {
        // doxygen style /** or /*!
        for$iter (
            str_s,
            it,
            str.slice.iter_split(str.slice.sub(decl->docs, 3, -2), "\n", &it.iterator)
        ) {
            str_s line = str.slice.strip(it.val);
            if (line.len == 0) { continue; }
            brief_str = line;
            break;
        }
    }
    isize bridx = (str.slice.index_of(brief_str, str$s("@brief")));
    if (bridx == -1) { bridx = str.slice.index_of(brief_str, str$s("\\brief")); }
    if (bridx != -1) {
        brief_str = str.slice.strip(str.slice.sub(brief_str, bridx + strlen("@brief"), 0));
    }

    return brief_str;
}

static Exception
_cexy__process_gen_struct(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
{
    cg$init(out_buf);
    str_s subn = { 0 };

    cg$scope("struct __cex_namespace__%S ", ns_prefix)
    {
        cg$pn("// Autogenerated by CEX");
        cg$pn("// clang-format off");
        cg$pn("");
        for$each (it, decls) {
            str_s clean_name = it->name;
            if (str.slice.starts_with(clean_name, str$s("cex_"))) {
                clean_name = str.slice.sub(clean_name, 4, 0);
            }
            clean_name = str.slice.sub(clean_name, ns_prefix.len + 1, 0);

            if (str.slice.starts_with(clean_name, str$s("_"))) {
                // sub-namespace
                isize ns_end = str.slice.index_of(clean_name, str$s("__"));
                uassert(ns_end >= 0);
                str_s new_ns = str.slice.sub(clean_name, 1, ns_end);
                if (!str.slice.eq(new_ns, subn)) {
                    if (subn.buf != NULL) {
                        // End previous sub-namespace
                        cg$dedent();
                        cg$pf("} %S;", subn);
                    }

                    // new subnamespace begin
                    cg$pn("");
                    cg$pf("struct {", subn);
                    cg$indent();
                    subn = new_ns;
                }
                clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
            }
            str_s brief_str = _cexy__process_make_brief_docs(it);
            if (brief_str.len) { 
                // Handling comments + special multi-line treatment
                for$iter(str_s, it, str.slice.iter_split(brief_str, "\n", &it.iterator)) {
                    str_s _line = str.slice.strip(it.val);
                    if (str.slice.starts_with(_line, str$s("///"))) {
                        _line = str.slice.sub(_line, 3, 0);
                    }
                    if (str.slice.starts_with(_line, str$s("* "))) {
                        _line = str.slice.sub(_line, 2, 0);
                    }
                    _line = str.slice.strip(_line);
                    if (_line.len) {
                        cg$pf("/// %S", _line); 
                    }
                }
            }
            cg$pf("%-15s (*%S)(%s);", it->ret_type, clean_name, it->args);
        }

        if (subn.buf != NULL) {
            // End previous sub-namespace
            cg$dedent();
            cg$pf("} %S;", subn);
        }
        cg$pn("");
        cg$pn("// clang-format on");
    }
    cg$pa("%s", ";");

    if (!cg$is_valid()) { return e$raise(Error.runtime, "Code generation error occured\n"); }
    return EOK;
}

static Exception
_cexy__process_gen_var_def(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
{
    cg$init(out_buf);
    str_s subn = { 0 };

    cg$scope("const struct __cex_namespace__%S %S = ", ns_prefix, ns_prefix)
    {
        cg$pn("// Autogenerated by CEX");
        cg$pn("// clang-format off");
        cg$pn("");
        for$each (it, decls) {
            str_s clean_name = it->name;
            if (str.slice.starts_with(clean_name, str$s("cex_"))) {
                clean_name = str.slice.sub(clean_name, 4, 0);
            }
            clean_name = str.slice.sub(clean_name, ns_prefix.len + 1, 0);

            if (str.slice.starts_with(clean_name, str$s("_"))) {
                // sub-namespace
                isize ns_end = str.slice.index_of(clean_name, str$s("__"));
                e$assert(ns_end >= 0);
                str_s new_ns = str.slice.sub(clean_name, 1, ns_end);
                if (!str.slice.eq(new_ns, subn)) {
                    if (subn.buf != NULL) {
                        // End previous sub-namespace
                        cg$dedent();
                        cg$pn("},");
                    }

                    // new subnamespace begin
                    cg$pn("");
                    cg$pf(".%S = {", new_ns);
                    cg$indent();
                    subn = new_ns;
                }
                clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
            }
            cg$pf(".%S = %S,", clean_name, it->name);
        }

        if (subn.buf != NULL) {
            // End previous sub-namespace
            cg$dedent();
            cg$pf("},", subn);
        }
        cg$pn("");
        cg$pn("// clang-format on");
    }
    cg$pa("%s", ";");

    if (!cg$is_valid()) { return e$raise(Error.runtime, "Code generation error occured\n"); }
    return EOK;
}

static Exception
_cexy__process_update_code(
    char* code_file,
    bool only_update,
    sbuf_c cex_h_struct,
    sbuf_c cex_h_var_decl,
    sbuf_c cex_c_var_def
)
{
    mem$scope(tmem$, _)
    {
        char* code = io.file.load(code_file, _);
        e$assert(code && "failed loading code");

        bool is_header = str.ends_with(code_file, ".h");

        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        sbuf_c new_code = sbuf.create(10 * 1024 + (lx.content_end - lx.content), _);
        CexParser.reset(&lx);

        str_s code_buf = { .buf = lx.content, .len = 0 };
        bool has_module_def = false;
        bool has_module_decl = false;
        bool has_module_struct = false;
        arr$(cex_token_s) items = arr$new(items, _);

#    define $dump_prev()                                                                           \
        code_buf.len = t.value.buf - code_buf.buf - ((t.value.buf > lx.content) ? 1 : 0);          \
        if (code_buf.buf != NULL)                                                                  \
            e$ret(sbuf.appendf(&new_code, "%.*S\n", code_buf.len, code_buf));                      \
        code_buf = (str_s){ 0 }
#    define $dump_prev_comment()                                                                   \
        for$each (it, items) {                                                                     \
            if (it.type == CexTkn__comment_single || it.type == CexTkn__comment_multi) {           \
                e$ret(sbuf.appendf(&new_code, "%.*S\n", it.value.len, it.value));                  \
            } else {                                                                               \
                break;                                                                             \
            }                                                                                      \
        }

        while ((t = CexParser.next_entity(&lx, &items)).type) {
            if (t.type == CexTkn__cex_module_def) {
                e$assert(!is_header && "expected in .c file buf got header");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_def) { e$ret(sbuf.appendf(&new_code, "%s\n", cex_c_var_def)); }
                has_module_def = true;
            } else if (t.type == CexTkn__cex_module_decl) {
                e$assert(is_header && "expected in .h file buf got source");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_decl) { e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_var_decl)); }
                has_module_decl = true;
            } else if (t.type == CexTkn__cex_module_struct) {
                e$assert(is_header && "expected in .h file buf got source");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_struct) { e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_struct)); }
                has_module_struct = true;
            } else {
                if (code_buf.buf == NULL) { code_buf.buf = t.value.buf; }
                code_buf.len = t.value.buf - code_buf.buf + t.value.len;
            }
        }
        if (code_buf.len) { e$ret(sbuf.appendf(&new_code, "%.*S\n", code_buf.len, code_buf)); }
        if (!is_header) {
            if (!has_module_def) {
                if (only_update) { return EOK; }
                e$ret(sbuf.appendf(&new_code, "\n%s\n", cex_c_var_def));
            }
            e$ret(io.file.save(code_file, new_code));
        } else {
            if (!has_module_struct) {
                if (only_update) { return EOK; }
                e$ret(sbuf.appendf(&new_code, "\n%s\n", cex_h_struct));
            }
            if (!has_module_decl) {
                if (only_update) { return EOK; }
                e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_var_decl));
            }
            e$ret(io.file.save(code_file, new_code));
        }
    }

#    undef $dump_prev
#    undef $dump_prev_comment
    return EOK;
}

static bool
_cexy__fn_match(str_s fn_name, str_s ns_prefix)
{

    mem$scope(tmem$, _)
    {
        char* fn_sub_pattern = str.fmt(_, "%S__[a-zA-Z0-9+]__*", ns_prefix);
        char* fn_sub_pattern_cex = str.fmt(_, "cex_%S__[a-zA-Z0-9+]__*", ns_prefix);
        char* fn_pattern = str.fmt(_, "%S_*", ns_prefix);
        char* fn_pattern_cex = str.fmt(_, "cex_%S_*", ns_prefix);
        char* fn_private = str.fmt(_, "%S__*", ns_prefix);
        char* fn_private_cex = str.fmt(_, "cex_%S__*", ns_prefix);

        if (str.slice.starts_with(fn_name, str$s("_"))) { return false; }

        if (str.slice.match(fn_name, fn_sub_pattern) ||
            str.slice.match(fn_name, fn_sub_pattern_cex)) {
            return true;
        } else if ((str.slice.match(fn_name, fn_private) ||
                    str.slice.match(fn_name, fn_private_cex)) ||
                   (!str.slice.match(fn_name, fn_pattern_cex) &&
                    !str.slice.match(fn_name, fn_pattern))) {
            return false;
        }
    }

    return true;
}

static str_s
_cexy__fn_dotted(str_s fn_name, char* expected_ns, IAllocator alloc)
{
    str_s clean_name = fn_name;
    if (str.slice.starts_with(clean_name, str$s("cex_"))) {
        clean_name = str.slice.sub(clean_name, 4, 0);
    }
    if (!str.eq(expected_ns, "cex") && !str.slice.starts_with(clean_name, str.sstr(expected_ns))) {
        return (str_s){ 0 };
    }

    isize ns_idx = str.slice.index_of(clean_name, str$s("_"));
    if (ns_idx < 0) { return (str_s){ 0 }; }
    str_s ns = str.slice.sub(clean_name, 0, ns_idx);
    clean_name = str.slice.sub(clean_name, ns.len + 1, 0);

    str_s subn = { 0 };
    if (str.slice.starts_with(clean_name, str$s("_"))) {
        // sub-namespace
        isize ns_end = str.slice.index_of(clean_name, str$s("__"));
        if (ns_end < 0) {
            // looks like a private func, str__something
            return (str_s){ 0 };
        }
        subn = str.slice.sub(clean_name, 1, ns_end);
        clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
    }
    if (subn.len) {
        return str.sstr(str.fmt(alloc, "%S.%S.%S", ns, subn, clean_name));
    } else {
        return str.sstr(str.fmt(alloc, "%S.%S", ns, clean_name));
    }
}

static Exception
cexy__cmd__process(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    char* process_help = ""
    "process command intended for building CEXy interfaces from your source code\n\n"
    "For example: you can create foo_fun1(), foo_fun2(), foo__bar__fun3(), foo__bar__fun4()\n"
    "   these functions will be processed and wrapped to a `foo` namespace, so you can     \n"
    "   access them via foo.fun1(), foo.fun2(), foo.bar.fun3(), foo.bar.fun4()\n\n" 

    "Requirements / caveats: \n"
    "1. You must have foo.c and foo.h in the same folder\n"
    "2. Filename must start with `foo` - namespace prefix\n"
    "3. Each function in foo.c that you'd like to add to namespace must start with `foo_`\n"
    "4. For adding sub-namespace use `foo__subname__` prefix\n"
    "5. Only one level of sub-namespace is allowed\n"
    "6. You may not declare function signature in header, and ony use .c static functions\n"
    "7. Functions with `static inline` are not included into namespace\n" 
    "8. Functions with prefix `foo__some` are considered internal and not included\n"
    "9. New namespace is created when you use exact foo.c argument, `all` just for updates\n"

    ;
    // clang-format on

    char* ignore_kw = cexy$process_ignore_kw;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "process [options] all|path/some_file.c",
        .description = process_help,
        .epilog = "\nUse `all` for updates, and exact path/some_file.c for creating new\n",
        argparse$opt_list(
            argparse$opt_group("Options"),
            argparse$opt_help(),
            argparse$opt(
                &ignore_kw,
                'i',
                "ignore",
                .help = "ignores `keyword` or `keyword()` from processed function signatures\n  uses cexy$process_ignore_kw"
            ),
        ),
    };
    e$ret(argparse.parse(&cmd_args, argc, argv));
    char* target = argparse.next(&cmd_args);

    if (target == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or path/some_file.c",
            target
        );
    }


    bool only_update = true;
    if (str.eq(target, "all")) {
        target = "*.c";
    } else {
        if (!os.path.exists(target)) {
            return e$raise(Error.not_found, "Target file not exists: '%s'", target);
        }
        only_update = false;
    }


    mem$scope(tmem$, _)
    {
        char* build_path = os.path.abs(cexy$build_dir, _);
        char* test_path = os.path.abs("./tests/", _);

        for$each (src_fn, os.fs.find(target, true, _)) {
            if (only_update) {
                char* abspath = os.path.abs(src_fn, _);
                if (str.starts_with(abspath, build_path) || str.starts_with(abspath, test_path)) {
                    continue;
                }
            }
            char* basename = os.path.basename(src_fn, _);
            if (str.starts_with(basename, "test") || str.eq(basename, "cex.c")) { continue; }
            mem$scope(tmem$, _)
            {
                e$assert(str.ends_with(src_fn, ".c") && "file must end with .c");

                char* hdr_fn = str.clone(src_fn, _);
                hdr_fn[str.len(hdr_fn) - 1] = 'h'; // .c -> .h

                str_s ns_prefix = str.sub(os.path.basename(src_fn, _), 0, -2); // src.c -> src
                log$debug(
                    "Cex Processing src: '%s' hdr: '%s' prefix: '%S'\n",
                    src_fn,
                    hdr_fn,
                    ns_prefix
                );
                if (!os.path.exists(hdr_fn)) {
                    if (only_update) {
                        log$debug("CEX skipped (no .h file for: %s)\n", src_fn);
                        continue;
                    } else {
                        return e$raise(Error.not_found, "Header file not exists: '%s'", hdr_fn);
                    }
                }
                char* code = io.file.load(src_fn, _);
                e$assert(code && "failed loading code");
                arr$(cex_token_s) items = arr$new(items, _);
                arr$(cex_decl_s*) decls = arr$new(decls, _, .capacity = 128);

                CexParser_c lx = CexParser.create(code, 0, true);
                cex_token_s t;
                while ((t = CexParser.next_entity(&lx, &items)).type) {
                    if (t.type == CexTkn__error) {
                        return e$raise(
                            Error.integrity,
                            "Error parsing file %s, at line: %d, cursor: %d",
                            src_fn,
                            lx.line,
                            (i32)(lx.cur - lx.content)
                        );
                    }
                    cex_decl_s* d = CexParser.decl_parse(&lx, t, items, cexy$process_ignore_kw, _);
                    if (d == NULL) { continue; }
                    if (d->type != CexTkn__func_def) { continue; }
                    if (d->is_inline && d->is_static) { continue; }
                    if (!_cexy__fn_match(d->name, ns_prefix)) { continue; }
                    log$trace("FN: %S ret_type: '%s' args: '%s'\n", d->name, d->ret_type, d->args);
                    arr$push(decls, d);
                }
                if (arr$len(decls) == 0) {
                    log$info("CEX skipped (no cex decls found in : %s)\n", src_fn);
                    continue;
                }

                arr$sort(decls, _cexy__decl_comparator);

                sbuf_c cex_h_struct = sbuf.create(10 * 1024, _);
                sbuf_c cex_h_var_decl = sbuf.create(1024, _);
                sbuf_c cex_c_var_def = sbuf.create(10 * 1024, _);

                e$ret(sbuf.appendf(
                    &cex_h_var_decl,
                    "CEX_NAMESPACE struct __cex_namespace__%S %S;\n",
                    ns_prefix,
                    ns_prefix
                ));
                e$ret(_cexy__process_gen_struct(ns_prefix, decls, &cex_h_struct));
                e$ret(_cexy__process_gen_var_def(ns_prefix, decls, &cex_c_var_def));
                e$ret(_cexy__process_update_code(
                    src_fn,
                    only_update,
                    cex_h_struct,
                    cex_h_var_decl,
                    cex_c_var_def
                ));
                e$ret(_cexy__process_update_code(
                    hdr_fn,
                    only_update,
                    cex_h_struct,
                    cex_h_var_decl,
                    cex_c_var_def
                ));

                // log$info("cex_h_struct: \n%s\n", cex_h_struct);
                log$info("CEX processed: %s\n", src_fn);
            }
        }
    }
    return EOK;
}

static Exception
cexy__cmd__stats(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    f64 tstart = os.timer();

    // clang-format off
    char* stats_help = ""
    "Parses full project code and calculates lines of code and code quality metrics"
    ;
    // clang-format on

    bool verbose = false;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "stats [options] [src/*.c] [path/some_file.c] [!exclude/*.c]",
        .description = stats_help,
        argparse$opt_list(
            argparse$opt_group("Options"),
            argparse$opt_help(),
            argparse$opt(&verbose, 'v', "verbose", .help = "prints individual file lines as well"),
        ),
    };
    e$ret(argparse.parse(&cmd_args, argc, argv));


    struct code_stats
    {
        u32 n_files;
        u32 n_asserts;
        u32 n_lines_code;
        u32 n_lines_comments;
        u32 n_lines_total;
    } code_stats = { 0 }, test_stats = { 0 };

    mem$scope(tmem$, _)
    {
        hm$(char*, bool) src_files = hm$new(src_files, _, .capacity = 1024);
        hm$(char*, bool) excl_files = hm$new(excl_files, _, .capacity = 128);

        char* target = argparse.next(&cmd_args);
        if (target == NULL) { target = "*.[ch]"; }

        do {
            bool is_exclude = false;
            if (target[0] == '!') {
                if (target[1] == '\0') { continue; }
                is_exclude = true;
                target++;
            }
            for$each (src_fn, os.fs.find(target, true, _)) {
                char* p = os.path.abs(src_fn, _);
                if (is_exclude) {
                    log$trace("Ignoring: %s\n", p);
                    hm$set(excl_files, p, true);
                } else {
                    hm$set(src_files, p, true);
                    // log$trace("Including: %s\n", p);
                }
            }
        } while ((target = argparse.next(&cmd_args)));

        // WARNING: sorting of hashmap items is a dead end, hash indexes get invalidated
        qsort(src_files, hm$len(src_files), sizeof(*src_files), str.qscmp);

        if (verbose) {
            io.printf("Files found: %d excluded: %d\n", hm$len(src_files), hm$len(excl_files));
        }
        for$each (src_fn, src_files) {
            if (hm$getp(excl_files, src_fn.key)) { continue; }

            char* basename = os.path.basename(src_fn.key, _);
            if (str.eq(basename, "cex.h")) { continue; }
            struct code_stats* stats = (str.find(basename, "test") != NULL) ? &test_stats
                                                                            : &code_stats;

            mem$scope(tmem$, _)
            {
                char* code = io.file.load(src_fn.key, _);
                if (!code) { return e$raise(Error.os, "Error opening file: '%s'", src_fn); }
                stats->n_files++;

                if (code[0] != '\0') { stats->n_lines_total++; }
                CexParser_c lx = CexParser.create(code, 0, false);
                cex_token_s t;

                u32 last_line = 0;
                u32 file_loc = 0;
                while ((t = CexParser.next_token(&lx)).type) {
                    if (t.type == CexTkn__error) {
                        return e$raise(
                            Error.integrity,
                            "Error parsing file %s, at line: %d, cursor: %d",
                            src_fn,
                            lx.line,
                            (i32)(lx.cur - lx.content)
                        );
                    }
                    switch (t.type) {
                        case CexTkn__ident:
                            if (t.value.len >= 6) {
                                for (usize i = 0; i < t.value.len; i++) {
                                    t.value.buf[i] = tolower(t.value.buf[i]);
                                }
                                if (str.slice.starts_with(t.value, str$s("uassert")) ||
                                    str.slice.starts_with(t.value, str$s("assert")) ||
                                    str.slice.starts_with(t.value, str$s("ensure")) ||
                                    str.slice.starts_with(t.value, str$s("enforce")) ||
                                    str.slice.starts_with(t.value, str$s("expect")) ||
                                    str.slice.starts_with(t.value, str$s("e$assert")) ||
                                    str.slice.starts_with(t.value, str$s("tassert"))) {
                                    stats->n_asserts++;
                                }
                            }
                            goto def;
                        case CexTkn__comment_multi: {
                            for$each (c, t.value.buf, t.value.len) {
                                if (c == '\n') { stats->n_lines_comments++; }
                            }
                        }
                            fallthrough();
                        case CexTkn__comment_single:
                            stats->n_lines_comments++;
                            break;
                        case CexTkn__preproc: {
                            for$each (c, t.value.buf, t.value.len) {
                                if (c == '\n') {
                                    stats->n_lines_code++;
                                    file_loc++;
                                }
                            }
                        }
                            fallthrough();
                        default:
                        def:
                            if (last_line < lx.line) {
                                stats->n_lines_code++;
                                file_loc++;
                                stats->n_lines_total += (lx.line - last_line);
                                last_line = lx.line;
                            }
                            break;
                    }
                }
                if (verbose) {
                    char* pcur = str.replace(src_fn.key, os.fs.getcwd(_), ".", _);
                    io.printf("%5d loc | %s\n", file_loc, pcur);
                }
            }
        }
    }

    // clang-format off
    io.printf("Project stats (parsed in %0.3fsec)\n", os.timer()-tstart);
    io.printf("--------------------------------------------------------\n");
    io.printf("%-25s|  %10s  |  %10s  |\n", "Metric", "Code   ", "Tests   ");
    io.printf("--------------------------------------------------------\n");
    io.printf("%-25s|  %10d  |  %10d  |\n", "Files", code_stats.n_files, test_stats.n_files);
    io.printf("%-25s|  %10d  |  %10d  |\n", "Asserts", code_stats.n_asserts, test_stats.n_asserts);
    io.printf("%-25s|  %10d  |  %10d  |\n", "Lines of code", code_stats.n_lines_code, test_stats.n_lines_code);
    io.printf("%-25s|  %10d  |  %10d  |\n", "Lines of comments", code_stats.n_lines_comments, test_stats.n_lines_comments);
    io.printf("%-25s|  %10.2f%% |  %10.2f%% |\n", "Asserts per LOC",
        (code_stats.n_lines_code) ? ((double)code_stats.n_asserts/(double)code_stats.n_lines_code*100.0) : 0.0, 
        (test_stats.n_lines_code) ? ((double)test_stats.n_asserts/(double)test_stats.n_lines_code*100.0) : 0.0
        ); 
    io.printf("%-25s|  %10.2f%% |         <<<  |\n", "Total asserts per LOC",
        (code_stats.n_lines_code) ? ((double)(code_stats.n_asserts+test_stats.n_asserts)/(double)code_stats.n_lines_code*100.0) : 0.0 
        );
    io.printf("--------------------------------------------------------\n");
    // clang-format on
    return EOK;
}

static bool
_cexy__is_str_pattern(char* s)
{
    if (s == NULL) { return false; }
    char pat[] = { '*', '?', '(', '[' };

    while (*s) {
        for (u32 i = 0; i < arr$len(pat); i++) {
            if (unlikely(pat[i] == *s)) { return true; }
        }
        s++;
    }
    return false;
}

static int
_cexy__help_qscmp_decls_type(const void* a, const void* b)
{
    // this struct fields must match to cexy.cmd.help() hm$
    const struct
    {
        str_s key;
        cex_decl_s* value;
    }* _a = a;
    typeof(_a) _b = b;
    if (_a->value->type != _b->value->type) {
        return _b->value->type - _a->value->type;
    } else {
        return str.slice.qscmp(&_a->key, &_b->key);
    }
}

static const char*
_cexy__colorize_ansi(str_s token, str_s exact_match, char current_char)
{
    if (token.len == 0) { return "\033[0m"; }

    static const char* types[] = {
        "\033[1;31m", // exact match red
        "\033[1;33m", // keywords
        "\033[1;32m", // types
        "\033[1;34m", // func/macro call
        "\033[1;35m", // #macro
        "\033[33m",   // macro const
    };
    static struct
    {
        str_s item;
        u8 style;
    } keywords[] = {
        { str$s("return"), 1 },
        { str$s("if"), 1 },
        { str$s("else"), 1 },
        { str$s("while"), 1 },
        { str$s("do"), 1 },
        { str$s("goto"), 1 },
        { str$s("mem$"), 1 },
        { str$s("tmem$"), 1 },
        { str$s("mem$scope"), 1 },
        { str$s("IAllocator"), 1 },
        { str$s("for$each"), 1 },
        { str$s("for$iter"), 1 },
        { str$s("e$except"), 1 },
        { str$s("e$except_silent"), 1 },
        { str$s("e$except_silent"), 1 },
        { str$s("char"), 2 },
        { str$s("var"), 2 },
        { str$s("arr$"), 2 },
        { str$s("Exception"), 2 },
        { str$s("Exc"), 2 },
        { str$s("const"), 2 },
        { str$s("FILE"), 2 },
        { str$s("int"), 2 },
        { str$s("bool"), 2 },
        { str$s("void"), 2 },
        { str$s("usize"), 2 },
        { str$s("isize"), 2 },
        { str$s("struct"), 2 },
        { str$s("enum"), 2 },
        { str$s("union"), 2 },
        { str$s("u8"), 2 },
        { str$s("i8"), 2 },
        { str$s("u16"), 2 },
        { str$s("i16"), 2 },
        { str$s("u32"), 2 },
        { str$s("i32"), 2 },
        { str$s("u64"), 2 },
        { str$s("i64"), 2 },
        { str$s("f64"), 2 },
        { str$s("f32"), 2 },
    };
    if (token.buf != NULL) {
        if (str.slice.eq(token, exact_match)) { return types[0]; }
        for$each (it, keywords) {
            if (it.item.len == token.len) {
                if (memcmp(it.item.buf, token.buf, token.len) == 0) {
                    uassert(it.style < arr$len(types));
                    return types[it.style];
                }
            }
        }
        // looks like function/macro call
        if (current_char == '(') { return types[3]; }

        // CEX style type suffix
        if (str.slice.ends_with(token, str$s("_s")) || str.slice.ends_with(token, str$s("_e")) ||
            str.slice.ends_with(token, str$s("_c")) || str.slice.ends_with(token, str$s("_kw"))) {
            return types[2];
        }

        // #preproc directive
        if (token.buf[0] == '#') { return types[4]; }

        // some$macro constant
        if (str.slice.index_of(token, str$s("$")) >= 0) { return types[5]; }
    }

    return "\033[0m"; // no color, not matched
}
static void
_cexy__colorize_print(str_s code, str_s name, FILE* output)
{
    if (code.buf == NULL || !io.isatty(output)) {
        io.fprintf(output, "%S", code);
        return;
    }

    str_s token = { .buf = code.buf, .len = 0 };
    bool in_token = false;
    (void)name;
    for (usize i = 0; i < code.len; i++) {
        char c = code.buf[i];

        if (!in_token) {
            if (isalpha(c) || c == '_' || c == '$' || c == '#') {
                in_token = true;
                token.buf = &code.buf[i];
                token.len = 1;
            } else {
                putc(c, output);
            }
        } else {
            switch (c) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                case '\v':
                case '\f':
                case '(':
                case ')':
                case '{':
                case '}':
                case '[':
                case ']':
                case ';':
                case '*':
                case '/':
                case '+':
                case '-':
                case ',': {
                    char _c = c;
                    if (c == ' ' && i < code.len - 1 && code.buf[i + 1] == '(') {
                        _c = '(';
                    } else if (c == ')' && i > 2 && token.buf[-1] == '*' && token.buf[-2] == '(') {
                        _c = '(';
                    }
                    io.fprintf(output, "%s%S\033[0m", _cexy__colorize_ansi(token, name, _c), token);
                    putc(c, output);
                    in_token = false;
                    break;
                }
                default:
                    token.len++;
            }
        }
    }

    if (in_token) {
        io.fprintf(output, "%s%S\033[0m", _cexy__colorize_ansi(token, name, 0), token);
    }
    return;
}

static Exception
_cexy__display_full_info(
    cex_decl_s* d,
    char* base_ns,
    bool show_source,
    bool show_example,
    arr$(cex_decl_s*) cex_ns_decls,
    FILE* output
)
{
    uassert(output);

    str_s name = d->name;
    str_s base_name = str.sstr(base_ns);

    mem$scope(tmem$, _)
    {
        if (!cex_ns_decls) {
            io.fprintf(output, "Symbol found at %s:%d\n\n", d->file, d->line + 1);
        }

        if (d->type == CexTkn__func_def) {
            name = _cexy__fn_dotted(d->name, base_ns, _);
            if (!name.buf) {
                // error converting to dotted name (fallback)
                name = d->name;
            }
        }
        if (d->docs.buf) {
            // strip doxygen tags
            if (str.slice.starts_with(d->docs, str$s("/**"))) {
                d->docs = str.slice.sub(d->docs, 3, 0);
            }
            if (str.slice.ends_with(d->docs, str$s("*/"))) {
                d->docs = str.slice.sub(d->docs, 0, -2);
            }
            _cexy__colorize_print(d->docs, name, output);
            io.fprintf(output, "\n");
        }

        if (output != stdout) {
            // For export using c code block (markdown compatible)
            io.fprintf(output, "\n```c\n");
        }

        cex_decl_s* ns_struct = NULL;

        if (cex_ns_decls) {
            // This only passed if we have cex namespace struct here
            mem$scope(tmem$, _)
            {

                hm$(str_s, cex_decl_s*) ns_symbols = hm$new(ns_symbols, _, .capacity = 128);
                for$each (it, cex_ns_decls) {
                    // Check if __namespace$ exists
                    if (it->type == CexTkn__macro_const &&
                        str.slice.starts_with(it->name, str$s("__"))) {
                        if (str.slice.eq(str.slice.sub(it->name, 2, -1), base_name)) {
                            ns_struct = it;
                        }
                    }
                    if (!str.slice.starts_with(it->name, base_name)) { continue; }

                    if ((it->type == CexTkn__macro_func || it->type == CexTkn__macro_const)) {
                        if (it->name.buf[base_name.len] != '$') { continue; }
                    } else if (it->type == CexTkn__typedef) {
                        if (it->name.buf[base_name.len] != '_') { continue; }
                        if (!(str.slice.ends_with(it->name, str$s("_c")) ||
                              str.slice.ends_with(it->name, str$s("_s")) || str.slice.ends_with(it->name, str$s("_kw")))) {
                            continue;
                        }
                    } else if (it->type == CexTkn__cex_module_struct) {
                        ns_struct = it;
                        continue; // does not add, use special treatment
                    } else {
                        continue;
                    }

                    if (hm$getp(ns_symbols, it->name) != NULL) {
                        // Maybe new with docs?
                        if (it->docs.buf) { hm$set(ns_symbols, it->name, it); }
                        continue; // duplicate
                    } else {
                        hm$set(ns_symbols, it->name, it);
                    }
                }
                // WARNING: sorting of hashmap items is a dead end, hash indexes get invalidated
                qsort(ns_symbols, hm$len(ns_symbols), sizeof(*ns_symbols), str.slice.qscmp);

                for$each (it, ns_symbols) {
                    if (it.value->docs.buf) {
                        str_s brief_str = _cexy__process_make_brief_docs(it.value);
                        if (brief_str.len) { io.fprintf(output, "/// %S\n", brief_str); }
                    }
                    char* l = NULL;
                    if (it.value->type == CexTkn__macro_func) {
                        l = str.fmt(_, "#define %S(%s)\n", it.value->name, it.value->args);
                    } else if (it.value->type == CexTkn__macro_const) {
                        l = str.fmt(_, "#define %S\n", it.value->name);
                    } else if (it.value->type == CexTkn__typedef) {
                        l = str.fmt(_, "%s %S\n", it.value->ret_type, it.value->name);
                    }
                    if (l) {
                        _cexy__colorize_print(str.sstr(l), name, output);
                        io.fprintf(output, "\n");
                    }
                }
            }
            io.fprintf(output, "\n\n");
        }

        if (d->type == CexTkn__macro_const || d->type == CexTkn__macro_func) {
            if (str.slice.starts_with(d->name, str$s("__"))) {
                if (output != stdout) { io.fprintf(output, "\n```\n"); }
                goto end; // NOTE: it's likely placeholder i.e. __foo$ - only for docs, just skip
            }
            io.fprintf(output, "#define ");
        }

        if (sbuf.len(&d->ret_type)) {
            _cexy__colorize_print(str.sstr(d->ret_type), name, output);
            io.fprintf(output, " ");
        }

        _cexy__colorize_print(name, name, output);
        io.fprintf(output, " ");

        if (sbuf.len(&d->args)) {
            io.fprintf(output, "(");
            _cexy__colorize_print(str.sstr(d->args), name, output);
            io.fprintf(output, ")");
        }
        if (!show_source && d->type == CexTkn__func_def) {
            io.fprintf(output, ";");
        } else if (d->body.buf) {
            _cexy__colorize_print(d->body, name, output);
            io.fprintf(output, ";");
        } else if (ns_struct) {
            _cexy__colorize_print(ns_struct->body, name, output);
        }
        io.fprintf(output, "\n");

        if (output != stdout) {
            // For export using c code block (markdown compatible)
            io.fprintf(output, "\n```\n");
        }

        // No examples for whole namespaces (early exit)
        if (!show_example || ns_struct) { goto end; }

        // Looking for a random example
        srand(time(NULL));
        io.fprintf(output, "\nSearching for examples of '%S'\n", name);
        arr$(char*) sources = os.fs.find("./*.[hc]", true, _);

        u32 n_used = 0;
        for$each (src_fn, sources) {
            mem$scope(tmem$, _)
            {
                char* code = io.file.load(src_fn, _);
                if (code == NULL) {
                    return e$raise(Error.not_found, "Error loading: %s\n", src_fn);
                }
                arr$(cex_token_s) items = arr$new(items, _);

                CexParser_c lx = CexParser.create(code, 0, true);
                cex_token_s t;
                while ((t = CexParser.next_entity(&lx, &items)).type) {
                    if (t.type == CexTkn__error) { break; }
                    if (t.type != CexTkn__func_def) { continue; }
                    cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, _);
                    if (d == NULL) { continue; }
                    if (d->body.buf == NULL) { continue; }
                    if (str.slice.index_of(d->body, name) != -1) {
                        n_used++;
                        double dice = (double)rand() / (RAND_MAX + 1.0);
                        if (dice < 0.25) {
                            io.fprintf(output, "\n\nFound at %s:%d\n", src_fn, d->line);

                            if (output != stdout) { io.fprintf(output, "\n```c\n"); }

                            io.fprintf(output, "%s %S(%s)\n", d->ret_type, d->name, d->args);
                            _cexy__colorize_print(d->body, name, output);
                            io.fprintf(output, "\n");

                            if (output != stdout) { io.fprintf(output, "\n```\n"); }

                            goto end;
                        }
                    }
                }
            }
        }
        if (n_used == 0) {
            io.fprintf(output, "No usages of %S in the codebase\n", name);
        } else {
            io.fprintf(
                output,
                "%d usages of %S in the codebase, but no random pick, try again!\n",
                n_used,
                name
            );
        }
    }
end:
    if (output != stdout) {
        if (io.fflush(output)) {};
        io.fclose(&output);
    }
    return EOK;
}

static Exception
cexy__cmd__help(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    char* process_help = "Symbol / documentation search tool for C projects";
    char* epilog_help = 
        "\nQuery examples: \n"
        "cex help                     - list all namespaces in project directory\n"
        "cex help foo                 - find any symbol containing 'foo' (case sensitive)\n"
        "cex help foo.                - find namespace prefix: foo$, Foo_func(), FOO_CONST, etc\n"
        "cex help os$                 - find CEX namespace help (docs, macros, functions, types)\n"
        "cex help 'foo_*_bar'         - find using pattern search for symbols (see 'cex help str.match')\n"
        "cex help '*_(bar|foo)'       - find any symbol ending with '_bar' or '_foo'\n"
        "cex help str.find            - display function documentation if exactly matched\n"
        "cex help 'os$PATH_SEP'       - display macro constant value if exactly matched\n"
        "cex help str_s               - display type info and documentation if exactly matched\n"
        "cex help --source str.find   - display function source if exactly matched\n"
        "cex help --example str.find  - display random function use in codebase if exactly matched\n"
    ;
    char* filter = "./*.[hc]";
    char* out_file = NULL;
    bool show_source = false;
    bool show_example = false;

    // clang-format on
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "help [options] [query]",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(
            argparse$opt_group("Options"),
            argparse$opt_help(),
            argparse$opt(&filter, 'f', "filter", .help = "file pattern for searching"),
            argparse$opt(&show_source, 's', "source", .help = "show full source on match"),
            argparse$opt(
                &show_example,
                'e',
                "example",
                .help = "finds random example in source base"
            ),
            argparse$opt(&out_file, 'o', "out", .help = "write output of command to file"),
        ),
    };
    if (argparse.parse(&cmd_args, argc, argv)) { return Error.argsparse; }
    char* query = argparse.next(&cmd_args);
    str_s query_s = str.sstr(query);

    FILE* output = NULL;
    if (out_file) {
        e$ret(io.fopen(&output, out_file, "w"));
    } else {
        output = stdout;
    }

    mem$arena(1024 * 100, arena)
    {

        arr$(char*) sources = os.fs.find(filter, true, arena);
        if (os.fs.stat("./cex.h").is_symlink) { arr$push(sources, "./cex.h"); }
        arr$sort(sources, str.qscmp);

        char* query_pattern = NULL;
        bool is_namespace_filter = false;
        if (str.match(query, "[a-zA-Z0-9+].") || str.match(query, "[a-zA-Z0-9+]$")) {
            query_pattern = str.fmt(arena, "%S[._$]*", str.sub(query, 0, -1));
            is_namespace_filter = true;
        } else if (_cexy__is_str_pattern(query)) {
            query_pattern = query;
        } else {
            query_pattern = str.fmt(arena, "*%s*", query);
        }
        char* build_path = os.path.abs(cexy$build_dir, arena);
        char* test_path = os.path.abs("./tests/", arena);

        hm$(str_s, cex_decl_s*) names = hm$new(names, arena, .capacity = 1024);
        hm$(char*, char*) cex_ns_map = hm$new(cex_ns_map, arena, .capacity = 256);
        hm$set(cex_ns_map, "./cex.h", "cex");

        for$each (src_fn, sources) {
            mem$scope(tmem$, _)
            {
                char* abspath = os.path.abs(src_fn, _);
                if (str.starts_with(abspath, build_path) || str.starts_with(abspath, test_path)) {
                    continue;
                }

                auto basename = os.path.basename(src_fn, _);
                if (str.starts_with(basename, "_") || str.starts_with(basename, "test_")) {
                    continue;
                }
                char* base_ns = str.fmt(_, "%S", str.sub(basename, 0, -2));
                log$trace("Loading: %s (namespace: %s)\n", src_fn, base_ns);

                char* code = io.file.load(src_fn, arena);
                if (code == NULL) {
                    return e$raise(Error.not_found, "Error loading: %s\n", src_fn);
                }
                arr$(cex_token_s) items = arr$new(items, _);
                arr$(cex_decl_s*) all_decls = arr$new(all_decls, _);
                cex_decl_s* ns_decl = NULL;

                CexParser_c lx = CexParser.create(code, 0, true);
                cex_token_s t;
                while ((t = CexParser.next_entity(&lx, &items)).type) {
                    if (t.type == CexTkn__error) {
                        log$error("Error parsing: %s at line: %d\n", src_fn, lx.line);
                        break;
                    }
                    cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, arena);
                    if (d == NULL) { continue; }
                    arr$push(all_decls, d);

                    d->file = src_fn;
                    if (d->type == CexTkn__cex_module_struct || d->type == CexTkn__cex_module_def) {
                        log$trace("Found cex namespace: %s (namespace: %s)\n", src_fn, base_ns);
                        hm$set(cex_ns_map, src_fn, str.clone(base_ns, arena));
                    }

                    if (query == NULL) {
                        if (d->type == CexTkn__macro_const || d->type == CexTkn__macro_func) {
                            isize dollar = str.slice.index_of(d->name, str$s("$"));
                            str_s macro_ns = str.slice.sub(d->name, 0, dollar + 1);
                            if (dollar > 0 && !hm$getp(names, macro_ns)) {
                                hm$set(names, macro_ns, d);
                            }
                        } else if (d->type == CexTkn__typedef ||
                                   d->type == CexTkn__cex_module_struct) {
                            if (!hm$getp(names, d->name)) { hm$set(names, d->name, d); }
                        }
                    } else {
                        if (d->type == CexTkn__func_def) {
                            if (str.eq(query, "cex.") &&
                                str.slice.starts_with(d->name, str$s("cex_"))) {
                                continue;
                            }
                        }
                        str_s fndotted = (d->type == CexTkn__func_def)
                                           ? _cexy__fn_dotted(d->name, base_ns, _)
                                           : d->name;

                        if (str.slice.eq(d->name, query_s) || str.slice.eq(fndotted, query_s)) {
                            if (d->type == CexTkn__cex_module_def) { continue; }
                            if (d->type == CexTkn__typedef && d->ret_type[0] == '\0') { continue; }
                            if (is_namespace_filter) { continue; }
                            // We have full match display full help
                            return _cexy__display_full_info(
                                d,
                                base_ns,
                                show_source,
                                show_example,
                                NULL,
                                output
                            );
                        }

                        bool has_match = false;
                        if (str.slice.match(d->name, query_pattern)) { has_match = true; }
                        if (str.slice.match(fndotted, query_pattern)) { has_match = true; }
                        if (is_namespace_filter) {
                            str_s prefix = str.sub(query, 0, -1);
                            str_s sub_name = str.slice.sub(d->name, 0, prefix.len);
                            if (prefix.buf[prefix.len] == '.') {
                                // query case: ./cex help foo.
                                if (str.slice.eqi(sub_name, prefix) &&
                                    sub_name.buf[prefix.len] == '_') {
                                    if (d->type == CexTkn__func_def && str.eqi(query, "cex.")) {
                                        // skipping other namespaces of cex, e.g. cex_str_len()
                                        continue;
                                    }
                                    has_match = true;
                                }
                            } else {
                                // query case: ./cex help foo$
                                if (d->type == CexTkn__macro_const &&
                                    str.slice.starts_with(d->name, str$s("__"))) {
                                    // include __foo$ (doc name)
                                    sub_name = str.slice.sub(d->name, 2, -1);
                                    if (str.slice.eq(sub_name, prefix)) {
                                        ns_decl = d;
                                        has_match = true;
                                    }
                                }
                                if (str.slice.eq(sub_name, prefix)) {
                                    if (d->type == CexTkn__cex_module_struct &&
                                        str.slice.eq(d->name, prefix)) {
                                        // full match of CEX namespace, query: os$, d->name = 'os'
                                        ns_decl = d;
                                        has_match = true;
                                    } else {
                                        switch (sub_name.buf[prefix.len]) {
                                            case '_':
                                                if (d->type != CexTkn__typedef) { continue; }
                                                if (!(str.slice.ends_with(d->name, str$s("_c")) ||
                                                      str.slice.ends_with(d->name, str$s("_s")))) {
                                                    continue;
                                                }
                                                fallthrough();
                                            case '$':
                                                if (!ns_decl) { ns_decl = d; }
                                                has_match = true;
                                                break;
                                            default:
                                                continue;
                                        }
                                    }
                                }
                            }
                        }
                        if (has_match) { hm$set(names, d->name, d); }
                    }
                }

                if (ns_decl) {
                    char* ns_prefix = str.slice.clone(str.sub(query, 0, -1), _);
                    return _cexy__display_full_info(
                        ns_decl,
                        ns_prefix,
                        false,
                        false,
                        all_decls,
                        output
                    );
                }
                if (arr$len(names) == 0) { continue; }
            }
        }

        // WARNING: sorting of hashmap items is a dead end, hash indexes get invalidated
        qsort(names, hm$len(names), sizeof(*names), _cexy__help_qscmp_decls_type);

        for$each (it, names) {
            if (query == NULL) {
                switch (it.value->type) {
                    case CexTkn__cex_module_struct:
                        io.fprintf(output, "%-20s", "cexy namespace");
                        break;
                    case CexTkn__macro_func:
                    case CexTkn__macro_const:
                        io.fprintf(output, "%-20s", "macro namespace");
                        break;
                    default:
                        io.fprintf(output, "%-20s", CexTkn_str[it.value->type]);
                }
                io.fprintf(output, " %-30S %s:%d\n", it.key, it.value->file, it.value->line + 1);
            } else {
                str_s name = it.key;
                char* cex_ns = hm$get(cex_ns_map, (char*)it.value->file);
                if (cex_ns && it.value->type == CexTkn__func_def) {
                    name = _cexy__fn_dotted(name, cex_ns, arena);
                    if (!name.buf) {
                        // something weird happened, fallback
                        log$trace(
                            "Failed to make dotted name from %S, cex_ns: %s\n",
                            it.key,
                            cex_ns
                        );
                        name = it.key;
                    }
                }

                io.fprintf(
                    output,
                    "%-20s %-30S %s:%d\n",
                    CexTkn_str[it.value->type],
                    name,
                    it.value->file,
                    it.value->line + 1
                );
            }
        }
    }

    if (output && output != stdout) { io.fclose(&output); }

    return EOK;
}

static Exception
cexy__cmd__config(int argc, char** argv, void* user_ctx)
{
    Exc result = EOK;
    (void)argc;
    (void)argv;
    (void)user_ctx;

    // clang-format off
    char* process_help = "Check project and system environment";
    char* epilog_help = 
        "\nProject setup examples: \n"
    ;

    // clang-format on
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "config [options]",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(argparse$opt_help(), ),
    };
    if (argparse.parse(&cmd_args, argc, argv)) { return Error.argsparse; }
    // clang-format off
#define $env                                                                                \
    "\ncexy$* variables used in build system, see `cex help 'cexy$cc'` for more info\n"            \
    "* CEX_LOG_LVL               " cex$stringize(CEX_LOG_LVL) "\n"                                             \
    "* cexy$build_dir            " cexy$build_dir "\n"                                             \
    "* cexy$src_dir              " cexy$src_dir "\n"                                             \
    "* cexy$cc                   " cexy$cc "\n"                                                    \
    "* cexy$cc_include           " cex$stringize(cexy$cc_include) "\n"                             \
    "* cexy$cc_args_sanitizer    " cex$stringize(cexy$cc_args_sanitizer) "\n"                                \
    "* cexy$cc_args              " cex$stringize(cexy$cc_args) "\n"                                \
    "* cexy$cc_args_test         " cex$stringize(cexy$cc_args_test) "\n"                           \
    "* cexy$ld_args              " cex$stringize(cexy$ld_args) "\n"                                \
    "* cexy$fuzzer               " cex$stringize(cexy$fuzzer) "\n"                                \
    "* cexy$debug_cmd            " cex$stringize(cexy$debug_cmd) "\n"                              \
    "* cexy$pkgconf_cmd          " cex$stringize(cexy$pkgconf_cmd) "\n"                              \
    "* cexy$pkgconf_libs         " cex$stringize(cexy$pkgconf_libs) "\n"                              \
    "* cexy$process_ignore_kw    " cex$stringize(cexy$process_ignore_kw) "\n"\
    "* cexy$cex_self_args        " cex$stringize(cexy$cex_self_args) "\n"\
    "* cexy$cex_self_cc          " cexy$cex_self_cc "\n" // clang-format on

    io.printf("%s", $env);

    mem$scope(tmem$, _)
    {
        bool has_git = os.cmd.exists("git");

        io.printf("\nTools installed (optional):\n");
        io.printf("* git                       %s\n", has_git ? "OK" : "Not found");
        char* pkgconf_cmd[] = { cexy$pkgconf_cmd };
        bool has_pkg_config = false;
        if (arr$len(pkgconf_cmd) && pkgconf_cmd[0] && pkgconf_cmd[0][0] != '\0') {
            has_pkg_config = os.cmd.exists(pkgconf_cmd[0]);
        }
        if (has_pkg_config) {
            io.printf(
                "* cexy$pkgconf_cmd          %s (%s)\n",
                "OK",
                cex$stringize(cexy$pkgconf_cmd)
            );
        } else {
            switch (os.platform.current()) {
                case OSPlatform__win:
                    io.printf(
                        "* cexy$pkgconf_cmd          %s\n",
                        "Not found (try `pacman -S pkg-config` via MSYS2 or try this https://github.com/skeeto/u-config)"
                    );
                    break;
                case OSPlatform__macos:
                    io.printf(
                        "* cexy$pkgconf_cmd          %s\n",
                        "Not found (try `brew install pkg-config`)"
                    );
                    break;
                default:
                    io.printf(
                        "* cexy$pkgconf_cmd          %s\n",
                        "Not found (try to install `pkg-config` using package manager)"
                    );
                    break;
            }
        }

        char* triplet[] = { cexy$vcpkg_triplet };
        char* vcpkg_root = cexy$vcpkg_root;

        // NOLINTNEXTLINE
        if (arr$len(triplet) && triplet[0] != NULL && triplet[0][0] != '\0') {
            io.printf("* cexy$vcpkg_root           %s\n", (vcpkg_root) ? vcpkg_root : "Not set)");
            io.printf("* cexy$vcpkg_triplet        %s\n", triplet[0]);
            if (!vcpkg_root) {
                log$error("Build system expects vcpkg to be installed and configured\n");
                result = "vcpkg not installed/misconfigured";
            }
        } else {
            io.printf("* cexy$vcpkg_root           %s\n", (vcpkg_root) ? vcpkg_root : "Not set");
            io.printf("* cexy$vcpkg_triplet        %s\n", "Not set");
        }

        char* pkgconf_libargs[] = { cexy$pkgconf_libs };
        if (arr$len(pkgconf_libargs)) {
            arr$(char*) args = arr$new(args, _);
            Exc err = cexy$pkgconf(_, &args, "--libs", cexy$pkgconf_libs);
            if (err == EOK) {
                io.printf(
                    "* cexy$pkgconf_libs         %s (all found)\n",
                    str.join(args, arr$len(args), " ", _)
                );
            } else {
                io.printf("* cexy$pkgconf_libs         %s [%s]\n", "ERROR", err);
                // NOLINTNEXTLINE
                if (err == Error.runtime && arr$len(triplet) && triplet[0] != NULL) {
                    // pkg-conf failed to find lib it could be a missing .pc
                    io.printf(
                        "WARNING: Looks like vcpkg library (.a file) exists, but missing/misspelled .pc file for pkg-conf. \n"
                    );
                    io.printf("\tTry to resolve it manually via cexy$ld_args.\n");
                    io.printf(
                        "\tPKG_CONFIG_LIBDIR: %s/installed/%s/lib/pkgconfig\n",
                        vcpkg_root,
                        triplet[0]
                    );
                }
                io.printf("\tCompile with `#define CEX_LOG_LVL 5` for more info\n");
                result = "Missing Libs";
            }
        }

        io.printf("\nGlobal environment:\n");
        io.printf(
            "* Cex Version               %d.%d.%d (%s)\n",
            cex$version_major,
            cex$version_minor,
            cex$version_patch,
            cex$version_date
        );
        io.printf("* Git Hash                  %s\n", has_git ? cexy.utils.git_hash(_) : "(no git)");
        io.printf("* os.platform.current()     %s\n", os.platform.to_str(os.platform.current()));
        io.printf("* ./cex -D<ARGS> config     %s\n", cex$stringize(_CEX_SELF_DARGS));
    }


#    undef $env

    return result;
}

static Exception
cexy__cmd__libfetch(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    char* process_help = "Fetching 3rd party libraries via git (by default it uses cex git repo as source)";
    char* epilog_help = 
        "\nCommand examples: \n"
        "cex libfetch lib/test/fff.h                            - fetch signle header lib from CEX repo\n"
        "cex libfetch -U cex.h                                  - update cex.h to most recent version\n"
        "cex libfetch lib/random/                               - fetch whole directory recursively from CEX lib\n"
        "cex libfetch --git-label=v2.0 file.h                   - fetch using specific label or commit\n"
        "cex libfetch -u https://github.com/m/lib.git file.h    - fetch from arbitrary repo\n"
        "cex help --example cexy.utils.git_lib_fetch            - you can call it from your cex.c (see example)\n"
    ;
    // clang-format on

    char* git_url = "https://github.com/alexveden/cex.git";
    char* git_label = "HEAD";
    char* out_dir = "./";
    bool update_existing = false;
    bool preserve_dirs = true;

    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "libfetch [options]",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(
            argparse$opt_help(),
            argparse$opt(&git_url, 'u', "git-url", .help = "Git URL of the repository"),
            argparse$opt(&git_label, 'l', "git-label", .help = "Git label"),
            argparse$opt(
                &out_dir,
                'o',
                "out-dir",
                .help = "Output directory relative to project root"
            ),
            argparse$opt(
                &update_existing,
                'U',
                "update",
                .help = "Force replacing existing code with repository files"
            ),
            argparse$opt(
                &preserve_dirs,
                'p',
                "preserve-dirs",
                .help = "Preserve directory structure as in repo"
            ),
        ),
    };
    if (argparse.parse(&cmd_args, argc, argv)) { return Error.argsparse; }

    e$ret(cexy.utils.git_lib_fetch(
        git_url,
        git_label,
        out_dir,
        update_existing,
        preserve_dirs,
        (char**)cmd_args.argv,
        cmd_args.argc
    ));

    return EOK;
}

static Exception
cexy__cmd__simple_test(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "test [options] {run,build,create,clean,debug} all|tests/test_file.c [--test-options]",
        .description = _cexy$cmd_test_help,
        .epilog = _cexy$cmd_test_epilog,
        argparse$opt_list(argparse$opt_help(), ),
    };

    e$ret(argparse.parse(&cmd_args, argc, argv));
    char* cmd = argparse.next(&cmd_args);
    char* target = argparse.next(&cmd_args);

    if (!str.match(cmd, "(run|build|create|clean|debug)") || target == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(Error.argsparse, "Invalid command: '%s' or target: '%s'", cmd, target);
    }

    if (str.eq(cmd, "create")) {
        e$ret(cexy.test.create(target, false));
        return EOK;
    } else if (str.eq(cmd, "clean")) {
        e$ret(cexy.test.clean(target));
        return EOK;
    }
    bool single_test = !str.eq(target, "all");
    e$ret(cexy.test.make_target_pattern(&target)); // validation + convert 'all' -> "tests/test_*.c"

    log$info("Tests building: %s\n", target);
    // Build stage
    u32 n_tests = 0;
    u32 n_built = 0;
    (void)n_tests;
    (void)n_built;
    mem$scope(tmem$, _)
    {
        for$each (test_src, os.fs.find(target, true, _)) {
            char* test_target = cexy.target_make(test_src, cexy$build_dir, ".test", _);
            log$trace("Test src: %s -> %s\n", test_src, test_target);
            fflush(stdout); // typically for CI
            n_tests++;
            if (!single_test && !cexy.src_include_changed(test_target, test_src, NULL)) {
                continue;
            }
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, cexy$cc, );
            // NOTE: reconstructing char*[] because some cexy$ variables might be empty
            char* cc_args_test[] = { cexy$cc_args_test };
            char* cc_include[] = { cexy$cc_include };
            char* cc_ld_args[] = { cexy$ld_args };
            arr$pusha(args, cc_args_test);
            arr$pusha(args, cc_include);
            arr$push(args, test_src);
            arr$pusha(args, cc_ld_args);
            char* pkgconf_libargs[] = { cexy$pkgconf_libs };
            if (arr$len(pkgconf_libargs)) {
                e$ret(cexy$pkgconf(_, &args, "--cflags", "--libs", cexy$pkgconf_libs));
            }
            arr$pushm(args, "-o", test_target);


            arr$push(args, NULL);
            e$ret(os$cmda(args));
            n_built++;
        }
    }

    log$info("Tests building: %d tests processed, %d tests built\n", n_tests, n_built);
    fflush(stdout);

    if (str.match(cmd, "(run|debug)")) {
        e$ret(cexy.test.run(target, str.eq(cmd, "debug"), cmd_args.argc, cmd_args.argv));
    }
    return EOK;
}

static Exception
cexy__utils__make_new_project(char* proj_dir)
{
    log$info("Creating new boilerplate CEX project in '%s'\n", proj_dir);
    mem$scope(tmem$, _)
    {
        if (!str.eq(proj_dir, ".")) {
            if (os.path.exists(proj_dir)) {
                return e$raise(Error.exists, "New project dir already exists: %s", proj_dir);
            }
            e$ret(os.fs.mkdir(proj_dir));
            log$info("Copying this 'cex.h' into '%s/cex.h'\n", proj_dir);
            e$ret(os.fs.copy("./cex.h", os$path_join(_, proj_dir, "cex.h")));
        } else {
            // Creating in the current dir
            if (os.path.exists("./cex.c")) {
                return e$raise(
                    Error.exists,
                    "New project seems to de already initialized, cex.c exists"
                );
            }
        }

        log$info("Making '%s/cex.c'\n", proj_dir);
        auto cex_c = os$path_join(_, proj_dir, "cex.c");
        auto lib_h = os$path_join(_, proj_dir, "lib", "mylib.h");
        auto lib_c = os$path_join(_, proj_dir, "lib", "mylib.c");
        auto app_c = os$path_join(_, proj_dir, "src", "myapp.c");
        e$assert(!os.path.exists(cex_c) && "cex.c already exists");
        e$assert(!os.path.exists(lib_h) && "mylib.h already exists");
        e$assert(!os.path.exists(lib_h) && "mylib.c already exists");
        e$assert(!os.path.exists(app_c) && "myapp.c already exists");

#    ifdef _cex_main_boilerplate
        e$ret(io.file.save(os$path_join(_, proj_dir, "cex.c"), _cex_main_boilerplate));
#    else
        e$ret(os.fs.copy("src/cex_boilerplate.c", cex_c));
#    endif

        // Simple touching file first ./cex call will update its flags
        e$ret(io.file.save(os$path_join(_, proj_dir, "compile_flags.txt"), ""));

        // Basic cex-related stuff
        char* git_ignore = "cex\n"
                           "build/\n"
                           "compile_flags.txt\n";
        e$ret(io.file.save(os$path_join(_, proj_dir, ".gitignore"), git_ignore));

        sbuf_c buf = sbuf.create(1024 * 10, _);
        {
            log$info("Making '%s'\n", lib_h);
            e$ret(os.fs.mkpath(lib_h));
            sbuf.clear(&buf);
            cg$init(&buf);
            cg$pn("#pragma once");
            cg$pn("#include \"cex.h\"");
            cg$pn("");
            cg$pn("i32 mylib_add(i32 a, i32 b);");
            e$ret(io.file.save(lib_h, buf));
        }

        {
            log$info("Making '%s'\n", lib_c);
            e$ret(os.fs.mkpath(lib_c));
            sbuf.clear(&buf);
            cg$init(&buf);
            cg$pn("#include \"mylib.h\"");
            cg$pn("#include \"cex.h\"");
            cg$pn("");
            cg$func("i32 mylib_add(i32 a, i32 b)", "")
            {
                cg$pn("return a + b;");
            }
            e$ret(io.file.save(lib_c, buf));
        }

        {
            log$info("Making '%s'\n", app_c);
            e$ret(os.fs.mkpath(app_c));
            sbuf.clear(&buf);
            cg$init(&buf);
            cg$pn("#define CEX_IMPLEMENTATION");
            cg$pn("#include \"cex.h\"");
            cg$pn("#include \"lib/mylib.c\"  /* NOTE: include .c to make unity build! */");
            cg$pn("");
            cg$func("int main(int argc, char** argv)", "")
            {
                cg$pn("(void)argc;");
                cg$pn("(void)argv;");
                cg$pn("io.printf(\"MOCCA - Make Old C Cexy Again!\\n\");");
                cg$pn("io.printf(\"1 + 2 = %d\\n\", mylib_add(1, 2));");
                cg$pn("return 0;");
            }
            e$ret(io.file.save(app_c, buf));
        }

        auto test_c = os$path_join(_, proj_dir, "tests", "test_mylib.c");
        log$info("Making '%s'\n", test_c);
        e$ret(cexy.test.create(test_c, true));
        log$info("Compiling new cex app for a new project...\n");
        auto old_dir = os.fs.getcwd(_);

        e$ret(os.fs.chdir(proj_dir));
        char* bin_path = "cex" cexy$build_ext_exe;
        char* old_name = str.fmt(_, "%s.old", bin_path);
        if (os.path.exists(bin_path)) {
            if (os.path.exists(old_name)) { e$ret(os.fs.remove(old_name)); }
            e$ret(os.fs.rename(bin_path, old_name));
        }
        e$ret(os$cmd(cexy$cex_self_cc, "-o", bin_path, "cex.c"));
        if (os.path.exists(old_name)) {
            if (os.fs.remove(old_name)) {
                // WTF: win32 might lock old_name, try to remove it, but maybe no luck
            }
        }
        e$ret(os.fs.chdir(old_dir));
        log$info("New project has been created in %s\n", proj_dir);
    }

    return EOK;
}

static Exception
cexy__cmd__new(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "new [new_proj_dir]",
        .description = "Creates new boilerplate CEX project",
        argparse$opt_list(argparse$opt_help(), ),
    };

    e$ret(argparse.parse(&cmd_args, argc, argv));
    char* proj_dir = argparse.next(&cmd_args);

    if (proj_dir == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(Error.argsparse, "Expected new project directory path.");
    }

    e$ret(cexy.utils.make_new_project(proj_dir));

    return EOK;
}

Exception
cexy__app__create(char* target)
{
    mem$scope(tmem$, _)
    {
        char* app_src = os$path_join(_, cexy$src_dir, target, str.fmt(_, "%s.c", target));
        if (os.path.exists(app_src)) {
            return e$raise(Error.exists, "App file already exists: %s", app_src, target);
        }
        e$ret(os.fs.mkpath(app_src));

        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        // clang-format off
        cg$pn("#include \"cex.h\"");
        cg$pn("");
        cg$func("Exception\n%s(int argc, char** argv)\n", target)
        {
            cg$pn("bool my_flag = false;");
            cg$scope("argparse_c args = ", target)
            {
                cg$pn(".description = \"New CEX App\",");
                cg$pn("argparse$opt_list(");
                cg$pn("    argparse$opt_help(),");
                cg$pn("    argparse$opt(&my_flag, 'c', \"ctf\", .help = \"Capture the flag\"),");
                cg$pn("),");
            }
            cg$pa(";\n", "");
            cg$pn("if (argparse.parse(&args, argc, argv)) { return Error.argsparse; }");
            cg$pn("io.printf(\"MOCCA - Make Old C Cexy Again!\\n\");");
            cg$pn("io.printf(\"%s\\n\", (my_flag) ? \"Flag is captured\" : \"Pass --ctf to capture the flag\");");

            cg$pn("return EOK;");
        };
        // clang-format on
        e$ret(io.file.save(app_src, buf));
    }

    mem$scope(tmem$, _)
    {
        char* app_src = os$path_join(_, cexy$src_dir, target, "main.c");
        if (os.path.exists(app_src)) {
            return e$raise(Error.exists, "App file already exists: %s", app_src, target);
        }
        e$ret(os.fs.mkpath(app_src));

        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        // clang-format off
        cg$pn("// NOTE: main.c serves as unity build container + detaching allows unit testing of app's code");
        cg$pn("#define CEX_IMPLEMENTATION");
        cg$pn("#include \"cex.h\"");
        cg$pf("#include \"%s.c\"", target);
        cg$pn("// TODO: add your app sources here (include .c)");
        cg$pn("");
        cg$func("int\nmain(int argc, char** argv)\n", "")
        {
            cg$pf("if (%s(argc, argv) != EOK) { return 1; }", target);
            cg$pn("return 0;");
        }
        // clang-format on
        e$ret(io.file.save(app_src, buf));
    }
    return EOK;
}

Exception
cexy__app__run(char* target, bool is_debug, int argc, char** argv)
{
    mem$scope(tmem$, _)
    {
        char* app_src;
        e$ret(cexy.app.find_app_target_src(_, target, &app_src));
        char* app_exe = cexy.target_make(app_src, cexy$build_dir, target, _);
        arr$(char*) args = arr$new(args, _);
        if (is_debug) { arr$pushm(args, cexy$debug_cmd); }
        arr$pushm(args, app_exe, );
        arr$pusha(args, argv, argc);
        arr$push(args, NULL);
        e$ret(os$cmda(args));
    }
    return EOK;
}

Exception
cexy__app__clean(char* target)
{
    mem$scope(tmem$, _)
    {
        char* app_src;
        e$ret(cexy.app.find_app_target_src(_, target, &app_src));
        char* app_exe = cexy.target_make(app_src, cexy$build_dir, target, _);
        if (os.path.exists(app_exe)) {
            log$info("Removing: %s\n", app_exe);
            e$ret(os.fs.remove(app_exe));
        }
    }
    return EOK;
}

Exception
cexy__app__find_app_target_src(IAllocator allc, char* target, char** out_result)
{
    uassert(out_result != NULL);
    *out_result = NULL;

    if (target == NULL) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            target
        );
    }
    if (str.eq(target, "all")) {
        return e$raise(Error.argsparse, "all target is not supported for this command");
    }

    if (_cexy__is_str_pattern(target)) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected alphanumerical name, patterns are not allowed",
            target
        );
    }
    if (!str.match(target, "[a-zA-Z0-9_+]")) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected alphanumerical name",
            target
        );
    }
    char* app_src = str.fmt(allc, "%s%c%s.c", cexy$src_dir, os$PATH_SEP, target);
    log$trace("Probing %s\n", app_src);
    if (!os.path.exists(app_src)) {
        mem$free(allc, app_src);

        app_src = os$path_join(allc, cexy$src_dir, target, "main.c");
        log$trace("Probing %s\n", app_src);
        if (!os.path.exists(app_src)) {
            mem$free(allc, app_src);
            return e$raise(Error.not_found, "App target source not found: %s", target);
        }
    }
    *out_result = app_src;
    return EOK;
}

static Exception
cexy__cmd__simple_app(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "app [options] {run,build,create,clean,debug} APP_NAME [--app-options app args]",
        argparse$opt_list(argparse$opt_help(), ),
    };

    e$ret(argparse.parse(&cmd_args, argc, argv));
    char* cmd = argparse.next(&cmd_args);
    char* target = argparse.next(&cmd_args);

    if (!str.match(cmd, "(run|build|create|clean|debug)") || target == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(Error.argsparse, "Invalid command: '%s' or target: '%s'", cmd, target);
    }

    if (str.eq(cmd, "create")) {
        e$ret(cexy.app.create(target));
        return EOK;
    } else if (str.eq(cmd, "clean")) {
        e$ret(cexy.app.clean(target));
        return EOK;
    }
    e$assert(os.path.exists(cexy$src_dir) && cexy$src_dir " not exists");

    mem$scope(tmem$, _)
    {
        char* app_src;
        e$ret(cexy.app.find_app_target_src(_, target, &app_src));
        char* app_exec = cexy.target_make(app_src, cexy$build_dir, target, _);
        log$trace("App src: %s -> %s\n", target, app_exec);
        if (!cexy.src_include_changed(app_exec, app_src, NULL)) { goto run; }
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, cexy$cc, );
        // NOTE: reconstructing char*[] because some cexy$ variables might be empty
        char* cc_args[] = { cexy$cc_args };
        char* cc_include[] = { cexy$cc_include };
        char* ld_args[] = { cexy$ld_args };
        char* pkgconf_libargs[] = { cexy$pkgconf_libs };
        arr$pusha(args, cc_args);
        arr$pusha(args, cc_include);
        if (arr$len(pkgconf_libargs)) {
            e$ret(cexy$pkgconf(_, &args, "--cflags", cexy$pkgconf_libs));
        }
        arr$push(args, (char*)app_src);
        arr$pusha(args, ld_args);
        if (arr$len(pkgconf_libargs)) {
            e$ret(cexy$pkgconf(_, &args, "--libs", cexy$pkgconf_libs));
        }
        arr$pushm(args, "-o", app_exec);


        arr$push(args, NULL);
        e$ret(os$cmda(args));

    run:
        if (str.match(cmd, "(run|debug)")) {
            e$ret(cexy.app.run(target, str.eq(cmd, "debug"), cmd_args.argc, cmd_args.argv));
        }
    }


    return EOK;
}

Exception
cexy__cmd__simple_fuzz(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    mem$scope(tmem$, _)
    {
        u32 max_time = 0;
        bool debug = false;
        u32 max_timeout_sec = 10;

        argparse_c cmd_args = {
            .program_name = "./cex",
            .usage = "fuzz [options] {run, create, debug} all|fuzz/some/fuzz_file.c [-fuz-options]",
            .description = "Compiles and runs fuzz test on target",
            argparse$opt_list(
                argparse$opt_help(),
                argparse$opt(
                    &max_time,
                    '\0',
                    "max-time",
                    .help = "Maximum time per fuzz in seconds (60 seconds if 'all' target)"
                ),
                argparse$opt(
                    &max_timeout_sec,
                    '\0',
                    "timeout",
                    .help = "Timeout in seconds for hanging task"
                ),
            ),
        };

        e$ret(argparse.parse(&cmd_args, argc, argv));
        char* cmd = argparse.next(&cmd_args);
        char* src = argparse.next(&cmd_args);
        if (src == NULL) {
            argparse.usage(&cmd_args);
            io.printf("Bad fuzz file argument\n");
            return Error.argsparse;
        }
        if (!str.match(cmd, "(run|create|debug)")) {
            argparse.usage(&cmd_args);
            return e$raise(Error.argsparse, "Invalid fuzz command: '%s'", cmd);
        }
        if (str.eq(cmd, "create")) {
            e$ret(cexy.fuzz.create(src));
            return EOK;
        } else if (str.eq(cmd, "debug")) {
            debug = true;
        }


        bool run_all = false;
        if (str.eq(src, "all")) {
            src = "fuzz/fuzz_*.c";
            run_all = true;
            if (max_time == 0) { max_time = 60; }
        } else {
            if (!os.path.exists(src)) {
                return e$raise(Error.not_found, "target not found: %s", src);
            }
        }

        char* proj_dir = os.path.abs(".", _);
        bool is_afl_fuzzer = false;

        for$each (src_file, os.fs.find(src, true, _)) {
            fflush(stdout); // typically for CI
            e$ret(os.fs.chdir(proj_dir));

            char* dir = os.path.dirname(src_file, _);
            char* file = os.path.basename(src_file, _);
            if (str.ends_with(dir, ".out") || str.ends_with(dir, ".afl") ||
                str.ends_with(dir, "_corpus")) {
                continue;
            }
            e$assert(str.ends_with(file, ".c"));
            str_s prefix = str.sub(file, 0, -2);

            char* target_exe = str.fmt(_, "%s/%S.fuzz", dir, prefix);
            arr$(char*) args = arr$new(args, _);
            arr$clear(args);
            if (!run_all || cexy.src_include_changed(target_exe, src_file, NULL)) {
                arr$pushm(args, cexy$fuzzer);
                e$assert(arr$len(args) > 0 && "empty cexy$fuzzer");
                e$assertf(os.cmd.exists(args[0]), "fuzzer command not found: %s", args[0]);
                if (str.find(args[0], "afl")) { is_afl_fuzzer = true; }
                if (is_afl_fuzzer) { arr$push(args, "-DCEX_FUZZ_AFL"); }

                char* cc_include[] = { cexy$cc_include };
                char* cc_ld_args[] = { cexy$ld_args };
                arr$pusha(args, cc_include);

                char* pkgconf_libargs[] = { cexy$pkgconf_libs };
                if (arr$len(pkgconf_libargs)) {
                    e$ret(cexy$pkgconf(_, &args, "--cflags", cexy$pkgconf_libs));
                }
                arr$push(args, src_file);
                arr$pusha(args, cc_ld_args);
                if (arr$len(pkgconf_libargs)) {
                    e$ret(cexy$pkgconf(_, &args, "--libs", cexy$pkgconf_libs));
                }
                arr$pushm(args, "-o", target_exe);
                arr$push(args, NULL);

                e$ret(os$cmda(args));
            }

            e$ret(os.fs.chdir(dir));

            arr$clear(args);
            if (is_afl_fuzzer) {
                // AFL++ or something
                e$ret(os.env.set("ASAN_OPTIONS", ""));

                if (debug) { e$assert(false && "AFL fuzzer debugging is not supported"); }
                arr$pushm(args, "afl-fuzz");

                if (cmd_args.argc > 0) {
                    // Fully user driven arguments
                    arr$pusha(args, cmd_args.argv, cmd_args.argc);
                } else {
                    char* corpus_dir = str.fmt(_, "%S_corpus", prefix);
                    char* corpus_dir_out = str.fmt(_, "%S_corpus.afl", prefix);
                    arr$pushm(args, "-i", corpus_dir, "-o", corpus_dir_out);

                    arr$pushm(args, "-t", str.fmt(_, "%d", max_timeout_sec * 1000));

                    if (max_time > 0) { arr$pushm(args, "-V", str.fmt(_, "%d", max_time)); }

                    char* dict_file = str.fmt(_, "%S.dict", prefix);
                    if (os.path.exists(dict_file)) { arr$pushm(args, "-x", dict_file); }

                    // adding exe file
                    arr$pushm(args, "--", str.fmt(_, "./%S.fuzz", prefix));
                }
            } else {
                // clang - libFuzzer
                if (debug) { arr$pushm(args, cexy$debug_cmd); }
                arr$pushm(
                    args,
                    str.fmt(_, "./%S.fuzz", prefix),
                    str.fmt(_, "-artifact_prefix=%S.", prefix)
                );
                if (cmd_args.argc > 0) {
                    arr$pusha(args, cmd_args.argv, cmd_args.argc);
                } else {
                    if (!debug) { arr$push(args, str.fmt(_, "-timeout=%d", max_timeout_sec)); }
                    if (max_time > 0) {
                        arr$pushm(args, str.fmt(_, "-max_total_time=%d", max_time));
                    }

                    char* dict_file = str.fmt(_, "%S.dict", prefix);
                    if (os.path.exists(dict_file)) {
                        arr$push(args, str.fmt(_, "-dict=%s", dict_file));
                    }

                    char* corpus_dir = str.fmt(_, "%S_corpus", prefix);
                    if (os.path.exists(corpus_dir)) {
                        char* corpus_dir_tmp = str.fmt(_, "%S_corpus.out", prefix);
                        e$ret(os.fs.mkdir(corpus_dir_tmp));

                        arr$push(args, corpus_dir_tmp);
                        arr$push(args, corpus_dir);
                    }
                }
            }

            arr$push(args, NULL);
            e$ret(os$cmda(args));
        }
    }
    return EOK;
}

static char*
cexy__utils__git_hash(IAllocator allc)
{
    mem$arena(1024, _)
    {
        if (!os.cmd.exists("git")) {
            log$error("git command not found, not installed or PATH issue\n");
            return NULL;
        }

        char* args[] = { "git", "rev-parse", "HEAD", NULL };
        os_cmd_c c = { 0 };
        e$except (
            err,
            os.cmd.create(&c, args, arr$len(args), &(os_cmd_flags_s){ .no_window = true })
        ) {
            return NULL;
        }
        char* output = os.cmd.read_all(&c, _);
        int err_code = 0;
        e$except_silent (err, os.cmd.join(&c, 0, &err_code)) {
            log$error("`git rev-parse HEAD` error: %s err_code: %d\n", err, err_code);
            return NULL;
        }
        if (output == NULL || output[0] == '\0') {
            log$error("`git rev-parse HEAD` returned empty result\n");
            return NULL;
        }

        str_s clean_hash = str.sstr(output);
        clean_hash = str.slice.strip(clean_hash);
        if (clean_hash.len > 50) {
            log$error("`git rev-parse HEAD` unexpectedly long hash, got: '%S'\n", clean_hash);
            return NULL;
        }

        if (!str.slice.match(clean_hash, "[0-9a-fA-F+]")) {
            log$error("`git rev-parse HEAD` expected hexadecimal hash, got: '%S'\n", clean_hash);
            return NULL;
        }

        return str.fmt(allc, "%S", clean_hash);
    }
    return NULL;
}

static Exception
_cexy__utils__pkgconf_parse(IAllocator allc, arr$(char*) * out_cc_args, char* pkgconf_output)
{
    e$assert(pkgconf_output != NULL);
    e$assert(out_cc_args != NULL);
    e$assert(allc != NULL);

    char* cur = pkgconf_output;
    char* arg = NULL;
    while (*cur) {
        switch (*cur) {
            case ' ':
            case '\t':
            case '\v':
            case '\f':
            case '\n':
            case '\r': {
                if (arg) {
                    str_s a = { .buf = arg, .len = cur - arg };
                    if (a.len > 0) { arr$push(*out_cc_args, str.slice.clone(a, allc)); }
                }
                arg = NULL;
                break;
            }
            case '"':
            case '\'': {
                if (!arg) { arg = cur; }
                char quote = *cur;
                cur++;
                while (*cur) {
                    if (*cur == '\\') {
                        cur += 2;
                        continue;
                    }
                    if (*cur == quote) { break; }
                    cur++;
                }
                break;
            }
            default: {
                if (!arg) { arg = cur; }
                if (*cur == '\\') { cur++; }
                break;
            }
        }

        if (*cur) { cur++; }
    }

    if (arg) {
        str_s a = { .buf = arg, .len = cur - arg };
        if (a.len > 0) { arr$push(*out_cc_args, str.slice.clone(a, allc)); }
    }

    return EOK;
}

static Exception
cexy__utils__pkgconf(
    IAllocator allc,
    arr$(char*) * out_cc_args,
    char** pkgconf_args,
    usize pkgconf_args_len
)
{
    e$assert(allc->meta.is_arena && "expected ArenaAllocator or mem$scope(tmem$, _)");
    e$assert(out_cc_args != NULL);
    e$assert(pkgconf_args != NULL);
    e$assert(pkgconf_args_len > 0);

    os_cmd_c c = { 0 };

    mem$arena(2048, _)
    {
        arr$(char*) args = arr$new(args, _);
        char* vcpkg_root = cexy$vcpkg_root;
        char* triplet[] = { cexy$vcpkg_triplet };

        if (vcpkg_root) {
            log$trace("Looking vcpkg libs at '%s' triplet='%s'\n", vcpkg_root, triplet[0]);
            if (!os.path.exists(vcpkg_root)) {
                return e$raise(Error.not_found, "cexy$vcpkg_root not exists: %s", vcpkg_root);
            }
            char* triplet_path = str.fmt(_, "%s/installed/%s", vcpkg_root, triplet[0]);
            if (!os.path.exists(triplet_path)) {
                return e$raise(Error.not_found, "vcpkg triplet path not exists: %s", triplet_path);
            }
            char* lib_path = str.fmt(_, "%s/lib/", triplet_path);
            if (!os.path.exists(lib_path)) {
                return e$raise(Error.not_found, "vcpkg lib path not exists: %s", lib_path);
            }
            char* inc_path = str.fmt(_, "%s/include/", triplet_path);
            if (!os.path.exists(inc_path)) {
                return e$raise(Error.not_found, "vcpkg include path not exists: %s", inc_path);
            }
            char* pkgconf_path = str.fmt(_, "%s/lib/pkgconfig", triplet_path);
            if (!os.path.exists(pkgconf_path)) {
                return e$raise(
                    Error.not_found,
                    "vcpkg lib/pkgconfig path not exists: %s",
                    pkgconf_path
                );
            }

            log$trace("Setting: PKG_CONFIG_LIBDIR='%s'\n", pkgconf_path);
            e$ret(os.env.set("PKG_CONFIG_LIBDIR", pkgconf_path));

            log$trace("Using PKG_CONFIG_LIBDIR from vcpkg, not all packages provide .pc files!\n");
            for$each (it, pkgconf_args, pkgconf_args_len) {
                if (str.starts_with(it, "--")) { continue; }

                // checking libfoo.a, and foo.a
                char* lib_search[] = {
                    str.fmt(
                        _,
                        "%s/%s%s" cexy$build_ext_lib_stat, // append platform specific .a or .lib
                        lib_path,
                        (str.starts_with(it, "lib")) ? "" : "lib",
                        it
                    ),
                    str.fmt(
                        _,
                        "%s/%S" cexy$build_ext_lib_stat, // append platform specific .a or .lib
                        lib_path,
                        (str.starts_with(it, "lib")) ? str.sub(it, 3, 0) : str.sstr(it)
                    )
                };
                bool is_found = false;
                for$each (lib_file_name, lib_search) {
                    log$trace("Probing lib: %s\n", lib_file_name);
                    if (os.path.exists(lib_file_name)) {
                        log$trace("Found lib: %s\n", lib_file_name);
                        is_found = true;
                    }
                }

                if (!is_found) {
                    return e$raise(
                        Error.not_found,
                        "vcpkg: lib '%s' not found (try `vcpkg install %s`) dir: %s",
                        it,
                        it,
                        lib_path
                    );
                }
            }
        }
        arr$pushm(args, cexy$pkgconf_cmd);
        arr$pusha(args, pkgconf_args, pkgconf_args_len);
        arr$push(args, NULL);
        log$trace("Looking system libs with: %s\n", args[0]);

#    if CEX_LOG_LVL > 4
        os$cmd(args[0], "--print-errors", "--variable", "pc_path", "pkg-config");
        io.printf("\n");
        log$trace("CMD: ");
        for (u32 i = 0; i < arr$len(args) - 1; i++) {
            char* a = args[i];
            io.printf(" ");
            if (str.find(a, " ")) {
                io.printf("\'%s\'", a);
            } else if (a == NULL || *a == '\0') {
                io.printf("\'(empty arg)\'");
            } else {
                io.printf("%s", a);
            }
        }
        io.printf("\n");

#    endif
        e$ret(
            os.cmd.create(&c, args, arr$len(args), &(os_cmd_flags_s){ .combine_stdouterr = true })
        );

        char* output = os.cmd.read_all(&c, _);
        e$except_silent (err, os.cmd.join(&c, 0, NULL)) {
            log$error("%s program error:\n%s\n", cexy$pkgconf_cmd, output);
            return err;
        }
        e$ret(_cexy__utils__pkgconf_parse(allc, out_cc_args, output));
    }

    return EOK;
}

static Exception
cexy__utils__make_compile_flags(
    char* flags_file,
    bool include_cexy_flags,
    arr$(char*) cc_flags_or_null
)
{
    mem$scope(tmem$, _)
    {
        e$assert(str.ends_with(flags_file, "compile_flags.txt") && "unexpected file name");

        arr$(char*) args = arr$new(args, _);
        if (include_cexy_flags) {
            char* cc_args[] = { cexy$cc_args };
            char* cc_include[] = { cexy$cc_include };
            arr$pusha(args, cc_args);
            arr$pusha(args, cc_include);
            char* pkgconf_libargs[] = { cexy$pkgconf_libs };
            if (arr$len(pkgconf_libargs)) {
                e$except (err, cexy$pkgconf(_, &args, "--cflags", cexy$pkgconf_libs)) {}
            }
        }
        if (cc_flags_or_null != NULL) { arr$pusha(args, cc_flags_or_null); }
        if (arr$len(args) == 0) { return e$raise(Error.empty, "Compiler flags are empty"); }

        FILE* fh;
        e$ret(io.fopen(&fh, flags_file, "w"));
        for$each (it, args) {
            if (str.starts_with(it, "-fsanitize")) { continue; }
            e$goto(io.file.writeln(fh, it), end); // write args
        }
    end:
        io.fclose(&fh);
    }

    return EOK;
}

static Exception
cexy__utils__git_lib_fetch(
    char* git_url,
    char* git_label,
    char* out_dir,
    bool update_existing,
    bool preserve_dirs,
    char** repo_paths,
    usize repo_paths_len
)
{
    if (git_url == NULL || git_url[0] == '\0') {
        return e$raise(Error.argument, "Empty or null git_url");
    }
    if (!str.ends_with(git_url, ".git")) {
        return e$raise(Error.argument, "git_url must end with .git, got: %s", git_url);
    }
    if (git_label == NULL || git_label[0] == '\0') { git_label = "HEAD"; }
    log$info("Checking libs from: %s @ %s\n", git_url, git_label);
    char* out_build_dir = cexy$build_dir "/cexy_git/";
    e$ret(os.fs.mkpath(out_build_dir));

    mem$scope(tmem$, _)
    {
        bool needs_update = false;
        for$each (it, repo_paths, repo_paths_len) {
            char* out_file = (preserve_dirs)
                               ? str.fmt(_, "%s/%s", out_dir, it)
                               : str.fmt(_, "%s/%s", out_dir, os.path.basename(it, _));

            log$info("Lib file: %s -> %s\n", it, out_file);
            if (!os.path.exists(out_file)) { needs_update = true; }
        }
        if (!needs_update && !update_existing) {
            log$info("Nothing to update, lib files exist, run with update flag if needed\n");
            return EOK;
        }

        str_s base_name = os.path.split(git_url, false);
        e$assert(base_name.len > 4);
        e$assert(str.slice.ends_with(base_name, str$s(".git")));
        base_name = str.slice.sub(base_name, 0, -4);

        char* repo_dir = str.fmt(_, "%s/%S/", out_build_dir, base_name);
        if (os.path.exists(repo_dir)) { e$ret(os.fs.remove_tree(repo_dir)); }

        e$ret(os$cmd(
            "git",
            "clone",
            "--depth=1",
            "--quiet",
            "--filter=blob:none",
            "--no-checkout",
            "--",
            git_url,
            repo_dir
        ));


        arr$(char*)
            git_checkout_args = arr$new(git_checkout_args, _, .capacity = repo_paths_len + 10);
        arr$pushm(git_checkout_args, "git", "-C", repo_dir, "checkout", git_label, "--");
        arr$pusha(git_checkout_args, repo_paths, repo_paths_len);
        arr$push(git_checkout_args, NULL);
        e$ret(os$cmda(git_checkout_args));

        for$each (it, repo_paths, repo_paths_len) {
            char* in_path = str.fmt(_, "%s/%s", repo_dir, it);

            char* out_path = (preserve_dirs)
                               ? str.fmt(_, "%s/%s", out_dir, it)
                               : str.fmt(_, "%s/%s", out_dir, os.path.basename(it, _));
            auto in_stat = os.fs.stat(in_path);
            if (!in_stat.is_valid) {
                return e$raise(in_stat.error, "Invalid stat for path: %s", in_path);
            }

            auto out_stat = os.fs.stat(out_path);
            if (!out_stat.is_valid || update_existing) {
                log$info("Updating file: %s -> %s\n", it, out_path);
                if (in_stat.is_directory) {
                    if (out_stat.is_valid && update_existing) {
                        e$assert(out_stat.is_directory && "out_path expected to be a directory");
                        str_s src_dir = str.sstr(in_path);
                        str_s dst_dir = str.sstr(out_path);
                        for$each (fname, os.fs.find(str.fmt(_, "%S/*", src_dir), true, _)) {
                            e$assert(str.starts_with(fname, in_path));
                            char* out_file = str.fmt(
                                _,
                                "%S/%S",
                                dst_dir,
                                str.sub(fname, src_dir.len, 0)
                            );
                            log$debug("Replacing: %s\n", out_file);
                            e$ret(os.fs.mkpath(out_file));
                            if (os.path.exists(out_file)) { e$ret(os.fs.remove(out_file)); }
                            e$ret(os.fs.copy(fname, out_file));
                        }

                    } else {
                        e$ret(os.fs.copy_tree(in_path, out_path));
                    }
                } else {
                    // Updating single file
                    if (out_stat.is_valid && update_existing) { e$ret(os.fs.remove(out_path)); }
                    e$ret(os.fs.mkpath(out_path));
                    e$ret(os.fs.copy(in_path, out_path));
                }
            }
        }

#    ifdef _WIN32
        // WTF, windows git makes some files read-only which fails remove_tree later!
        e$ret(os$cmd("attrib", "-r", str.fmt(_, "%s/*", out_build_dir), "/s", "/d"));
#    endif
    }
    // We are done, cleanup!
    e$ret(os.fs.remove_tree(out_build_dir));

    return EOK;
}

const struct __cex_namespace__cexy cexy = {
    // Autogenerated by CEX
    // clang-format off

    .build_self = cexy_build_self,
    .src_changed = cexy_src_changed,
    .src_include_changed = cexy_src_include_changed,
    .target_make = cexy_target_make,

    .app = {
        .clean = cexy__app__clean,
        .create = cexy__app__create,
        .find_app_target_src = cexy__app__find_app_target_src,
        .run = cexy__app__run,
    },

    .cmd = {
        .config = cexy__cmd__config,
        .help = cexy__cmd__help,
        .libfetch = cexy__cmd__libfetch,
        .new = cexy__cmd__new,
        .process = cexy__cmd__process,
        .simple_app = cexy__cmd__simple_app,
        .simple_fuzz = cexy__cmd__simple_fuzz,
        .simple_test = cexy__cmd__simple_test,
        .stats = cexy__cmd__stats,
    },

    .fuzz = {
        .create = cexy__fuzz__create,
    },

    .test = {
        .clean = cexy__test__clean,
        .create = cexy__test__create,
        .make_target_pattern = cexy__test__make_target_pattern,
        .run = cexy__test__run,
    },

    .utils = {
        .git_hash = cexy__utils__git_hash,
        .git_lib_fetch = cexy__utils__git_lib_fetch,
        .make_compile_flags = cexy__utils__make_compile_flags,
        .make_new_project = cexy__utils__make_new_project,
        .pkgconf = cexy__utils__pkgconf,
    },

    // clang-format on
};
#endif // defined(CEX_BUILD)

#endif



/*
*                          src/CexParser.c
*/
#if !defined(cex$enable_minimal)

#include <ctype.h>

const char* CexTkn_str[] = {
#define X(name) cex$stringize(name),
    _CexTknList
#undef X
};

// NOTE: lx$ are the temporary macros (will be #undef at the end of this file)
#define lx$next(lx)                                                                                \
    ({                                                                                             \
        char res = '\0';                                                                           \
        if ((lx->cur < lx->content_end)) {                                                         \
            if (*lx->cur == '\n') { lx->line++; lx->col = 0;}                                                  \
            lx->col++;\
            res = *(lx->cur++);                                                                    \
        }                                                                                          \
        res;                                                                                       \
    })

#define lx$rewind(lx)                                                                              \
    ({                                                                                             \
        if (lx->cur > lx->content) {                                                               \
            lx->cur--;                                                                             \
            if (*lx->cur == '\n') { lx->line--; }                                                  \
        }                                                                                          \
    })

#define lx$peek(lx) ((lx->cur < lx->content_end) ? *lx->cur : '\0')

#define lx$peek_next(lx) ((lx->cur + 1 < lx->content_end) ? lx->cur[1] : '\0')

#define lx$skip_space(lx, c)                                                                       \
    while (c && isspace((c))) {                                                                    \
        lx$next(lx);                                                                               \
        (c) = lx$peek(lx);                                                                         \
    }

CexParser_c
CexParser_create(char* content, u32 content_len, bool fold_scopes)
{
    if (content_len == 0) { content_len = str.len(content); }
    CexParser_c lx = { .content = content,
                       .content_end = (content) ? content + content_len : NULL,
                       .cur = content,
                       .fold_scopes = fold_scopes };
    return lx;
}

void
CexParser_reset(CexParser_c* lx)
{
    uassert(lx != NULL);
    lx->cur = lx->content;
    lx->line = 0;
}

static cex_token_s
_CexParser__scan_ident(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__ident, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        if (!(isalnum(c) || c == '_' || c == '$')) {
            lx$rewind(lx);
            break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
_CexParser__scan_number(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__number, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '/':
            case '*':
            case '+':
            case '-':
            case ',':
            case ')':
            case ']':
            case '}':
                lx$rewind(lx);
                return t;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
_CexParser__scan_string(CexParser_c* lx)
{
    cex_token_s t = { .type = (*lx->cur == '"' ? CexTkn__string : CexTkn__char),
                      .value = { .buf = lx->cur + 1, .len = 0 } };
    lx$next(lx);
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case '\\': // escape char, unconditionally skip next
                lx$next(lx);
                t.value.len++;
                break;
            case '\'':
                if (t.type == CexTkn__char) { goto end; }
                break;
            case '"':
                if (t.type == CexTkn__string) { goto end; }
                break;
            default: {
                if (unlikely((u8)c < 0x20)) {
                    t.type = CexTkn__error;
                    t.value = (str_s){ 0 };
                    return t;
                }
            }
        }
        t.value.len++;
    }
end:
    if (unlikely(t.value.buf + t.value.len > lx->content_end)) {
        t = (cex_token_s){ .type = CexTkn__error };
    }
    return t;
}

static cex_token_s
_CexParser__scan_comment(CexParser_c* lx)
{
    cex_token_s t = { .type = lx->cur[1] == '/' ? CexTkn__comment_single : CexTkn__comment_multi,
                      .value = { .buf = lx->cur, .len = 2 } };
    lx$next(lx);
    lx$next(lx);
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case '\n':
                if (t.type == CexTkn__comment_single) {
                    lx$rewind(lx);
                    goto end;
                }
                break;
            case '*':
                if (t.type == CexTkn__comment_multi) {
                    if (lx$peek(lx) == '/') {
                        lx$next(lx);
                        t.value.len += 2;
                        goto end;
                    }
                }
                break;
        }
        t.value.len++;
    }
end:
    if (unlikely(t.value.buf + t.value.len > lx->content_end)) {
        t = (cex_token_s){ .type = CexTkn__error };
    }
    return t;
}

static cex_token_s
_CexParser__scan_preproc(CexParser_c* lx)
{
    lx$next(lx);

    if (lx->cur >= lx->content_end) { return (cex_token_s){ .type = CexTkn__error }; }

    char c = *lx->cur;
    lx$skip_space(lx, c);
    cex_token_s t = { .type = CexTkn__preproc, .value = { .buf = lx->cur, .len = 0 } };

    while ((c = lx$next(lx))) {
        switch (c) {
            case '\n':
                goto end;
            case '\\':
                // next line concat for #define
                t.value.len++;
                if (!lx$next(lx)) {
                    // Oops, we expected next symbol here, but got EOF, invalidate token then
                    t.value.len++;
                    goto end;
                }
                break;
        }
        t.value.len++;
    }
end:
    if (unlikely(t.value.buf + t.value.len > lx->content_end)) {
        t = (cex_token_s){ .type = CexTkn__error };
    }
    return t;
}

static cex_token_s
_CexParser__scan_scope(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__unk, .value = { .buf = lx->cur, .len = 0 } };

    if (!lx->fold_scopes) {
        char c = lx$peek(lx);
        switch (c) {
            case '(':
                t.type = CexTkn__lparen;
                break;
            case '[':
                t.type = CexTkn__lbracket;
                break;
            case '{':
                t.type = CexTkn__lbrace;
                break;
            default:
                unreachable();
        }
        t.value.len = 1;
        lx$next(lx);
        if (unlikely(t.value.buf + t.value.len > lx->content_end)) {
            t = (cex_token_s){ .type = CexTkn__error };
        }
        return t;
    } else {
        char scope_stack[128] = { 0 };
        u32 scope_depth = 0;

#define scope$push(c) /* temp macro! */                                                            \
    if (++scope_depth < sizeof(scope_stack)) { scope_stack[scope_depth - 1] = c; }
#define scope$pop_if(c) /* temp macro! */                                                          \
    if (scope_depth > 0 && scope_depth <= sizeof(scope_stack) &&                                   \
        scope_stack[scope_depth - 1] == c) {                                                       \
        scope_stack[--scope_depth] = '\0';                                                         \
    }

        char c = lx$peek(lx);
        switch (c) {
            case '(':
                t.type = CexTkn__paren_block;
                break;
            case '[':
                t.type = CexTkn__bracket_block;
                break;
            case '{':
                t.type = CexTkn__brace_block;
                break;
            default:
                unreachable();
        }

        while ((c = lx$peek(lx))) {
            switch (c) {
                case '{':
                    scope$push(c);
                    break;
                case '[':
                    scope$push(c);
                    break;
                case '(':
                    scope$push(c);
                    break;
                case '}':
                    scope$pop_if('{');
                    break;
                case ']':
                    scope$pop_if('[');
                    break;
                case ')':
                    scope$pop_if('(');
                    break;
                case '"':
                case '\'': {
                    auto s = _CexParser__scan_string(lx);
                    t.value.len += s.value.len + 2;
                    continue;
                }
                case '/': {
                    if (lx$peek_next(lx) == '/' || lx$peek_next(lx) == '*') {
                        auto s = _CexParser__scan_comment(lx);
                        t.value.len += s.value.len;
                        continue;
                    }
                    break;
                }
                case '#': {
                    char* ppstart = lx->cur;
                    auto s = _CexParser__scan_preproc(lx);
                    if (s.value.buf) {
                        t.value.len += s.value.len + (s.value.buf - ppstart) + 1;
                    } else {
                        goto end;
                    }
                    continue;
                }
            }
            if (lx$next(lx)) { t.value.len++; }

            if (scope_depth == 0) { goto end; }
        }

#undef scope$push
#undef scope$pop_if

end:
    if (unlikely(scope_depth != 0 || t.value.buf + t.value.len > lx->content_end)) {
        t = (cex_token_s){ .type = CexTkn__error };
    }
    return t;
    }
}

cex_token_s
CexParser_next_token(CexParser_c* lx)
{

#define tok$new(tok_type)                                                                          \
    ({                                                                                             \
        lx$next(lx);                                                                               \
        (cex_token_s){ .type = tok_type, .value = { .buf = lx->cur - 1, .len = 1 } };              \
    })

    char c;
    while ((c = lx$peek(lx))) {
        lx$skip_space(lx, c);
        if (!c) { break; }

        if (isalpha(c) || c == '_' || c == '$') { return _CexParser__scan_ident(lx); }
        if (isdigit(c)) { return _CexParser__scan_number(lx); }

        switch (c) {
            case '\'':
            case '"':
                return _CexParser__scan_string(lx);
            case '/':
                if (lx$peek_next(lx) == '/' || lx$peek_next(lx) == '*') {
                    return _CexParser__scan_comment(lx);
                } else {
                    break;
                }
                break;
            case '*':
                return tok$new(CexTkn__star);
            case '.':
                return tok$new(CexTkn__dot);
            case ',':
                return tok$new(CexTkn__comma);
            case ';':
                return tok$new(CexTkn__eos);
            case ':':
                return tok$new(CexTkn__colon);
            case '-':
                return tok$new(CexTkn__minus);
            case '+':
                return tok$new(CexTkn__plus);
            case '?':
                return tok$new(CexTkn__question);
            case '=': {
                if (lx->cur > lx->content) {
                    switch (lx->cur[-1]) {
                        case '=':
                        case '*':
                        case '/':
                        case '~':
                        case '!':
                        case '+':
                        case '-':
                        case '>':
                        case '<':
                            goto unkn;
                        default:
                            break;
                    }
                }
                switch (lx$peek_next(lx)) {
                    case '=':
                        goto unkn;
                    default:
                        break;
                }

                return tok$new(CexTkn__eq);
            }
            case '{':
            case '(':
            case '[':
                return _CexParser__scan_scope(lx);
            case '}':
                return tok$new(CexTkn__rbrace);
            case ')':
                return tok$new(CexTkn__rparen);
            case ']':
                return tok$new(CexTkn__rbracket);
            case '#':
                return _CexParser__scan_preproc(lx);
            default:
                break;
        }


    unkn:
        return tok$new(CexTkn__unk);
    }

#undef tok$new
    return (cex_token_s){ 0 }; // EOF
}

cex_token_s
CexParser_next_entity(CexParser_c* lx, arr$(cex_token_s) * children)
{
    uassert(lx->fold_scopes && "CexParser must be with fold_scopes=true");
    uassert(children != NULL && "non initialized arr$");
    uassert(*children != NULL && "non initialized arr$");
    cex_token_s result = { 0 };

#ifdef CEX_TEST
    log$trace("New entity check...\n");
#endif
    arr$clear(*children);
    cex_token_s t;
    bool has_cex_namespace = false;
    u32 i = 0;
    (void)i;
    while ((t = CexParser_next_token(lx)).type) {
#ifdef CEX_TEST
        log$trace("%02d: %-15s %S\n", i, CexTkn_str[t.type], t.value);
#endif
        if (unlikely(t.type == CexTkn__error)) { goto error; }

        if (unlikely(!result.type)) {
            result.type = CexTkn__global_misc;
            result.value = t.value;
            if (t.type == CexTkn__preproc) {
                result.value.buf--;
                result.value.len++;
            }
        } else {
            // Extend token text
            result.value.len = t.value.buf - result.value.buf + t.value.len;
            uassert(result.value.len < 1024 * 1024 && "token diff it too high, bad pointers?");
        }
        arr$push(*children, t);
        switch (t.type) {
            case CexTkn__preproc: {
                if (str.slice.starts_with(t.value, str$s("define "))) {
                    CexParser_c _lx = CexParser.create(t.value.buf, t.value.len, true);
                    cex_token_s _t = CexParser.next_token(&_lx);

                    _t = CexParser.next_token(&_lx);
                    if (unlikely(_t.type != CexTkn__ident)) {
                        log$trace("Expected ident at %S line: %d\n", t.value, lx->line);
                        goto error;
                    }
                    result.type = CexTkn__macro_const;

                    _t = CexParser.next_token(&_lx);
                    if (_t.type == CexTkn__paren_block) { result.type = CexTkn__macro_func; }
                } else {
                    result.type = CexTkn__preproc;
                }
                goto end;
            }
            case CexTkn__paren_block: {
                if (result.type == CexTkn__var_decl) {
                    if (!str.slice.match(t.value, "\\(\\(*\\)\\)")) {
                        result.type = CexTkn__func_decl; // Check if not __attribute__(())
                    }
                } else {
                    if (i > 0 && children[0][i - 1].type == CexTkn__ident) {
                        if (!str.slice.match(children[0][i - 1].value, "__attribute__")) {
                            result.type = CexTkn__func_decl;
                        }
                    }
                }
                break;
            }
            case CexTkn__brace_block: {
                if (result.type == CexTkn__func_decl) {
                    result.type = CexTkn__func_def;
                    goto end;
                }
                break;
            }
            case CexTkn__eq: {
                if (has_cex_namespace) {
                    result.type = CexTkn__cex_module_def;
                } else {
                    result.type = CexTkn__var_def;
                }
                break;
            }

            case CexTkn__ident: {
                if (str.slice.match(t.value, "(typedef|struct|enum|union)")) {
                    if (result.type != CexTkn__var_decl) { result.type = CexTkn__typedef; }
                } else if (str.slice.match(t.value, "CEX_NAMESPACE") || str.slice.match(t.value, "extern")) {
                    result.type = CexTkn__var_decl;
                } else if (str.slice.match(t.value, "__cex_namespace__*")) {
                    has_cex_namespace = true;
                    if (result.type == CexTkn__var_decl) {
                        result.type = CexTkn__cex_module_decl;
                    } else if (result.type == CexTkn__typedef) {
                        result.type = CexTkn__cex_module_struct;
                    }
                }
                break;
            }
            case CexTkn__eos: {
                goto end;
            }
            default: {
            }
        }
        i++;
        uassert(arr$len(*children) == i);
    }
end:
    return result;

error:
    arr$clear(*children);
    result = (cex_token_s){ .type = CexTkn__error };
    goto end;
}

void
CexParser_decl_free(cex_decl_s* decl, IAllocator alloc)
{
    (void)alloc; // maybe used in the future
    if (decl) {
        sbuf.destroy(&decl->args);
        sbuf.destroy(&decl->ret_type);
        mem$free(alloc, decl);
    }
}

cex_decl_s*
CexParser_decl_parse(
    CexParser_c* lx,
    cex_token_s decl_token,
    arr$(cex_token_s) children,
    char* ignore_keywords_pattern,
    IAllocator alloc
)
{
    (void)children;
    switch (decl_token.type) {
        case CexTkn__func_decl:
        case CexTkn__func_def:
        case CexTkn__macro_func:
        case CexTkn__macro_const:
        case CexTkn__typedef:
        case CexTkn__cex_module_struct:
        case CexTkn__cex_module_def:
            break;
        default:
            return NULL;
    }
    cex_decl_s* result = mem$new(alloc, cex_decl_s);
    result->args = sbuf.create(128, alloc);
    result->ret_type = sbuf.create(128, alloc);
    result->type = decl_token.type;

    // CexParser line is at the end of token, find, the beginning
    result->line = lx->line;

    char* ignore_pattern =
        "(__attribute__|static|inline|__asm__|extern|volatile|restrict|register|__declspec|noreturn|_Noreturn)";


    cex_token_s prev_t = { 0 };
    bool prev_skipped = false;
    isize name_idx = -1;
    isize args_idx = -1;
    isize idx = -1;
    isize prev_idx = -1;

    for$each (it, children) {
        idx++;
        switch (it.type) {
            case CexTkn__ident: {
                if (str.slice.eq(it.value, str$s("static"))) { result->is_static = true; }
                if (str.slice.eq(it.value, str$s("inline"))) { result->is_inline = true; }
                if (str.slice.match(it.value, ignore_pattern)) {
                    prev_skipped = true;
                    continue;
                }
                if (ignore_keywords_pattern && str.slice.match(it.value, ignore_keywords_pattern)) {
                    prev_skipped = true;
                    continue;
                }
                if (decl_token.type == CexTkn__typedef) {
                    if (str.slice.match(prev_t.value, "(struct|enum|union)")) {
                        name_idx = idx;
                        result->name = it.value;
                    }
                }
                if (decl_token.type == CexTkn__func_decl) {
                    // Function pointer typedef
                    if (str.slice.eq(it.value, str$s("typedef"))) {
                        result->type = CexTkn__typedef;
                    }
                } else if (decl_token.type == CexTkn__cex_module_struct ||
                           decl_token.type == CexTkn__cex_module_def) {
                    str_s ns_prefix = str$s("__cex_namespace__");
                    if (str.slice.starts_with(it.value, ns_prefix)) {
                        result->name = str.slice.sub(it.value, ns_prefix.len, 0);
                    }
                }
                prev_skipped = false;
                break;
            }
            case CexTkn__preproc: {
                if (decl_token.type == CexTkn__macro_func ||
                    decl_token.type == CexTkn__macro_const) {
                    args_idx = -1;
                    uassert(it.value.len > 0);
                    CexParser_c _lx = CexParser.create(it.value.buf, it.value.len, true);

                    cex_token_s _t = CexParser.next_token(&_lx);
                    uassert(str.slice.starts_with(_t.value, str$s("define")));

                    _t = CexParser.next_token(&_lx);
                    if (_t.type != CexTkn__ident) {
                        log$trace("Expected ident at %S\n", it.value);
                        goto fail;
                    }
                    result->name = _t.value;
                    char* prev_end = _t.value.buf + _t.value.len;
                    _t = CexParser.next_token(&_lx);
                    if (_t.type == CexTkn__paren_block) {
                        auto _args = _t.value.len > 2 ? str.slice.sub(_t.value, 1, -1) : str$s("");
                        e$goto(sbuf.appendf(&result->args, "%S", _args), fail);
                        prev_end = _t.value.buf + _t.value.len;
                        _t = CexParser.next_token(&_lx);
                    } // Macro body
                    if (_t.type) {
                        isize decl_len = decl_token.value.len - (prev_end - decl_token.value.buf);
                        uassert(decl_len > 0);
                        result->body = (str_s){
                            .buf = prev_end,
                            .len = decl_len,
                        };
                    }
                }

                break;
            }
            case CexTkn__comment_multi:
            case CexTkn__comment_single: {
                if (result->name.buf == NULL && sbuf.len(&result->ret_type) == 0 &&
                    sbuf.len(&result->args) == 0) {
                    str_s cmt = str.slice.strip(it.value);
                    // Use only doxygen style comments for docs
                    if (str.slice.match(cmt, "(/**|/*!)*")) {
                        result->docs = cmt;
                    } else if (str.slice.starts_with(cmt, str$s("///"))) {
                        if (prev_t.type == CexTkn__comment_single && result->docs.buf) {
                            // Trying extend previous /// comment
                            result->docs.len = (cmt.buf - result->docs.buf) + cmt.len;
                        } else {
                            result->docs = cmt;
                        }
                    }
                }
                break;
            }
            case CexTkn__bracket_block: {
                if (str.slice.starts_with(it.value, str$s("[["))) {
                    // Clang/C23 attribute
                    continue;
                }
                fallthrough();
            }
            case CexTkn__brace_block: {
                result->body = it.value;
                fallthrough();
            }
            case CexTkn__eos: {
                if (prev_t.type == CexTkn__paren_block) { args_idx = prev_idx; }
                if (decl_token.type == CexTkn__typedef && prev_t.type == CexTkn__ident &&
                    !str.slice.match(prev_t.value, "(struct|enum|union)")) {
                    if (name_idx < 0) { name_idx = idx; }
                    result->name = prev_t.value;
                }
                break;
            }
            case CexTkn__paren_block: {
                if (prev_skipped) {
                    prev_skipped = false;
                    continue;
                }

                if (prev_t.type == CexTkn__ident) {
                    if (result->name.buf == NULL) {
                        if (str.slice.match(it.value, "\\(\\**\\)")) {
#if defined(CEX_TEST)
                            // this looks a function returning function pointer,
                            // we intentionally don't support this, use typedef func ptr
                            log$error(
                                "Skipping function (returns raw fn pointer, use typedef): \n%S\n",
                                decl_token.value
                            );
#endif
                            goto fail;
                        }
                    }
                    // NOTE: name_idx may change until last {} or ;
                    // because in C is common to use MACRO(foo) which may look as args block
                    // we'll check it after the end of entity block
                    result->name = prev_t.value;
                    name_idx = idx;
                }

                break;
            }
            default:
                break;
        }

        prev_skipped = false;
        prev_t = it;
        prev_idx = idx;
    }

#define $append_fmt(buf, tok) /* temp macro, formats return type and arguments */                  \
    switch ((tok).type) {                                                                          \
        case CexTkn__eof:                                                                          \
        case CexTkn__comma:                                                                        \
        case CexTkn__star:                                                                         \
        case CexTkn__dot:                                                                          \
        case CexTkn__rbracket:                                                                     \
        case CexTkn__lbracket:                                                                     \
        case CexTkn__lparen:                                                                       \
        case CexTkn__rparen:                                                                       \
        case CexTkn__paren_block:                                                                  \
        case CexTkn__bracket_block:                                                                \
            break;                                                                                 \
        default:                                                                                   \
            if (sbuf.len(&(buf)) > 0) { e$goto(sbuf.append(&(buf), " "), fail); }                  \
    }                                                                                              \
    e$goto(sbuf.appendf(&(buf), "%S", (tok).value), fail);
    //  <<<<<  #define $append_fmt

    // Exclude items with starting _ or special functions or other stuff
    if (str.slice.match(result->name, "(_Static_assert|static_assert)")) {
        goto fail;
    }

    if (name_idx > 0) {
        // NOTE: parsing return type
        prev_skipped = false;
        for$each (it, children, name_idx - 1) {
            switch (it.type) {
                case CexTkn__ident: {
                    if (str.slice.match(it.value, ignore_pattern)) {
                        prev_skipped = true;
                        // GCC attribute
                        continue;
                    }
                    if (ignore_keywords_pattern &&
                        str.slice.match(it.value, ignore_keywords_pattern)) {
                        prev_skipped = true;
                        // GCC attribute
                        continue;
                    }
                    $append_fmt(result->ret_type, it);
                    break;
                }
                case CexTkn__bracket_block: {
                    if (str.slice.starts_with(it.value, str$s("[["))) {
                        // Clang/C23 attribute
                        continue;
                    }
                    break;
                }
                case CexTkn__brace_block:
                case CexTkn__comment_multi:
                case CexTkn__comment_single:
                    continue;
                case CexTkn__paren_block: {
                    if (prev_skipped) {
                        prev_skipped = false;
                        continue;
                    }
                    fallthrough();
                }
                default:
                    $append_fmt(result->ret_type, it);
            }
            prev_skipped = false;
        }
    }

    // NOTE: parsing arguments of a function
    if (args_idx >= 0) {
        prev_t = children[args_idx];
        str_s clean_paren_block = str.slice.sub(str.slice.strip(prev_t.value), 1, -1);
        CexParser_c lx = CexParser_create(clean_paren_block.buf, clean_paren_block.len, true);
        cex_token_s t = { 0 };
        bool skip_next = false;
        while ((t = CexParser_next_token(&lx)).type) {
#ifdef CEX_TEST
            log$trace("arg token: type: %s '%S'\n", CexTkn_str[t.type], t.value);
#endif
            switch (t.type) {
                case CexTkn__ident: {
                    if (str.slice.match(t.value, ignore_pattern)) {
                        skip_next = true;
                        continue;
                    }
                    if (ignore_keywords_pattern &&
                        str.slice.match(t.value, ignore_keywords_pattern)) {
                        skip_next = true;
                        continue;
                    }
                    break;
                }
                case CexTkn__paren_block: {
                    if (skip_next) {
                        skip_next = false;
                        continue;
                    }
                    break;
                }
                case CexTkn__comment_multi:
                case CexTkn__comment_single:
                    continue;
                case CexTkn__bracket_block: {
                    if (str.slice.starts_with(t.value, str$s("[["))) { continue; }
                    fallthrough();
                }
                default: {
                    break;
                }
            }
            $append_fmt(result->args, t);
            skip_next = false;
        }
    }

    if (decl_token.value.len > 0 && result->name.buf) {
        char* cur = lx->cur - 1;
        while (cur > result->name.buf) {
            if (*cur == '\n') {
                if (result->line > 0) { result->line--; }
            }
            cur--;
        }
    }
    if (!result->name.buf) {
        log$trace(
            "Decl without name of type %s, at line: %d\n",
            CexTkn_str[result->type],
            result->line
        );
        goto fail;
    }
#undef $append_fmt
    return result;

fail:
    CexParser_decl_free(result, alloc);
    return NULL;
}


#undef lx$next
#undef lx$peek
#undef lx$skip_space

const struct __cex_namespace__CexParser CexParser = {
    // Autogenerated by CEX
    // clang-format off

    .create = CexParser_create,
    .decl_free = CexParser_decl_free,
    .decl_parse = CexParser_decl_parse,
    .next_entity = CexParser_next_entity,
    .next_token = CexParser_next_token,
    .reset = CexParser_reset,

    // clang-format on
};

#endif



/*
*                          src/cex_maker.c
*/
#if !defined(cex$enable_minimal)

#if defined(CEX_NEW)
#    if __has_include("cex.c")
#        error                                                                                     \
            "cex.c already exists, CEX project seems initialized, try run `gcc[clang] ./cex.c -o cex`"
#    endif

int
main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    e$except (err, cexy.utils.make_new_project(".")) { return 1; }
    io.printf("\n\nMOCCA - Make Old C Cexy Again!\n");
    io.printf("Cex project has been initialized!\n");
    io.printf("See the 'cex.c' building script for more info.\n");
    io.printf("\nNow you can use `./cex` command to do stuff:\n");
    io.printf("- `./cex --help` for getting CEX capabilities\n");
    io.printf("- `./cex test run all` for running all tests\n");
    io.printf("- `./cex app run myapp` for running sample app\n");
    io.printf("- `./cex help --help` for searching projec symbols and examples\n");
    return 0;
}
#endif

#endif



/*
*                          src/fuzz.c
*/
#if !defined(cex$enable_minimal)


/// Creates new fuzz data generator, for fuzz-driven randomization
cex_fuzz_s
cex_fuzz_create(const u8* data, usize size)
{
    return (cex_fuzz_s){
        .cur = data,
        .end = data + size,
    };
}

/// Get result from random data into buffer (returns false if not enough data)
bool
cex_fuzz_dget(cex_fuzz_s* fz, void* out_result, usize result_size)
{
    uassert(out_result != NULL);
    if (fz->end - fz->cur < (isize)result_size) { return false; }
    memcpy(out_result, fz->cur, result_size);
    fz->cur += result_size;
    return true;
}

/// Get current corpus dir relative tho the `this_file_name`
char*
cex_fuzz_corpus_dir(char* this_file_name)
{
    static char buf[256] = {0};

    if (buf[0] != '\0') {
        // already filled 
        return buf;
    }

    uassert(this_file_name);
    uassert(this_file_name[0] != '\0');
    uassert(str.len(this_file_name) < sizeof(buf)-1);
    uassert(str.ends_with(this_file_name, ".c"));
    if(str.sprintf(buf, sizeof(buf), "%S_corpus", str.sub(this_file_name, 0, -2))) {
        return NULL;
    }

    return buf;
}

/// Get probability using fuzz data, based on threshold
bool
cex_fuzz_dprob(cex_fuzz_s* fz, double threshold)
{
    uassert(
        threshold > 0 && threshold < 1.0 &&
        "Probability must be > 0 and < 1.0, 0.1 = 10% probability"
    );
    uassert(threshold > 1.0 / 255.0 && "Probability granularity is too low");
    if (fz->cur >= fz->end) { return false; }

    double p = (double)fz->cur[0];
    fz->cur++;

    return (p / 255.0) <= threshold;
}

const struct __cex_namespace__fuzz fuzz = {
    // Autogenerated by CEX
    // clang-format off

    .corpus_dir = cex_fuzz_corpus_dir,
    .create = cex_fuzz_create,
    .dget = cex_fuzz_dget,
    .dprob = cex_fuzz_dprob,

    // clang-format on
};

#endif



/*
*                          src/cex_footer.c
*/
/* NOLINTBEGIN */                                                                          \
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
/* NOLINTEND */                                                                            \

/*
## Credits

CEX contains some code and ideas from the following projects, all of them licensed under MIT license (or Public Domain):

1. [nob.h](https://github.com/tsoding/nob.h) - by Tsoding / Alexey Kutepov, MIT/Public domain, great idea of making self-contained build system, great youtube channel btw
2. [stb_ds.h](https://github.com/nothings/stb/blob/master/stb_ds.h) - MIT/Public domain, by Sean Barrett, CEX arr$/hm$ are refactored versions of STB data structures, great idea 
3. [stb_sprintf.h](https://github.com/nothings/stb/blob/master/stb_sprintf.h) - MIT/Public domain, by Sean Barrett, I refactored it, fixed all UB warnings from UBSAN, added CEX specific formatting
4. [minirent.h](https://github.com/tsoding/minirent) - Alexey Kutepov, MIT license, WIN32 compatibility lib 
5. [subprocess.h](https://github.com/sheredom/subprocess.h) - by Neil Henning, public domain, used in CEX as a daily driver for `os$cmd` and process communication
6. [utest.h](https://github.com/sheredom/utest.h) - by Neil Henning, public domain, CEX test$ runner borrowed some ideas of macro magic for making declarative test cases
7. [c3-lang](https://github.com/c3lang/c3c) - I borrowed some ideas about language features from C3, especially `mem$scope`, `mem$`/`tmem$` global allocators, scoped macros too.


## License

```

MIT License

Copyright (c) 2024-2025 Aleksandr Vedeneev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

```
*/



#endif // ifndef CEX_IMPLEMENTATION


#endif // ifndef CEX_HEADER_H
