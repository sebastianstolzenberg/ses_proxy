/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
 * Copyright 2018      Sebastian Stolzenberg <https://github.com/sebastianstolzenberg>
 * Copyright 2018      BenDroid    <ben@graef.in>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CRYPTONIGHT_X86_H__
#define __CRYPTONIGHT_X86_H__


#ifdef __GNUC__
#   include <x86intrin.h>
#else
#   include <intrin.h>
#   define __restrict__ __restrict
#endif


#include "CryptoNight.h"
#include "soft_aes.h"


extern "C"
{
#include "c_keccak.h"
#include "c_groestl.h"
#include "c_blake256.h"
#include "c_jh.h"
#include "c_skein.h"
}

static inline void do_blake_hash(const uint8_t *input, size_t len, uint8_t *output) {
    blake256_hash(output, input, len);
}


static inline void do_groestl_hash(const uint8_t *input, size_t len, uint8_t *output) {
    groestl(input, len * 8, output);
}


static inline void do_jh_hash(const uint8_t *input, size_t len, uint8_t *output) {
    jh_hash(32 * 8, input, 8 * len, output);
}


static inline void do_skein_hash(const uint8_t *input, size_t len, uint8_t *output) {
    xmr_skein(input, output);
}


void (* const extra_hashes[4])(const uint8_t *, size_t, uint8_t *) = {do_blake_hash, do_groestl_hash, do_jh_hash, do_skein_hash};

#if defined(__x86_64__) || defined(_M_AMD64)
#   define EXTRACT64(X) _mm_cvtsi128_si64(X)

#   ifdef __GNUC__

static inline uint64_t __umul128(uint64_t a, uint64_t b, uint64_t* hi)
{
    unsigned __int128 r = (unsigned __int128) a * (unsigned __int128) b;
    *hi = r >> 64;
    return (uint64_t) r;
}

#   else
#define __umul128 _umul128
#   endif
#elif defined(__i386__) || defined(_M_IX86)
#   define HI32(X) \
    _mm_srli_si128((X), 4)


#   define EXTRACT64(X) \
    ((uint64_t)(uint32_t)_mm_cvtsi128_si32(X) | \
    ((uint64_t)(uint32_t)_mm_cvtsi128_si32(HI32(X)) << 32))

static inline uint64_t __umul128(uint64_t multiplier, uint64_t multiplicand, uint64_t *product_hi) {
    // multiplier   = ab = a * 2^32 + b
    // multiplicand = cd = c * 2^32 + d
    // ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
    uint64_t a = multiplier >> 32;
    uint64_t b = multiplier & 0xFFFFFFFF;
    uint64_t c = multiplicand >> 32;
    uint64_t d = multiplicand & 0xFFFFFFFF;

    //uint64_t ac = a * c;
    uint64_t ad = a * d;
    //uint64_t bc = b * c;
    uint64_t bd = b * d;

    uint64_t adbc = ad + (b * c);
    uint64_t adbc_carry = adbc < ad ? 1 : 0;

    // multiplier * multiplicand = product_hi * 2^64 + product_lo
    uint64_t product_lo = bd + (adbc << 32);
    uint64_t product_lo_carry = product_lo < bd ? 1 : 0;
    *product_hi = (a * c) + (adbc >> 32) + (adbc_carry << 32) + product_lo_carry;

    return product_lo;
}
#endif


// This will shift and xor tmp1 into itself as 4 32-bit vals such as
// sl_xor(a1 a2 a3 a4) = a1 (a2^a1) (a3^a2^a1) (a4^a3^a2^a1)
static inline __m128i sl_xor(__m128i tmp1)
{
    __m128i tmp4;
    tmp4 = _mm_slli_si128(tmp1, 0x04);
    tmp1 = _mm_xor_si128(tmp1, tmp4);
    tmp4 = _mm_slli_si128(tmp4, 0x04);
    tmp1 = _mm_xor_si128(tmp1, tmp4);
    tmp4 = _mm_slli_si128(tmp4, 0x04);
    tmp1 = _mm_xor_si128(tmp1, tmp4);
    return tmp1;
}


template<uint8_t rcon>
static inline void aes_genkey_sub(__m128i* xout0, __m128i* xout2)
{
    __m128i xout1 = _mm_aeskeygenassist_si128(*xout2, rcon);
    xout1 = _mm_shuffle_epi32(xout1, 0xFF); // see PSHUFD, set all elems to 4th elem
    *xout0 = sl_xor(*xout0);
    *xout0 = _mm_xor_si128(*xout0, xout1);
    xout1 = _mm_aeskeygenassist_si128(*xout0, 0x00);
    xout1 = _mm_shuffle_epi32(xout1, 0xAA); // see PSHUFD, set all elems to 3rd elem
    *xout2 = sl_xor(*xout2);
    *xout2 = _mm_xor_si128(*xout2, xout1);
}


template<uint8_t rcon>
static inline void soft_aes_genkey_sub(__m128i* xout0, __m128i* xout2)
{
    __m128i xout1 = soft_aeskeygenassist<rcon>(*xout2);
    xout1 = _mm_shuffle_epi32(xout1, 0xFF); // see PSHUFD, set all elems to 4th elem
    *xout0 = sl_xor(*xout0);
    *xout0 = _mm_xor_si128(*xout0, xout1);
    xout1 = soft_aeskeygenassist<0x00>(*xout0);
    xout1 = _mm_shuffle_epi32(xout1, 0xAA); // see PSHUFD, set all elems to 3rd elem
    *xout2 = sl_xor(*xout2);
    *xout2 = _mm_xor_si128(*xout2, xout1);
}


static inline void
aes_genkey(const __m128i* memory, __m128i* k0, __m128i* k1, __m128i* k2, __m128i* k3, __m128i* k4, __m128i* k5,
           __m128i* k6, __m128i* k7, __m128i* k8, __m128i* k9)
{
    __m128i xout0 = _mm_load_si128(memory);
    __m128i xout2 = _mm_load_si128(memory + 1);
    *k0 = xout0;
    *k1 = xout2;

    soft_aes_genkey_sub<0x01>(&xout0, &xout2);
    *k2 = xout0;
    *k3 = xout2;

    soft_aes_genkey_sub<0x02>(&xout0, &xout2);
    *k4 = xout0;
    *k5 = xout2;

    soft_aes_genkey_sub<0x04>(&xout0, &xout2);
    *k6 = xout0;
    *k7 = xout2;

    soft_aes_genkey_sub<0x08>(&xout0, &xout2);
    *k8 = xout0;
    *k9 = xout2;
}


static inline void
aes_round(__m128i key, __m128i* x0, __m128i* x1, __m128i* x2, __m128i* x3, __m128i* x4, __m128i* x5, __m128i* x6,
          __m128i* x7)
{
    *x0 = soft_aesenc((uint32_t*)x0, key);
    *x1 = soft_aesenc((uint32_t*)x1, key);
    *x2 = soft_aesenc((uint32_t*)x2, key);
    *x3 = soft_aesenc((uint32_t*)x3, key);
    *x4 = soft_aesenc((uint32_t*)x4, key);
    *x5 = soft_aesenc((uint32_t*)x5, key);
    *x6 = soft_aesenc((uint32_t*)x6, key);
    *x7 = soft_aesenc((uint32_t*)x7, key);
}

inline void mix_and_propagate(__m128i& x0, __m128i& x1, __m128i& x2, __m128i& x3, __m128i& x4, __m128i& x5, __m128i& x6, __m128i& x7)
{
    __m128i tmp0 = x0;
    x0 = _mm_xor_si128(x0, x1);
    x1 = _mm_xor_si128(x1, x2);
    x2 = _mm_xor_si128(x2, x3);
    x3 = _mm_xor_si128(x3, x4);
    x4 = _mm_xor_si128(x4, x5);
    x5 = _mm_xor_si128(x5, x6);
    x6 = _mm_xor_si128(x6, x7);
    x7 = _mm_xor_si128(x7, tmp0);
}

template<size_t MEM>
static inline void cn_explode_scratchpad(const __m128i* input, __m128i* output)
{
    __m128i xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7;
    __m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aes_genkey(input, &k0, &k1, &k2, &k3, &k4, &k5, &k6, &k7, &k8, &k9);

    xin0 = _mm_load_si128(input + 4);
    xin1 = _mm_load_si128(input + 5);
    xin2 = _mm_load_si128(input + 6);
    xin3 = _mm_load_si128(input + 7);
    xin4 = _mm_load_si128(input + 8);
    xin5 = _mm_load_si128(input + 9);
    xin6 = _mm_load_si128(input + 10);
    xin7 = _mm_load_si128(input + 11);

    for (size_t i = 0; i < MEM / sizeof(__m128i); i += 8) {
        aes_round(k0, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k1, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k2, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k3, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k4, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k5, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k6, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k7, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k8, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k9, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);

        _mm_store_si128(output + i + 0, xin0);
        _mm_store_si128(output + i + 1, xin1);
        _mm_store_si128(output + i + 2, xin2);
        _mm_store_si128(output + i + 3, xin3);
        _mm_store_si128(output + i + 4, xin4);
        _mm_store_si128(output + i + 5, xin5);
        _mm_store_si128(output + i + 6, xin6);
        _mm_store_si128(output + i + 7, xin7);
    }
}

template<size_t MEM>
static inline void cn_explode_scratchpad_heavy(const __m128i* input, __m128i* output)
{
    __m128i xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7;
    __m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aes_genkey(input, &k0, &k1, &k2, &k3, &k4, &k5, &k6, &k7, &k8, &k9);

    xin0 = _mm_load_si128(input + 4);
    xin1 = _mm_load_si128(input + 5);
    xin2 = _mm_load_si128(input + 6);
    xin3 = _mm_load_si128(input + 7);
    xin4 = _mm_load_si128(input + 8);
    xin5 = _mm_load_si128(input + 9);
    xin6 = _mm_load_si128(input + 10);
    xin7 = _mm_load_si128(input + 11);

    for (size_t i = 0; i < 16; i++) {
        aes_round(k0, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k1, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k2, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k3, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k4, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k5, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k6, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k7, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k8, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k9, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);

        mix_and_propagate(xin0, xin1, xin2, xin3, xin4, xin5, xin6, xin7);
    }

    for (size_t i = 0; i < MEM / sizeof(__m128i); i += 8) {
        aes_round(k0, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k1, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k2, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k3, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k4, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k5, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k6, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k7, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k8, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);
        aes_round(k9, &xin0, &xin1, &xin2, &xin3, &xin4, &xin5, &xin6, &xin7);

        _mm_store_si128(output + i + 0, xin0);
        _mm_store_si128(output + i + 1, xin1);
        _mm_store_si128(output + i + 2, xin2);
        _mm_store_si128(output + i + 3, xin3);
        _mm_store_si128(output + i + 4, xin4);
        _mm_store_si128(output + i + 5, xin5);
        _mm_store_si128(output + i + 6, xin6);
        _mm_store_si128(output + i + 7, xin7);
    }
}


template<size_t MEM>
static inline void cn_implode_scratchpad(const __m128i* input, __m128i* output)
{
    __m128i xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7;
    __m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aes_genkey(output + 2, &k0, &k1, &k2, &k3, &k4, &k5, &k6, &k7, &k8, &k9);

    xout0 = _mm_load_si128(output + 4);
    xout1 = _mm_load_si128(output + 5);
    xout2 = _mm_load_si128(output + 6);
    xout3 = _mm_load_si128(output + 7);
    xout4 = _mm_load_si128(output + 8);
    xout5 = _mm_load_si128(output + 9);
    xout6 = _mm_load_si128(output + 10);
    xout7 = _mm_load_si128(output + 11);

    for (size_t i = 0; i < MEM / sizeof(__m128i); i += 8) {
        xout0 = _mm_xor_si128(_mm_load_si128(input + i + 0), xout0);
        xout1 = _mm_xor_si128(_mm_load_si128(input + i + 1), xout1);
        xout2 = _mm_xor_si128(_mm_load_si128(input + i + 2), xout2);
        xout3 = _mm_xor_si128(_mm_load_si128(input + i + 3), xout3);
        xout4 = _mm_xor_si128(_mm_load_si128(input + i + 4), xout4);
        xout5 = _mm_xor_si128(_mm_load_si128(input + i + 5), xout5);
        xout6 = _mm_xor_si128(_mm_load_si128(input + i + 6), xout6);
        xout7 = _mm_xor_si128(_mm_load_si128(input + i + 7), xout7);

        aes_round(k0, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k1, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k2, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k3, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k4, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k5, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k6, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k7, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k8, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k9, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
    }

    _mm_store_si128(output + 4, xout0);
    _mm_store_si128(output + 5, xout1);
    _mm_store_si128(output + 6, xout2);
    _mm_store_si128(output + 7, xout3);
    _mm_store_si128(output + 8, xout4);
    _mm_store_si128(output + 9, xout5);
    _mm_store_si128(output + 10, xout6);
    _mm_store_si128(output + 11, xout7);
}

template<size_t MEM>
static inline void cn_implode_scratchpad_heavy(const __m128i* input, __m128i* output)
{
    __m128i xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7;
    __m128i k0, k1, k2, k3, k4, k5, k6, k7, k8, k9;

    aes_genkey(output + 2, &k0, &k1, &k2, &k3, &k4, &k5, &k6, &k7, &k8, &k9);

    xout0 = _mm_load_si128(output + 4);
    xout1 = _mm_load_si128(output + 5);
    xout2 = _mm_load_si128(output + 6);
    xout3 = _mm_load_si128(output + 7);
    xout4 = _mm_load_si128(output + 8);
    xout5 = _mm_load_si128(output + 9);
    xout6 = _mm_load_si128(output + 10);
    xout7 = _mm_load_si128(output + 11);

    for (size_t i = 0; i < MEM / sizeof(__m128i); i += 8) {
        xout0 = _mm_xor_si128(_mm_load_si128(input + i + 0), xout0);
        xout1 = _mm_xor_si128(_mm_load_si128(input + i + 1), xout1);
        xout2 = _mm_xor_si128(_mm_load_si128(input + i + 2), xout2);
        xout3 = _mm_xor_si128(_mm_load_si128(input + i + 3), xout3);
        xout4 = _mm_xor_si128(_mm_load_si128(input + i + 4), xout4);
        xout5 = _mm_xor_si128(_mm_load_si128(input + i + 5), xout5);
        xout6 = _mm_xor_si128(_mm_load_si128(input + i + 6), xout6);
        xout7 = _mm_xor_si128(_mm_load_si128(input + i + 7), xout7);

        aes_round(k0, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k1, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k2, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k3, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k4, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k5, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k6, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k7, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k8, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k9, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);

        mix_and_propagate(xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
    }

    for (size_t i = 0; i < MEM / sizeof(__m128i); i += 8) {
        xout0 = _mm_xor_si128(_mm_load_si128(input + i + 0), xout0);
        xout1 = _mm_xor_si128(_mm_load_si128(input + i + 1), xout1);
        xout2 = _mm_xor_si128(_mm_load_si128(input + i + 2), xout2);
        xout3 = _mm_xor_si128(_mm_load_si128(input + i + 3), xout3);
        xout4 = _mm_xor_si128(_mm_load_si128(input + i + 4), xout4);
        xout5 = _mm_xor_si128(_mm_load_si128(input + i + 5), xout5);
        xout6 = _mm_xor_si128(_mm_load_si128(input + i + 6), xout6);
        xout7 = _mm_xor_si128(_mm_load_si128(input + i + 7), xout7);

        aes_round(k0, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k1, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k2, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k3, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k4, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k5, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k6, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k7, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k8, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k9, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);

        mix_and_propagate(xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
    }

    for (size_t i = 0; i < 16; i++) {
        aes_round(k0, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k1, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k2, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k3, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k4, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k5, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k6, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k7, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k8, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);
        aes_round(k9, &xout0, &xout1, &xout2, &xout3, &xout4, &xout5, &xout6, &xout7);

        mix_and_propagate(xout0, xout1, xout2, xout3, xout4, xout5, xout6, xout7);
    }

    _mm_store_si128(output + 4, xout0);
    _mm_store_si128(output + 5, xout1);
    _mm_store_si128(output + 6, xout2);
    _mm_store_si128(output + 7, xout3);
    _mm_store_si128(output + 8, xout4);
    _mm_store_si128(output + 9, xout5);
    _mm_store_si128(output + 10, xout6);
    _mm_store_si128(output + 11, xout7);
}

template<size_t ITERATIONS, size_t INDEX_SHIFT, size_t MEM, size_t MASK>
class CryptoNightHash
{
public:
    inline static void hash(const uint8_t* __restrict__ input,
                            size_t size,
                            uint8_t* __restrict__ output,
                            ScratchPad& __restrict__ scratchPad)
    {
        const uint8_t* l;
        uint64_t* h;
        uint64_t al;
        uint64_t ah;
        __m128i bx;
        uint64_t idx;

        keccak(static_cast<const uint8_t*>(input), (int) size, scratchPad.state, 200);

        l = scratchPad.memory;
        h = reinterpret_cast<uint64_t*>(scratchPad.state);

        cn_explode_scratchpad<MEM>((__m128i*) h, (__m128i*) l);

        al = h[0] ^ h[4];
        ah = h[1] ^ h[5];
        bx = _mm_set_epi64x(h[3] ^ h[7], h[2] ^ h[6]);
        idx = h[0] ^ h[4];

        for (size_t i = 0; i < ITERATIONS; i++) {
            __m128i cx;

            cx = soft_aesenc((uint32_t*)&l[idx & MASK], _mm_set_epi64x(ah, al));

            _mm_store_si128((__m128i*) &l[idx & MASK], _mm_xor_si128(bx, cx));
            idx = EXTRACT64(cx);
            bx = cx;

            uint64_t hi, lo, cl, ch;
            cl = ((uint64_t*) &l[idx & MASK])[0];
            ch = ((uint64_t*) &l[idx & MASK])[1];
            lo = __umul128(idx, cl, &hi);

            al += hi;
            ah += lo;

            ((uint64_t*) &l[idx & MASK])[0] = al;
            ((uint64_t*) &l[idx & MASK])[1] = ah;

            ah ^= ch;
            al ^= cl;
            idx = al;
        }

        cn_implode_scratchpad<MEM>((__m128i*) l, (__m128i*) h);
        keccakf(h, 24);
        extra_hashes[scratchPad.state[0] & 3](scratchPad.state, 200, output);
    }

    inline static void hashPowV2(const uint8_t* __restrict__ input,
                                 size_t size,
                                 uint8_t* __restrict__ output,
                                 ScratchPad& __restrict__ scratchPad)
    {
        const uint8_t* l;
        uint64_t* h;
        uint64_t al;
        uint64_t ah;
        __m128i bx;
        uint64_t idx;

        keccak(static_cast<const uint8_t*>(input), (int) size, scratchPad.state, 200);

        uint64_t tweak1_2 = (*reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(input) + 35) ^
                             *(reinterpret_cast<const uint64_t*>(scratchPad.state) + 24));
        l = scratchPad.memory;
        h = reinterpret_cast<uint64_t*>(scratchPad.state);

        cn_explode_scratchpad<MEM>((__m128i*) h, (__m128i*) l);

        al = h[0] ^ h[4];
        ah = h[1] ^ h[5];
        bx = _mm_set_epi64x(h[3] ^ h[7], h[2] ^ h[6]);
        idx = h[0] ^ h[4];

        for (size_t i = 0; i < ITERATIONS; i++) {
            __m128i cx;

            cx = soft_aesenc((uint32_t*)&l[idx & MASK], _mm_set_epi64x(ah, al));

            _mm_store_si128((__m128i*) &l[idx & MASK], _mm_xor_si128(bx, cx));
            const uint8_t tmp = reinterpret_cast<const uint8_t*>(&l[idx & MASK])[11];
            static const uint32_t table = 0x75310;
            const uint8_t index = (((tmp >> INDEX_SHIFT) & 6) | (tmp & 1)) << 1;
            ((uint8_t*)(&l[idx & MASK]))[11] = tmp ^ ((table >> index) & 0x30);

            idx = EXTRACT64(cx);
            bx = cx;

            uint64_t hi, lo, cl, ch;
            cl = ((uint64_t*) &l[idx & MASK])[0];
            ch = ((uint64_t*) &l[idx & MASK])[1];
            lo = __umul128(idx, cl, &hi);

            al += hi;
            ah += lo;

            ah ^= tweak1_2;
            ((uint64_t*) &l[idx & MASK])[0] = al;
            ((uint64_t*) &l[idx & MASK])[1] = ah;
            ah ^= tweak1_2;

            ah ^= ch;
            al ^= cl;
            idx = al;
        }

        cn_implode_scratchpad<MEM>((__m128i*) l, (__m128i*) h);
        keccakf(h, 24);
        extra_hashes[scratchPad.state[0] & 3](scratchPad.state, 200, output);
    }

    inline static void hashLiteTube(const uint8_t* __restrict__ input,
                                 size_t size,
                                 uint8_t* __restrict__ output,
                                 ScratchPad& __restrict__ scratchPad)
    {
        const uint8_t* l;
        uint64_t* h;
        uint64_t al;
        uint64_t ah;
        __m128i bx;
        uint64_t idx;

        keccak(static_cast<const uint8_t*>(input), (int) size, scratchPad.state, 200);

        uint64_t tweak1_2 = (*reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(input) + 35) ^
                             *(reinterpret_cast<const uint64_t*>(scratchPad.state) + 24));
        l = scratchPad.memory;
        h = reinterpret_cast<uint64_t*>(scratchPad.state);

        cn_explode_scratchpad<MEM>((__m128i*) h, (__m128i*) l);

        al = h[0] ^ h[4];
        ah = h[1] ^ h[5];
        bx = _mm_set_epi64x(h[3] ^ h[7], h[2] ^ h[6]);
        idx = h[0] ^ h[4];

        for (size_t i = 0; i < ITERATIONS; i++) {
            __m128i cx;

            cx = soft_aesenc((uint32_t*)&l[idx & MASK], _mm_set_epi64x(ah, al));

            _mm_store_si128((__m128i*) &l[idx & MASK], _mm_xor_si128(bx, cx));
            const uint8_t tmp = reinterpret_cast<const uint8_t*>(&l[idx & MASK])[11];
            static const uint32_t table = 0x75310;
            const uint8_t index = (((tmp >> INDEX_SHIFT) & 6) | (tmp & 1)) << 1;
            ((uint8_t*)(&l[idx & MASK]))[11] = tmp ^ ((table >> index) & 0x30);
            idx = EXTRACT64(cx);
            bx = cx;

            uint64_t hi, lo, cl, ch;
            cl = ((uint64_t*) &l[idx & MASK])[0];
            ch = ((uint64_t*) &l[idx & MASK])[1];
            lo = __umul128(idx, cl, &hi);

            al += hi;
            ah += lo;

            ah ^= tweak1_2;
            ((uint64_t*) &l[idx & MASK])[0] = al;
            ((uint64_t*) &l[idx & MASK])[1] = ah;
            ah ^= tweak1_2;

            ((uint64_t*)&l[idx & MASK])[1] ^= ((uint64_t*)&l[idx & MASK])[0];

            ah ^= ch;
            al ^= cl;
            idx = al;
        }

        cn_implode_scratchpad<MEM>((__m128i*) l, (__m128i*) h);
        keccakf(h, 24);
        extra_hashes[scratchPad.state[0] & 3](scratchPad.state, 200, output);
    }

    inline static void hashHeavy(const uint8_t* __restrict__ input,
                            size_t size,
                            uint8_t* __restrict__ output,
                            ScratchPad& __restrict__ scratchPad)
    {
        const uint8_t* l;
        uint64_t* h;
        uint64_t al;
        uint64_t ah;
        __m128i bx;
        uint64_t idx;

        keccak(static_cast<const uint8_t*>(input), (int) size, scratchPad.state, 200);

        l = scratchPad.memory;
        h = reinterpret_cast<uint64_t*>(scratchPad.state);

        cn_explode_scratchpad_heavy<MEM>((__m128i*) h, (__m128i*) l);

        al = h[0] ^ h[4];
        ah = h[1] ^ h[5];
        bx = _mm_set_epi64x(h[3] ^ h[7], h[2] ^ h[6]);
        idx = h[0] ^ h[4];

        for (size_t i = 0; i < ITERATIONS; i++) {
            __m128i cx;

            cx = soft_aesenc((uint32_t*)&l[idx & MASK], _mm_set_epi64x(ah, al));

            _mm_store_si128((__m128i*) &l[idx & MASK], _mm_xor_si128(bx, cx));
            idx = EXTRACT64(cx);
            bx = cx;

            uint64_t hi, lo, cl, ch;
            cl = ((uint64_t*) &l[idx & MASK])[0];
            ch = ((uint64_t*) &l[idx & MASK])[1];
            lo = __umul128(idx, cl, &hi);

            al += hi;
            ah += lo;

            ((uint64_t*) &l[idx & MASK])[0] = al;
            ((uint64_t*) &l[idx & MASK])[1] = ah;

            ah ^= ch;
            al ^= cl;
            idx = al;

            int64_t n  = ((int64_t*)&l[idx & MASK])[0];
            int32_t d  = ((int32_t*)&l[idx & MASK])[2];
            int64_t q = n / (d | 0x5);

            ((int64_t*)&l[idx & MASK])[0] = n ^ q;
            idx = d ^ q;
        }

        cn_implode_scratchpad_heavy<MEM>((__m128i*) l, (__m128i*) h);
        keccakf(h, 24);
        extra_hashes[scratchPad.state[0] & 3](scratchPad.state, 200, output);
    }

    inline static void hashHeavyHaven(const uint8_t* __restrict__ input,
                                 size_t size,
                                 uint8_t* __restrict__ output,
                                 ScratchPad& __restrict__ scratchPad)
    {
        const uint8_t* l;
        uint64_t* h;
        uint64_t al;
        uint64_t ah;
        __m128i bx;
        uint64_t idx;

        keccak(static_cast<const uint8_t*>(input), (int) size, scratchPad.state, 200);

        l = scratchPad.memory;
        h = reinterpret_cast<uint64_t*>(scratchPad.state);

        cn_explode_scratchpad_heavy<MEM>((__m128i*) h, (__m128i*) l);

        al = h[0] ^ h[4];
        ah = h[1] ^ h[5];
        bx = _mm_set_epi64x(h[3] ^ h[7], h[2] ^ h[6]);
        idx = h[0] ^ h[4];

        for (size_t i = 0; i < ITERATIONS; i++) {
            __m128i cx;

            cx = soft_aesenc((uint32_t*)&l[idx & MASK], _mm_set_epi64x(ah, al));

            _mm_store_si128((__m128i*) &l[idx & MASK], _mm_xor_si128(bx, cx));
            idx = EXTRACT64(cx);
            bx = cx;

            uint64_t hi, lo, cl, ch;
            cl = ((uint64_t*) &l[idx & MASK])[0];
            ch = ((uint64_t*) &l[idx & MASK])[1];
            lo = __umul128(idx, cl, &hi);

            al += hi;
            ah += lo;

            ((uint64_t*) &l[idx & MASK])[0] = al;
            ((uint64_t*) &l[idx & MASK])[1] = ah;

            ah ^= ch;
            al ^= cl;
            idx = al;

            int64_t n  = ((int64_t*)&l[idx & MASK])[0];
            int32_t d  = ((int32_t*)&l[idx & MASK])[2];
            int64_t q = n / (d | 0x5);

            ((int64_t*)&l[idx & MASK])[0] = n ^ q;
            idx = (~d) ^ q;
        }

        cn_implode_scratchpad_heavy<MEM>((__m128i*) l, (__m128i*) h);
        keccakf(h, 24);
        extra_hashes[scratchPad.state[0] & 3](scratchPad.state, 200, output);
    }

    inline static void hashHeavyTube(const uint8_t* __restrict__ input,
                                     size_t size,
                                     uint8_t* __restrict__ output,
                                     ScratchPad& __restrict__ scratchPad)
    {
        const uint8_t* l;
        uint64_t* h;
        uint64_t al;
        uint64_t ah;
        __m128i bx;
        uint64_t idx;

        keccak(static_cast<const uint8_t*>(input), (int) size, scratchPad.state, 200);

        uint64_t tweak1_2 = (*reinterpret_cast<const uint64_t*>(reinterpret_cast<const uint8_t*>(input) + 35) ^
                             *(reinterpret_cast<const uint64_t*>(scratchPad.state) + 24));

        l = scratchPad.memory;
        h = reinterpret_cast<uint64_t*>(scratchPad.state);

        cn_explode_scratchpad_heavy<MEM>((__m128i*) h, (__m128i*) l);

        al = h[0] ^ h[4];
        ah = h[1] ^ h[5];
        bx = _mm_set_epi64x(h[3] ^ h[7], h[2] ^ h[6]);
        idx = h[0] ^ h[4];

        union alignas(16) {
            uint32_t k[4];
            uint64_t v64[2];
        };
        alignas(16) uint32_t x[4];

#define BYTE(p, i) ((unsigned char*)&p)[i]
        for (size_t i = 0; i < ITERATIONS; i++) {
            __m128i cx = _mm_load_si128((__m128i*) &l[idx & MASK]);

            const __m128i& key = _mm_set_epi64x(ah, al);

            _mm_store_si128((__m128i*)k, key);
            cx = _mm_xor_si128(cx, _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()));
            _mm_store_si128((__m128i*)x, cx);

            k[0] ^= saes_table[0][BYTE(x[0], 0)] ^ saes_table[1][BYTE(x[1], 1)] ^ saes_table[2][BYTE(x[2], 2)] ^ saes_table[3][BYTE(x[3], 3)];
            x[0] ^= k[0];
            k[1] ^= saes_table[0][BYTE(x[1], 0)] ^ saes_table[1][BYTE(x[2], 1)] ^ saes_table[2][BYTE(x[3], 2)] ^ saes_table[3][BYTE(x[0], 3)];
            x[1] ^= k[1];
            k[2] ^= saes_table[0][BYTE(x[2], 0)] ^ saes_table[1][BYTE(x[3], 1)] ^ saes_table[2][BYTE(x[0], 2)] ^ saes_table[3][BYTE(x[1], 3)];
            x[2] ^= k[2];
            k[3] ^= saes_table[0][BYTE(x[3], 0)] ^ saes_table[1][BYTE(x[0], 1)] ^ saes_table[2][BYTE(x[1], 2)] ^ saes_table[3][BYTE(x[2], 3)];

            cx = _mm_load_si128((__m128i*)k);

            _mm_store_si128((__m128i*) &l[idx & MASK], _mm_xor_si128(bx, cx));
            const uint8_t tmp = reinterpret_cast<const uint8_t*>(&l[idx & MASK])[11];
            static const uint32_t table = 0x75310;
            const uint8_t index = (((tmp >> INDEX_SHIFT) & 6) | (tmp & 1)) << 1;
            ((uint8_t*)(&l[idx & MASK]))[11] = tmp ^ ((table >> index) & 0x30);

            idx = EXTRACT64(cx);
            bx = cx;

            uint64_t hi, lo, cl, ch;
            cl = ((uint64_t*) &l[idx & MASK])[0];
            ch = ((uint64_t*) &l[idx & MASK])[1];
            lo = __umul128(idx, cl, &hi);

            al += hi;
            ah += lo;

            ah ^= tweak1_2;
            ((uint64_t*) &l[idx & MASK])[0] = al;
            ((uint64_t*) &l[idx & MASK])[1] = ah;
            ah ^= tweak1_2;

            ((uint64_t*)&l[idx & MASK])[1] ^= ((uint64_t*)&l[idx & MASK])[0];

            ah ^= ch;
            al ^= cl;
            idx = al;

            int64_t n  = ((int64_t*)&l[idx & MASK])[0];
            int32_t d  = ((int32_t*)&l[idx & MASK])[2];
            int64_t q = n / (d | 0x5);

            ((int64_t*)&l[idx & MASK])[0] = n ^ q;
            idx = d ^ q;
        }
#undef BYTE

        cn_implode_scratchpad_heavy<MEM>((__m128i*) l, (__m128i*) h);
        keccakf(h, 24);
        extra_hashes[scratchPad.state[0] & 3](scratchPad.state, 200, output);
    }
};


#endif /* __CRYPTONIGHT_X86_H__ */
