#pragma once
#if !defined(cex$enable_minimal)
#include "all.h"
#include "argparse.h"

typedef Exception (*_cex_test_case_f)(void);

/// Max captured test assertion message length, default 512
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

## Unit testing

### Running/building tests
```sh
./cex test create tests/test_mytest.c
./cex test run tests/test_mytest.c
./cex test run all
./cex test debug tests/test_mytest.c
./cex test clean all
./cex test --help
```

### Unit Test structure
```c
test$setup_case() { return EOK; }     // optional, runs before each case
test$teardown_case() { return EOK; }  // optional, runs after each case
test$setup_suite() { return EOK; }    // optional, runs once before the suite
test$teardown_suite() { return EOK; } // optional, runs once after the suite

test$case(my_test_case)
{
    e$ret(foo("raise")); // fails the test if foo() raises Exception
    return EOK;          // must return EOK to pass
}

test$main(); // mandatory at the end of each test file
```

### Test checks
```c
test$case(my_test_case)
{
    // generic type assertions — fail and print both values
    tassert_eq(1, 1);
    tassert_eq(str, "foo");
    tassert_eq(str_slice, str$s("expected"));

    tassert(condition && "oops");
    tassertf(condition, "oops: %s", s);

    tassert_er(Error.argument, raising_exc_foo(-1)); // Exception result
    tassert_eq_almost(PI, 3.14, 0.01);               // float tolerance
    tassert_eq_ptr(a, b);                            // raw pointers
    tassert_eq_mem(a, b);                            // raw buffers (same size)
    tassert_eq_arr(a, b);                            // arrays (static or dynamic)

    tassert_ne(1, 0);
    tassert_le(a, b); // also: lt, gt, ge

    return EOK;
}
```

### Test allocator
```c
test$case(my_test_case)
{
    // `test$alloc` is a per-case arena (1 MB page, scopes disabled), created
    // before each case and destroyed after — no manual free, not leak-tracked
    int* buf = mem$malloc(test$alloc, 256 * sizeof(int));

    return EOK;
}
```

### Simulating OOM
```c
test$case(my_test_case)
{
    // 0.0 = never fail (default, reset before each case)
    // 1.0 = always fail, 0.5 = ~50% failure rate
    test$alloc_set_oom_probability(1.0);

    void* p = mem$malloc(test$alloc, 64);
    tassert(p == NULL); // exercise the OOM path of your code

    return EOK;
}
```

### Replacing the global allocator (CEX_TEST mode)

In `CEX_TEST` builds `mem$` is an assignable global: point it at any `IAllocator` and all code
using `mem$` will use it. The runner saves `mem$` before each case and restores it after, so no
manual cleanup is needed. A common use is routing `mem$` through `test$alloc` to OOM-test code
that allocates with `mem$`.

```c
test$case(my_test_case)
{
    // route every mem$ allocation through the per-case arena, and make it fail on demand
    mem$ = test$alloc;
    test$alloc_set_oom_probability(1.0);

    // any function allocating via mem$ now exercises its OOM path
    tassert(mem$malloc(mem$, 64) == NULL);

    // no manual restore needed - the runner restores mem$ after the case
    return EOK;
}
```

### Test file & rebuild requirements
```c
// tests/ folder only, name test_*.c, include sources directly (unity build)
#include "src/foo.c"   // only #include "" is tracked for rebuilds
// linker/compiler extras: cexy$ld_libs, cexy$ld_args, cexy$cc_args_test
```

### Mocking namespaces (CEX_TEST mode)
```c
// test$mock_scope saves 1-8 namespace states and restores them on any scope
// exit (return, break, goto); replace any namespace function pointer inside
f64 timer_mock(void) { return 777888.9; }

test$case(my_test_case)
{
    test$mock_scope(os) {
        os.timer = timer_mock;
        tassert_eq(777888.9, os.timer());
    }                                  // os restored here

    test$mock_scope(os, io) {          // several namespaces at once
        os.timer = timer_mock;
        io.printf = NULL;
    }

    // without scope: restore manually, or use test$teardown_case()
    os.timer = timer_mock;
    os.timer = cex_os_timer;

    return EOK;
}
```

### Benchmarking
```c
// runs only via `./cex test bench tests/test_foo.c`; compiled with -O3
// keep data setup in test$setup_case(), not in the bench body
test$bench(my_bench)
{
    some_function_of_interest(&g_table);
    return EOK;
}
// output: my_bench........ cold: 282.000ns  hot: 30.000ns  [PASS]
```

### Coverage

```sh
# whole suite
./cex coverage run all                       # build+run all tests instrumented
./cex coverage report all                    # aggregate + print text report
./cex coverage report --format=json all      # machine-readable JSON report
./cex coverage report --format=html all      # HTML report -> cexy$build_dir/coverage
./cex coverage report --file 'src/foo*.c' all # only sources matching the glob
./cex coverage export -o coverage.info all   # lcov .info tracefile for external tools
./cex coverage clean all                     # remove raw coverage artifacts

# one test file (target is any tests/test_*.c)
./cex coverage run tests/test_foo.c          # instrument+run just this file
./cex coverage report tests/test_foo.c       # report for that target
./cex coverage export -o foo.info tests/test_foo.c
```

Engines: `auto` (default: llvm for clang, lcov for gcc) or `--engine=llvm|lcov`. llvm uses
`-fprofile-instr-generate` + `llvm-profdata`/`llvm-cov` (.profraw); lcov uses compiler
`--coverage` + `lcov`/`genhtml` (.gcno/.gcda). Raw data lives in `cexy$build_dir`. `run all`
wipes all raw data; `run tests/test_foo.c` resets only that target's counters, so one test can
be re-run without losing the rest. llvm scopes the report to the target's binaries, lcov
aggregates every `.gcda` in `cexy$build_dir`. `export` writes an lcov tracefile (merge several
with `lcov -a a.info -o merged.info`). The lower-level `./cex test --coverage run all` only
leaves raw data (no report). Not supported with `bench`.

JSON fields: `total{lines_hit,lines_found,funcs_hit,funcs_found}`,
`files[]{path,lines_hit,lines_found,funcs_hit,funcs_found,missed_lines,uncovered_funcs,fully_uncovered}`.

The `coverage` command ships in `cexstd/testing/coverage/`; wire it into `cex.c` per
`cexstd/testing/coverage/README.md` (fetch with `./cex libfetch cexstd/`).

### Test runner args (single test file only)
```sh
# args after the file path are forwarded to the built-in test runner
./cex test run tests/test_file.c --filter='case*'  # run matching cases only (-f)
./cex test run tests/test_file.c --breakpoint      # debugger on tassert failure (-b)
./cex test run tests/test_file.c --no-capture      # stream stdout as tests run (-o)
./cex test run tests/test_file.c --help            # full runner help
# runner flags: -f/--filter, -b/--breakpoint, -o/--no-capture, --bench, -q/--quiet
```

### Test-mode extras
```c
// os.random.seed(0) runs before each case -> deterministic random sequences
// uassert() reporting can be toggled (test mode only)
uassert_disable();
run_bad_stuff(NULL);
uassert_enable();
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


/// Dedicated arena created fresh before each test case and destroyed afterward, always growing,
/// test$alloc is a dedicated arena created fresh before each test case and destroyed afterwards, 
//  no manual free needed
#define test$alloc (_cex__default_global__allocator_test)
/// Sets the probability (0.0–1.0) of simulated allocation failures for `test$alloc`:
/// - 0.0 = never fail (default)
/// - 1.0 = always fail
/// - 0.5 = approx 50% failure rate
/// Use to test OOM paths in test cases. Only available in CEX_TEST mode.
/// Automatically reset to 0.0 before each test case.
#define test$alloc_set_oom_probability(prob) ({ \
    uassert(prob >= 0 && prob <= 1.0 && "test$alloc_set_oom_probability out of range"); \
    ((AllocatorArena_c*)_cex__default_global__allocator_test)->test_oom_probability = (f32)prob; \
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

/// Benchmark case (runs only via ./cex test bench)
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
        int ret_code = _cex_test_main_fn(argc, argv);                                               \
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

/// State bundle for test$mock_scope
typedef struct _cex_test_mockns_s {
    void* ns_ptr;
    usize ns_size;
    void* orig_ns;
} _cex_test_mockns_s;

/// Saves namespace state before test$mock_scope scope
_cex_test_mockns_s _cex_test_ns_save(void* ns, usize ns_size);
/// Restores namespace state on test$mock_scope scope exit (__cleanup__ callback)
void _cex_test_ns_restore(_cex_test_mockns_s* mock);

/* ---- test$mock_scope: namespace mock scope guard ---- */

#define _test$ns_mock_once(ns)                                                         \
    for (_cex_test_mockns_s cex$tmpname(_ns_save)                                          \
             __attribute__((__cleanup__(_cex_test_ns_restore))) __attribute__((unused)) = _cex_test_ns_save(&(ns), sizeof(ns)),              \
         *cex$tmpname(_ns_end) = 0;                                                \
         cex$tmpname(_ns_end) == 0;                                                \
         cex$tmpname(_ns_end) = (void*)(uintptr_t)1)

#define _test$ns_mock_N(_1,_2,_3,_4,_5,_6,_7,_8,N,...)  _test$ns_mock_map_##N

#define _test$ns_mock_CHOOSER(...) \
    _test$ns_mock_N(__VA_ARGS__, 8,7,6,5,4,3,2,1,0)

#define _test$ns_mock_map_0()
#define _test$ns_mock_map_1(ns)                _test$ns_mock_once(ns)
#define _test$ns_mock_map_2(ns, ...)           _test$ns_mock_once(ns) _test$ns_mock_map_1(__VA_ARGS__)
#define _test$ns_mock_map_3(ns, ...)           _test$ns_mock_once(ns) _test$ns_mock_map_2(__VA_ARGS__)
#define _test$ns_mock_map_4(ns, ...)           _test$ns_mock_once(ns) _test$ns_mock_map_3(__VA_ARGS__)
#define _test$ns_mock_map_5(ns, ...)           _test$ns_mock_once(ns) _test$ns_mock_map_4(__VA_ARGS__)
#define _test$ns_mock_map_6(ns, ...)           _test$ns_mock_once(ns) _test$ns_mock_map_5(__VA_ARGS__)
#define _test$ns_mock_map_7(ns, ...)           _test$ns_mock_once(ns) _test$ns_mock_map_6(__VA_ARGS__)
#define _test$ns_mock_map_8(ns, ...)           _test$ns_mock_once(ns) _test$ns_mock_map_7(__VA_ARGS__)

/// Saves namespace(s) before scope — mock any function pointer inside, auto-restored on exit via
/// __cleanup__. Accepts 1-8 namespaces.
#define test$mock_scope(...)                   _test$ns_mock_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

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
           e$traceback_reset();                                                                    \
           return _test$log_err(#A);                                                               \
       }                                                                                           \
    })

/// Test assertion with user formatted output, supports CEX formatting engine
#define tassertf(A, M, ...)                                                                        \
    ({ /* ONLY for test$case USE */                                                                \
       if (!(A)) {                                                                                 \
           _test$tassert_breakpoint();                                                             \
           e$traceback_reset();                                                                    \
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
            e$traceback_reset();                                                                   \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check expected error, or EOK (if no error expected)
#define tassert_er(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        e$traceback_reset();                                                                       \
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
            e$traceback_reset();                                                                   \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

/// Check pointer address equality
#define tassert_eq_ptr(a, b)                                                                       \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        if ((cex$tmpname(err) = _check_eq_ptr((a), (b), __LINE__))) {                              \
            _test$tassert_breakpoint();                                                            \
            e$traceback_reset();                                                                   \
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
            e$traceback_reset();                                                                   \
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
            e$traceback_reset();                                                                   \
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
                    e$traceback_reset();                                                           \
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
            e$traceback_reset();                                                                   \
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
            e$traceback_reset();                                                                   \
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
            e$traceback_reset();                                                                   \
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
            e$traceback_reset();                                                                   \
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
            e$traceback_reset();                                                                   \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })
#endif
