#include "src/all.c"

int
fuzz$case(const u8* data, usize size)
{
    if (size < sizeof(u64)) return -1;
    if (size > 64) return -1;

    u64 seed = 0;
    memcpy(&seed, data, sizeof(u64));

    os.random.seed(seed);
    u32 a1 = os.random.next();
    u32 a2 = os.random.next();

    os.random.seed(seed);
    if (os.random.next() != a1) return 1;
    if (os.random.next() != a2) return 2;

    for (int i = 0; i < 100; i++) {
        f32 f = os.random.f32();
        if (f < 0.0f || f >= 1.0f) return 3;
    }

    for (int i = 0; i < 100; i++) {
        i32 v = os.random.i32(-1000, 1000);
        if (v < -1000 || v >= 1000) return 4;
    }

    for (int i = 0; i < 100; i++) {
        usize v = os.random.range(0, 100);
        if (v >= 100) return 5;
    }

    u8 buf[64];
    os.random.seed(seed);
    if (os.random.buf(buf, sizeof(buf)) != buf) return 6;
    bool all_zero = true;
    for (usize i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 0) { all_zero = false; break; }
    }
    if (all_zero) return 7;

    os.random.seed(seed);
    u8 buf2[sizeof(buf)];
    os.random.buf(buf2, sizeof(buf2));
    for (usize i = 0; i < sizeof(buf); i++) {
        if (buf[i] != buf2[i]) return 8;
    }

    if (os.random.buf(NULL, 100) != NULL) return 9;

    u8 marker = 0xAB;
    if (os.random.buf(&marker, 0) != &marker) return 10;
    if (marker != 0xAB) return 11;

    return 0;
}

fuzz$main();
