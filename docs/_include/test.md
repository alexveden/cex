

### Unit testing

#### Running/building tests
```sh
./cex test create tests/test_mytest.c
./cex test run tests/test_mytest.c
./cex test run all
./cex test debug tests/test_mytest.c
./cex test clean all
./cex test --help
```

#### Unit Test structure
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

#### Test checks
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

#### Test allocator
```c
test$case(my_test_case)
{
    // `test$alloc` is a per-case arena (1 MB page, scopes disabled), created
    // before each case and destroyed after — no manual free, not leak-tracked
    int* buf = mem$malloc(test$alloc, 256 * sizeof(int));

    return EOK;
}
```

#### Simulating OOM
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

#### Replacing the global allocator (CEX_TEST mode)

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

#### Test file & rebuild requirements
```c
// tests/ folder only, name test_*.c, include sources directly (unity build)
#include "src/foo.c"   // only #include "" is tracked for rebuilds
// linker/compiler extras: cexy$ld_libs, cexy$ld_args, cexy$cc_args_test
```

#### Mocking namespaces (CEX_TEST mode)
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

#### Benchmarking
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

#### Coverage

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

#### Test runner args (single test file only)
```sh
# args after the file path are forwarded to the built-in test runner
./cex test run tests/test_file.c --filter='case*'  # run matching cases only (-f)
./cex test run tests/test_file.c --breakpoint      # debugger on tassert failure (-b)
./cex test run tests/test_file.c --no-capture      # stream stdout as tests run (-o)
./cex test run tests/test_file.c --help            # full runner help
# runner flags: -f/--filter, -b/--breakpoint, -o/--no-capture, --bench, -q/--quiet
```

#### Test-mode extras
```c
// os.random.seed(0) runs before each case -> deterministic random sequences
// uassert() reporting can be toggled (test mode only)
uassert_disable();
run_bad_stuff(NULL);
uassert_enable();
```



```c
/// Dedicated arena created fresh before each test case and destroyed afterward, always growing,
/// test$alloc is a dedicated arena created fresh before each test case and destroyed afterwards,
#define test$alloc(_cex__default_global__allocator_test)

/// Sets the probability (0.0–1.0) of simulated allocation failures for `test$alloc`:
/// - 0.0 = never fail (default)
/// - 1.0 = always fail
/// - 0.5 = approx 50% failure rate
/// Use to test OOM paths in test cases. Only available in CEX_TEST mode.
/// Automatically reset to 0.0 before each test case.
#define test$alloc_set_oom_probability(prob)

/// Benchmark case (runs only via ./cex test bench)
#define test$bench(NAME)

/// Unit-test test case
#define test$case(NAME)

/// main() function for test suite, you must place it into test file at the end
#define test$main()

/// Saves namespace(s) before scope — mock any function pointer inside, auto-restored on exit via
/// __cleanup__. Accepts 1-8 namespaces.
#define test$mock_scope(...)

/// Attribute for function which disables optimization for test cases or other functions
#define test$noopt

/// Optional: called before each test$case() starts
#define test$setup_case()

/// Optional: initializes at test suite once at start
#define test$setup_suite()

/// Optional: called after each test$case() ends
#define test$teardown_case()

/// Optional: shut down test suite once at the end
#define test$teardown_suite()




```
