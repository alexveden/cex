#include "cex.h"
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
    sbuf_c listdir = {0};
    tassert_eqe(EOK, sbuf.create(&listdir, 1024, allocator));

    tassert_eqe(EOK, os.listdir(s$("tests/data/"), &listdir));
    tassert(sbuf.len(&listdir) > 0);

    u32 nit = 0;
    const char* expected[] = {
        "allocator_fopen.txt",       "text_file_50b.txt",
        "text_file_empty.txt",       "text_file_fprintf.txt",
        "text_file_line_4095.txt",   "text_file_only_newline.txt",
        "text_file_win_newline.txt", "text_file_write.txt",
        "text_file_zero_byte.txt",   "",
    };
    (void)expected;
    for$iter(str_c, it, sbuf.iter_split(&listdir, "\n", &it.iterator))
    {
        // io.printf("%d: %S\n", it.idx.i, *it.val);
        tassert_eqi(it.idx.i, nit);

        bool is_found = false;
        for$array(itf, expected, arr$len(expected))
        {
            if (str.cmp(*it.val, s$(*itf.val)) == 0) {
                is_found = true;
                break;
            }
        }
        if (!is_found) {
            io.printf("Not found in expected: %S\n", *it.val);
            tassert(is_found && "file not found in expected");
        }
        nit++;
    }
    tassert_eqi(nit, arr$len(expected));

    tassert(sbuf.len(&listdir) > 0);
    tassert_eqe(Error.not_found, os.listdir(s$("tests/data/unknownfolder"), &listdir));
    tassert_eqi(sbuf.len(&listdir), 0);

    sbuf.destroy(&listdir);
    return EOK;
}

test$case(test_os_getcwd)
{
    sbuf_c s = {0};
    tassert_eqe(EOK, sbuf.create(&s, 10, allocator));
    tassert_eqe(EOK, os.getcwd(&s));
    io.printf("'%S'\n", sbuf.to_str(&s));
    tassert_eqi(true, str.ends_with(sbuf.to_str(&s), s$("cex")));

    sbuf.destroy(&s);
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_os_listdir);
    test$run(test_os_getcwd);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
