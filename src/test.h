#pragma once
#if !defined(cex$enable_minimal)
#include "all.h"
#include "argparse.h"

typedef Exception (*_cex_test_case_f)(void);

#define CEX_TEST_AMSG_MAX_LEN 512
struct _cex_test_case_s
{
    _cex_test_case_f test_fn;
    char* test_name;
    u32 test_line;
    bool is_benchmark;
};

struct _cex_test_context_s
{
    arr$(struct _cex_test_case_s) test_cases;
    int orig_stderr_fd; // initial stdout
    int orig_stdout_fd; // initial stderr
    FILE* out_stream;   // test case captured output
    int tests_run;      // number of tests run
    int tests_failed;   // number of tests failed
    int tests_skipped;  // number of tests skipped (filtered out)
    bool quiet_mode;    // quiet mode (for run all)
    char* case_name;    // current running case name
    _cex_test_case_f setup_case_fn;
    _cex_test_case_f teardown_case_fn;
    _cex_test_case_f setup_suite_fn;
    _cex_test_case_f teardown_suite_fn;
    bool has_ansi;
    bool no_stdout_capture;
    bool breakpoint;
    bool is_benchmark;
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

extern
#    if !cex$is_freestanding
    _Thread_local
#    endif
    IAllocator _cex__default_global__allocator_test;

#define test$alloc (_cex__default_global__allocator_test)


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

#define test$bench(NAME)                                                                             \
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
                                       .is_benchmark = true,                                        \
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
            u64: _check_eq_u64,                                                                    \
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
