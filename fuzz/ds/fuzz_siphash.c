#define CEX_LOG_LVL 0
#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FUZZ_SEEDS 7
#define MAX_CAP 512

fuzz$setup()
{
    io.printf("CORPUS: %s\n", fuzz$corpus_dir);
    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    u64 seeds[FUZZ_SEEDS] = {
        0,
        1,
        0xffffffffffffffffULL,
        0x736f6d6570736575ULL,
        0xdeadbeefcafebabeULL,
        0x0123456789abcdefULL,
        0xaaaaaaaaaaaaaaaaULL,
    };

    char* inputs[] = {
        "",
        "\x00",
        "\xff",
        "AAAA",
        "AAAAAAAA",
        "AAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAA",
        "AAAAAAAAAAAAAAAAA",
        "\x01\x02\x03\x04\x05\x06\x07",
        "The quick brown fox jumps over the lazy dog",
        "\x00\x00\x00\x00\x00\x00\x00\x00",
        "\xff\xff\xff\xff\xff\xff\xff\xff",
        "abc\0def",
        "short",
        "x",
        "\x01",
        "\x01\x02",
        "\x01\x02\x03",
    };

    mem$scope(tmem$, _)
    {
        u64 idx = 0;
        for (u64 s = 0; s < FUZZ_SEEDS; s++) {
            for (u64 i = 0; i < arr$len(inputs); i++) {
                usize input_len = str.len(inputs[i]);
                usize total = sizeof(u64) + input_len;
                u8* buf = mem$calloc(_, total, 1);
                memcpy(buf, &seeds[s], sizeof(u64));
                memcpy(buf + sizeof(u64), inputs[i], input_len);
                char* fn = str.fmt(_, "%s/%04zu", fuzz$corpus_dir, idx++);
                FILE* fh = NULL;
                e$except(err, io.fopen(&fh, fn, "wb"))
                {
                    uassertf(false, "Error opening file: %s", fn);
                }
                e$except(err, io.fwrite(fh, buf, total))
                {
                    io.fclose(&fh);
                    uassertf(false, "Error writing file: %s", fn);
                }
                io.fclose(&fh);
            }
        }
    }
}

int
fuzz$case(const u8* data, usize size)
{
    if (size < 9) { return -1; }
    if (size > 4096) { return -1; }

    u64 seed = 0;
    memcpy(&seed, data, sizeof(seed));

    const void* input = data + 8;
    usize input_len = size - 8;
    u8 mode = ((const u8*)input)[0];
    usize remaining = input_len - 1;

    if (mode & 1) {
        usize str_cap = (remaining > MAX_CAP) ? MAX_CAP : remaining;
        u64 h = _cexds__hash_string((const char*)input + 1, str_cap, seed);
        (void)h;
    } else {
        if (remaining > 0) {
            u64 h = _cexds__siphash_bytes(input + 1, remaining, seed);
            (void)h;
        }
    }

    return 0;
}

fuzz$main();
