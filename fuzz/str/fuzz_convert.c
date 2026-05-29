#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

fuzz$setup()
{
    io.printf("CORPUS: %s\n", fuzz$corpus_dir);
    if (os.fs.mkdir(fuzz$corpus_dir)) {}

    uassert(os.path.exists(fuzz$corpus_dir));
    char* corp_seeds[] = { "123", "255",      "123.3", "23e+1",     "nan",
                           "inf", "infinity", "-inf",  "-infinity", "283e-1" };
    mem$scope(tmem$, _)
    {
        for (u32 i = 0; i < arr$len(corp_seeds); i++) {
            char* fn = str.fmt(_, "%s/%03d", fuzz$corpus_dir, i);
            if (io.file.save(fn, corp_seeds[i])) { uassertf(false, "Error writing file: %s", fn); }
        }
    }
}

int
fuzz$case(const u8* data, usize size)
{
    if (size == 0) { return -1; }
    if (size > 100) { return -1; }
    str_s s = { .buf = (char*)data, .len = size };

    u32 r_u8 = 0;
    i32 r_i8 = 0;
    u16 r_u16 = 0;
    i16 r_i16 = 0;
    u32 r_u32 = 0;
    i32 r_i32 = 0;
    u64 r_u64 = 0;
    i64 r_i64 = 0;
    f64 r_f64 = 0;
    f32 r_f32 = 0;

    if (str$convert(s, &r_u8)) {}
    if (str$convert(s, &r_i8)) {}
    if (str$convert(s, &r_u16)) {}
    if (str$convert(s, &r_i16)) {}
    if (str$convert(s, &r_i32)) {}
    if (str$convert(s, &r_u32)) {}
    if (str$convert(s, &r_i64)) {}
    if (str$convert(s, &r_u64)) {}
    if (str$convert(s, &r_f64)) {}
    if (str$convert(s, &r_f32)) {}

    return 0;
}

fuzz$main();
