/**
 * @file cex/cex.h
 * @brief CEX core file
 */
#pragma once
#include "cex_header.h"

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
Error.null_or_empty = "NullOrEmptyError";           // value is null or resource is empty
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
    Exc null_or_empty;
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
