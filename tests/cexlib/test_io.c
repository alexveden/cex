#include <cex/cex.h>
#include <cex/cexlib/allocators.c>
#include <cex/cexlib/io.c>
#include <cex/cexlib/sview.c>
#include <cex/cexlib/io.h>
#include <cex/cextest/cextest.h>
#include <stdio.h>

const Allocator_c* allocator;
/*
 * SUITE INIT / SHUTDOWN
 */
void
my_test_shutdown_func(void)
{
    AllocatorHeap_free();
}

ATEST_SETUP_F(void)
{
    allocator = AllocatorHeap_new();
    uassert_enable();

    // return NULL;   // if no shutdown logic needed
    return &my_test_shutdown_func; // return pointer to void some_shutdown_func(void)
}

/*
 *
 *   TEST SUITE
 *
 */

ATEST_F(test_io)
{
    io_c file = { 0 };
    atassert_eqs(Error.io, io.fopen(&file, "test_not_exist.txt", "r", allocator));

    uassert_disable();
    atassert_eqs(Error.argument, io.fopen(&file, "test_not_exist.txt", "r", NULL));
    atassert_eqs(Error.argument, io.fopen(&file, "test_not_exist.txt", NULL, allocator));
    atassert_eqs(Error.argument, io.fopen(&file, NULL, "r", allocator));
    atassert_eqs(Error.argument, io.fopen(NULL, "test.txt", "r", allocator));
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_read)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r", allocator));
    atassert(file._fh != NULL);
    atassert_eql(file._fsize, 0); // not calculated yet!
    atassert(file._fbuf == NULL);
    atassert(file._fbuf_size == 0);
    atassert(file._allocator == allocator);

    sview_c content;
    atassert_eqs(Error.ok, io.readall(&file, &content));
    atassert_eqi(50, io.size(&file));
    atassert_eqi(50 + 1 + 16, file._fbuf_size); // +1 null term
    atassert_eqi(0, file._fbuf[file._fsize]);
    atassert_eqs(
        "000000001\n"
        "000000002\n"
        "000000003\n"
        "000000004\n"
        "000000005\n",
        content.buf
    );


    io.close(&file);
    atassert(file._fh == NULL);
    atassert_eql(file._fsize, 0);
    atassert(file._fbuf == NULL);

    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_read_empty)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r", allocator));
    atassert(file._fh != NULL);
    atassert_eql(file._fsize, 0); // not calculated yet!
    atassert(file._fbuf == NULL);
    atassert(file._fbuf_size == 0);
    atassert(file._allocator == allocator);

    sview_c content;
    atassert_eqs(Error.eof, io.readall(&file, &content));
    atassert_eqi(0, io.size(&file));
    atassert(file._fbuf == NULL);
    atassert(file._fbuf_size == 0);
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);


    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_readall_stdin)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fattach(&file, stdin, allocator));
    atassert(file._fh == stdin);
    atassert_eql(file._fsize, 0); // not calculated yet!
    atassert(file._fbuf == NULL);
    atassert(file._fbuf_size == 0);
    atassert(file._allocator == allocator);
    atassert_eqi(0, io.size(&file));

    sview_c content;
    atassert_eqs(Error.io, io.readall(&file, &content));
    atassert_eql(file._fsize, 0); // not calculated yet!
    atassert(file._fbuf == NULL);
    atassert(file._fbuf_size == 0);

    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_read_line)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r", allocator));

    sview_c content;
    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000001");
    atassert_eqi(content.len, 9);

    atassert_eqi(file._fbuf_size, 4096-1);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000002");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000003");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000004");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000005");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.eof, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);

    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_read_line_empty_file)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r", allocator));

    sview_c content;
    atassert_eqs(Error.eof, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);

    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_read_line_binary_file_with_zero_char)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_zero_byte.txt", "r", allocator));

    sview_c content;
    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000001");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.integrity, io.readline(&file, &content));
    atassert_eqs(content.buf, NULL);
    atassert_eqi(content.len, 0);

    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_read_line_win_new_line)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_win_newline.txt", "r", allocator));

    sview_c content;
    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000001");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000002");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000003");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000004");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000005");
    atassert_eqi(content.len, 9);

    atassert_eqs(Error.eof, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);

    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_read_line_only_new_lines)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_only_newline.txt", "r", allocator));

    sview_c content;
    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);

    atassert_eqs(Error.eof, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);

    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_read_all_then_read_line)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r", allocator));
    atassert(file._fh != NULL);
    atassert_eql(file._fsize, 0); // not calculated yet!
    atassert(file._fbuf == NULL);
    atassert(file._fbuf_size == 0);
    atassert(file._allocator == allocator);

    sview_c content;
    atassert_eqs(Error.ok, io.readall(&file, &content));
    atassert_eqi(50, io.size(&file));
    atassert_eqi(file._fbuf_size, 67);

    atassert_eqs(Error.eof, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);


    atassert_eqs(Error.ok, io.seek(&file, 0, SEEK_SET));
    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqs(content.buf, "000000001");
    atassert_eqi(content.len, 9);

    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_read_long_line)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r", allocator));
    atassert(file._fh != NULL);
    atassert_eql(file._fsize, 0); // not calculated yet!
    atassert(file._fbuf == NULL);
    atassert(file._fbuf_size == 0);
    atassert(file._allocator == allocator);

    sview_c content;
    atassert_eqi(4096+4095+2, io.size(&file));

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqi(file._fbuf_size, 4096-1);
    atassert(sview.starts_with(content, sview.cstr("4095")));
    atassert_eqi(content.len, 4095);
    atassert_eqi(0, content.buf[content.len]); // null term

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqi(file._fbuf_size, 4096*2-1); // buffer resized
    atassert(sview.starts_with(content, sview.cstr("4096")));
    atassert_eqi(content.len, 4096);
    atassert_eqi(0, content.buf[content.len]); // null term


    atassert_eqs(Error.eof, io.readline(&file, &content));
    atassert_eqs(content.buf, "");
    atassert_eqi(content.len, 0);

    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

ATEST_F(test_readall_realloc)
{
    io_c file = { 0 };
    atassert_eqs(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r", allocator));
    atassert(file._fh != NULL);
    atassert_eql(file._fsize, 0); // not calculated yet!
    atassert(file._fbuf == NULL);
    atassert(file._fbuf_size == 0);
    atassert(file._allocator == allocator);

    sview_c content;
    atassert_eqi(4096+4095+2, io.size(&file));

    atassert_eqs(Error.ok, io.readline(&file, &content));
    atassert_eqi(file._fbuf_size, 4096-1);
    atassert(sview.starts_with(content, sview.cstr("4095")));
    atassert_eqi(content.len, 4095);
    atassert_eqi(0, content.buf[content.len]); // null term


    io.rewind(&file);

    atassert_eqs(Error.ok, io.readall(&file, &content));
    atassert_eqi(file._fbuf_size, 4095+4096+2+16+1);
    atassert(sview.starts_with(content, sview.cstr("4095")));
    atassert_eqi(sview.indexof(content, sview.cstr("4096"), 0, 0), 4096);
    atassert_eqi(content.len, 4095+4096+2);

    io.close(&file);
    return NULL; // Every ATEST_F() must return NULL to succeed!
}

/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    ATEST_PARSE_MAINARGS(argc, argv);
    ATEST_PRINT_HEAD();  // >>> all tests below
    
    ATEST_RUN(test_io);
    ATEST_RUN(test_read);
    ATEST_RUN(test_read_empty);
    ATEST_RUN(test_readall_stdin);
    ATEST_RUN(test_read_line);
    ATEST_RUN(test_read_line_empty_file);
    ATEST_RUN(test_read_line_binary_file_with_zero_char);
    ATEST_RUN(test_read_line_win_new_line);
    ATEST_RUN(test_read_line_only_new_lines);
    ATEST_RUN(test_read_all_then_read_line);
    ATEST_RUN(test_read_long_line);
    ATEST_RUN(test_readall_realloc);
    
    ATEST_PRINT_FOOTER();  // ^^^^^ all tests runs are above
    return ATEST_EXITCODE();
}
