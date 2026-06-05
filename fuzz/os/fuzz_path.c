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
        // normalize edge cases
        "a/b/c",         "/a/b/c",       ".",            "..",
        "./a",           "../a",          "a/./b",        "a/../b",
        "a//b",          "///a",          "/",            "",
        "/../a",         "/../../",       "a/../../b",    "/a/../b",
        "a/b/./../c",    "a///b///c",     "./././.",      "../../..",
        "/.././../",     "a/b/c/",        "/a/b/c/",      "\\a\\b\\c",
        "/a\\b\\c",      "a/..",          "a/b/../..",    "/a/b/../..",
        "a/./././b",     "/./././b",      "a/../../..",   "/a/../../..",
        "a/.hidden",     "/.hidden",      "a/b/../c/d/",  "a/b/../c/d/../e",
        // unicode edge cases
        "привет/мир",    "ファイル/ドキュメント",
        "ファイル/./ドキュメント",
        "ファイル/../ドキュメント",
        "test／file",    "a/‥/b",
        "テスト//ドキュメント",
        "/日本語/a/../b",
        // absolute-specific edge cases
        "/",             "/a/b/c",        "/a/../b",      "/../x",
        "/a/../../b",    "/.//",          "///",          "//",
        "foo/bar",       "./foo",         "../foo",       ".",
        "..",            "foo/..",        "foo/../..",    "foo/./bar",
        // garbled / boundary
        "\0",            "\n",            "\t",           "\x01\x02\x03",
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

    // fuzz os.path.normalize()
    {
        if (size == 0) {
            char* result = os.path.normalize(NULL, mem$);
            if (result) { mem$free(mem$, result); }
        } else {
            char* buf = mem$malloc(mem$, size + 1);
            if (buf == NULL) { return 0; }
            memcpy(buf, data, size);
            buf[size] = '\0';
            char* result = os.path.normalize(buf, mem$);
            mem$free(mem$, buf);
            if (result) { mem$free(mem$, result); }
        }
    }

    // fuzz os.path.absolute()
    {
        if (size == 0) {
            char* result = os.path.absolute(NULL, mem$);
            if (result) { mem$free(mem$, result); }
        } else {
            char* buf = mem$malloc(mem$, size + 1);
            if (buf == NULL) { return 0; }
            memcpy(buf, data, size);
            buf[size] = '\0';
            char* result = os.path.absolute(buf, mem$);
            mem$free(mem$, buf);
            if (result) { mem$free(mem$, result); }
        }
    }

    return 0;
}

fuzz$main();
