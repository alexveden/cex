#define CEX_LOG_LVL 0
#define CEX_TEST
#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int
fuzz$case(const u8* data, usize size)
{
    if (size < 12 || size > 1024) { return -1; }

    fuzz$dnew(data, size);

    u32 raw_page = 0;
    fuzz$dget(&raw_page);
    u32 page_size = 1024 + (raw_page % (256 * 1024 - 1024 + 1));
    page_size = (page_size / 1024) * 1024;

    AllocatorArena_kw kw = { .page_size = page_size, .disable_scopes = true };
    IAllocator arena = AllocatorArena.create(&kw);
    if (arena == NULL) { return -1; }

    u8* last_p = NULL;
    u16 last_n = 0;

    u16 raw = 0;
    while (fuzz$dget(&raw)) {
        u8 op = (u8)(raw & 3);
        u16 n = (raw >> 2) + 1;

        switch (op) {
        case 0: { // alloc
            u8* p = mem$malloc(arena, n);
            if (p == NULL) { break; }
            uassert(!mem$asan_poison_check(p, n));
            for (u32 i = 0; i < n; i++) { p[i] = (u8)(n ^ i); }
            for (u32 i = 0; i < n; i++) { uassert(p[i] == (u8)(n ^ i)); }
            last_p = p;
            last_n = n;
            break;
        }
        case 1: { // realloc last
            if (last_p == NULL) { break; }
            u16 new_n = (u16)((usize)n * 2 + 1);
            u8* rp = mem$realloc(arena, last_p, new_n);
            if (rp == NULL) { break; }
            uassert(!mem$asan_poison_check(rp, new_n));
            for (u32 i = 0; i < last_n && i < new_n; i++) {
                uassert(rp[i] == (u8)(last_n ^ i));
            }
            for (u32 i = 0; i < new_n; i++) { rp[i] = (u8)(new_n ^ i); }
            for (u32 i = 0; i < new_n; i++) { uassert(rp[i] == (u8)(new_n ^ i)); }
            last_p = rp;
            last_n = new_n;
            break;
        }
        case 2: { // free last
            if (last_p == NULL) { break; }
            arena->free(arena, last_p);
            uassert(mem$asan_poison_check(last_p, last_n));
            last_p = NULL;
            last_n = 0;
            break;
        }
        case 3: { // alloc + verify survives mem$scope
            u8* p = mem$malloc(arena, n);
            if (p == NULL) { break; }
            for (u32 i = 0; i < n; i++) { p[i] = (u8)(n ^ i); }

            mem$scope(arena, _)
            {
                u8* sp = mem$malloc(_, n > 1 ? n / 2 : 1);
                if (sp) {
                    for (u32 i = 0; i < (n > 1 ? n / 2 : 1); i++) { sp[i] = 0xBB; }
                }
            }

            for (u32 i = 0; i < n; i++) { uassert(p[i] == (u8)(n ^ i)); }
            uassert(!mem$asan_poison_check(p, n));
            last_p = p;
            last_n = n;
            break;
        }
        }
    }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return 0;
}

static void
_write_corpus_file(char* path, u8* data, usize len)
{
    FILE* f;
    e$except_silent(err, io.fopen(&f, path, "wb")) { uassertf(false, "fopen: %s", path); }
    e$except_silent(err, io.fwrite(f, data, len)) { uassertf(false, "fwrite: %s", path); }
    io.fclose(&f);
}

fuzz$setup()
{
    io.printf("CORPUS: %s\n", fuzz$corpus_dir);
    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    struct
    {
        u8 data[48];
        usize len;
    } seeds[] = {
        { { 4, 0, 0, 0, 10, 0, 20, 0, 30, 0, 40, 0 }, 12 },
        { { 4, 0, 0, 0, 1, 0, 5, 0, 10, 0, 0, 0 }, 12 },
        { { 4, 0, 0, 0, 100, 0, 200, 0, 50, 0, 25, 0 }, 12 },
        { { 4, 0, 0, 0, 1, 0, 2, 0, 3, 0 }, 10 },
        { { 4, 0, 0, 0, 10, 0, 3, 0, 64, 0 }, 10 },
        { { 8, 0, 0, 0, 10, 0, 2, 0, 20, 0, 1, 0, 30, 0, 0, 0, 40, 0 }, 18 },
    };
    mem$scope(tmem$, _)
    {
        for (u32 i = 0; i < arr$len(seeds); i++) {
            char* fn = str.fmt(_, "%s/%03d", fuzz$corpus_dir, i);
            _write_corpus_file(fn, seeds[i].data, seeds[i].len);
        }
    }
}

fuzz$main();
