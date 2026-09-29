

### Unit testing

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

- Test checks
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

- Test allocator
```c
test$case(my_test_case)
{
    // `test$alloc` is a per-case arena (1 MB page, scopes disabled), created
    // before each case and destroyed after — no manual free, not leak-tracked
    int* buf = mem$malloc(test$alloc, 256 * sizeof(int));

    return EOK;
}
```

- Simulating OOM
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
