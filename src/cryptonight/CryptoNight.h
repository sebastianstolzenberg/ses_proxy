/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
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

#ifndef __CRYPTONIGHT_H__
#define __CRYPTONIGHT_H__


#include <cstddef>
#include <cstdint>
#include <memory>

//#include "Options.h"
#include "proxy/algorithm.hpp"

struct ScratchPad {
  alignas(16) uint8_t state[208]; // 208 instead of 200 to maintain aligned to 16 byte boundaries
  alignas(16) uint8_t* memory;
};

class Job;
class JobResult;

class CryptoNight
{
public:
  CryptoNight(ses::proxy::Algorithm algorithm);

  void hash(const uint8_t* input, size_t size, uint8_t* output);

  ses::proxy::Algorithm getAlgorithm() const;

private:
//    static bool selfTest(Algo algorithm, PowVariant powVersion);

private:
  std::unique_ptr<ScratchPad> scratchPad_;
  ses::proxy::Algorithm algorithm_;
};


#endif /* __CRYPTONIGHT_H__ */
