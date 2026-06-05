#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FUZZ_MAX_PATH 4096

fuzz$setup()
{
    io.printf("CORPUS: %s\n", fuzz$corpus_dir);
    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    uassert(os.path.exists(fuzz$corpus_dir));
    char* corp_seeds[] = {
        "a/b/c",         "/a/b/c",       ".",            "..",
        "./a",           "../a",          "a/./b",        "a/../b",
        "a//b",          "///a",          "/",            "",
        "/../a",         "/../../",       "a/../../b",    "/a/../b",
        "a/b/./../c",    "a///b///c",     "./././.",      "../../..",
        "/.././../",     "a/b/c/",        "/a/b/c/",      "\\a\\b\\c",
        "/a\\b\\c",      "a/..",          "a/b/../..",    "/a/b/../..",
        "a/./././b",     "/./././b",      "a/../../..",   "/a/../../..",
        "a/.hidden",     "/.hidden",      "a/b/../c/d/",  "a/b/../c/d/../e",
    };
    mem$scope(tmem$, _)
    {
        for (u32 i = 0; i < arr$len(corp_seeds); i++) {
            char* fn = str.fmt(_, "%s/%05d", fuzz$corpus_dir, i);
            if (io.file.save(fn, corp_seeds[i])) {
                uassertf(false, "Error writing file: %s", fn);
            }
        }
    }
}

int
fuzz$case(const u8* data, usize size)
{
    if (size > FUZZ_MAX_PATH) { return -1; }

    if (size == 0) {
        char* result = os.path.normalize(NULL, mem$);
        if (result) { mem$free(mem$, result); }
        return 0;
    }

    char* buf = mem$malloc(mem$, size + 1);
    if (buf == NULL) { return 0; }
    memcpy(buf, data, size);
    buf[size] = '\0';

    char* result = os.path.normalize(buf, mem$);
    mem$free(mem$, buf);
    if (result) { mem$free(mem$, result); }

    return 0;
}

fuzz$main();
