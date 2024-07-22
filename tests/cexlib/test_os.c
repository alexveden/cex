#include <cex.c>
#include <cex/os/os.c>

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

test$case(test_os_listdir)
{
    sbuf_c listdir;
    tassert_eqe(EOK, os.listdir(s$("./"), &listdir, allocator));

    tassert(sbuf.len(&listdir) > 0);

    for$iter(str_c, it, str.iter_split(sbuf.tostr(&listdir), "\n", &it.iterator))
    {
        io.printf("%S\n", *it.val);
    }

    sbuf.destroy(&listdir);
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_os_listdir);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
