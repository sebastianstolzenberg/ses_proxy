#pragma once

#include <cstdint>
#include <vector>

namespace cryptonote
{
std::vector<uint8_t> convert_blob(const std::vector<uint8_t>& templateBlob);
//std::vector<uint8_t> construct_block_blob(const std::vector<uint8_t>& templateBlob, uint32_t nonce);
}