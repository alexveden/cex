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
//#define __cex__fprintf(stream, format, ...) (true)

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

#define e$(_func)                                                                                  \
    for (Exc __CEX_TMPNAME(__cex_err_traceback_) = _func; unlikely(                                \
             __CEX_TMPNAME(__cex_err_traceback_) != NULL &&                                        \
             (uptraceback(__CEX_TMPNAME(__cex_err_traceback_), #_func))                            \
         );                                                                                        \
         __CEX_TMPNAME(__cex_err_traceback_) = EOK)                                                \
    return __CEX_TMPNAME(__cex_err_traceback_)

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

#ifdef CEXTEST
// this prevents spamming on stderr (i.e. cextest.h output stream in silent mode)
#define CEXERRORF_OUT__ stdout
#define uassert_disable() __cex_test_uassert_enabled = 0
#define uassert_enable() __cex_test_uassert_enabled = 1
#define uassert_is_enabled() (__cex_test_uassert_enabled)
#else
#define uassert_disable() (void)0
#define uassert_enable() (void)0
#define uassert_is_enabled() true
#define CEXERRORF_OUT__ stderr
#endif

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

int __cex_test_uassert_enabled = 1;


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

#define uerrcheck(condition)                                                                       \
    do {                                                                                           \
        if (unlikely(!((condition)))) {                                                            \
            __cex__fprintf(                                                                        \
                CEXERRORF_OUT__,                                                                   \
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


/*
*                   allocators.h
*/
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


typedef struct
{
    alignas(64) const Allocator_i base;
    // below goes sanity check stuff
    u64 magic;
    struct
    {
        u32 n_allocs;
        u32 n_reallocs;
        u32 n_free;
        u32 n_fopen;
        u32 n_fclose;
        u32 n_open;
        u32 n_close;
    } stats;
} allocator_heap_s;
_Static_assert(sizeof(allocator_heap_s) == 128, "size!");
_Static_assert(offsetof(allocator_heap_s, base) == 0, "base must be the 1st struct member");

typedef struct
{
    alignas(64) const Allocator_i base;
    void* mem;
    void* next;
    void* max;
    // below goes sanity check stuff for debug builds
    u64 magic;
    struct
    {
        u32 n_allocs;
        u32 n_reallocs;
        u32 n_free;
        u32 n_fopen;
        u32 n_fclose;
        u32 n_open;
        u32 n_close;
    } stats;

} allocator_staticarena_s;
_Static_assert(sizeof(allocator_staticarena_s) == 192, "size!");
_Static_assert(offsetof(allocator_staticarena_s, base) == 0, "base must be the 1st struct member");

struct __module__allocators
{
    // Autogenerated by CEX
    // clang-format off


struct {  // sub-module .heap >>>
    /**
     * @brief  heap-based allocator (simple proxy for malloc/free/realloc)
     */
    const Allocator_i*
    (*create)(void);

    const Allocator_i*
    (*destroy)(void);

} heap;  // sub-module .heap <<<

struct {  // sub-module .staticarena >>>
    /**
     * @brief Static arena allocator (can be heap or stack arena)
     *
     * Static allocator should be created at the start of the application (maybe in main()),
     * and freed after app shutdown.
     *
     * Note: memory leaks are not caught by sanitizers, if you forget to call
     * allocators.staticarena.destroy() sanitizers will be silent.
     *
     * No realloc() supported by this arena!
     *
     * @param buffer - pointer to memory buffer
     * @param capacity - capacity of a buffer (minimal requires is 1024)
     * @return  allocator instance
     */
    const Allocator_i*
    (*create)(char* buffer, size_t capacity);

    const Allocator_i*
    (*destroy)(void);

} staticarena;  // sub-module .staticarena <<<
    // clang-format on
};
extern const struct __module__allocators allocators; // CEX Autogen

/*
*                   str.h
*/
#include <stdalign.h>
#include <stdint.h>
#include <string.h>


typedef struct
{
    // NOTE: len comes first which prevents bad casting of str_c to char*
    // stb_sprintf() has special case of you accidentally pass str_c to
    // %s format specifier (it will show -> (str_c->%S))
    size_t len;
    char* buf;
} str_c;

static inline str_c
_str__propagate_inline_small_func(str_c s)
{
    // this function only for s$(<str_c>)
    return s;
}

// clang-format off
#define s$(string)                                             \
    _Generic(string,                                           \
        char*: str.cstr,                                     \
        const char*: str.cstr,                               \
        str_c: _str__propagate_inline_small_func           \
    )(string)
// clang-format on

_Static_assert(sizeof(str_c) == 16, "size");
_Static_assert(alignof(str_c) == 8, "size");


struct __module__str
{
    // Autogenerated by CEX
    // clang-format off

str_c
(*cstr)(const char* ccharptr);

str_c
(*cbuf)(char* s, size_t length);

str_c
(*sub)(str_c s, ssize_t start, ssize_t end);

Exception
(*copy)(str_c s, char* dest, size_t destlen);

size_t
(*len)(str_c s);

bool
(*is_valid)(str_c s);

char*
(*iter)(str_c s, cex_iterator_s* iterator);

ssize_t
(*find)(str_c s, str_c needle, size_t start, size_t end);

ssize_t
(*rfind)(str_c s, str_c needle, size_t start, size_t end);

bool
(*contains)(str_c s, str_c needle);

bool
(*starts_with)(str_c s, str_c needle);

bool
(*ends_with)(str_c s, str_c needle);

str_c
(*remove_prefix)(str_c s, str_c prefix);

str_c
(*remove_suffix)(str_c s, str_c suffix);

str_c
(*lstrip)(str_c s);

str_c
(*rstrip)(str_c s);

str_c
(*strip)(str_c s);

int
(*cmp)(str_c self, str_c other);

int
(*cmpi)(str_c self, str_c other);

str_c*
(*iter_split)(str_c s, const char* split_by, cex_iterator_s* iterator);

    // clang-format on
};
extern const struct __module__str str; // CEX Autogen

/*
*                   sbuf.h
*/

typedef char* sbuf_c;

typedef struct
{
    struct
    {
        u32 magic : 16;   // used for sanity checks
        u32 elsize : 8;   // maybe multibyte strings in the future?
        u32 nullterm : 8; // always zero to prevent usage of direct buffer
    } header;
    u32 length;
    u32 capacity;
    const Allocator_i* allocator;
} __attribute__((packed)) sbuf_head_s;

_Static_assert(sizeof(sbuf_head_s) == 20, "size");
_Static_assert(alignof(sbuf_head_s) == 1, "align");
_Static_assert(alignof(sbuf_head_s) == alignof(char), "align");


struct __module__sbuf
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*create)(sbuf_c* self, u32 capacity, const Allocator_i* allocator);

Exception
(*create_static)(sbuf_c* self, char* buf, size_t buf_size);

Exception
(*grow)(sbuf_c* self, u32 capacity);

Exception
(*append_c)(sbuf_c* self, char* s);

Exception
(*replace)(sbuf_c* self, const str_c oldstr, const str_c newstr);

Exception
(*append)(sbuf_c* self, str_c s);

void
(*clear)(sbuf_c* self);

u32
(*len)(const sbuf_c* self);

u32
(*capacity)(const sbuf_c* self);

sbuf_c
(*destroy)(sbuf_c* self);

Exception
(*sprintf)(sbuf_c* self, const char* format, ...);

str_c
(*tostr)(sbuf_c* self);

    // clang-format on
};
extern const struct __module__sbuf sbuf; // CEX Autogen

/*
*                   list.h
*/


/**
 * @brief Dynamic array (list) implementation
 */
typedef struct
{
    // NOTE: do not assign this pointer to local variables, is can become dangling after dlist
    // operations (e.g. after realloc()). So local pointers can be pointing to invalid area!
    void* arr;
    size_t len;
} list_c;

#define list$define(eltype)                                                                        \
    /* NOTE: shadow struct the same as list_c, only used for type safety. const  prevents user to  \
     * overwrite struct arr.arr pointer (elements are ok), and also arr.count */                   \
    struct __CEX_TMPNAME(__cexlist__)                                                              \
    {                                                                                              \
        eltype* const arr;                                                                         \
        const size_t len;                                                                          \
    }

#define list$new(self, capacity, allocator)                                                        \
    (list.create(                                                                                  \
        (list_c*)self,                                                                             \
        capacity,                                                                                  \
        sizeof(typeof(*(((self))->arr))),                                                          \
        alignof(typeof(*(((self))->arr))),                                                         \
        allocator                                                                                  \
    ))

#define list$new_static(self, buf, buf_len)                                                        \
    (list.create_static(                                                                           \
        (list_c*)self,                                                                             \
        buf,                                                                                       \
        buf_len,                                                                                   \
        sizeof(typeof(*(((self))->arr))),                                                          \
        alignof(typeof(*(((self))->arr)))                                                          \
    ))

typedef struct
{
    struct
    {
        size_t magic : 16;
        size_t elsize : 16;
        size_t elalign : 16;
    } header;
    size_t count;
    size_t capacity;
    const Allocator_i* allocator;
    // NOTE: we must use packed struct, because elements of the list my have absolutely different
    // alignment, so the list_head_s takes place at (void*)1st_el - sizeof(list_head_s)
} __attribute__((packed)) list_head_s;

_Static_assert(sizeof(list_head_s) == 32, "size");
_Static_assert(alignof(list_head_s) == 1, "align");

struct __module__list
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*create)(list_c* self, size_t capacity, size_t elsize, size_t elalign, const Allocator_i* allocator);

Exception
(*create_static)(list_c* self, void* buf, size_t buf_len, size_t elsize, size_t elalign);

Exception
(*insert)(void* self, void* item, size_t index);

Exception
(*del)(void* self, size_t index);

void
(*sort)(void* self, int (*comp)(const void*, const void*));

Exception
(*append)(void* self, void* item);

void
(*clear)(void* self);

Exception
(*extend)(void* self, void* items, size_t nitems);

size_t
(*len)(void* self);

size_t
(*capacity)(void* self);

void*
(*destroy)(void* self);

void*
(*iter)(void* self, cex_iterator_s* iterator);

    // clang-format on
};
extern const struct __module__list list; // CEX Autogen

/*
*                   dict.h
*/
#include <string.h>

typedef struct dict_c
{
    void* hashmap; // any generic hashmap implementation
} dict_c;

typedef u64 (*dict_hash_func_f)(const void* item, u64 seed0, u64 seed1);
typedef i32 (*dict_compare_func_f)(const void* a, const void* b, void* udata);
typedef void (*dict_elfree_func_f)(void* item);


// Hack for getting hash/cmp functions by a type of key field
// https://gustedt.wordpress.com/2015/05/11/the-controlling-expression-of-_generic/
// FIX: this is not compatible with MSVC
#define _dict$hashfunc_field(strucfield)                                                           \
    _Generic(&(strucfield), u64 *: hm_int_hash, char(*)[]: hm_str_static_hash)

#define _dict$cmpfunc_field(strucfield)                                                            \
    _Generic(&(strucfield), u64 *: hm_int_compare, char(*)[]: hm_str_static_compare)

#define _dict$hashfunc(struct, field) _dict$hashfunc_field(((struct){ 0 }.field))

#define _dict$cmpfunc(struct, field) _dict$cmpfunc_field(((struct){ 0 }.field))



#define dict$new(self, struct_type, key_field_name, allocator)                                     \
    dict.create(                                                                                   \
        self,                                                                                      \
        sizeof(struct_type),                                                                       \
        _Alignof(struct_type),                                                                     \
        offsetof(struct_type, key_field_name),                                                     \
        0, /* capacity = 0, default is 16 */                                                       \
        _dict$hashfunc(struct_type, key_field_name),                                               \
        _dict$cmpfunc(struct_type, key_field_name),                                                \
        allocator,                                                                                 \
        NULL, /* elfree - function for clearing elements */                                        \
        NULL  /* udata - passed as a context for cmp funcs */                                      \
    )


struct __module__dict
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*create)(dict_c* self, size_t item_size, size_t item_align, size_t item_key_offsetof, size_t capacity, dict_hash_func_f hash_func, dict_compare_func_f compare_func, const Allocator_i* allocator, dict_elfree_func_f elfree, void* udata);

/**
 * @brief Set or replace dict item
 *
 * @param self dict() instance
 * @param item  item key/value struct
 * @return error code, EOK (0!) on success, positive on failure
 */
Exception
(*set)(dict_c* self, const void* item);

/**
 * @brief Get item by integer key
 *
 * @param self dict() instance
 * @param key u64 key
 */
void*
(*geti)(dict_c* self, u64 key);

/**
 * @brief Get item by generic key pointer (including strings)
 *
 * @param self dict() instance
 * @param key generic pointer key
 */
void*
(*get)(dict_c* self, const void* key);

/**
 * @brief Number elements in dict()
 *
 * @param self  dict() instance
 * @return number
 */
size_t
(*len)(dict_c* self);

/**
 * @brief Free dict() instance
 *
 * @param self  dict() instance
 * @return always NULL
 */
void
(*destroy)(dict_c* self);

/**
 * @brief Clear all elements in dict (but allocated capacity unchanged)
 *
 * @param self dict() instane
 */
void
(*clear)(dict_c* self);

/**
 * @brief Delete item by integer key
 *
 * @param self dict() instance
 * @param key u64 key
 */
void*
(*deli)(dict_c* self, u64 key);

/**
 * @brief Delete item by generic key pointer (including strings)
 *
 * @param self dict() instance
 * @param key generic pointer key
 */
void*
(*del)(dict_c* self, const void* key);

void*
(*iter)(dict_c* self, cex_iterator_s* iterator);

Exception
(*tolist)(dict_c* self, void* listptr, const Allocator_i* allocator);

    // clang-format on
};
extern const struct __module__dict dict; // CEX Autogen

/*
*                   io.h
*/
#include <stdio.h>


typedef struct io_c
{
    FILE* _fh;
    size_t _fsize;
    char* _fbuf;
    size_t _fbuf_size;
    const Allocator_i* _allocator;
    struct
    {
        u32 is_attached : 1;

    } _flags;
} io_c;


struct __module__io
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*fopen)(io_c* self, const char* filename, const char* mode, const Allocator_i* allocator);

Exception
(*fattach)(io_c* self, FILE* fh, const Allocator_i* allocator);

int
(*fileno)(io_c* self);

bool
(*isatty)(io_c* self);

Exception
(*flush)(io_c* self);

Exception
(*seek)(io_c* self, long offset, int whence);

void
(*rewind)(io_c* self);

Exception
(*tell)(io_c* self, size_t* size);

size_t
(*size)(io_c* self);

Exception
(*read)(io_c* self, void* obj_buffer, size_t obj_el_size, size_t* obj_count);

Exception
(*readall)(io_c* self, str_c* s);

Exception
(*readline)(io_c* self, str_c* s);

Exception
(*fprintf)(io_c* self, const char* format, ...);

Exception
(*write)(io_c* self, void* obj_buffer, size_t obj_el_size, size_t obj_count);

void
(*close)(io_c* self);

    // clang-format on
};
extern const struct __module__io io; // CEX Autogen

/*
*                   deque.h
*/

typedef struct
{
    alignas(64) struct
    {
        size_t magic : 16;             // deque magic number for sanity checks
        size_t elsize : 16;            // element size
        size_t elalign : 8;            // align of element type
        size_t eloffset : 8;           // offset in bytes to the 1st element array
        size_t rewrite_overflowed : 8; // allow rewriting old unread values if que overflowed
    } header;
    size_t idx_head;
    size_t idx_tail;
    size_t max_capacity; // maximum capacity when deque stops growing (must be pow of 2!)
    size_t capacity;
    const Allocator_i* allocator; // can be NULL for static deque
} deque_head_s;
_Static_assert(sizeof(deque_head_s) == 64, "size");
_Static_assert(alignof(deque_head_s) == 64, "align");

struct _deque_c
{
    deque_head_s _head;
    // NOTE: data is hidden, it makes no sense to access it directly
};
typedef struct _deque_c* deque_c;


#define deque$new(self, eltype, max_capacity, rewrite_overflowed, allocator)                       \
    (deque.create(                                                                                 \
        self,                                                                                      \
        max_capacity,                                                                              \
        rewrite_overflowed,                                                                        \
        sizeof(eltype),                                                                            \
        alignof(eltype),                                                                           \
        allocator                                                                                  \
    ))

#define deque$new_static(self, eltype, buf, buf_len, rewrite_overflowed)                           \
    (deque.create_static(self, buf, buf_len, rewrite_overflowed, sizeof(eltype), alignof(eltype)))

struct __module__deque
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*validate)(deque_c *self);

Exception
(*create)(deque_c* self, size_t max_capacity, bool rewrite_overflowed, size_t elsize, size_t elalign, const Allocator_i* allocator);

Exception
(*create_static)(deque_c* self, void* buf, size_t buf_len, bool rewrite_overflowed, size_t elsize, size_t elalign);

Exception
(*append)(deque_c* self, const void* item);

Exception
(*enqueue)(deque_c* self, const void* item);

Exception
(*push)(deque_c* self, const void* item);

void*
(*dequeue)(deque_c* self);

void*
(*pop)(deque_c* self);

void*
(*get)(deque_c* self, size_t index);

size_t
(*len)(const deque_c* self);

void
(*clear)(deque_c* self);

void*
(*destroy)(deque_c* self);

void*
(*iter_get)(deque_c* self, i32 direction, cex_iterator_s* iterator);

void*
(*iter_fetch)(deque_c* self, i32 direction, cex_iterator_s* iterator);

    // clang-format on
};
extern const struct __module__deque deque; // CEX Autogen

/*
*                   argparse.h
*/
/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdint.h>

struct argparse_c;
struct argparse_opt_s;

typedef Exception argparse_callback_f(struct argparse_c* self, const struct argparse_opt_s* option);


enum argparse_option_flags
{
    OPT_NONEG = 1, /* disable negation */
};
/**
 *  argparse option
 *
 *  `type`:
 *    holds the type of the option
 *
 *  `short_name`:
 *    the character to use as a short option name, '\0' if none.
 *
 *  `long_name`:
 *    the long option name, without the leading dash, NULL if none.
 *
 *  `value`:
 *    stores pointer to the value to be filled.
 *
 *  `help`:
 *    the short help message associated to what the option does.
 *    Must never be NULL (except for ARGPARSE_OPT_END).
 *
 *  `required`:
 *    if 'true' option presence is mandatory (default: false)
 *
 *
 *  `callback`:
 *    function is called when corresponding argument is parsed.
 *
 *  `data`:
 *    associated data. Callbacks can use it like they want.
 *
 *  `flags`:
 *    option flags.
 *
 *  `is_present`:
 *    true if option present in args
 */
typedef struct argparse_opt_s
{
    u8 type;
    const char short_name;
    const char* long_name;
    void* value;
    const char* help;
    bool required;
    argparse_callback_f* callback;
    intptr_t data;
    int flags;
    bool is_present; // also setting in in argparse$opt* macro, allows optional parameter sugar
} argparse_opt_s;

/**
 * argpparse
 */
typedef struct argparse_c
{
    // user supplied options
    argparse_opt_s* options;
    u32 options_len;

    const char* usage;        // usage text (can be multiline), each line prepended by program_name
    const char* description;  // a description after usage
    const char* epilog;       // a description at the end
    const char* program_name; // program name in usage (by default: argv[0])

    struct
    {
        u32 stop_at_non_option : 1;
        u32 ignore_unknown_args : 1;
    } flags;
    //
    //
    // internal context
    struct
    {
        int argc;
        char** argv;
        char** out;
        int cpidx;
        const char* optvalue; // current option value
        bool has_argument;
    } _ctx;
} argparse_c;


// built-in option macros
// clang-format off
#define argparse$opt_bool(...)    { 2 /*ARGPARSE_OPT_BOOLEAN*/, __VA_ARGS__, .is_present=0}
#define argparse$opt_bit(...)     { 3 /*ARGPARSE_OPT_BIT*/, __VA_ARGS__, .is_present=0 }
#define argparse$opt_i64(...)     { 4 /*ARGPARSE_OPT_INTEGER*/, __VA_ARGS__, .is_present=0 }
#define argparse$opt_f32(...)     { 5 /*ARGPARSE_OPT_FLOAT*/, __VA_ARGS__, .is_present=0 }
#define argparse$opt_str(...)     { 6 /*ARGPARSE_OPT_STRING*/, __VA_ARGS__, .is_present=0 }
#define argparse$opt_group(h)     { 1 /*ARGPARSE_OPT_GROUP*/, 0, NULL, NULL, h, false, NULL, 0, 0, .is_present=0 }
#define argparse$opt_help()       argparse$opt_bool('h', "help", NULL,                           \
                                                    "show this help message and exit", false,    \
                                                    NULL, 0, OPT_NONEG)
// clang-format on

struct __module__argparse
{
    // Autogenerated by CEX
    // clang-format off

void
(*usage)(argparse_c* self);

Exception
(*parse)(argparse_c* self, int argc, char** argv);

u32
(*argc)(argparse_c* self);

char**
(*argv)(argparse_c* self);

    // clang-format on
};
extern const struct __module__argparse argparse; // CEX Autogen
#endif