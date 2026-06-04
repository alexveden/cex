

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



```c
/// Dedicated arena created fresh before each test case and destroyed afterward, always growing,
/// test$alloc is a dedicated arena created fresh before each test case and destroyed afterwards,
#define test$alloc(_cex__default_global__allocator_test)

#define test$bench(NAME)

/// Unit-test test case
#define test$case(NAME)

/// main() function for test suite, you must place it into test file at the end
#define test$main()

/// Saves namespace(s) before scope — mock any function pointer inside, auto-restored on exit via
/// __cleanup__. Accepts 1-8 namespaces.
#define test$mock_ns(...)

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
