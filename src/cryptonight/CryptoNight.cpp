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

#include "CryptoNight.h"

#include "CryptoNight_softAES.h"

#include "CryptoNight_test.h"


#define MEMORY       2097152 /* 2 MiB */
#define MEMORY_LITE  1048576 /* 1 MiB */
#define MEMORY_HEAVY 4194304 /* 4 MiB */

#define POW_DEFAULT_INDEX_SHIFT 3
#define POW_XLT_V4_INDEX_SHIFT 4


static void cryptonight_softaes(ses::proxy::AlgorithmVariant powVersion, const uint8_t* input, size_t size, uint8_t* output, ScratchPad& scratchPad) {
    if (powVersion == ses::proxy::AlgorithmVariant::V1) {
        CryptoNightHash<0x80000, POW_DEFAULT_INDEX_SHIFT, MEMORY, 0x1FFFF0>::hashPowV2(input, size, output, scratchPad);
    } else if (powVersion == ses::proxy::AlgorithmVariant::ALLOY) {
        CryptoNightHash<0x100000, POW_DEFAULT_INDEX_SHIFT, MEMORY, 0x1FFFF0>::hash(input, size, output, scratchPad);
    } else if (powVersion == ses::proxy::AlgorithmVariant::XTL) {
        CryptoNightHash<0x80000, POW_XLT_V4_INDEX_SHIFT, MEMORY, 0x1FFFF0>::hashPowV2(input, size, output, scratchPad);
    } else if (powVersion == ses::proxy::AlgorithmVariant::MSR) {
        CryptoNightHash<0x40000, POW_DEFAULT_INDEX_SHIFT, MEMORY, 0x1FFFF0>::hashPowV2(input, size, output, scratchPad);
    } else if (powVersion == ses::proxy::AlgorithmVariant::RTO) {
        CryptoNightHash<0x80000, POW_DEFAULT_INDEX_SHIFT, MEMORY, 0x1FFFF0>::hashLiteTube(input, size, output, scratchPad);
    } else {
        CryptoNightHash<0x80000, POW_DEFAULT_INDEX_SHIFT, MEMORY, 0x1FFFF0>::hash(input, size, output, scratchPad);
    }
}

static void cryptonight_lite_softaes(ses::proxy::AlgorithmVariant powVersion, const uint8_t* input, size_t size, uint8_t* output, ScratchPad& scratchPad) {
    if (powVersion == ses::proxy::AlgorithmVariant::V1) {
        CryptoNightHash<0x40000, POW_DEFAULT_INDEX_SHIFT, MEMORY_LITE, 0xFFFF0>::hashPowV2(input, size, output, scratchPad);
    } else if (powVersion == ses::proxy::AlgorithmVariant::TUBE) {
        CryptoNightHash<0x40000, POW_DEFAULT_INDEX_SHIFT, MEMORY_LITE, 0xFFFF0>::hashLiteTube(input, size, output, scratchPad);
    } else {
        CryptoNightHash<0x40000, POW_DEFAULT_INDEX_SHIFT, MEMORY_LITE, 0xFFFF0>::hash(input, size, output, scratchPad);
    }
}

static void cryptonight_heavy_softaes(ses::proxy::AlgorithmVariant powVersion, const uint8_t* input, size_t size, uint8_t* output, ScratchPad& scratchPad) {
    if (powVersion == ses::proxy::AlgorithmVariant::XHV) {
        CryptoNightHash<0x40000, POW_DEFAULT_INDEX_SHIFT, MEMORY_HEAVY, 0x3FFFF0>::hashHeavyHaven(input, size, output, scratchPad);
    }
    else if (powVersion == ses::proxy::AlgorithmVariant::TUBE) {
        CryptoNightHash<0x40000, POW_DEFAULT_INDEX_SHIFT, MEMORY_HEAVY, 0x3FFFF0>::hashHeavyTube(input, size, output, scratchPad);
    }
    else {
        CryptoNightHash<0x40000, POW_DEFAULT_INDEX_SHIFT, MEMORY_HEAVY, 0x3FFFF0>::hashHeavy(input, size, output, scratchPad);
    }
}

CryptoNight::CryptoNight(ses::proxy::Algorithm algorithm)
    : scratchPad_(new ScratchPad), algorithm_(algorithm)
{
  size_t scratchPadSize;
  switch (algorithm_.getAlgorithmType_())
  {
    case ses::proxy::AlgorithmType::CRYPTONIGHT_LITE:
      scratchPadSize = MEMORY_LITE;
      break;
    case ses::proxy::AlgorithmType::CRYPTONIGHT_HEAVY:
      scratchPadSize = MEMORY_HEAVY;
      break;
    case ses::proxy::AlgorithmType::CRYPTONIGHT:
    default:
      scratchPadSize = MEMORY;
      break;
  }
  scratchPad_->memory = static_cast<uint8_t*>(_mm_malloc(scratchPadSize, 4096));
//    selfTest(algorithm);
}

void CryptoNight::hash(const uint8_t* input, size_t size, uint8_t* output)
{
  switch (algorithm_.getAlgorithmType_())
  {
      case ses::proxy::AlgorithmType::CRYPTONIGHT:
          cryptonight_softaes(algorithm_.getAlgorithmVariant_(), input, size, output, *scratchPad_);
          break;
      case ses::proxy::AlgorithmType::CRYPTONIGHT_LITE:
          cryptonight_lite_softaes(algorithm_.getAlgorithmVariant_(), input, size, output, *scratchPad_);
          break;
      case ses::proxy::AlgorithmType::CRYPTONIGHT_HEAVY:
          cryptonight_heavy_softaes(algorithm_.getAlgorithmVariant_(), input, size, output, *scratchPad_);
          break;
      default:
          // no hash calculation
          break;
  }
}

ses::proxy::Algorithm CryptoNight::getAlgorithm() const
{
    return algorithm_;
}

/*
bool CryptoNight::selfTest(int algo)
{
    if (cryptonight_hash_ctx[0] == nullptr
#if MAX_NUM_HASH_BLOCKS > 1
        || cryptonight_hash_ctx[1] == nullptr
#endif
#if MAX_NUM_HASH_BLOCKS > 2
        || cryptonight_hash_ctx[2] == nullptr
#endif
#if MAX_NUM_HASH_BLOCKS > 3
        || cryptonight_hash_ctx[3] == nullptr
#endif
#if MAX_NUM_HASH_BLOCKS > 4
        || cryptonight_hash_ctx[4] == nullptr
#endif
    ) {
        return false;
    }

    uint8_t output[160];

    ScratchPad* scratchPads [MAX_NUM_HASH_BLOCKS];

    for (size_t i = 0; i < MAX_NUM_HASH_BLOCKS; ++i) {
        ScratchPad* scratchPad = static_cast<ScratchPad *>(_mm_malloc(sizeof(ScratchPad), 4096));
        scratchPad->memory     = (uint8_t *) _mm_malloc(MEMORY * 6, 16);

        scratchPads[i] = scratchPad;
    }

    bool result = true;
    bool resultLite = true;
    bool resultHeavy = true;

    if (algo == Options::ALGO_CRYPTONIGHT_HEAVY) {
        // cn-heavy

        cryptonight_hash_ctx[0](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        resultHeavy = resultHeavy && memcmp(output, test_output_heavy, 32) == 0;

        #if MAX_NUM_HASH_BLOCKS > 1
        cryptonight_hash_ctx[1](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        resultHeavy = resultHeavy && memcmp(output, test_output_heavy, 64) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 2
        cryptonight_hash_ctx[2](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        resultHeavy = resultHeavy && memcmp(output, test_output_heavy, 96) == 0;
        #endif

        // cn-heavy haven

        cryptonight_hash_ctx[0](PowVariant::POW_XHV, test_input, 76, output, scratchPads);
        resultHeavy = resultHeavy && memcmp(output, test_output_heavy_haven, 32) == 0;

        #if MAX_NUM_HASH_BLOCKS > 1
        cryptonight_hash_ctx[1](PowVariant::POW_XHV, test_input, 76, output, scratchPads);
        resultHeavy = resultHeavy && memcmp(output, test_output_heavy_haven, 64) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 2
        cryptonight_hash_ctx[2](PowVariant::POW_XHV, test_input, 76, output, scratchPads);
        resultHeavy = resultHeavy && memcmp(output, test_output_heavy_haven, 96) == 0;
        #endif

        // cn-heavy bittube

        cryptonight_hash_ctx[0](PowVariant::POW_TUBE, test_input, 76, output, scratchPads);
        resultHeavy = resultHeavy && memcmp(output, test_output_heavy_tube, 32) == 0;

        #if MAX_NUM_HASH_BLOCKS > 1
        cryptonight_hash_ctx[1](PowVariant::POW_TUBE, test_input, 76, output, scratchPads);
        resultHeavy = resultHeavy && memcmp(output, test_output_heavy_tube, 64) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 2
        cryptonight_hash_ctx[2](PowVariant::POW_TUBE, test_input, 76, output, scratchPads);
        resultHeavy = resultHeavy && memcmp(output, test_output_heavy_tube, 96) == 0;
        #endif
    } else if (algo == Options::ALGO_CRYPTONIGHT_LITE) {
        // cn-lite v0

        cryptonight_hash_ctx[0](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_v0_lite, 32) == 0;

        #if MAX_NUM_HASH_BLOCKS > 1
        cryptonight_hash_ctx[1](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_v0_lite, 64) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 2
        cryptonight_hash_ctx[2](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_v0_lite, 96) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 3
        cryptonight_hash_ctx[3](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_v0_lite, 128) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 4
        cryptonight_hash_ctx[4](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_v0_lite, 160) == 0;
        #endif

        // cn-lite v7 tests

        cryptonight_hash_ctx[0](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output,  test_output_v1_lite, 32) == 0;

        #if MAX_NUM_HASH_BLOCKS > 1
        cryptonight_hash_ctx[1](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_v1_lite, 64) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 2
        cryptonight_hash_ctx[2](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output,  test_output_v1_lite, 96) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 3
        cryptonight_hash_ctx[3](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output,  test_output_v1_lite, 128) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 4
        cryptonight_hash_ctx[4](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output,  test_output_v1_lite, 160) == 0;
        #endif


        // cn-lite ibpc tests

        cryptonight_hash_ctx[0](PowVariant::POW_TUBE, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_ipbc_lite, 32) == 0;

        #if MAX_NUM_HASH_BLOCKS > 1
        cryptonight_hash_ctx[1](PowVariant::POW_TUBE, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_ipbc_lite, 64) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 2
        cryptonight_hash_ctx[2](PowVariant::POW_TUBE, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_ipbc_lite, 96) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 3
        cryptonight_hash_ctx[3](PowVariant::POW_TUBE, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_ipbc_lite, 128) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 4
        cryptonight_hash_ctx[4](PowVariant::POW_TUBE, test_input, 76, output, scratchPads);
        resultLite = resultLite && memcmp(output, test_output_ipbc_lite, 160) == 0;
        #endif

    } else {
        // cn v0

        cryptonight_hash_ctx[0](PowVariant::POW_V0,test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v0, 32) == 0;

        #if MAX_NUM_HASH_BLOCKS > 1
        cryptonight_hash_ctx[1](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v0, 64) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 2
        cryptonight_hash_ctx[2](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v0, 96) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 3
        cryptonight_hash_ctx[3](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v0, 128) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 4
        cryptonight_hash_ctx[4](PowVariant::POW_V0, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v0, 160) == 0;
        #endif

        // cn v7

        cryptonight_hash_ctx[0](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v1, 32) == 0;

        #if MAX_NUM_HASH_BLOCKS > 1
        cryptonight_hash_ctx[1](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v1, 64) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 2
        cryptonight_hash_ctx[2](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v1, 96) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 3
        cryptonight_hash_ctx[3](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v1, 128) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 4
        cryptonight_hash_ctx[4](PowVariant::POW_V1, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_v1, 160) == 0;
        #endif

        // cn xtl

        cryptonight_hash_ctx[0](PowVariant::POW_XTL,test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_xtl, 32) == 0;

        #if MAX_NUM_HASH_BLOCKS > 1
        cryptonight_hash_ctx[1](PowVariant::POW_XTL, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_xtl, 64) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 2
        cryptonight_hash_ctx[2](PowVariant::POW_XTL, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_xtl, 96) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 3
        cryptonight_hash_ctx[3](PowVariant::POW_XTL, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_xtl, 128) == 0;
        #endif

        #if MAX_NUM_HASH_BLOCKS > 4
        cryptonight_hash_ctx[4](PowVariant::POW_XTL, test_input, 76, output, scratchPads);
        result = result && memcmp(output, test_output_xtl, 160) == 0;
        #endif
    }

    for (size_t i = 0; i < MAX_NUM_HASH_BLOCKS; ++i) {
        _mm_free(scratchPads[i]->memory);
        _mm_free(scratchPads[i]);
    }

    return result && resultLite & resultHeavy;
}
*/