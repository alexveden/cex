#include "cex.h"
#include <cex.c>
#include <cex/os/os.c>
#include <limits.h>

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
    sbuf_c listdir = { 0 };
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
    sbuf_c s = { 0 };
    tassert_eqe(EOK, sbuf.create(&s, 10, allocator));
    tassert_eqe(EOK, os.getcwd(&s));
    io.printf("'%S'\n", sbuf.to_str(&s));
    tassert_eqi(true, str.ends_with(sbuf.to_str(&s), s$("cex")));

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_os_path_exists)
{
    tassert_eqe(Error.argument, os.path.exists(str.cstr(NULL)));
    tassert_eqe(Error.argument, os.path.exists(str.cstr("")));
    tassert_eqe(EOK, os.path.exists(s$(".")));
    tassert_eqe(EOK, os.path.exists(s$("..")));
    tassert_eqe(EOK, os.path.exists(s$("./tests")));
    tassert_eqe(EOK, os.path.exists(s$("./tests/cexlib/test_os_posix.c")));
    tassert_eqe(Error.not_found, os.path.exists(s$("./tests/cexlib/test_os_posix.cpp")));

    tassert_eqe(EOK, os.path.exists(s$("tests/")));

    // substrings also work
    tassert_eqe(EOK, os.path.exists(str.sub(s$("tests/asldjalsdj"), 0, 6)));

    char buf[PATH_MAX + 10];
    memset(buf, 'a', arr$len(buf));
    buf[PATH_MAX + 8] = '\0';
    str_c s = str.cbuf(buf, arr$len(buf));
    s.len++;

    // Path is too long, and exceeds PATH_MAX buffer size
    tassert_eqe(Error.overflow, os.path.exists(s));

    s.len--;
    tassert_eqe("File name too long", os.path.exists(s));

    return EOK;
}

test$case(test_os_path_join)
{
    sbuf_c s = { 0 };
    tassert_eqe(EOK, sbuf.create(&s, 10, allocator));
    tassert_eqe(EOK, os.path.join(&s, "%S/%s_%d.txt", s$("cexstr"), "foo", 10));
    tassert_eqs("cexstr/foo_10.txt", s);
    sbuf.destroy(&s);
    return EOK;
}

test$case(test_os_setenv)
{
    // get non existing
    tassert_eqs(os.getenv("test_os_posix", NULL),  NULL);
    // get non existing, with default
    tassert_eqs(os.getenv("test_os_posix", "envdef"),  "envdef");

    // set env
    os.setenv("test_os_posix", "foo", true);
    tassert_eqs(os.getenv("test_os_posix", NULL),  "foo");

    // set without replacing
    os.setenv("test_os_posix", "bar", false);
    tassert_eqs(os.getenv("test_os_posix", NULL),  "foo");

    // set with replacing
    os.setenv("test_os_posix", "bar", true);
    tassert_eqs(os.getenv("test_os_posix", NULL),  "bar");

    // unset env
    os.unsetenv("test_os_posix");
    tassert_eqs(os.getenv("test_os_posix", NULL),  NULL);

    return EOK;
}
test$case(test_os_path_splitext)
{
    tassert_eqi(str.cmp(os.path.splitext(s$("foo.bar"), true), s$(".bar")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("foo.bar"), false), s$("foo")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("foo"), true), s$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("foo"), false), s$("foo")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("foo.bar.exe"), true), s$(".exe")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("foo.bar.exe"), false), s$("foo.bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("foo/bar.exe"), true), s$(".exe")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("foo/bar.exe"), false), s$("foo/bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar.exe"), true), s$(".exe")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar.exe"), false), s$("bar.foo/bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar"), true), s$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar"), false), s$("bar.foo/bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$(".gitingnore"), true), s$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$(".gitingnore"), false), s$(".gitingnore")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("...gitingnore"), true), s$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("...gitingnore"), false), s$("...gitingnore")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar..."), true), s$(".")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar..."), false), s$("bar.foo/bar..")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar."), true), s$(".")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar."), false), s$("bar.foo/bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("."), true), s$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("."), false), s$(".")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$(".."), true), s$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$(".."), false), s$("..")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar..exe"), true), s$(".exe")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$("bar.foo/bar..exe"), false), s$("bar.foo/bar.")), 0);

    tassert_eqi(str.cmp(os.path.splitext(s$(""), true), s$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(s$(""), false), s$("")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str.cstr(NULL), false), s$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str.cstr(NULL), true), s$("")), 0);
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_os_listdir);
    test$run(test_os_getcwd);
    test$run(test_os_path_exists);
    test$run(test_os_path_join);
    test$run(test_os_setenv);
    test$run(test_os_path_splitext);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
