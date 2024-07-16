#include <cex.c>

const Allocator_i* allocator;

cextest$teardown(){
    allocator = allocators.heap.destroy(); // this also nullifies allocator
    return EOK;
}

cextest$setup(){
    uassert_enable(); // re-enable if you disabled it in some test case
    allocator = allocators.heap.create();
    return EOK;
}

cextest$case(my_test)
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
    cextest$args_parse(argc, argv);
    cextest$print_header();  // >>> all tests below
    
    cextest$run(my_test);
    
    cextest$print_footer();  // ^^^^^ all tests runs are above
    return cextest$exit_code();
}
