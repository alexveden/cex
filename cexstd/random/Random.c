#include "Random.h"
/*
This code is based on https://github.com/mattiasgustavsson/libs/blob/main/rnd.h

#### PCG - Permuted Congruential Generator

PCG is a family of simple fast space-efficient statistically good algorithms for random number
generation. Unlike many general-purpose RNGs, they are also hard to predict.

More information can be found here:

http://www.pcg-random.org/

------------------------------------------------------------------------------
contributors:
        Jonatan Hedborg (unsigned int to normalized float conversion)

------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.
Based on public domain implementation - original licenses can be found next to
the relevant implementation sections of this file.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2016 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/

// Convert a randomized RND_U32 value to a float value x in the range 0.0f <= x < 1.0f. Contributed
// by Jonatan Hedborg
static inline f32
rnd_internal_f32_normalized_from_u32(u32 value)
{
    u32 exponent = 127;
    u32 mantissa = value >> 9;
    u32 result = (exponent << 23) | mantissa;
    f32 fresult = 0.0f;
    memcpy(&fresult, &result, sizeof(u32));
    return fresult - 1.0f;
}


static inline u32
rnd_internal_murmur3_avalanche32(u32 h)
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}


static inline u64
rnd_internal_murmur3_avalanche64(u64 h)
{
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;
    return h;
}

/**
 * @brief Next random u32 number
 *
 * @param self 
 * @return 
 */
u32
Random_next(Random_c* self)
{
    uassert(self != NULL);
    uassert((self->state[0] != 0 || self->state[1] != 0) && "self not seed'ed");

    u64 oldstate = self->state[0];
    self->state[0] = oldstate * 0x5851f42d4c957f2dULL + self->state[1];
    u32 xorshifted = (u32)(((oldstate >> 18ULL) ^ oldstate) >> 27ULL);
    u32 rot = (u32)(oldstate >> 59ULL);
    self->n_ticks++;
    return (xorshifted >> rot) | (xorshifted << ((-(i32)rot) & 31));
}

/**
 * @brief Resets seed of random generator (must be called before first use)
 *
 * @param self 
 * @param seed 
 */
void
Random_seed(Random_c* self, u64 seed)
{
    uassert(self != NULL);

    u64 value = (seed << 1ULL) | 1ULL;
    value = rnd_internal_murmur3_avalanche64(value);
    self->state[0] = 0U;
    self->state[1] = (value << 1ULL) | 1ULL;
    Random_next(self);
    self->state[0] += rnd_internal_murmur3_avalanche64(value);
    Random_next(self);

    uassert(self->state[0] != 0 || self->state[1] != 0);
    self->n_ticks = 0;
}


/**
 * @brief Random floating point number in [0;1.0] range
 *
 * @param self 
 * @return 
 */
f32
Random_f32(Random_c* self)
{
    uassert(self != NULL);

    f32 result = rnd_internal_f32_normalized_from_u32(Random_next(self));
    uassert(result >= 0.0f && result <= 1.0f);

    return result;
}

/**
 * @brief Random i32 number in [min; max] range
 *
 * @param self 
 * @param min  minimum for range
 * @param max maximum for range (included!)
 * @return 
 */
i32
Random_i32(Random_c* self, i32 min, i32 max)
{
    uassert(self != NULL);
    uassert(max > min);

    i32 const range = (max - min) + 1;
    i32 const value = (i32)(Random_f32(self) * range);
    return min + value;
}

/**
 * @brief Random probability simulation (rolling a dice)
 *
 * @param self 
 * @param prob_threshold target probability of dice roll, between [0.0f;1.0f]
 * @return true dire roll if succeeded
 */
bool
Random_prob(Random_c* self, f32 prob_threshold)
{
    uassert(self != NULL);
    uassert(prob_threshold >= 0.0f && prob_threshold <= 1.0f);

    f32 rnd = Random_f32(self);
    return rnd <= prob_threshold;
}


/**
 * @brief Returning random range in [min; max). Useful for arrays.
 *
 * @param self 
 * @param min minimum start range (included)
 * @param max maximum end range (excluded)
 * @return 
 */
usize
Random_range(Random_c* self, usize min, usize max)
{
    uassert(self != NULL);
    uassert(max > min);

    usize const range = (max - min);
    usize const value = (usize)(Random_f32(self) * range);
    return min + value;
}

/**
 * @brief Fill buffer with randomized data
 *
 * @param self 
 * @param buf any buffer of memory
 * @param buf_len buffer capacity
 */
void
Random_buf(Random_c* self, void* buf, usize buf_len)
{
    uassert(self != NULL);
    uassert(buf != NULL);

    usize reminder = buf_len % 4;
    usize buf_aligned_len = buf_len - reminder;
    static_assert(sizeof(u32) == 4, "u32 size mismatch");

    // Fill buf, with 4 byte at a step
    for (usize i = 0; i < buf_aligned_len; i += 4) {
        u32 r = Random_next(self);
        memcpy((char*)buf + i, &r, sizeof(u32));
    }
    if (reminder > 0) {
        u32 r = Random_next(self);

        for (usize i = 0; i < reminder; i++) {
            ((u8*)buf)[buf_aligned_len + i] = ((u8*)&r)[i];
        }
    }
}

const struct __cex_namespace__Random Random = {
    // Autogenerated by CEX
    // clang-format off

    .buf = Random_buf,
    .f32 = Random_f32,
    .i32 = Random_i32,
    .next = Random_next,
    .prob = Random_prob,
    .range = Random_range,
    .seed = Random_seed,

    // clang-format on
};
