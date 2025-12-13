#define NDEBUG /* NO ASSERTS */
#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct fuzz_match_s
{
    char pattern[100];
    char null_term;
    char text[300];
    char null_term2;
} fuzz_match_s;

Exception
match_make(char* out_file, char* text, char* pattern)
{
    fuzz_match_s f = { 0 };
    e$ret(str.copy(f.text, text, sizeof(f.text)));
    e$ret(str.copy(f.pattern, pattern, sizeof(f.pattern)));

    FILE* fh;
    e$ret(io.fopen(&fh, out_file, "wb"));
    e$ret(io.fwrite(fh, &f, sizeof(f)));
    io.fclose(&fh);

    return EOK;
}

fuzz$setup()
{
    if (os.fs.mkdir(fuzz$corpus_dir)) {}
    struct
    {
        char* text;
        char* pattern;
    } match_tuple[] = {
        { "test", "*" },
        { "", "*" },
        { ".txt", "*.txt" },
        { "test.txt", "" },
        { "test.txt", "*txt" },
        { "test.txt", "*.tx" },
        { "test.txt", "*.txt" },
        { "test.txt", "*xt" },
        { "txt", "???" },
        { "txta", "???a" },
        { "txt", "???a" },
        { "txt", "??" },
        { "t", "?" },
        { "t", "*" },
        { "test1.txt", "test?.txt" },
        { "test.txt", "*.*" },
        { "test1.txt", "t*t?.txt" },
        { "test.txt", "*?txt" },
        { "test.txt", "*.???" },
        { "test.txt", "*?t?*" },
        { "a*b", "a\\*b" },
        { "backup.txt", "[!a]*.txt" },
        { "image.png", "image.[jp][pn]g" },
        { "a", "[]" },
        { "a", "[!]" },
        { "a", "[!" },
        { "a", "[" },
        { "a", "[ab]" },
        { "b", "[ab]" },
        { "b", "[abc]" },
        { "c", "[abc]" },
        { "a", "[a-c]" },
        { "b", "[a-c]" },
        { "c", "[a-c]" },
        { "A", "[a-cA-C]" },
        { "0", "[a-cA-C0-9]" },
        { "D", "[a-cA-C0-9]" },
        { "d", "[a-cA-C1-9]" },
        { "0", "[a-cA-C1-9]" },
        { "_", "[_a-cd]" },
        { "a", "[_a-cd]" },
        { "b", "[_a-cd]" },
        { "c", "[_a-cd]" },
        { "d", "[_a-cd]" },
        { "_", "[a-c_A-C1-9]" },
        { "-", "[a-z-]" },
        { "*", "[a-c*]" },
        { "?", "[?]" },
        { "-", "[?-]" },
        { "*", "[*-]" },
        { ")", "[)]" },
        { "(", "[(]" },
        { "e$*", "*[*]" },
        { "d", "[a-c*]" },
        { "*", "[*a-z]" },
        { "*", "[a-z\\*]" },
        { "]", "[\\]]" },
        { "[", "[\\[]" },
        { "\\", "[\\\\]" },
        { "#abc=", "[a-c+]=" },
        { "#abc=", "[a-c+]*=*" },
        { "abc  =", "[a-c +]*=*" },
        { "abc=", "[a-c +]=*" },
        { "abc", "[a-c+]" },
        { "zabc", "[a-c+]" },
        { "abcfed", "[a-c+]*" },
        { "abc@", "[a-c+]@" },
        { "abdef", "[a-c+][d-f+]" },
        { "abcf", "[a-c+]" },
        { "abc+", "[+a-c+]" },
        { "abcf", "[a-c+]?" },
        { "", "[a-c+]" },
        { "abcd", "[!d-f+]?" },
        { "abcf", "[!d-f+]?" },
        { "abcff", "[!d-f+]?" },
        { "1234567890abcdefABCDEF", "[0-9a-fA-F+]" },
        { "f5fca082882b848dd28c470c4d1f111c995cb7bd", "[0-9a-fA-F+]" },
        { "abc", "(\\" },
        { "abc", "(abc)" },
        { "def", "(abc|def)" },
        { "abcdef", "abc(abc|def)" },
        { "abXdef", "ab?(abc|def)" },
        { "abcdef", "(abc|def)(abc|def)" },
        { "ldkjdef", "*(abc|def)" },
        { "adef", "(cdef)" },
        { "bdef", "(cdef)" },
        { "f", "(cdef)" },
        { "f", "(cdef)" },
        { "fas", "(fa)" },
        { "fas", "(fa)" },
        { "fa@", "(fa)@" },
        { "faB", "(fa)B" },
        { "faZ", "(fa)?" },
        { "faZ", "(fa)*" },
        { "faZ", "(fa)[A-Z]" },
        { "fas", "(fa|)" },
        { "fa", "(fa|)" },
        { "fa", "(|fa)" },
        { "fa", "(|)" },
        { "fa", "()" },
        { "fa", "fa()" },
        { "fa)", "(fa\\))" },
        { "fa|", "(fa\\|)" },
        { "fa\\", "(fa\\\\)" },
        { "abc+", "[a-c\\+]" },
        { "a+", "a[a-c\\+]" },
        { "clean", "(clean)" },
        { "run", "(run|clean)" },
        { "build", "(run|build|clean)" },
        { "create", "(run|build|create|clean)" },
        { "build", "(run|build|create|clean)" },
        { "foo", "(run|build|create|clean|foo)" },
        { "clean", "(run|build|create|clean)" },
        { "clean", "(run|build|cleany|clean)" },
    };
    mem$scope(tmem$, _)
    {
        for (u32 i = 0; i < arr$len(match_tuple); i++) {
            char* fn = str.fmt(_, "%s/%05d", fuzz$corpus_dir, i);
            e$except (err, match_make(fn, match_tuple[i].text, match_tuple[i].pattern)) {
                uassertf(false, "Error writing file: %s", fn);
            }
        }
    }
}

int
fuzz$case(const u8* data, usize size)
{
    if (size != sizeof(fuzz_match_s)) { return -1; }

    fuzz_match_s f = { 0 };
    memcpy(&f, data, sizeof(fuzz_match_s));
    f.null_term = '\0';
    f.null_term2 = '\0';
    
    char* text = NULL;
    usize text_len = str.len(f.text);

    if (text_len >= 0) {
        text = malloc(text_len);
        uassert(text && "mem error");
        // Important using non null terminated string, it should respect str_s.len
        memcpy(text, f.text, text_len);
    }

    str.slice.match((str_s){.buf = text, .len = text_len}, f.pattern);

    free(text);

    return 0;
}

fuzz$main();
