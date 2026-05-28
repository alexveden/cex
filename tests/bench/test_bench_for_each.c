#define CEX_FOREACH_MAX_COPY_SIZE 4096
#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"

#define ELEM_COUNT 200000

typedef struct { u64 data; u8 pad[8]; }    S16;
static_assert(sizeof(S16) == 16, "size mismatch");
typedef struct { u64 data; u8 pad[24]; }   S32;
static_assert(sizeof(S32) == 32, "size mismatch");
typedef struct { u64 data; u8 pad[56]; }   S64;
static_assert(sizeof(S64) == 64, "size mismatch");
typedef struct { u64 data; u8 pad[120]; }  S128;
static_assert(sizeof(S128) == 128, "size mismatch");
typedef struct { u64 data; u8 pad[248]; }  S256;
static_assert(sizeof(S256) == 256, "size mismatch");
typedef struct { u64 data; u8 pad[504]; }  S512;
static_assert(sizeof(S512) == 512, "size mismatch");
typedef struct { u64 data; u8 pad[1016]; } S1024;
static_assert(sizeof(S1024) == 1024, "size mismatch");

arr$(S16) g_a16;
arr$(S32) g_a32;
arr$(S64) g_a64;
arr$(S128) g_a128;
arr$(S256) g_a256;
arr$(S512) g_a512;
arr$(S1024) g_a1024;

test$setup_suite()
{
    g_a16 = arr$new(g_a16, mem$, .capacity = ELEM_COUNT);
    g_a32 = arr$new(g_a32, mem$, .capacity = ELEM_COUNT);
    g_a64 = arr$new(g_a64, mem$, .capacity = ELEM_COUNT);
    g_a128 = arr$new(g_a128, mem$, .capacity = ELEM_COUNT);
    g_a256 = arr$new(g_a256, mem$, .capacity = ELEM_COUNT);
    g_a512 = arr$new(g_a512, mem$, .capacity = ELEM_COUNT);
    g_a1024 = arr$new(g_a1024, mem$, .capacity = ELEM_COUNT);

    for (u32 i = 0; i < ELEM_COUNT; i++) {
        arr$push(g_a16, ((S16){ .data = i }));
        arr$push(g_a32, ((S32){ .data = i }));
        arr$push(g_a64, ((S64){ .data = i }));
        arr$push(g_a128, ((S128){ .data = i }));
        arr$push(g_a256, ((S256){ .data = i }));
        arr$push(g_a512, ((S512){ .data = i }));
        arr$push(g_a1024, ((S1024){ .data = i }));
    }
    return EOK;
}

test$teardown_suite()
{
    arr$free(g_a16);
    arr$free(g_a32);
    arr$free(g_a64);
    arr$free(g_a128);
    arr$free(g_a256);
    arr$free(g_a512);
    arr$free(g_a1024);
    return EOK;
}

test$bench(val_16)
{
    u64 sum = 0;
    for$each (it, g_a16) { sum += it.data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(ptr_16)
{
    u64 sum = 0;
    for$eachp (it, g_a16) { sum += it->data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(val_32)
{
    u64 sum = 0;
    for$each (it, g_a32) { sum += it.data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(ptr_32)
{
    u64 sum = 0;
    for$eachp (it, g_a32) { sum += it->data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(val_64)
{
    u64 sum = 0;
    for$each (it, g_a64) { sum += it.data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(ptr_64)
{
    u64 sum = 0;
    for$eachp (it, g_a64) { sum += it->data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(val_128)
{
    u64 sum = 0;
    for$each (it, g_a128) { sum += it.data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(ptr_128)
{
    u64 sum = 0;
    for$eachp (it, g_a128) { sum += it->data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(val_256)
{
    u64 sum = 0;
    for$each (it, g_a256) { sum += it.data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(ptr_256)
{
    u64 sum = 0;
    for$eachp (it, g_a256) { sum += it->data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(val_512)
{
    u64 sum = 0;
    for$each (it, g_a512) { sum += it.data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(ptr_512)
{
    u64 sum = 0;
    for$eachp (it, g_a512) { sum += it->data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(val_1024)
{
    u64 sum = 0;
    for$each (it, g_a1024) { sum += it.data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$bench(ptr_1024)
{
    u64 sum = 0;
    for$eachp (it, g_a1024) { sum += it->data; }
    volatile u64 _ = sum;
    (void)_;
    return EOK;
}

test$main();
