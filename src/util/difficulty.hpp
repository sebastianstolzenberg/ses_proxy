#pragma once

#include "util/target.hpp"

namespace ses {
namespace util {

uint32_t targetToDifficulty(const Target& target);
Target difficultyToTarget(uint32_t difficulty);

uint32_t difficultyFromHashBuffer(const uint8_t* data, size_t size);

} // namespace util
} // namespace ses
