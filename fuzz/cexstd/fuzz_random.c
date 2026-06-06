#include "src/all.c"
#include <cexstd/random/Random.c>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

fuzz$setup()
{
    io.printf("CORPUS: %s\n", fuzz$corpus_dir);
    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    u64 seeds[] = { 0, 1, 42, 0xDEADBEEF, 0xFFFFFFFFFFFFFFFF };
    mem$scope(tmem$, _)
    {
        for (u32 i = 0; i < arr$len(seeds); i++) {
            char* fn = str.fmt(_, "%s/%03d", fuzz$corpus_dir, i);
            FILE* fh = NULL;
            e$except (err, io.fopen(&fh, fn, "wb")) {
                uassertf(false, "Error opening: %s", fn);
            }
            e$except (err, io.fwrite(fh, &seeds[i], sizeof(seeds[i]))) {
                io.fclose(&fh);
                uassertf(false, "Error writing: %s", fn);
            }
            io.fclose(&fh);
        }
    }
}

int
fuzz$case(const u8* data, usize size)
{
    if (size < sizeof(u64)) return -1;
    if (size > 64) return -1;

    u64 seed = 0;
    memcpy(&seed, data, sizeof(u64));

    Random_c rnd = { 0 };
    Random.seed(&rnd, seed);

    Random_c rnd2 = { 0 };
    Random.seed(&rnd2, seed);
    if (Random.next(&rnd) != Random.next(&rnd2)) return 1;

    Random_c rnd3 = { 0 };
    Random.seed(&rnd3, seed);

    for (int i = 0; i < 100; i++) {
        f32 f = Random.f32(&rnd);
        if (f < 0.0f || f > 1.0f) return 2;
    }

    for (int i = 0; i < 100; i++) {
        i32 v = Random.i32(&rnd, -1000, 1000);
        if (v < -1000 || v > 1000) return 3;
    }

    for (int i = 0; i < 100; i++) {
        usize v = Random.range(&rnd, 0, 100);
        if (v >= 100) return 4;
    }

    bool p1 = Random.prob(&rnd, 1.0f);
    if (!p1) return 5;
    bool p0 = Random.prob(&rnd, 0.0f);
    if (p0) return 6;

    u8 buf[64];
    Random.buf(&rnd3, buf, sizeof(buf));
    bool all_zero = true;
    for (usize i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 0) { all_zero = false; break; }
    }
    if (all_zero) return 7;

    return 0;
}

fuzz$main();
