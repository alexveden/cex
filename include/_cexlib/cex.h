#pragma once
#include <errno.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

#define var __auto_type

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define likely(expr) __builtin_expect(!!(expr), 1)

// __CEX_TMPNAME - internal macro for generating temporary variable names (unique__line_num)
#define __CEX_CONCAT_INNER(a, b) a##b
#define __CEX_CONCAT(a, b) __CEX_CONCAT_INNER(a, b)
#define __CEX_TMPNAME(base) __CEX_CONCAT(base, __LINE__)

// TODO: decide what to do with it
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

/*
 *                 ERRORS
 */

typedef const char* Exc;

#define Exception Exc __attribute__((warn_unused_result))
#define ExceptionSkip Exc __attribute__((warn_unused_result))


#define EOK (Exc) NULL
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
    Exc sanity_check;
    Exc empty;
    Exc eof;
    Exc argsparse;
} Error;

/// Strips full path of __FILENAME__ to the file basename
#define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)


#ifndef __cex__fprintf
/// analog of fprintf, but preserves errno for user space
// NOTE: you may try to define our own fprintf
#define __cex__fprintf fprintf

// If you define this macro it will turn off all debug printing
// #define __cex__fprintf(stream, format, ...) (true)

#endif


#define uptraceback(uerr, fail_func)                                                               \
    (__cex__fprintf(                                                                               \
         stdout,                                                                                   \
         "[^STCK] ( %s:%d %s() ) ^^^^^ [%s] in %s\n",                                              \
         __FILENAME__,                                                                             \
         __LINE__,                                                                                 \
         __func__,                                                                                 \
         uerr,                                                                                     \
         fail_func                                                                                 \
     ),                                                                                            \
     1)

#define uperrorf(format, ...)                                                                      \
    (__cex__fprintf(                                                                               \
        stdout,                                                                                    \
        "[ERROR] ( %s:%d %s() ) " format,                                                          \
        __FILENAME__,                                                                              \
        __LINE__,                                                                                  \
        __func__,                                                                                  \
        ##__VA_ARGS__                                                                              \
    ))


static inline bool
_uhf_errors_is_error__uerr(Exc* e)
{
    return *e != NULL;
}


// WARNING: DO NOT USE break/continue inside except* {scope!}
#define except(_var_name, _func)                                                                   \
    for (Exc _var_name = _func; unlikely(_uhf_errors_is_error__uerr(&_var_name)); _var_name = EOK)

#define except_traceback(_var_name, _func)                                                         \
    for (Exc _var_name = _func; unlikely(_var_name != NULL && (uptraceback(_var_name, #_func)));   \
         _var_name = EOK)

#define e$ret(_func)                                                                                  \
    for (Exc __CEX_TMPNAME(__cex_err_traceback_) = _func; unlikely(                                \
             __CEX_TMPNAME(__cex_err_traceback_) != NULL &&                                        \
             (uptraceback(__CEX_TMPNAME(__cex_err_traceback_), #_func))                            \
         );                                                                                        \
         __CEX_TMPNAME(__cex_err_traceback_) = EOK)                                                \
    return __CEX_TMPNAME(__cex_err_traceback_)

#define e$goto(_func, _label)                                                                                  \
    for (Exc __CEX_TMPNAME(__cex_err_traceback_) = result = _func; unlikely(                                \
             __CEX_TMPNAME(__cex_err_traceback_) != NULL &&                                        \
             (uptraceback(__CEX_TMPNAME(__cex_err_traceback_), #_func))                            \
         );                                                                                        \
         __CEX_TMPNAME(__cex_err_traceback_) = EOK)                                                \
    goto _label

#define except_errno(_expression)                                                                  \
    errno = 0;                                                                                     \
    for (int __CEX_TMPNAME(__cex_errno_traceback_ctr) = 0,                                         \
             __CEX_TMPNAME(__cex_errno_traceback_) = (_expression);                                \
         __CEX_TMPNAME(__cex_errno_traceback_ctr) == 0 &&                                          \
         __CEX_TMPNAME(__cex_errno_traceback_) == -1 &&                                            \
         uperrorf("`%s` failed errno: %d, msg: %s\n", #_expression, errno, strerror(errno));       \
         __CEX_TMPNAME(__cex_errno_traceback_ctr)++)

#define except_null(_expression)                                                                   \
    if (((_expression) == NULL) && uperrorf("`%s` returned NULL\n", #_expression))

#define raise_exc(return_uerr, error_msg, ...)                                                     \
    (uperrorf("[%s] " error_msg, return_uerr, ##__VA_ARGS__), (return_uerr))


/**
 *                 ASSERTIONS MACROS
 */



#ifdef NDEBUG
#define utracef(format, ...) ((void)(0))
#define uassertf(cond, format, ...) ((void)(0))
#define uassert(cond) ((void)(0))
#else
#define utracef(format, ...)                                                                       \
    (__cex__fprintf(                                                                               \
        stdout,                                                                                    \
        "[TRACE] ( %s:%d %s() ) " format,                                                          \
        __FILENAME__,                                                                              \
        __LINE__,                                                                                  \
        __func__,                                                                                  \
        ##__VA_ARGS__                                                                              \
    ))


#ifdef __SANITIZE_ADDRESS__
// This should be linked when gcc sanitizer enabled
void __sanitizer_print_stack_trace();
#define sanitizer_stack_trace() __sanitizer_print_stack_trace()
#else
#define sanitizer_stack_trace() ((void)(0))
#endif

#define uassert(A)                                                                                 \
    do {                                                                                           \
        if (unlikely(!((A)))) {                                                                    \
            if (uassert_is_enabled()) {                                                            \
                __cex__fprintf(                                                                    \
                    stderr,                                                                        \
                    "[ASSERT] ( %s:%d %s() ) %s\n",                                                \
                    __FILENAME__,                                                                  \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    #A                                                                             \
                );                                                                                 \
                sanitizer_stack_trace();                                                           \
                abort();                                                                           \
            }                                                                                      \
        }                                                                                          \
    } while (0)

#define uassertf(A, format, ...)                                                                   \
    do {                                                                                           \
        if (unlikely(!((A)))) {                                                                    \
            if (uassert_is_enabled()) {                                                            \
                __cex__fprintf(                                                                    \
                    stderr,                                                                        \
                    "[ASSERT] ( %s:%d %s() ) " format "\n",                                        \
                    __FILENAME__,                                                                  \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    ##__VA_ARGS__                                                                  \
                );                                                                                 \
                sanitizer_stack_trace();                                                           \
                abort();                                                                           \
            }                                                                                      \
        }                                                                                          \
    } while (0)
#endif


#ifdef CEXTEST
// this prevents spamming on stderr (i.e. cextest.h output stream in silent mode)
#define CEXERRORF_OUT__ stdout

int __cex_test_uassert_enabled = 0;
#define uassert_disable() __cex_test_uassert_enabled = 0
#define uassert_enable() __cex_test_uassert_enabled = 1
#define uassert_is_enabled() (__cex_test_uassert_enabled)
#else
#define uassert_disable()                                                                          \
    uassert(false && "uassert_disable() allowed only when compiled with -DCEXTEST")
#define uassert_enable() (void)0
#define uassert_is_enabled() true
#define CEXERRORF_OUT__ stderr
#endif

/*
 *                  ARRAY / ITERATORS INTERFACE
 */

#define arr$len(arr) (sizeof(arr) / sizeof(arr[0]))

/**
 * @brief Iterates through array: itvar is struct {eltype* val, size_t index}
 */
#define for$array(it, array, length)                                                               \
    struct __CEX_TMPNAME(__cex_arriter_)                                                           \
    {                                                                                              \
        typeof(*array)* val;                                                                       \
        size_t idx;                                                                                \
    };                                                                                             \
    size_t __CEX_TMPNAME(__cex_arriter__length) = (length); /* prevents multi call of (length)*/   \
    for (struct __CEX_TMPNAME(__cex_arriter_) it = { .val = array, .idx = 0 };                     \
         it.idx < __CEX_TMPNAME(__cex_arriter__length);                                            \
         it.val++, it.idx++)

/**
 * @brief cex_iterator_s - CEX iterator interface
 */
typedef struct
{
    // NOTE: fields order must match with for$iter union!
    void* val;
    struct
    {
        union
        {
            size_t i;
            u64 ikey;
            char* skey;
            void* pkey;
        };
    } idx;
    char _ctx[48];
} cex_iterator_s;
_Static_assert(sizeof(size_t) == sizeof(void*), "size_t expected as sizeof ptr");
_Static_assert(sizeof(size_t) == sizeof(unsigned long), "size_t expected as u64");
_Static_assert(alignof(cex_iterator_s) == alignof(void*), "alignof");
_Static_assert(sizeof(cex_iterator_s) == 64, "cex size");

/**
 * @brief Iterates via iterator function (see usage below)
 *
 * Iterator function signature:
 * u32* array_iterator(u32 array[], u32 len, cex_iterator_s* iter)
 *
 * for$iter(u32, it, array_iterator(arr2, arr$len(arr2), &it.iterator))
 */
#define for$iter(eltype, it, iter_func)                                                            \
    union __CEX_TMPNAME(__cex_iter_)                                                               \
    {                                                                                              \
        cex_iterator_s iterator;                                                                   \
        struct /* NOTE:  iterator above and this struct shadow each other */                       \
        {                                                                                          \
            typeof(eltype)* val;                                                                   \
            struct                                                                                 \
            {                                                                                      \
                union                                                                              \
                {                                                                                  \
                    size_t i;                                                                      \
                    u64 key;                                                                       \
                    char* skey;                                                                    \
                };                                                                                 \
            } idx;                                                                                 \
        };                                                                                         \
    };                                                                                             \
                                                                                                   \
    for (union __CEX_TMPNAME(__cex_iter_) it = { .val = (iter_func) }; it.val != NULL;             \
         it.val = (iter_func))


/*
 *                 RESOURCE ALLOCATORS
 */

typedef struct Allocator_i
{
    // >>> cacheline
    void* (*malloc)(size_t size);
    void* (*calloc)(size_t nmemb, size_t size);
    void* (*realloc)(void* ptr, size_t new_size);
    void* (*malloc_aligned)(size_t alignment, size_t size);
    void* (*realloc_aligned)(void* ptr, size_t alignment, size_t new_size);
    void (*free)(void* ptr);
    FILE* (*fopen)(const char* filename, const char* mode);
    int (*open)(const char* pathname, int flags, mode_t mode);
    //<<< 64 byte cacheline
    int (*fclose)(FILE* stream);
    int (*close)(int fd);
} Allocator_i;
_Static_assert(sizeof(Allocator_i) == 80, "size");
_Static_assert(alignof(Allocator_i) == 8, "size");
