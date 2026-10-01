#include "src/all.c"
#include "src/ds.h"
#include "src/test.h"
#include <stdio.h>

test$teardown_suite()
{
    if (os.fs.remove("tests/data/text_file_write.txt")) {}
    if (os.fs.remove("tests/data/text_file_fprintf.txt")) {}
    return EOK;
}

test$case(test_io)
{
    FILE* file = (void*)221;
    tassert_eq(Error.not_found, io.fopen(&file, "test_not_exist.txt", "r"));
    tassert(file == NULL);

    tassert_eq(Error.argument, io.fopen(&file, "test_not_exist.txt", NULL));
    tassert_eq(Error.argument, io.fopen(&file, NULL, "r"));
    tassert_eq(Error.argument, io.fopen(NULL, "test.txt", "r"));

    str_s s = { 0 };
    usize pos = 0;
    tassert_eq(Error.argument, io.fflush(NULL));
    tassert_eq(Error.argument, io.fseek(NULL, 0, SEEK_SET));
    tassert_eq(Error.argument, io.ftell(NULL, &pos));
    tassert_eq(Error.argument, io.fprintf(NULL, "x"));
    tassert_eq(Error.argument, io.fwrite(NULL, "x", 1));
    tassert_eq(Error.argument, io.file.writeln(NULL, "x"));
    tassert_eq(Error.argument, io.fread_all(NULL, &s, mem$));
    tassert_eq(Error.argument, io.fread_line(NULL, &s, mem$));
    return EOK;
}

test$case(test_io_null_args)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r"));
    tassert_eq(Error.argument, io.fread_all(file, NULL, mem$));
    tassert_eq(Error.argument, io.fread_line(file, NULL, mem$));
    tassert_eq(Error.argument, io.fread_all(file, &(str_s){ 0 }, NULL));
    io.fclose(&file);
    return EOK;
}

test$case(test_readall)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    str_s content;
    tassert_eq(Error.ok, io.fread_all(file, &content, mem$));

    auto stat = os.fs.stat("tests/data/text_file_50b.txt");
    tassert_eq(stat.is_valid, 1);
    tassert_eq(stat.size, 50);
    tassert_eq(50, io.file.size(file));
    tassert_eq(
        "000000001\n"
        "000000002\n"
        "000000003\n"
        "000000004\n"
        "000000005\n",
        content.buf
    );
    tassert_eq(content.len, 50);

    mem$free(mem$, content.buf);

    tassert(file != NULL);
    io.fclose(&file);
    tassert(file == NULL);
    return EOK;
}


test$case(test_read_all_empty)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r"));
    str_s content;
    tassert_eq(Error.ok, io.fread_all(file, &content, mem$));
    tassert_eq(0, io.file.size(file));
    tassert(content.buf != NULL);
    tassert(content.len == 0);
    tassert_eq(content.buf, "");

    mem$free(mem$, content.buf);

    io.fclose(&file);
    return EOK;
}

test$case(test_is_atty)
{
    // Not the case when running on CI
    if (io.isatty(stderr)) {
        tassert_eq(1, io.isatty(stderr));
        tassert_eq(1, io.isatty(stdin));
    }
    if (_cex_test__mainfn_state.out_stream) {
        // NOTE: stdout - in test runner is captured to file
        tassert_eq(0, io.isatty(stdout));
    }
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));
    tassert_eq(0, io.isatty(file));
    io.fclose(&file);

    return EOK;
}

test$case(test_read_all_stdin)
{
    if (io.isatty(stdin)) {
        tassert_eq(0, io.file.size(stdin));
        str_s content;
        tassert_eq(
            "io.fread_all() not allowed for pipe/socket/std[in/out/err]",
            io.fread_all(stdin, &content, mem$)
        );
    }
    return EOK;
}
test$case(test_file_size)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));
    tassert_eq(50, io.file.size(file));
    tassert_eq(0, io.file.size(NULL));
    tassert_eq(0, io.file.size(stdin));
    if (io.isatty(stderr)) { tassert_eq(0, io.file.size(stderr)); }
    if (io.isatty(stdout)) { tassert_eq(0, io.file.size(stdout)); }
    io.fclose(&file);

    return EOK;
}

test$case(test_read_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));
    tassert_eq(50, io.file.size(file));

    str_s content;
    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000001");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000002");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    // filesize doesn't wreck file internal cursor
    tassert_eq(50, io.file.size(file));

    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000003");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000004");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000005");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    tassert_eq(Error.eof, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, NULL);
    tassert_eq(content.len, 0);


    usize fsize = 0;
    tassert_eq(EOK, io.ftell(file, &fsize));
    tassert_eq(fsize, 50);

    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_stream)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));


    mem$scope(tmem$, _)
    {
        tassert_eq(io.file.readln(file, _), "000000001");
        tassert_eq(io.file.readln(file, _), "000000002");
        tassert_eq(io.file.readln(file, _), "000000003");
        tassert_eq(io.file.readln(file, _), "000000004");
        tassert_eq(io.file.readln(file, _), "000000005");
        tassert_eq(io.file.readln(file, _), NULL);
        tassert_eq(io.file.readln(file, _), NULL);
    }
    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_empty_file)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r"));

    str_s content;
    tassert_eq(Error.eof, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, NULL);
    tassert_eq(content.len, 0);

    tassert_eq(io.file.readln(file, mem$), NULL);

    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_binary_file_with_zero_char)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_zero_byte.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000001");
        tassert_eq(content.len, 9);

        tassert_eq(Error.integrity, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_fread_line_binary_file_with_zero_char)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_zero_byte.txt", "r"));

    mem$scope(tmem$, _)
    {
        tassert_eq("000000001", io.file.readln(file, _));
        tassert_eq(io.file.readln(file, _), NULL);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_win_new_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_win_newline.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000001");
        tassert_eq(content.len, 9);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000002");
        tassert_eq(content.len, 9);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000003");
        tassert_eq(content.len, 9);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000004");
        tassert_eq(content.len, 9);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000005");
        tassert_eq(content.len, 9);

        tassert_eq(Error.eof, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_fread_line_win_new_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_win_newline.txt", "r"));

    mem$scope(tmem$, _)
    {
        tassert_eq("000000001", io.file.readln(file, _));
        tassert_eq("000000002", io.file.readln(file, _));
        tassert_eq("000000003", io.file.readln(file, _));
        tassert_eq("000000004", io.file.readln(file, _));
        tassert_eq("000000005", io.file.readln(file, _));

        tassert_eq(io.file.readln(file, _), NULL);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_only_new_lines)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_only_newline.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "");
        tassert_eq(content.len, 0);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "");
        tassert_eq(content.len, 0);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "");
        tassert_eq(content.len, 0);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "");
        tassert_eq(content.len, 0);

        tassert_eq(Error.eof, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_all_then_read_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_all(file, &content, _));
        tassert_eq(50, io.file.size(file));

        tassert_eq(Error.eof, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);

        tassert_eq(Error.ok, io.fseek(file, 0, SEEK_SET));
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000001");
        tassert_eq(content.len, 9);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_long_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(4096 + 4095 + 2, io.file.size(file));

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert(str.slice.starts_with(content, str.sstr("4095")));
        tassert_eq(content.len, 4095);
        tassert_eq(0, content.buf[content.len]); // null term

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert(str.slice.starts_with(content, str.sstr("4096")));
        tassert_eq(content.len, 4096);
        tassert_eq(0, content.buf[content.len]); // null term


        tassert_eq(Error.eof, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_fread_long_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r"));
    tassert_eq(4096 + 4095 + 2, io.file.size(file));

    mem$scope(tmem$, _)
    {
        char* line = io.file.readln(file, _);
        tassert(line);
        tassert(str.starts_with(line, "4095"));
        tassert_eq(str.len(line), 4095);

        line = io.file.readln(file, _);
        tassert(line);
        tassert(str.starts_with(line, "4096"));
        tassert_eq(str.len(line), 4096);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_all_realloc)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r"));

    str_s content;
    tassert_eq(4096 + 4095 + 2, io.file.size(file));

    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert(str.slice.starts_with(content, str.sstr("4095")));
        tassert_eq(content.len, 4095);
        tassert_eq(0, content.buf[content.len]); // null term

        io.rewind(file);

        tassert_eq(Error.ok, io.fread_all(file, &content, _));
        tassert(str.slice.starts_with(content, str.sstr("4095")));
        tassert_eq(content.len, 4095 + 4096 + 2);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r"));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    tassert_eq(4, io.fread(file, buf, 4));
    tassert_eq(memcmp(buf, "4095", 4), 0);

    io.fclose(&file);
    return EOK;
}

test$case(test_read_error)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_non_existing.txt", "w"));
    tassert(!ferror(file));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    tassert_lt(io.fread(file, buf, 4), 0);

    io.fclose(&file);

    e$ret(os.fs.remove("tests/data/text_file_non_existing.txt"));
    return EOK;
}

test$case(test_read_empty)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r"));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    tassert_eq(0, io.fread(file, buf, arr$len(buf)));
    tassert_eq(memcmp(buf, "zzzzzzzz", 8), 0); // untouched!

    io.fclose(&file);
    return EOK;
}

test$case(test_read_loop)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    isize nread = 0;
    while((nread = io.fread(file, buf, 10))) {
        if (nread < 0) {
            tassert(false && "Unexpected file io error");
            break;
        }
        
        tassert_eq(nread, 10);
        tassert_eq(buf[0], '0');
        tassert_eq(buf[9], '\n');

        buf[10] = '\0';
        io.printf("%s", buf);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_not_all)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    tassert_eq(50, io.fread(file, buf, arr$len(buf)));

    // NOTE: read method does not null terminate!
    tassert_eq(buf[50], 'z');

    buf[50] = '\0'; // null terminate to compare string result below
    tassert_eq(
        "000000001\n"
        "000000002\n"
        "000000003\n"
        "000000004\n"
        "000000005\n",
        buf
    );

    tassert_eq(0, io.fread(file, buf, arr$len(buf)));

    io.fclose(&file);
    return EOK;
}

test$case(test_write_error)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));
    tassert(!ferror(file));

    char buf[] = "foobar";
    tassert_ne(EOK, io.fwrite(file, buf, arr$len(buf)));

    io.fclose(&file);
    return EOK;
}

test$case(test_fprintf_to_file)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_fprintf.txt", "w+"));

    char buf[] = { "1234" };
    str_s s1 = str.sbuf(buf, 4);
    tassert_eq(s1.len, 4);
    tassert_eq(s1.buf[3], '4');

    tassert_eq(EOK, io.fprintf(file, "io.fprintf: str_c: %S\n", s1));

    str_s content;
    io.rewind(file);

    tassert_eq(EOK, io.fread_all(file, &content, mem$));
    tassert_eq(1, str.slice.eq(content, str$s("io.fprintf: str_c: 1234\n")));

    mem$free(mem$, content.buf);
    io.fclose(&file);
    return EOK;
}

test$case(test_write)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_write.txt", "w+"));

    char buf[5] = { "1234" };
    tassert_eq(EOK, io.fwrite(file, buf, 4));

    str_s content;
    io.rewind(file);
    tassert_eq(EOK, io.fread_all(file, &content, mem$));
    tassert_eq(1, str.slice.eq(content, str$s("1234")));

    mem$free(mem$, content.buf);
    io.fclose(&file);
    return EOK;
}

test$case(test_fload_save)
{
    tassert_eq(Error.ok, io.file.save("tests/data/text_file_write.txt", "Hello from CEX!\n"));
    char* content = io.file.load("tests/data/text_file_write.txt", mem$);
    tassert(content);
    tassert_eq(content, "Hello from CEX!\n");
    mem$free(mem$, content);
    return EOK;
}

test$case(test_fload_not_found)
{
    tassert_ne(io.file.save("tests/", "Hello from CEX!\n"), EOK);
    return EOK;
}

test$case(test_write_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_write.txt", "w+"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(EOK, io.file.writeln(file, "hello"));
        tassert_eq(EOK, io.file.writeln(file, "world"));

        io.rewind(file);
        tassert_eq("hello", io.file.readln(file, _));
        tassert_er(EOK, io.fread_line(file, &content, _));
        tassert(str.slice.eq(content, str$s("world")));
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_fload)
{
    char* content = io.file.load("tests/data/text_file_line_4095.txt", mem$);
    tassert(str.starts_with(content, "409500000000"));
    mem$free(mem$, content);

    content = io.file.load("tests/data/text_file_empty.txt", mem$);
    tassert_eq("", content);
    mem$free(mem$, content);

    content = io.file.load("tests/data/asdjaldhashdajlkhuci.txt", mem$);
    tassert_eq(content, NULL);
    tassert_eq(ENOENT, errno);
    tassert_eq("No such file or directory", strerror(errno));

    content = io.file.load(NULL, mem$);
    tassert_eq(content, NULL);
    tassert_eq(EINVAL, errno);

    content = io.file.load("tests/data/text_file_empty.txt", mem$);
    tassert_eq("", content);
    mem$free(mem$, content);

    content = io.file.load("tests/data/text_file_zero_byte.txt", mem$);
    tassert_eq("000000001\n0", content);
    mem$free(mem$, content);

    if (os.platform.current() != OSPlatform__macos) {
        content = io.file.load("/dev/console", mem$);
        tassert_ne(0, errno);
        tassert_eq(content, NULL);
    }

    mem$free(mem$, content);

    return EOK;
}

test$case(test_fflush)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));
    tassert_eq(Error.ok, io.fflush(file));
    io.fclose(&file);
    return EOK;
}

test$case(test_fseek_invalid_whence)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));
    tassert_eq(Error.argument, io.fseek(file, 0, 12345));
    io.fclose(&file);
    return EOK;
}

test$case(test_fread_overflow_guard)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    char buf[8];
    uassert_disable();
    tassert_eq(-1, io.fread(file, buf, PTRDIFF_MAX));
    uassert_enable();

    io.fclose(&file);
    return EOK;
}

test$case(test_readln_null_file)
{
    tassert(io.file.readln(NULL, mem$) == NULL);
    tassert_eq(EINVAL, errno);
    return EOK;
}

test$case(test_read_all_eof)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r"));

    str_s content;
    tassert_eq(Error.ok, io.fread_all(file, &content, mem$));
    mem$free(mem$, content.buf);

    tassert_eq(Error.ok, io.fread_all(file, &content, mem$));
    tassert(content.buf == NULL);
    tassert_eq(content.len, 0);

    io.fclose(&file);
    return EOK;
}

test$case(test_fread_all_oom)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    test$alloc_set_oom_probability(1.0);
    str_s content;
    tassert_eq(Error.memory, io.fread_all(file, &content, test$alloc));
    tassert(content.buf == NULL);
    tassert_eq(content.len, 0);
    test$alloc_set_oom_probability(0.0);

    io.fclose(&file);
    return EOK;
}

test$case(test_fread_line_oom)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    test$alloc_set_oom_probability(1.0);
    str_s content;
    tassert_eq(Error.memory, io.fread_line(file, &content, test$alloc));
    tassert(content.buf == NULL);
    tassert_eq(content.len, 0);
    test$alloc_set_oom_probability(0.0);

    io.fclose(&file);
    return EOK;
}

test$main();
