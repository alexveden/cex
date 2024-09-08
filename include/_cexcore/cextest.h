/**
 * @brief CEX built-in testing framework
 *
 * Supported features:
 * - setup/teardown functions prior and past each test
 * - integrated CEX Exceptions used as test case result success measures
 * - special test asserts
 * - individual test runs
 * - mocking (with fff.h)
 * - new cases automatically added when you run the test via cex cli
 * - testing static function via #include ".c"
 * - enabling/disabling uassert() for testing code with production -DNDEBUG simulation
 *
 * Generic test composition
 *
 *
```
#include <cex.c>

const Allocator_i* allocator;

test$teardown(){
    allocator = allocators.heap.destroy(); // this also nullifies allocator
    return EOK;
}

test$setup(){
    uassert_enable(); // re-enable if you disabled it in some test case
    allocator = allocators.heap.create();
    return EOK;
}

test$case(my_test)
{
    // Has malloc, but no free(), allocator will send memory leak warning
    void* a = allocator->malloc(100);

    tassert(true == 1);
    tassert_eqi(1, 1);
    tassert_eql(1, 1L);
    tassert_eqf(1.0, 1.0);
    tassert_eqe(EOK, Error.ok);

    uassert_disable();
    uassert(false && "this will be disabled, no abort!");

    tassertf(true == 0, "true != %d", false);

    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below

    test$run(my_test);

    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
```
 */
#pragma once
#include "cex.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef NAN
#error "NAN is undefined on this platform"
#endif

#ifndef FLT_EPSILON
#error "FLT_EPSILON is undefined on this platform"
#endif

#define CEXTEST_AMSG_MAX_LEN 512
#define CEXTEST_CRED "\033[0;31m"
#define CEXTEST_CGREEN "\033[0;32m"
#define CEXTEST_CNONE "\033[0m"

#ifdef _WIN32
#define CEXTEST_NULL_DEVICE "NUL:"
#else
#define CEXTEST_NULL_DEVICE "/dev/null"
#endif

#if defined(__clang__)
#define test$NOOPT __attribute__((optnone))
#elif defined(__GNUC__) || defined(__GNUG__)
#define test$NOOPT __attribute__((optimize("O0")))
#elif defined(_MSC_VER)
#warning "MSVS is untested"
#endif

//
//  atest.h settings, you can alter them in main() function, for example replace
//  out_stream by fopen() file
//
struct __CexTestContext_s
{
    FILE* out_stream; // by default uses stdout, however you can replace it by any FILE*
                      // stream
    int tests_run;    // number of tests run
    int tests_failed; // number of tests failed
    int verbosity;    // verbosity level: 0 - only stats, 1 - short test results (. or F) +
                      // stats, 2 - all tests results, 3 - include logs
    const char* in_test;
    // Internal buffer for aassertf() string formatting
    char _str_buf[CEXTEST_AMSG_MAX_LEN];
};


/*
 *
 *  ASSERTS
 *
 */
//
// tassert( condition ) - simple test assert, fails when condition false
//
#define tassert(A)                                                                                 \
    do {                                                                                           \
        if (!(A)) {                                                                                \
            return __CEXTEST_LOG_ERR(#A);                                                          \
        }                                                                                          \
    } while (0)

//
// tassertf( condition, format_string, ...) - test only assert with printf() style formatting
// error message
//
#define tassertf(A, M, ...)                                                                        \
    do {                                                                                           \
        if (!(A)) {                                                                                \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR(M),                                                              \
                ##__VA_ARGS__                                                                      \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    } while (0)

//
// tassert_eqs(char * actual, char * expected) - compares strings via strcmp() (NULL tolerant)
//
#define tassert_eqs(_ac, _ex)                                                                      \
    do {                                                                                           \
        const char* ac = (_ac);                                                                    \
        const char* ex = (_ex);                                                                    \
        if ((ac) == NULL && (ex) != NULL) {                                                        \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("NULL != '%s'"),                                                 \
                (char*)(ex)                                                                        \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        } else if ((ac) != NULL && (ex) == NULL) {                                                 \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("'%s' != NULL"),                                                 \
                (char*)(ac)                                                                        \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        } else if (((ac) != NULL && (ex) != NULL) &&                                               \
                   (strcmp(__CEXTEST_NON_NULL((ac)), __CEXTEST_NON_NULL((ex))) != 0)) {            \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("'%s' != '%s'"),                                                 \
                (char*)(ac),                                                                       \
                (char*)(ex)                                                                        \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    } while (0)

//
// tassert_eqs(Exception actual, Exception expected) - compares Exceptions (Error.ok = NULL = EOK)
//
#define tassert_eqe(_ac, _ex)                                                                      \
    do {                                                                                           \
        const char* ac = (_ac);                                                                    \
        const char* ex = (_ex);                                                                    \
        if ((ac) == NULL && (ex) != NULL) {                                                        \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("Error.ok != '%s'"),                                             \
                (char*)(ex)                                                                        \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        } else if ((ac) != NULL && (ex) == NULL) {                                                 \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("'%s' != Error.ok"),                                             \
                (char*)(ac)                                                                        \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        } else if (((ac) != NULL && (ex) != NULL) &&                                               \
                   (strcmp(__CEXTEST_NON_NULL((ac)), __CEXTEST_NON_NULL((ex))) != 0)) {            \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("'%s' != '%s'"),                                                 \
                (char*)(ac),                                                                       \
                (char*)(ex)                                                                        \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    } while (0)

//
// tassert_eqi(int actual, int expected) - compares equality of signed integers
//
#define tassert_eqi(_ac, _ex)                                                                      \
    do {                                                                                           \
        long int ac = (_ac);                                                                       \
        long int ex = (_ex);                                                                       \
        if ((ac) != (ex)) {                                                                        \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("%ld != %ld"),                                                   \
                (ac),                                                                              \
                (ex)                                                                               \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    } while (0)

//
// tassert_eql(int actual, int expected) - compares equality of long signed integers
//
#define tassert_eql(_ac, _ex)                                                                      \
    do {                                                                                           \
        long long ac = (_ac);                                                                      \
        long long ex = (_ex);                                                                      \
        if ((ac) != (ex)) {                                                                        \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("%lld != %lld"),                                                 \
                (ac),                                                                              \
                (ex)                                                                               \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    } while (0)

//
// tassert_eqf(f64 actual, f64 expected) - compares equality of f64 / double numbers
// NAN friendly, i.e. tassert_eqf(NAN, NAN) -- passes
//
#define tassert_eqf(_ac, _ex)                                                                      \
    do {                                                                                           \
        f64 ac = (_ac);                                                                            \
        f64 ex = (_ex);                                                                            \
        int __at_assert_passed = 1;                                                                \
        if (isnan(ac) && !isnan(ex))                                                               \
            __at_assert_passed = 0;                                                                \
        else if (!isnan(ac) && isnan(ex))                                                          \
            __at_assert_passed = 0;                                                                \
        else if (isnan(ac) && isnan(ex))                                                           \
            __at_assert_passed = 1;                                                                \
        else if (fabs((ac) - (ex)) > (f64)FLT_EPSILON)                                             \
            __at_assert_passed = 0;                                                                \
        if (__at_assert_passed == 0) {                                                             \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("%.10e != %.10e (diff: %0.10e epsilon: %0.10e)"),                \
                (ac),                                                                              \
                (ex),                                                                              \
                ((ac) - (ex)),                                                                     \
                (f64)FLT_EPSILON                                                                   \
            );                                                                                     \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    } while (0)


/*
 *
 *  TEST FUNCTIONS
 *
 */

#define test$setup()                                                                               \
    struct __CexTestContext_s __CexTestContext = {                                                 \
        .out_stream = NULL,                                                                        \
        .tests_run = 0,                                                                            \
        .tests_failed = 0,                                                                         \
        .verbosity = 3,                                                                            \
        .in_test = NULL,                                                                           \
    };                                                                                             \
    static test$NOOPT Exc cextest_setup_func()

#define test$teardown() static test$NOOPT Exc cextest_teardown_func()

//
// Typical test function template
//  MUST return Error.ok / EOK on test success, or char* with error message when failed!
//
#define test$case(test_case_name) static test$NOOPT Exc test_case_name()


#define test$run(test_case_name)                                                                   \
    do {                                                                                           \
        if (argc >= 3 && strcmp(argv[2], #test_case_name) != 0) {                                  \
            break;                                                                                 \
        }                                                                                          \
        __CexTestContext.in_test = #test_case_name;                                                \
        Exc err = cextest_setup_func();                                                            \
        if (err != EOK) {                                                                          \
            fprintf(                                                                               \
                __atest_stream,                                                                    \
                "[%s] %s in test$setup() before %s (suite %s stopped)\n",                          \
                CEXTEST_CRED "FAIL" CEXTEST_CNONE,                                                 \
                err,                                                                               \
                #test_case_name,                                                                   \
                __FILE__                                                                           \
            );                                                                                     \
            return 1;                                                                              \
        } else {                                                                                   \
            err = (test_case_name());                                                              \
        }                                                                                          \
                                                                                                   \
        __CexTestContext.tests_run++;                                                              \
        if (err == EOK) {                                                                          \
            if (__CexTestContext.verbosity > 0) {                                                  \
                if (__CexTestContext.verbosity == 1) {                                             \
                    fprintf(__atest_stream, ".");                                                  \
                } else {                                                                           \
                    fprintf(                                                                       \
                        __atest_stream,                                                            \
                        "[%s] %s\n",                                                               \
                        CEXTEST_CGREEN "PASS" CEXTEST_CNONE,                                       \
                        #test_case_name                                                            \
                    );                                                                             \
                }                                                                                  \
            }                                                                                      \
        } else {                                                                                   \
            fprintf(                                                                               \
                __atest_stream,                                                                    \
                "[%s] %s %s\n",                                                                    \
                CEXTEST_CRED "FAIL" CEXTEST_CNONE,                                                 \
                err,                                                                               \
                #test_case_name                                                                    \
            );                                                                                     \
            __CexTestContext.tests_failed++;                                                       \
        }                                                                                          \
        err = cextest_teardown_func();                                                             \
        if (err != EOK) {                                                                          \
            fprintf(                                                                               \
                __atest_stream,                                                                    \
                "[%s] %s in test$teardown() after %s\n",                                           \
                CEXTEST_CRED "FAIL" CEXTEST_CNONE,                                                 \
                err,                                                                               \
                #test_case_name                                                                    \
            );                                                                                     \
        }                                                                                          \
        __CexTestContext.in_test = NULL;                                                           \
        fflush(__atest_stream);                                                                    \
        fflush(stdout);                                                                            \
        fflush(stderr);                                                                            \
    } while (0)

//
//  Prints current test header
//
#define test$print_header()                                                                        \
    do {                                                                                           \
        setbuf(__atest_stream, NULL);                                                              \
        if (__CexTestContext.verbosity > 0) {                                                      \
            fprintf(__atest_stream, "-------------------------------------\n");                    \
            fprintf(__atest_stream, "Running Tests: %s\n", __FILE__);                              \
            fprintf(__atest_stream, "-------------------------------------\n\n");                  \
        }                                                                                          \
        fflush(stdout);                                                                            \
        fflush(stderr);                                                                            \
    } while (0)

//
// Prints current tests stats total / passed / failed
//
#define test$print_footer()                                                                        \
    do {                                                                                           \
        if (__CexTestContext.verbosity > 0) {                                                      \
            fprintf(__atest_stream, "\n-------------------------------------\n");                  \
            fprintf(                                                                               \
                __atest_stream,                                                                    \
                "Total: %d Passed: %d Failed: %d\n",                                               \
                __CexTestContext.tests_run,                                                        \
                __CexTestContext.tests_run - __CexTestContext.tests_failed,                        \
                __CexTestContext.tests_failed                                                      \
            );                                                                                     \
            fprintf(__atest_stream, "-------------------------------------\n");                    \
        } else {                                                                                   \
            fprintf(                                                                               \
                __atest_stream,                                                                    \
                "[%s] %-70s [%2d/%2d]\n",                                                          \
                __CexTestContext.tests_failed == 0 ? CEXTEST_CGREEN "PASS" CEXTEST_CNONE           \
                                                   : CEXTEST_CRED "FAIL" CEXTEST_CNONE,            \
                __FILE__,                                                                          \
                __CexTestContext.tests_run - __CexTestContext.tests_failed,                        \
                __CexTestContext.tests_run                                                         \
            );                                                                                     \
        }                                                                                          \
        fflush(__atest_stream);                                                                    \
        fflush(stdout);                                                                            \
        fflush(stderr);                                                                            \
    } while (0)

//
//  Utility macro for returning main() exit code based on test failed/run, if no tests
//  run it's an error too
//
#define test$args_parse(argc, argv)                                                                \
    do {                                                                                           \
        if (argc == 1)                                                                             \
            break;                                                                                 \
        if (argc > 3) {                                                                            \
            fprintf(                                                                               \
                __atest_stream,                                                                    \
                "Too many arguments: use test_name_exec vvv|q test_case_name\n"                    \
            );                                                                                     \
            return 1;                                                                              \
        }                                                                                          \
        char a;                                                                                    \
        int i = 0;                                                                                 \
        int _has_quiet = 0;                                                                        \
        int _verb = 0;                                                                             \
        while ((a = argv[1][i]) != '\0') {                                                         \
            if (a == 'q')                                                                          \
                _has_quiet = 1;                                                                    \
            if (a == 'v')                                                                          \
                _verb++;                                                                           \
            i++;                                                                                   \
        }                                                                                          \
        __CexTestContext.verbosity = _has_quiet ? 0 : _verb;                                       \
        if (__CexTestContext.verbosity == 0) {                                                     \
            freopen(CEXTEST_NULL_DEVICE, "w", stdout);                                             \
        }                                                                                          \
    } while (0);

#define test$exit_code() (-1 ? __CexTestContext.tests_run == 0 : __CexTestContext.tests_failed)

/*
 *
 * Utility non public macros
 *
 */
#define __atest_stream (__CexTestContext.out_stream == NULL ? stderr : __CexTestContext.out_stream)
#define __STRINGIZE_DETAIL(x) #x
#define __STRINGIZE(x) __STRINGIZE_DETAIL(x)
#define __CEXTEST_LOG_ERR(msg) (__FILE__ ":" __STRINGIZE(__LINE__) " -> " msg)
#define __CEXTEST_NON_NULL(x) ((x != NULL) ? (x) : "")
