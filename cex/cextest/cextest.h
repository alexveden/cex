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
extern struct __ATestContext_s __ATestContext;

/*
 *
 * Benchmarking
 *
 */
#ifndef WIN32
// Windows sorry :)
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>
#define ATEST_NOOPT __attribute__((optimize("O0")))
#define ATEST_MIN(X, Y) ((X) < (Y) ? (X) : (Y))
// #define ATEST_NOOPT __attribute__((optnone))

#define ATEST_BENCH(N, __FUNC)                                                                     \
    do {                                                                                           \
        if (N < 0) {                                                                               \
            printf("N < 0, skipping");                                                             \
        } else {                                                                                   \
            struct timespec __atest_tv;                                                            \
            clock_gettime(CLOCK_MONOTONIC_RAW, &__atest_tv);                                       \
            f64 tv_start = __atest_tv.tv_nsec * 1.0 / 1000000000 + __atest_tv.tv_sec;              \
            clock_t __atest_start_time = clock();                                                  \
            u64 rdtsc_s, rdtsc_e;                                                                  \
            u64 rdtsc_min = UINT64_MAX;                                                            \
            for (int __atest_i = 0; __atest_i < (N); __atest_i++) {                                \
                rdtsc_s = __rdtsc();                                                               \
                (__FUNC);                                                                          \
                rdtsc_e = __rdtsc();                                                               \
                if (rdtsc_e - rdtsc_s > 10) {                                                      \
                    rdtsc_min = ATEST_MIN(rdtsc_min, rdtsc_e - rdtsc_s);                           \
                }                                                                                  \
            }                                                                                      \
            f64 __atest_elapsed_time = (f64)(clock() - __atest_start_time) / CLOCKS_PER_SEC;       \
            clock_gettime(CLOCK_MONOTONIC_RAW, &__atest_tv);                                       \
            f64 tv_end = __atest_tv.tv_nsec * 1.0 / 1000000000 + __atest_tv.tv_sec;                \
            f64 __atest_wall_elapsed = tv_end - tv_start;                                          \
            struct perf_event_attr pe = { 0 };                                                     \
            long long clock_count = 0;                                                             \
            pe.type = PERF_TYPE_HARDWARE;                                                          \
            pe.size = sizeof(struct perf_event_attr);                                              \
            pe.config = PERF_COUNT_HW_INSTRUCTIONS;                                                \
            pe.disabled = 1;                                                                       \
            pe.exclude_kernel = 1;                                                                 \
            pe.exclude_hv = 1;                                                                     \
            int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);                             \
            if (fd == -1) {                                                                        \
                perror("ATEST_BENCH: ERROR perf counter setup failed (must be run as "             \
                       "root)");                                                                   \
            } else {                                                                               \
                printf("ATEST_BENCH: calling function again for calculating CPU "                  \
                       "instructions\n");                                                          \
                ioctl(fd, PERF_EVENT_IOC_RESET, 0);                                                \
                ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);                                               \
                (__FUNC);                                                                          \
                ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);                                              \
                read(fd, &clock_count, sizeof(long long));                                         \
                close(fd);                                                                         \
            }                                                                                      \
            const char* duration = "sec";                                                          \
            f64 factor = 1;                                                                        \
            f64 sec = __atest_elapsed_time / N;                                                    \
            if (sec < 1) {                                                                         \
                if (sec < 10e-4) {                                                                 \
                    if (sec < 10e-7) {                                                             \
                        duration = "ns ";                                                          \
                        factor = 10e8;                                                             \
                    } else {                                                                       \
                        duration = "mcs";                                                          \
                        factor = 10e5;                                                             \
                    }                                                                              \
                } else {                                                                           \
                    duration = "ms ";                                                              \
                    factor = 10e2;                                                                 \
                }                                                                                  \
            }                                                                                      \
            printf("ATEST_BENCH: %s: \n", #__FUNC);                                                \
            printf("\t| Clock   |   Total(sec) |   Steps    |      Avg      |  "                   \
                   "AvgPerSec  \n");                                                               \
            printf(                                                                                \
                "\t| CPU     |   %7.4f    |    %*d |   %7.3f %s |  %0.3f  \n",                     \
                __atest_elapsed_time,                                                              \
                6,                                                                                 \
                N,                                                                                 \
                sec* factor,                                                                       \
                duration,                                                                          \
                1 / sec                                                                            \
            );                                                                                     \
            duration = "sec";                                                                      \
            factor = 1;                                                                            \
            sec = __atest_wall_elapsed / N;                                                        \
            if (sec < 1) {                                                                         \
                if (sec < 10e-4) {                                                                 \
                    if (sec < 10e-7) {                                                             \
                        duration = "ns ";                                                          \
                        factor = 10e8;                                                             \
                    } else {                                                                       \
                        duration = "mcs";                                                          \
                        factor = 10e5;                                                             \
                    }                                                                              \
                } else {                                                                           \
                    duration = "ms ";                                                              \
                    factor = 10e2;                                                                 \
                }                                                                                  \
            }                                                                                      \
            printf(                                                                                \
                "\t| WALL    |   %7.4f    |    %*d |   %7.3f %s |  %0.3f  \n",                     \
                __atest_wall_elapsed,                                                              \
                6,                                                                                 \
                N,                                                                                 \
                sec* factor,                                                                       \
                duration,                                                                          \
                1 / sec                                                                            \
            );                                                                                     \
            printf("\t| RDTSC (cycles per call)  min: %010ld\n", rdtsc_min);                       \
            printf("\t| CPU Instructions (per call) : %010lld\n", clock_count);                    \
        }                                                                                          \
    } while (0);

#endif

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
// #define ATEST_F(TESTNAME) char* TESTNAME()
#define ATEST_F(TESTNAME) __attribute__((optimize("O0"))) const char* TESTNAME()

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
        const char* result = (TESTNAME());                                                               \
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
            freopen("/dev/null", "w", stdout);                                                     \
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
