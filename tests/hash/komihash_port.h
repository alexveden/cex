/**
 * @file komihash.h
 *
 * @version 5.28
 *
 * @brief The header file for the "komihash" 64-bit hash function,
 * the "komirand" 64-bit PRNG, and the streamed "komihash" implementation.
 *
 * The source code is written in ISO C99, with full C++ compliance enabled
 * conditionally and automatically when compiled with a C++ compiler.
 *
 * This function is named the way it is named is to honor the Komi Republic
 * (located in Russia), native to the author.
 *
 * Description is available at https://github.com/avaneev/komihash
 *
 * Email: aleksey.vaneev@gmail.com or info@voxengo.com
 *
 * LICENSE:
 *
 * Copyright (c) 2021-2025 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef KOMIHASH_INCLUDED
#define KOMIHASH_INCLUDED

#define KOMIHASH_VER_STR "5.28" ///< KOMIHASH source code version string.

#include <stdint.h>
#include <string.h>

#define KOMIHASH_U64_C(x) (uint64_t)x
#define KOMIHASH_NOEX

#define KOMIHASH_IVAL1 KOMIHASH_U64_C(0x243F6A8885A308D3)
#define KOMIHASH_IVAL2 KOMIHASH_U64_C(0x13198A2E03707344)
#define KOMIHASH_IVAL3 KOMIHASH_U64_C(0xA4093822299F31D0)
#define KOMIHASH_IVAL4 KOMIHASH_U64_C(0x082EFA98EC4E6C89)
#define KOMIHASH_IVAL5 KOMIHASH_U64_C(0x452821E638D01377)
#define KOMIHASH_IVAL6 KOMIHASH_U64_C(0xBE5466CF34E90C6C)
#define KOMIHASH_IVAL7 KOMIHASH_U64_C(0xC0AC29B7C97C50DD)
#define KOMIHASH_IVAL8 KOMIHASH_U64_C(0x3F84D5B5B5470917)
#define KOMIHASH_VAL01 KOMIHASH_U64_C(0x5555555555555555)
#define KOMIHASH_VAL10 KOMIHASH_U64_C(0xAAAAAAAAAAAAAAAA)


#if !defined(KOMIHASH_LITTLE_ENDIAN)
#    if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) ||                  \
        (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) ||                              \
        defined(__LITTLE_ENDIAN__) || defined(_LITTLE_ENDIAN) || defined(_WIN32) ||                \
        defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) ||               \
        defined(_M_AMD64) || defined(_X86_) || defined(__x86_64) || defined(__x86_64__) ||         \
        defined(__amd64) || defined(__amd64__) || defined(_M_ARM)

#        define KOMIHASH_LITTLE_ENDIAN 1

#    elif (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) ||                   \
        (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) || defined(__BIG_ENDIAN__) ||      \
        defined(_BIG_ENDIAN) || defined(__SYSC_ZARCH__) || defined(__zarch__) ||                   \
        defined(__s390x__) || defined(__sparc) || defined(__sparc__)

#        define KOMIHASH_LITTLE_ENDIAN 0
#        define KOMIHASH_COND_EC(vl, vb) (vb)

#    else

#        warning KOMIHASH: cannot determine endianness, assuming little-endian.

#        define KOMIHASH_LITTLE_ENDIAN 1

#    endif // defined( __cplusplus )
#endif     // !defined( KOMIHASH_LITTLE_ENDIAN )


#if KOMIHASH_LITTLE_ENDIAN

#    define KOMIHASH_EC32(v) (v)
#    define KOMIHASH_EC64(v) (v)

#else // KOMIHASH_LITTLE_ENDIAN

#    define KOMIHASH_EC32(v) KOMIHASH_COND_EC(v, __builtin_bswap32(v))
#    define KOMIHASH_EC64(v) KOMIHASH_COND_EC(v, __builtin_bswap64(v))

#endif // KOMIHASH_LITTLE_ENDIAN


#    define KOMIHASH_LIKELY(x) (__builtin_expect(x, 1))
#    define KOMIHASH_UNLIKELY(x) (__builtin_expect(x, 0))
#    define KOMIHASH_PREFETCH(a) __builtin_prefetch(a, 0, 3)
#    define KOMIHASH_INLINE static __attribute__((unused)) inline
#    define KOMIHASH_INLINE_F KOMIHASH_INLINE __attribute__((always_inline))


KOMIHASH_INLINE_F uint64_t
kh_lu32ec(const uint8_t* const p) KOMIHASH_NOEX
{
#if defined(KOMIHASH_EC32)

    uint32_t v;
    memcpy(&v, p, 4);

    return (KOMIHASH_EC32(v));

#else // defined( KOMIHASH_EC32 )

    return ((uint32_t)(p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24));

#endif // defined( KOMIHASH_EC32 )
}

KOMIHASH_INLINE_F uint64_t
kh_lu64ec(const uint8_t* const p) KOMIHASH_NOEX
{
#if defined(KOMIHASH_EC64)

    uint64_t v;
    memcpy(&v, p, 8);

    return (KOMIHASH_EC64(v));

#else // defined( KOMIHASH_EC64 )

    return (kh_lu32ec(p) | kh_lu32ec(p + 4) << 32);

#endif // defined( KOMIHASH_EC64 )
}



#if defined(__SIZEOF_INT128__) || (defined(KOMIHASH_ICC_GCC) && defined(__x86_64__))

#    define KOMIHASH_M128_IMPL                                                                     \
        __uint128_t r = u;                                                                         \
        r *= v;                                                                                    \
        const uint64_t rh = (uint64_t)(r >> 64);                                                   \
        *rl = (uint64_t)r;                                                                         \
        *rha += rh;

#elif (defined(__IBMC__) || defined(__IBMCPP__)) && defined(__LP64__)

#    define KOMIHASH_M128_IMPL                                                                     \
        const uint64_t rh = __mulhdu(u, v);                                                        \
        *rl = u * v;                                                                               \
        *rha += rh;

#else // defined( __IBMC__ )

#        define KOMIHASH_EMULU(u, v) ((uint64_t)(u) * (v))

#endif // defined( __IBMC__ )


#if defined(KOMIHASH_M128_IMPL)
KOMIHASH_INLINE_F
#else  // defined( KOMIHASH_M128_IMPL )
KOMIHASH_INLINE
#endif // defined( KOMIHASH_M128_IMPL )

void
kh_m128(const uint64_t u, const uint64_t v, uint64_t* const rl, uint64_t* const rha) KOMIHASH_NOEX
{
#if defined(KOMIHASH_M128_IMPL)

    KOMIHASH_M128_IMPL

#    undef KOMIHASH_M128_IMPL

#else // defined( KOMIHASH_M128_IMPL )

    // _umul128() code for 32-bit systems, adapted from Hacker's Delight,
    // Henry S. Warren, Jr.

    *rl = u * v;

    const uint32_t u0 = (uint32_t)u;
    const uint32_t v0 = (uint32_t)v;
    const uint64_t w0 = KOMIHASH_EMULU(u0, v0);
    const uint32_t u1 = (uint32_t)(u >> 32);
    const uint32_t v1 = (uint32_t)(v >> 32);
    const uint64_t t = KOMIHASH_EMULU(u1, v0) + (uint32_t)(w0 >> 32);
    const uint64_t w1 = KOMIHASH_EMULU(u0, v1) + (uint32_t)t;

    *rha += KOMIHASH_EMULU(u1, v1) + (uint32_t)(w1 >> 32) + (uint32_t)(t >> 32);

#    undef KOMIHASH_EMULU

#endif // defined( KOMIHASH_M128_IMPL )
}

#define KOMIHASH_HASHROUND()                                                                       \
    kh_m128(Seed1, Seed5, &Seed1, &Seed5);                                                         \
    Seed1 ^= Seed5


#define KOMIHASH_HASH16(m)                                                                         \
    kh_m128(kh_lu64ec(m) ^ Seed1, kh_lu64ec(m + 8) ^ Seed5, &Seed1, &Seed5);                       \
    Seed1 ^= Seed5

#define KOMIHASH_HASHFIN()                                                                         \
    kh_m128(r1h, r2h, &Seed1, &Seed5);                                                             \
    Seed1 ^= Seed5;                                                                                \
    KOMIHASH_HASHROUND();                                                                          \
    return (Seed1)

#define KOMIHASH_HASHLOOP64()                                                                      \
    do {                                                                                           \
        kh_m128(kh_lu64ec(Msg) ^ Seed1, kh_lu64ec(Msg + 32) ^ Seed5, &Seed1, &Seed5);              \
                                                                                                   \
        kh_m128(kh_lu64ec(Msg + 8) ^ Seed2, kh_lu64ec(Msg + 40) ^ Seed6, &Seed2, &Seed6);          \
                                                                                                   \
        kh_m128(kh_lu64ec(Msg + 16) ^ Seed3, kh_lu64ec(Msg + 48) ^ Seed7, &Seed3, &Seed7);         \
                                                                                                   \
        kh_m128(kh_lu64ec(Msg + 24) ^ Seed4, kh_lu64ec(Msg + 56) ^ Seed8, &Seed4, &Seed8);         \
                                                                                                   \
        Msg += 64;                                                                                 \
        MsgLen -= 64;                                                                              \
                                                                                                   \
        KOMIHASH_PREFETCH(Msg);                                                                    \
                                                                                                   \
        Seed4 ^= Seed7;                                                                            \
        Seed1 ^= Seed8;                                                                            \
        Seed3 ^= Seed6;                                                                            \
        Seed2 ^= Seed5;                                                                            \
                                                                                                   \
    } while KOMIHASH_LIKELY(MsgLen > 63)

KOMIHASH_INLINE_F uint64_t
komihash_epi(const uint8_t* Msg, size_t MsgLen, uint64_t Seed1, uint64_t Seed5) KOMIHASH_NOEX
{
    uint64_t r1h, r2h;

    if (MsgLen > 31) {
        KOMIHASH_HASH16(Msg);
        KOMIHASH_HASH16(Msg + 16);

        MsgLen -= 32;
        Msg += 32;
    }

    if (MsgLen > 15) {
        KOMIHASH_HASH16(Msg);

        MsgLen -= 16;
        Msg += 16;
    }

    int ml8 = (int)(MsgLen * 8);

    if (MsgLen < 8) {
        ml8 ^= 56;
        r1h = kh_lu64ec(Msg + MsgLen - 8) >> 8 | (uint64_t)1 << 56;
        r2h = Seed5;
        r1h = (r1h >> ml8) ^ Seed1;
    } else {
        r2h = kh_lu64ec(Msg + MsgLen - 8) >> 8 | (uint64_t)1 << 56;
        ml8 ^= 120;
        r1h = kh_lu64ec(Msg) ^ Seed1;
        r2h = (r2h >> ml8) ^ Seed5;
    }

    KOMIHASH_HASHFIN();
}

KOMIHASH_INLINE uint64_t
komihash(const void* const Msg0, size_t MsgLen, const uint64_t UseSeed) KOMIHASH_NOEX
{
    const uint8_t* Msg = (const uint8_t*)Msg0;

    uint64_t Seed1 = KOMIHASH_IVAL1 ^ (UseSeed & KOMIHASH_VAL01);
    uint64_t Seed5 = KOMIHASH_IVAL5 ^ (UseSeed & KOMIHASH_VAL10);
    uint64_t r1h, r2h;

    KOMIHASH_PREFETCH(Msg);

    KOMIHASH_HASHROUND(); // Required for Perlin Noise.

    if KOMIHASH_LIKELY (MsgLen < 16) {
        r1h = Seed1;
        r2h = Seed5;

        if (MsgLen > 7) {
            // The following XOR instructions are equivalent to mixing a
            // message with a cryptographic one-time-pad (bitwise modulo 2
            // addition). Message's statistics and distribution are thus
            // unimportant.

            r1h ^= kh_lu64ec(Msg);

            if (MsgLen < 12) {
                int ml8 = (int)(MsgLen * 8);
                const uint64_t m = (uint64_t)(Msg[MsgLen - 3] | Msg[MsgLen - 1] << 16 | 1 << 24 |
                                              Msg[MsgLen - 2] << 8);

                ml8 ^= 88;
                r2h ^= m >> ml8;
            } else {
                const int mhs = (int)(128 - MsgLen * 8);
                const uint64_t mh = (kh_lu32ec(Msg + MsgLen - 4) | (uint64_t)1 << 32) >> mhs;

                const uint64_t ml = kh_lu32ec(Msg + 8);

                r2h ^= mh << 32 | ml;
            }
        } else if KOMIHASH_LIKELY (MsgLen != 0) {
            const int ml8 = (int)(MsgLen * 8);

            if (MsgLen < 4) {
                r1h ^= (uint64_t)1 << ml8;
                r1h ^= (uint64_t)Msg[0];

                if (MsgLen != 1) {
                    r1h ^= (uint64_t)Msg[1] << 8;

                    if (MsgLen != 2) { r1h ^= (uint64_t)Msg[2] << 16; }
                }
            } else {
                const int mhs = 64 - ml8;
                const uint64_t mh = (kh_lu32ec(Msg + MsgLen - 4) | (uint64_t)1 << 32) >> mhs;

                const uint64_t ml = kh_lu32ec(Msg);

                r1h ^= mh << 32 | ml;
            }
        }
    } else {
        if KOMIHASH_UNLIKELY (MsgLen > 31) { goto _long; }

        KOMIHASH_HASH16(Msg);

        int ml8 = (int)(MsgLen * 8);

        if (MsgLen < 24) {
            ml8 ^= 184;
            r1h = kh_lu64ec(Msg + MsgLen - 8) >> 8 | (uint64_t)1 << 56;
            r2h = Seed5;
            r1h = (r1h >> ml8) ^ Seed1;

            KOMIHASH_HASHFIN();
        } else {
            r2h = kh_lu64ec(Msg + MsgLen - 8) >> 8 | (uint64_t)1 << 56;
            ml8 ^= 248;
            r1h = kh_lu64ec(Msg + 16) ^ Seed1;
            r2h = (r2h >> ml8) ^ Seed5;
        }
    }

    KOMIHASH_HASHFIN();

_long:
    if KOMIHASH_LIKELY (MsgLen > 63) {
        uint64_t Seed2 = KOMIHASH_IVAL2 ^ Seed1;
        uint64_t Seed3 = KOMIHASH_IVAL3 ^ Seed1;
        uint64_t Seed4 = KOMIHASH_IVAL4 ^ Seed1;
        uint64_t Seed6 = KOMIHASH_IVAL6 ^ Seed5;
        uint64_t Seed7 = KOMIHASH_IVAL7 ^ Seed5;
        uint64_t Seed8 = KOMIHASH_IVAL8 ^ Seed5;

        KOMIHASH_HASHLOOP64();

        Seed5 ^= Seed6 ^ Seed7 ^ Seed8;
        Seed1 ^= Seed2 ^ Seed3 ^ Seed4;
    }

    return (komihash_epi(Msg, MsgLen, Seed1, Seed5));
}


KOMIHASH_INLINE uint64_t
komihash_str(const void* const Msg0, size_t MsgLen, const uint64_t UseSeed) KOMIHASH_NOEX
{
    const uint8_t* Msg = (const uint8_t*)Msg0;

    uint64_t Seed1 = KOMIHASH_IVAL1 ^ (UseSeed & KOMIHASH_VAL01);
    uint64_t Seed5 = KOMIHASH_IVAL5 ^ (UseSeed & KOMIHASH_VAL10);
    uint64_t r1h, r2h;

    KOMIHASH_PREFETCH(Msg);

    KOMIHASH_HASHROUND(); // Required for Perlin Noise.

    if KOMIHASH_LIKELY (MsgLen < 16) {
        r1h = Seed1;
        r2h = Seed5;

        if (MsgLen > 7) {
            // The following XOR instructions are equivalent to mixing a
            // message with a cryptographic one-time-pad (bitwise modulo 2
            // addition). Message's statistics and distribution are thus
            // unimportant.

            r1h ^= kh_lu64ec(Msg);

            if (MsgLen < 12) {
                int ml8 = (int)(MsgLen * 8);
                const uint64_t m = (uint64_t)(Msg[MsgLen - 3] | Msg[MsgLen - 1] << 16 | 1 << 24 |
                                              Msg[MsgLen - 2] << 8);

                ml8 ^= 88;
                r2h ^= m >> ml8;
            } else {
                const int mhs = (int)(128 - MsgLen * 8);
                const uint64_t mh = (kh_lu32ec(Msg + MsgLen - 4) | (uint64_t)1 << 32) >> mhs;

                const uint64_t ml = kh_lu32ec(Msg + 8);

                r2h ^= mh << 32 | ml;
            }
        } else if KOMIHASH_LIKELY (MsgLen != 0) {
            const int ml8 = (int)(MsgLen * 8);

            if (MsgLen < 4) {
                r1h ^= (uint64_t)1 << ml8;
                r1h ^= (uint64_t)Msg[0];

                if (MsgLen != 1) {
                    r1h ^= (uint64_t)Msg[1] << 8;

                    if (MsgLen != 2) { r1h ^= (uint64_t)Msg[2] << 16; }
                }
            } else {
                const int mhs = 64 - ml8;
                const uint64_t mh = (kh_lu32ec(Msg + MsgLen - 4) | (uint64_t)1 << 32) >> mhs;

                const uint64_t ml = kh_lu32ec(Msg);

                r1h ^= mh << 32 | ml;
            }
        }
    } else {
        if KOMIHASH_UNLIKELY (MsgLen > 31) { goto _long; }

        KOMIHASH_HASH16(Msg);

        int ml8 = (int)(MsgLen * 8);

        if (MsgLen < 24) {
            ml8 ^= 184;
            r1h = kh_lu64ec(Msg + MsgLen - 8) >> 8 | (uint64_t)1 << 56;
            r2h = Seed5;
            r1h = (r1h >> ml8) ^ Seed1;

            KOMIHASH_HASHFIN();
        } else {
            r2h = kh_lu64ec(Msg + MsgLen - 8) >> 8 | (uint64_t)1 << 56;
            ml8 ^= 248;
            r1h = kh_lu64ec(Msg + 16) ^ Seed1;
            r2h = (r2h >> ml8) ^ Seed5;
        }
    }

    KOMIHASH_HASHFIN();

_long:
    if KOMIHASH_LIKELY (MsgLen > 63) {
        uint64_t Seed2 = KOMIHASH_IVAL2 ^ Seed1;
        uint64_t Seed3 = KOMIHASH_IVAL3 ^ Seed1;
        uint64_t Seed4 = KOMIHASH_IVAL4 ^ Seed1;
        uint64_t Seed6 = KOMIHASH_IVAL6 ^ Seed5;
        uint64_t Seed7 = KOMIHASH_IVAL7 ^ Seed5;
        uint64_t Seed8 = KOMIHASH_IVAL8 ^ Seed5;

        KOMIHASH_HASHLOOP64();

        Seed5 ^= Seed6 ^ Seed7 ^ Seed8;
        Seed1 ^= Seed2 ^ Seed3 ^ Seed4;
    }

    return (komihash_epi(Msg, MsgLen, Seed1, Seed5));
}

KOMIHASH_INLINE_F uint64_t
komirand(uint64_t* const Seed1, uint64_t* const Seed2) KOMIHASH_NOEX
{
    uint64_t s1 = *Seed1;
    uint64_t s2 = *Seed2;

    kh_m128(s1, s2, &s1, &s2);
    s2 += KOMIHASH_VAL10;
    s1 ^= s2;

    *Seed2 = s2;
    *Seed1 = s1;

    return (s1);
}

#undef KOMIHASH_NS_CUSTOM
#undef KOMIHASH_U64_C
#undef KOMIHASH_NOEX
#undef KOMIHASH_IVAL1
#undef KOMIHASH_IVAL2
#undef KOMIHASH_IVAL3
#undef KOMIHASH_IVAL4
#undef KOMIHASH_IVAL5
#undef KOMIHASH_IVAL6
#undef KOMIHASH_IVAL7
#undef KOMIHASH_IVAL8
#undef KOMIHASH_VAL01
#undef KOMIHASH_VAL10
#undef KOMIHASH_COND_EC
#undef KOMIHASH_ICC_GCC
#undef KOMIHASH_BMI2
#undef KOMIHASH_EC32
#undef KOMIHASH_LIKELY
#undef KOMIHASH_UNLIKELY
#undef KOMIHASH_PREFETCH
#undef KOMIHASH_INLINE
#undef KOMIHASH_INLINE_F
#undef KOMIHASH_HASHROUND
#undef KOMIHASH_HASH16
#undef KOMIHASH_HASHFIN
#undef KOMIHASH_HASHLOOP64

#endif // KOMIHASH_INCLUDED
