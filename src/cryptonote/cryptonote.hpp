#pragma once

#include <cstdint>
#include <vector>

namespace cryptonote
{
std::vector<uint8_t> convert_blob(const std::vector<uint8_t>& blob);
}