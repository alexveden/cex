//
// C mimimalist testing framework
//
//
#ifndef __ATEST_H
#define __ATEST_H

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

#define ATEST_AMSG_MAX_LEN 512 // max length for aassertf() string formatting
#define ATEST_CRED "\033[0;31m"
#define ATEST_CGREEN "\033[0;32m"
#define ATEST_CNONE "\033[0m"
#ifdef _WIN32
#define ATEST_NULL_DEVICE "NUL:"
#else
#define ATEST_NULL_DEVICE "/dev/null"
#endif

#if defined(__clang__)
#define ATEST_NOOPT __attribute__((optnone))
#elif defined(__GNUC__) || defined(__GNUG__)
#define ATEST_NOOPT __attribute__((optimize("O0")))
#elif defined(_MSC_VER)
#warning "MSVS is untested"
#endif

//
//  atest.h settings, you can alter them in main() function, for example replace
//  out_stream by fopen() file
//
struct __ATestContext_s
{
    FILE* out_stream; // by default uses stdout, however you can replace it by any FILE*
                      // stream
    int tests_run;    // number of tests run
    int tests_failed; // number of tests failed
    int verbosity;    // verbosity level: 0 - only stats, 1 - short test results (. or F) +
                      // stats, 2 - all tests results, 3 - include logs
    const char* in_test;
    // Internal buffer for aassertf() string formatting
    char _str_buf[ATEST_AMSG_MAX_LEN];
};


/*
 *
 *  ASSERTS
 *
 */
//
// atassert( condition ) - simple assert, failes when condition false
//
#define atassert(A)                                                                                \
    do {                                                                                           \
        if (!(A)) {                                                                                \
            return __ATEST_LOG_ERR(#A);                                                            \
        }                                                                                          \
    } while (0)

//
// atassertf( condition, format_string, ...) - assert with printf() style formatting
// error message
//
#define atassertf(A, M, ...)                                                                       \
    do {                                                                                           \
        if (!(A)) {                                                                                \
            snprintf(                                                                              \
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR(M),                                                                \
                ##__VA_ARGS__                                                                      \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        }                                                                                          \
    } while (0)

//
// atassert_eqs(char * actual, char * expected) - compares strings via strcmp(), (NULL
// friendly)
//
#define atassert_eqs(_ac, _ex)                                                                     \
    do {                                                                                           \
        const char* ac = (_ac);                                                                    \
        const char* ex = (_ex);                                                                    \
        if ((ac) == NULL && (ex) != NULL) {                                                        \
            snprintf(                                                                              \
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR("NULL != '%s'"),                                                   \
                (char*)(ex)                                                                        \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        } else if ((ac) != NULL && (ex) == NULL) {                                                 \
            snprintf(                                                                              \
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR("'%s' != NULL"),                                                   \
                (char*)(ac)                                                                        \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        } else if (((ac) != NULL && (ex) != NULL) &&                                               \
                   (strcmp(__ATEST_NON_NULL((ac)), __ATEST_NON_NULL((ex))) != 0)) {                \
            snprintf(                                                                              \
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR("'%s' != '%s'"),                                                   \
                (char*)(ac),                                                                       \
                (char*)(ex)                                                                        \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        }                                                                                          \
    } while (0)

#define atassert_eqe(_ac, _ex)                                                                     \
    do {                                                                                           \
        const char* ac = (_ac);                                                                    \
        const char* ex = (_ex);                                                                    \
        if ((ac) == NULL && (ex) != NULL) {                                                        \
            snprintf(                                                                              \
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR("Error.ok != '%s'"),                                               \
                (char*)(ex)                                                                        \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        } else if ((ac) != NULL && (ex) == NULL) {                                                 \
            snprintf(                                                                              \
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR("'%s' != Error.ok"),                                               \
                (char*)(ac)                                                                        \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        } else if (((ac) != NULL && (ex) != NULL) &&                                               \
                   (strcmp(__ATEST_NON_NULL((ac)), __ATEST_NON_NULL((ex))) != 0)) {                \
            snprintf(                                                                              \
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR("'%s' != '%s'"),                                                   \
                (char*)(ac),                                                                       \
                (char*)(ex)                                                                        \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        }                                                                                          \
    } while (0)

//
// atassert_eqi(int actual, int expected) - compares equality of integers
//
#define atassert_eqi(_ac, _ex)                                                                     \
    do {                                                                                           \
        int ac = (_ac);                                                                            \
        int ex = (_ex);                                                                            \
        if ((ac) != (ex)) {                                                                        \
            snprintf(                                                                              \
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR("%d != %d"),                                                       \
                (ac),                                                                              \
                (ex)                                                                               \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        }                                                                                          \
    } while (0)

//
// atassert_eql(int actual, int expected) - compares equality of long integers
//
#define atassert_eql(_ac, _ex)                                                                     \
    do {                                                                                           \
        long ac = (_ac);                                                                           \
        long ex = (_ex);                                                                           \
        if ((ac) != (ex)) {                                                                        \
            snprintf(                                                                              \
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR("%ld != %ld"),                                                     \
                (ac),                                                                              \
                (ex)                                                                               \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        }                                                                                          \
    } while (0)

//
// atassert_eqf(f64 actual, f64 expected) - compares equiality of f32ing point
// numbers, NAN friendly, i.e. atassert_eqf(NAN, NAN) -- passes
//
#define atassert_eqf(_ac, _ex)                                                                     \
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
                __ATestContext._str_buf,                                                           \
                ATEST_AMSG_MAX_LEN - 1,                                                            \
                __ATEST_LOG_ERR("%f != %f (diff: %0.10f epsilon: %0.10f)"),                        \
                (ac),                                                                              \
                (ex),                                                                              \
                ((ac) - (ex)),                                                                     \
                (f64)FLT_EPSILON                                                                   \
            );                                                                                     \
            return __ATestContext._str_buf;                                                        \
        }                                                                                          \
    } while (0)

//
// atlogf() - simple log message with additional formatting and information about what
// file and line of code was caller of the alogf()
//
#define atlogf(M, ...)                                                                             \
    if (__ATestContext.verbosity >= 3) {                                                           \
        fprintf(__atest_stream, __ATEST_LOG(M), ##__VA_ARGS__);                                    \
    }

/*
 *
 *  TEST FUNCTIONS
 *
 */

//
//  ATEST_SETUP_F() is mandatory for every test file,
//  only one instance of this function is allowed.
//
//  This function is launched before every test is run.
//
//  ATEST_SETUP_F() may return pointer to atest_shutdown function, or NULL if no such
//  function.
//
// // Bacic function, no setup and no shutdown
// ATEST_SETUP_F(void)
// {
//     return NULL;
// }
//
// // Shutting down after every tests
// void shutdown(void){
//     printf("atest_shutdown()\n");
// }
//
// ATEST_SETUP_F(void)
// {
//     printf("atest_setup()\n");
//     return &shutdown;
// }
typedef void (*atest_shutdown_ptr)(void);
atest_shutdown_ptr __atest_setup_func();

#define ATEST_SETUP_F                                                                              \
    struct __ATestContext_s __ATestContext = {                                                     \
        .out_stream = NULL,                                                                        \
        .tests_run = 0,                                                                            \
        .tests_failed = 0,                                                                         \
        .verbosity = 3,                                                                            \
        .in_test = NULL,                                                                           \
    };                                                                                             \
    atest_shutdown_ptr __atest_setup_func
//
// Typical test function template
//  MUST return NULL on test success, or char* with error message when failed!
//
#define ATEST_F(TESTNAME) ATEST_NOOPT char* TESTNAME()

//
// To be used in main() test launching function
//
//  ATEST_F(test_ok){  .... tests code here ..}
//
//  int main(...) { ...
//        ATEST_RUN(test_ok);
//  }
#define ATEST_RUN(TESTNAME)                                                                        \
    do {                                                                                           \
        if (argc >= 3 && strcmp(argv[2], #TESTNAME) != 0) {                                        \
            break;                                                                                 \
        }                                                                                          \
        __ATestContext.in_test = #TESTNAME;                                                        \
        atest_shutdown_ptr shutdown_fun_p = __atest_setup_func();                                  \
        char* result = (TESTNAME());                                                               \
        __ATestContext.tests_run++;                                                                \
        if (result == NULL) {                                                                      \
            if (__ATestContext.verbosity > 0) {                                                    \
                if (__ATestContext.verbosity == 1) {                                               \
                    fprintf(__atest_stream, ".");                                                  \
                } else {                                                                           \
                    fprintf(                                                                       \
                        __atest_stream,                                                            \
                        "[%s] %s\n",                                                               \
                        ATEST_CGREEN "PASS" ATEST_CNONE,                                           \
                        #TESTNAME                                                                  \
                    );                                                                             \
                }                                                                                  \
            }                                                                                      \
        } else {                                                                                   \
            if (__ATestContext.verbosity >= 0) {                                                   \
                if (__ATestContext.verbosity == 0) {                                               \
                    fprintf(                                                                       \
                        __atest_stream,                                                            \
                        "[%s] %s %s\n",                                                            \
                        ATEST_CRED "FAIL" ATEST_CNONE,                                             \
                        result,                                                                    \
                        #TESTNAME                                                                  \
                    );                                                                             \
                } else {                                                                           \
                    fprintf(                                                                       \
                        __atest_stream,                                                            \
                        "[%s] %s %s\n",                                                            \
                        ATEST_CRED "FAIL" ATEST_CNONE,                                             \
                        result,                                                                    \
                        #TESTNAME                                                                  \
                    );                                                                             \
                }                                                                                  \
            }                                                                                      \
            __ATestContext.tests_failed++;                                                         \
        }                                                                                          \
        if (shutdown_fun_p != NULL) {                                                              \
            (*shutdown_fun_p)();                                                                   \
        }                                                                                          \
        __ATestContext.in_test = NULL;                                                             \
    } while (0)

//
//  Prints current test header
//
#define ATEST_PRINT_HEAD()                                                                         \
    do {                                                                                           \
        setbuf(__atest_stream, NULL);                                                              \
        if (__ATestContext.verbosity > 0) {                                                        \
            fprintf(__atest_stream, "-------------------------------------\n");                    \
            fprintf(__atest_stream, "Running Tests: %s\n", __FILE__);                              \
            fprintf(__atest_stream, "-------------------------------------\n\n");                  \
        }                                                                                          \
        fflush(0);                                                                                 \
    } while (0)

//
// Prints current tests stats total / passed / failed
//
#define ATEST_PRINT_FOOTER()                                                                       \
    do {                                                                                           \
        if (__ATestContext.verbosity > 0) {                                                        \
            fprintf(__atest_stream, "\n-------------------------------------\n");                  \
            fprintf(                                                                               \
                __atest_stream,                                                                    \
                "Total: %d Passed: %d Failed: %d\n",                                               \
                __ATestContext.tests_run,                                                          \
                __ATestContext.tests_run - __ATestContext.tests_failed,                            \
                __ATestContext.tests_failed                                                        \
            );                                                                                     \
            fprintf(__atest_stream, "-------------------------------------\n");                    \
        } else {                                                                                   \
            fprintf(                                                                               \
                __atest_stream,                                                                    \
                "[%s] %-70s [%2d/%2d]\n",                                                          \
                __ATestContext.tests_failed == 0 ? ATEST_CGREEN "PASS" ATEST_CNONE                 \
                                                 : ATEST_CRED "FAIL" ATEST_CNONE,                  \
                __FILE__,                                                                          \
                __ATestContext.tests_run - __ATestContext.tests_failed,                            \
                __ATestContext.tests_run                                                           \
            );                                                                                     \
        }                                                                                          \
    } while (0)

//
//  Utility macro for returning main() exit code based on test failed/run, if no tests
//  run it's an error too
//
#define ATEST_PARSE_MAINARGS(argc, argv)                                                           \
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
        __ATestContext.verbosity = _has_quiet ? 0 : _verb;                                         \
        if (__ATestContext.verbosity == 0) {                                                       \
            freopen(ATEST_NULL_DEVICE, "w", stdout);                                               \
        }                                                                                          \
    } while (0);

#define ATEST_EXITCODE() (-1 ? __ATestContext.tests_run == 0 : __ATestContext.tests_failed)

/*
 *
 * Utility non public macros
 *
 */
#define __atest_stream (__ATestContext.out_stream == NULL ? stderr : __ATestContext.out_stream)
#define __STRINGIZE_DETAIL(x) #x
#define __STRINGIZE(x) __STRINGIZE_DETAIL(x)
#define __ATEST_LOG_ERR(msg) (__FILE__ ":" __STRINGIZE(__LINE__) " -> " msg)
#define __ATEST_LOG(msg) ("[ LOG] " __FILE__ ":" __STRINGIZE(__LINE__) " -> " msg)
#define __ATEST_NON_NULL(x) ((x != NULL) ? (x) : "")

#endif // !__ATEST_H
