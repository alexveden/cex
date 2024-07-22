#include <_cexcore/_stb_sprintf.c>
#include <_cexcore/allocators.c>
#include <_cexcore/cex.c>
#include <_cexcore/cextest.h>
#include <_cexcore/io.c>
#include <_cexcore/io.h>
#include <_cexcore/str.c>
#include <stdio.h>

const Allocator_i* allocator;
/*
 * SUITE INIT / SHUTDOWN
 */
test$teardown()
{
    allocator = allocators.heap.destroy();
    return EOK;
}

test$setup()
{
    uassert_enable();
    allocator = allocators.heap.create();
    return EOK;
}

/*
 *
 *   TEST SUITE
 *
 */

test$case(test_io)
{
    io_c file = { 0 };
    tassert_eqs(Error.not_found, io.fopen(&file, "test_not_exist.txt", "r", allocator));

    uassert_disable();
    tassert_eqs(Error.argument, io.fopen(&file, "test_not_exist.txt", "r", NULL));
    tassert_eqs(Error.argument, io.fopen(&file, "test_not_exist.txt", NULL, allocator));
    tassert_eqs(Error.argument, io.fopen(&file, NULL, "r", allocator));
    tassert_eqs(Error.argument, io.fopen(NULL, "test.txt", "r", allocator));
    return EOK;
}

test$case(test_readall)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r", allocator));
    tassert(file._fh != NULL);
    tassert_eql(file._fsize, 0); // not calculated yet!
    tassert(file._fbuf == NULL);
    tassert(file._fbuf_size == 0);
    tassert(file._allocator == allocator);

    str_c content;
    tassert_eqs(Error.ok, io.readall(&file, &content));

    tassert_eqi(50, io.size(&file));
    tassert_eqi(50 + 16, file._fbuf_size); // +1 null term
    tassert_eqi(0, file._fbuf[file._fsize]);
    tassert_eqs(
        "000000001\n"
        "000000002\n"
        "000000003\n"
        "000000004\n"
        "000000005\n",
        content.buf
    );


    io.close(&file);
    tassert(file._fh == NULL);
    tassert_eql(file._fsize, 0);
    tassert(file._fbuf == NULL);
    return EOK;
}

test$case(test_readall_empty)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r", allocator));

    tassert(file._fh != NULL);
    tassert_eql(file._fsize, 0); // not calculated yet!
    tassert(file._fbuf == NULL);
    tassert(file._fbuf_size == 0);
    tassert(file._allocator == allocator);

    str_c content;
    tassert_eqs(Error.eof, io.readall(&file, &content));
    tassert_eqi(0, io.size(&file));
    tassert(file._fbuf == NULL);
    tassert_eqi(file._fbuf_size, 0);
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);


    io.close(&file);
    return EOK;
}

test$case(test_readall_stdin)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fattach(&file, stdin, allocator));
    tassert(file._fh == stdin);
    tassert_eql(file._fsize, 0); // not calculated yet!
    tassert(file._fbuf == NULL);
    tassert(file._fbuf_size == 0);
    tassert(file._allocator == allocator);
    tassert_eqi(0, io.size(&file));

    str_c content;
    tassert_eqs(
        "io.readall() not allowed for pipe/socket/std[in/out/err]",
        io.readall(&file, &content)
    );
    tassert_eql(file._fsize, 0); // not calculated yet!
    tassert(file._fbuf == NULL);
    tassert(file._fbuf_size == 0);


    io.close(&file);
    return EOK;
}

test$case(test_read_line)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r", allocator));

    str_c content;
    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000001");
    tassert_eqi(content.len, 9);

    tassert_eqi(file._fbuf_size, 4096 - 1);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000002");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000003");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000004");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000005");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.eof, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);

    io.close(&file);
    return EOK;
}

test$case(test_read_line_empty_file)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r", allocator));

    str_c content;
    tassert_eqs(Error.eof, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);

    io.close(&file);
    return EOK;
}

test$case(test_read_line_binary_file_with_zero_char)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_zero_byte.txt", "r", allocator));

    str_c content;
    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000001");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.integrity, io.readline(&file, &content));
    tassert_eqs(content.buf, NULL);
    tassert_eqi(content.len, 0);

    io.close(&file);
    return EOK;
}

test$case(test_read_line_win_new_line)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_win_newline.txt", "r", allocator));

    str_c content;
    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000001");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000002");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000003");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000004");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000005");
    tassert_eqi(content.len, 9);

    tassert_eqs(Error.eof, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);

    io.close(&file);
    return EOK;
}

test$case(test_read_line_only_new_lines)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_only_newline.txt", "r", allocator));

    str_c content;
    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);

    tassert_eqs(Error.eof, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);

    io.close(&file);
    return EOK;
}

test$case(test_read_all_then_read_line)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r", allocator));
    tassert(file._fh != NULL);
    tassert_eql(file._fsize, 0); // not calculated yet!
    tassert(file._fbuf == NULL);
    tassert(file._fbuf_size == 0);
    tassert(file._allocator == allocator);

    str_c content;
    tassert_eqs(Error.ok, io.readall(&file, &content));
    tassert_eqi(50, io.size(&file));
    tassert_eqi(file._fbuf_size, 66);

    tassert_eqs(Error.eof, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);


    tassert_eqs(Error.ok, io.seek(&file, 0, SEEK_SET));
    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqs(content.buf, "000000001");
    tassert_eqi(content.len, 9);

    io.close(&file);
    return EOK;
}

test$case(test_read_long_line)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r", allocator));
    tassert(file._fh != NULL);
    tassert_eql(file._fsize, 0); // not calculated yet!
    tassert(file._fbuf == NULL);
    tassert(file._fbuf_size == 0);
    tassert(file._allocator == allocator);

    str_c content;
    tassert_eqi(4096 + 4095 + 2, io.size(&file));

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqi(file._fbuf_size, 4096 - 1);
    tassert(str.starts_with(content, str.cstr("4095")));
    tassert_eqi(content.len, 4095);
    tassert_eqi(0, content.buf[content.len]); // null term

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqi(file._fbuf_size, 4096 * 2 - 1); // buffer resized
    tassert(str.starts_with(content, str.cstr("4096")));
    tassert_eqi(content.len, 4096);
    tassert_eqi(0, content.buf[content.len]); // null term


    tassert_eqs(Error.eof, io.readline(&file, &content));
    tassert_eqs(content.buf, "");
    tassert_eqi(content.len, 0);

    io.close(&file);
    return EOK;
}

test$case(test_readall_realloc)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r", allocator));
    tassert(file._fh != NULL);
    tassert_eql(file._fsize, 0); // not calculated yet!
    tassert(file._fbuf == NULL);
    tassert(file._fbuf_size == 0);
    tassert(file._allocator == allocator);

    str_c content;
    tassert_eqi(4096 + 4095 + 2, io.size(&file));

    tassert_eqs(Error.ok, io.readline(&file, &content));
    tassert_eqi(file._fbuf_size, 4096 - 1);
    tassert(str.starts_with(content, str.cstr("4095")));
    tassert_eqi(content.len, 4095);
    tassert_eqi(0, content.buf[content.len]); // null term


    io.rewind(&file);

    tassert_eqs(Error.ok, io.readall(&file, &content));
    tassert_eqi(file._fbuf_size, 4095 + 4096 + 2 + 16);
    tassert(str.starts_with(content, str.cstr("4095")));
    tassert_eqi(str.find(content, str.cstr("4096"), 0, 0), 4096);
    tassert_eqi(content.len, 4095 + 4096 + 2);

    io.close(&file);
    return EOK;
}

test$case(test_read)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r", allocator));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    size_t read_len = 4;
    tassert_eqs(Error.ok, io.read(&file, buf, 2, &read_len));
    tassert_eqi(memcmp(buf, "40950000", 8), 0);
    tassert_eqi(read_len, 4);


    io.close(&file);
    return EOK;
}

test$case(test_read_empty)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r", allocator));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    size_t read_len = 4;
    tassert_eqs(Error.eof, io.read(&file, buf, 2, &read_len));
    tassert_eqi(memcmp(buf, "zzzzzzzz", 8), 0); // untouched!
    tassert_eqi(read_len, 0);

    io.close(&file);
    return EOK;
}

test$case(test_read_not_all)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r", allocator));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    size_t read_len = 100;
    tassert_eqs(Error.ok, io.read(&file, buf, 1, &read_len));
    tassert_eqi(read_len, 50);

    // NOTE: read method does not null terminate!
    tassert_eqi(buf[read_len], 'z');

    buf[read_len] = '\0'; // null terminate to compare string result below
    tassert_eqs(
        "000000001\n"
        "000000002\n"
        "000000003\n"
        "000000004\n"
        "000000005\n",
        buf
    );

    tassert_eqs(Error.eof, io.read(&file, buf, 1, &read_len));
    tassert_eqi(read_len, 0);

    io.close(&file);
    return EOK;
}

test$case(test_fprintf)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fattach(&file, stdout, allocator));
    tassert(file._fh == stdout);
    tassert(file._fbuf == NULL);
    tassert(file._fbuf_size == 0);
    tassert(file._allocator == allocator);
    tassert_eqi(0, io.size(&file));


    tassert_eqs(EOK, io.fprintf(&file, "Hi there it's a io.fprintf\n"));

    io.close(&file);
    return EOK;
}

test$case(test_fprintf_to_file)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_fprintf.txt", "w+", allocator));

    char buf[4] = { "1234" };
    str_c s1 = str.cbuf(buf, 4);
    tassert_eqi(s1.len, 4);
    tassert_eqi(s1.buf[3], '4');
    tassert_eqs(EOK, io.fprintf(&file, "io.fprintf: str_c: %S\n", s1));

    str_c content;
    io.rewind(&file);

    tassert_eqs(EOK, io.readall(&file, &content));
    tassert_eqi(0, str.cmp(content, s$("io.fprintf: str_c: 1234\n")));


    io.close(&file);
    return EOK;
}

test$case(test_write)
{
    io_c file = { 0 };
    tassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_write.txt", "w+", allocator));

    char buf[4] = { "1234" };

    tassert_eqs(EOK, io.write(&file, buf, sizeof(char), arr$len(buf)));

    str_c content;
    io.rewind(&file);
    tassert_eqs(EOK, io.readall(&file, &content));
    tassert_eqi(0, str.cmp(content, s$("1234")));

    io.close(&file);
    return EOK;
}
/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_io);
    test$run(test_readall);
    test$run(test_readall_empty);
    test$run(test_readall_stdin);
    test$run(test_read_line);
    test$run(test_read_line_empty_file);
    test$run(test_read_line_binary_file_with_zero_char);
    test$run(test_read_line_win_new_line);
    test$run(test_read_line_only_new_lines);
    test$run(test_read_all_then_read_line);
    test$run(test_read_long_line);
    test$run(test_readall_realloc);
    test$run(test_read);
    test$run(test_read_empty);
    test$run(test_read_not_all);
    test$run(test_fprintf);
    test$run(test_fprintf_to_file);
    test$run(test_write);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
