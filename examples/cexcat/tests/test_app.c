// NOTE: we do "#include .c" to have granular control over test behavior, also exposing static
// functions for testing, and making test build process trivial, i.e. "gcc test_name.c"
#include "../src/App.c"
#include <cex.c>

const Allocator_i* allocator;

test$teardown()
{
    allocator = allocators.heap.destroy(); // this also nullifies allocator
    return EOK;
}

test$setup()
{
    uassert_enable(); // re-enable if you disabled it in some test case
    allocator = allocators.heap.create();
    return EOK;
}

test$case(app_create_test_bad_args)
{
    // Let's test App initialization
    App_c app = { 0 };
    char* argv[] = { "app_name" };
    u32 argc = arr$len(argv);

    // If there is a problem with arguments Error.argsparse returned
    tassert_eqe(Error.argsparse, App.create(&app, argc, argv, allocator));
    return EOK;
}

test$case(app_create_test_valid_args)
{
    App_c app = { 0 };
    char* argv[] = { "app_name", "file1", "file-b", "file.c" };
    u32 argc = arr$len(argv);

    // All good!
    tassert_eqe(Error.ok, App.create(&app, argc, argv, allocator));

    // Cool check if we have correctly processed argv and put them into App_c
    tassert_eqi(app.is_csv, false);
    tassert_eqi(app.files_count, 3);
    tassert_eqs(app.files[0], "file1");
    tassert_eqs(app.files[1], "file-b");
    tassert_eqs(app.files[2], "file.c");

    // NOTE: EOK is a shortcut for Error.ok
    return EOK;
}

test$case(app_main_read_all)
{
    char* files[] = { "file1", "file-b", "file.c" };
    App_c app = { .files_count = arr$len(files), .files = files, .is_csv = false };
    tassert_eqi(app.is_csv, false);
    tassert_eqi(app.files_count, 3);

    // Let's loop through all files and parse them
    // NOTE: for$array is an iterator for plain arrays, `it` is an iterator variable which has
    // automatic type based on array element. it.val is a pointer to data, it.idx.i - index.
    for$array(it, app.files, app.files_count)
    {
        utracef("Checking file #%ld: %s\n", it.idx, *it.val);
        tassert_eqs(*it.val, files[it.idx]);
    }

    tassert_eqe(Error.io, App.main(&app, allocator));
    // Oops, main has failed, no such file or directory!
    // But, look now you have a traceback in the console!
    // [ERROR] ( cex.c:4891 io_fopen() ) [IOError] No such file or directory, file: file1
    // [^STCK] ( App.c:57 App_main() ) ^^^^^ [IOError] in io.fopen(&f, *it.val, "r", allocator)

    return EOK;
}

test$case(app__process_plain_empty)
{
    App_c app = {0};

    io_c file; 
    e$ret(io.fopen(&file, "tests/file_empty.txt", "r", allocator));


    // NOTE: when #include.c, we are able to unit test static functions!
    //
    // Errors can be standard i.e. EOK / Error.memory / Error.io, etc.
    // However sometimes it's more useful to use string literals as errors
    // if you never intended to catch them.
    tassert_eqe("zero file size", App__process_plain(&app, &file));


    // io.close(&file);  // forgetting the io.close() triggers the allocator warning

    // Look at the error message (allocator detected that we didn't properly closed the file!)
    // [TRACE] ( cex.c:3446 allocators__heap__destroy() ) 
    //      Allocator: Possible FILE* leaks: allocator->fopen() [1] != allocator->fclose() [0]!
    tassert(file._fh == NULL && "oops, this is intentional fail, look into comment in the test");

    return EOK;
}

test$case(app__process_plain_one_line)
{
    App_c app = {0};

    io_c file; 
    e$ret(io.fopen(&file, "tests/file_oneline.txt", "r", allocator));


    // NOTE: when #include.c, we are able to unit test static functions!
    tassert_eqe(EOK, App__process_plain(&app, &file));


    io.close(&file);  // close also frees all allocated temporary memory if any
    return EOK;
}

test$case(app__process_csv)
{
    // mocking app allocator
    App_c app = {.allocator = allocator};

    io_c file; 
    e$ret(io.fopen(&file, "tests/file_csv_good.csv", "r", allocator));

    tassert_eqe(EOK, App__process_csv(&app, &file));

    io.close(&file);  // close also frees all allocated temporary memory if any
    return EOK;
}

test$case(app__process_csv_io_fail)
{
    // mocking app allocator
    App_c app = {.allocator = allocator};

    io_c file = {._fh = stdout}; 

    tassert_eqe(EOK, App__process_csv(&app, &file));

    io.close(&file);  // close also frees all allocated temporary memory if any
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(app_create_test_bad_args);
    test$run(app_create_test_valid_args);
    test$run(app_main_read_all);
    test$run(app__process_plain_empty);
    test$run(app__process_plain_one_line);
    test$run(app__process_csv);
    test$run(app__process_csv_io_fail);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
