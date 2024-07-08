
#pragma once
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

#define len(arr) (sizeof(arr) / sizeof(arr[0]))
#define let __auto_type

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define likely(expr) __builtin_expect(!!(expr), 1)

#define __CEX_CONCAT_INNER(a, b) a##b
#define __CEX_CONCAT(a, b) __CEX_CONCAT_INNER(a, b)
#define __CEX_TMPNAME(base) __CEX_CONCAT(base, __LINE__)

/*
 *                 ERRORS
 */

typedef const char* Exc;
#define EOK (Exc) NULL
#define Exception Exc __attribute__((warn_unused_result))
#define ExceptionSkip Exc __attribute__((warn_unused_result))

const struct
{
    Exc ok; // must be the 1st
    Exc memory;
    Exc io;
    Exc overflow;
    Exc argument;
    Exc integrity;
    Exc exists;
    Exc not_found;
    Exc shared_mem;
    Exc quote_pool;
    Exc protocol;
    Exc timeout;
    Exc offline;
    Exc delay;
    Exc algo_order;
    Exc skip;
    Exc strategy;
    Exc network;
    Exc check;
} Error = {
    // FIX: implement Error struct validation func/macro
    .ok = EOK,                      // Success
    .memory = "MemoryError",        // memory allocation error
    .io = "IOError",                // IO error
    .overflow = "OverflowError",    // buffer overflow
    .argument = "ArgumentError",    // function argument error
    .integrity = "IntegrityError",  // data integrity error
    .exists = "ExistsError",        // entity or key already exists
    .not_found = "NotFoundError",   // entity or key already exists
    .shared_mem = "SharedMemError", // shared memory error
    .quote_pool = "QuotePoolError", // quote pool error
    .protocol = "ProtocolError",    // protocol error
    .timeout = "TimeoutError",      // resource timeout
    .offline = "OfflineError",      // resource offline
    .delay = "DelayError",          // resource delay
    .algo_order = "AlgoOrderError", // algo order / protocol sequence error
    .skip = "ShouldBeSkipped",      // NOT an error, function result must be skipped
    .strategy = "StrategyError",    // generic UHF strategy error
    .network = "NetworkError",      // generic network error
    .check = "SanityCheckError",    // generic error checking failed
};


/// Strips full path of __FILENAME__ to the file basename
#define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)

#define uptraceback(uerr, fail_func)                                                               \
    (fprintf(                                                                                      \
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
    (fprintf(                                                                                      \
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

static inline bool
_uhf_errors_is_error__system(int syscall_res, int* out_result)
{
    if (out_result != NULL) {
        *out_result = syscall_res;
    }
    return syscall_res == -1;
}

// WARNING: DO NOT USE break/continue inside except* {scope!}
#define except(_var_name, _func)                                                                   \
    for (Exc _var_name = _func; unlikely(_uhf_errors_is_error__uerr(&_var_name)); _var_name = EOK)

#define except_traceback(_var_name, _func)                                                         \
    for (Exc _var_name = _func; unlikely(_var_name != NULL && (uptraceback(_var_name, #_func)));   \
         _var_name = EOK)

#define except_system(_result_out_ptr, _func)                                                      \
    for (int __ctr = 0, __except_system_var = _func;                                               \
         __ctr == 0 &&                                                                             \
         unlikely(_uhf_errors_is_error__system(__except_system_var, _result_out_ptr));             \
         __ctr++)

#define except_null(_expression)                                                                   \
    if (((_expression) == NULL) && uperrorf("`%s` returned NULL\n", #_expression))

#define raise_exc(return_uerr, error_msg, ...)                                                     \
    (uperrorf("[%s] " error_msg, return_uerr, ##__VA_ARGS__), (return_uerr))


/**
 *                 ASSERTIONS MACROS
 */

#ifdef UHFTEST
// this prevents spamming on stderr (i.e. atest.h output stream in silent mode)
#define UPERRORF_OUT__ stdout
#else
#define UPERRORF_OUT__ stderr
#endif


#ifdef NDEBUG
#define utracef(format, ...) ((void)(0))
#define uassertf(cond, format, ...) ((void)(0))
#define uassert(cond) ((void)(0))
#else
#define utracef(format, ...)                                                                       \
    (fprintf(                                                                                      \
        stdout,                                                                                    \
        "[TRACE] ( %s:%d %s() ) " format,                                                          \
        __FILENAME__,                                                                              \
        __LINE__,                                                                                  \
        __func__,                                                                                  \
        ##__VA_ARGS__                                                                              \
    ))

int __cex_test_uassert_enabled = 1;

#ifdef UHFTEST
#define uassert_disable() __cex_test_uassert_enabled = 0
#define uassert_enable() __cex_test_uassert_enabled = 1
#define uassert_is_enabled() (__cex_test_uassert_enabled)
#else
#define uassert_disable() (void)0
#define uassert_enable() (void)0
#define uassert_is_enabled() true
#endif

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
                fprintf(                                                                           \
                    uassert_is_enabled() ? stderr : UPERRORF_OUT__,                                \
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
                fprintf(                                                                           \
                    uassert_is_enabled() ? stderr : UPERRORF_OUT__,                                \
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

#define uerrcheck(condition)                                                                       \
    do {                                                                                           \
        if (unlikely(!((condition)))) {                                                            \
            fprintf(                                                                               \
                UPERRORF_OUT__,                                                                    \
                "[UERRCHK] ( %s:%d %s() ) Check failed: %s\n",                                     \
                __FILENAME__,                                                                      \
                __LINE__,                                                                          \
                __func__,                                                                          \
                #condition                                                                         \
            );                                                                                     \
            return Error.check;                                                                    \
        }                                                                                          \
    } while (0)

/*
 *                  ARRAY / ITERATORS INTERFACE
 */


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
 * for$iter(u32, it, array_iterator(arr2, len(arr2), &it.iterator))
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
 *                 ALLOCATORS
 */

typedef struct Allocator_c
{
    u64 magic;
    void* (*alloc)(size_t);
    void* (*realloc)(void*, size_t);
    void* (*alloc_aligned)(size_t, size_t);
    void* (*realloc_aligned)(void*, size_t, size_t);
    void* (*free)(void*);
} Allocator_c;
_Static_assert(sizeof(Allocator_c) == 48, "size!");
