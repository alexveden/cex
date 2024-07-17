
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
    free(a); // free without allocator to keep sanitizer calm

    tassert(true == 1);
    tassert_eqi(1, 1);
    tassert_eql(1, 1L);
    tassert_eqf(1.0, 1.0);
    tassert_eqe(EOK, Error.ok);

    uassert_disable();
    uassert(false && "this will be disabled, no abort!");

    tassertf(true == 0, "true != %d", false);

    // allocator->free(a); // if commented this triggers memory leak warning by allocator

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
