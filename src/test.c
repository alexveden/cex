#include "all.h"
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
_check_eq_u64(u64 a, u64 b, int line, enum _cex_test_eq_op_e op)
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
            "%s:%d -> %lu %s %lu",
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
                            for (usize z = 0; z < max_len; z++) {
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
                if (n_errors == 0) {
                    // Weird case when the only difference is new line, str.split_lines() skip that
                    sbuf.appendf(&errors, "\tA at line: `%S`\n", str.slice.sub(str.sstr(a), -10, 0));
                    sbuf.appendf(&errors, "\tB at line: `%S`\n", str.slice.sub(str.sstr(b), -10, 0));
                    sbuf.appendf(&errors, "^ New line diff (only last part displayed)\n");
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

Exc test$noopt __attribute__((noinline))
_cex_test_flush_cpu_cache(void)
{
    u64 buffer_size = 128 * 1024 * 1024;

    volatile char* flush_buffer = (volatile char*)malloc(buffer_size);
    if (!flush_buffer) { return Error.memory; }

    // The 'volatile' keyword prevents the compiler from optimizing this loop away
    for (size_t i = 0; i < buffer_size; i += 64) { // Step by typical cache line size (64 bytes)
        flush_buffer[i] = (char)(i & 0xFF);
    }
    free((void*)flush_buffer);
    return Error.ok;
}

Exc test$noopt __attribute__((noinline))
_cex_test_bench_call_timer_overhead(void)
{
    os.timer();
    return EOK;
}

static Exc test$noopt __attribute__((noinline))
cex_test_run_bench_case(struct _cex_test_case_s* case_ctx)
{
    uassert(case_ctx->is_benchmark);

    Exc result = EOK;

    f64 t = os.timer();
    f64 t_overhead = 100000000.0;

    // Assess empty function call overhead
    for (u32 i = 0; i < 100; i++) {
        t = os.timer();
        result = _cex_test_bench_call_timer_overhead();
        f64 t2 = os.timer();
        f64 t3 = os.timer(); // this one for excluding t2 call time
        f64 t_elapsed = t2 - t - (t3 - t2);
        if (t_elapsed > 0 && t_elapsed < t_overhead) { t_overhead = t_elapsed; }
    }
    uassert(t_overhead > 0 && t_overhead < 0.001);

    // Evict CPU cache to make sure next function call is cold cached
    t = os.timer();
    _cex_test_flush_cpu_cache();
    f64 t_elapsed = os.timer() - t;
    uassert(t_elapsed > 0.001 && "cpu cache flush happened too fast");
    // printf("cpu cache flush took: %fsec, call overhead: %fns\n", t_elapsed, t_overhead*1e9);


    // Cold start handle
    t = os.timer();
    result = case_ctx->test_fn();
    t_elapsed = os.timer() - t;
    if (result) { return result; }

    f64 cold_time = t_elapsed;
    f64 hot_time = t_elapsed;

    u32 n = (cold_time > 0.010) ? 10 : 100;

    for (u32 i = 0; i < n; i++) {
        t = os.timer();
        result = case_ctx->test_fn();
        t_elapsed = os.timer() - t;
        if (result) { return result; }
        // Instead of using averaging, we use minimum non zero time statistic,
        // which should converge to the statistical mode of the distribution (most frequency of
        // measurements) Inspired by code::dive conference 2015 - Andrei Alexandrescu - Writing Fast
        // Code I https://www.youtube.com/watch?v=vrfYLlR8X8k&t=1036s
        if (t_elapsed > 0 && t_elapsed < hot_time) { hot_time = t_elapsed; }
    }

    if (cold_time > t_overhead) { cold_time -= t_overhead; }
    if (hot_time > t_overhead) { hot_time -= t_overhead; }

    char* duration = "s  ";
    f64 factor = 1.0;
    if (hot_time < 1) {
        if (hot_time < 10e-4) {
            if (hot_time < 10e-7) {
                duration = "ns ";
                factor = 10e8;
            } else {
                duration = "us ";
                factor = 10e5;
            }
        } else {
            duration = "ms ";
            factor = 10e2;
        }
    }
    fprintf(
        stderr,
        " cold: %8.3f%s hot: %8.3f%s ",
        cold_time * factor,
        duration,
        hot_time * factor,
        duration
    );

    return result;
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
        argparse$opt(&ctx->is_benchmark, '\0', "bench", .help = "run test$bench() functions"),
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

    if (ctx->quiet_mode) {
        fprintf(stderr, "%s ", ctx->suite_file);
        if (ctx->is_benchmark) { fprintf(stderr, " >>>>\n"); }
    }

    if (ctx->is_benchmark && mem$asan_enabled()) {
        log$warn(
            "ASAN is enabled, it will take performance impact! Try recompile with enabled optimization and ASAN off.\n"
        );
    }

    for$each (t, ctx->test_cases) {
        ctx->case_name = t.test_name;
        ctx->tests_run++;
        if (ctx->is_benchmark != t.is_benchmark) { continue; }
        if (ctx->case_filter && !(str.match(t.test_name, ctx->case_filter) || str.find(t.test_name, ctx->case_filter)) {
            continue; }

        if (!ctx->quiet_mode || ctx->is_benchmark) {
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

        if (ctx->is_benchmark) {
            // NOTE: we don't mute bench output because muting uses files on disk,
            //       therefore has huge performance impact
            err = cex_test_run_bench_case(&t);
        } else {
            cex_test_mute();
            err = t.test_fn();
            if (ctx->quiet_mode && err != EOK) {
                fprintf(stdout, "[%s] %s\n", ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL", err);
                fprintf(stdout, "Test suite: %s case: %s\n", ctx->suite_file, t.test_name);
            }
            cex_test_unmute(err);
        }

        if (err == EOK) {
            if (ctx->quiet_mode) {
                fprintf(stderr, ".");
                if (ctx->is_benchmark) { fprintf(stderr, "\n"); }
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
                if (ctx->is_benchmark) { fprintf(stderr, "\n"); }
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
        if (ctx->is_benchmark) { fprintf(stderr, "<<<< %s\n", ctx->suite_file); }
    }

    if (ctx->out_stream) {
        fclose(ctx->out_stream);
        ctx->out_stream = NULL;
    }
    return ctx->tests_run == 0 || ctx->tests_failed > 0;
}
#    endif // ifdef CEX_TEST
#endif
