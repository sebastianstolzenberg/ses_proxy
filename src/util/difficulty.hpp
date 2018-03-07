#pragma once

#include <cstdint>

namespace ses {
namespace util {

uint32_t targetToDifficulty(uint32_t target);
uint32_t difficultyToTarget(uint32_t difficulty);
uint32_t difficultyFromHashBuffer(const uint8_t* data, size_t size);

} // namespace util
} // namespace ses
