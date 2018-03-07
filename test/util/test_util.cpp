#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "util/difficulty.hpp"
#include "util/hex.hpp"

SCENARIO("difficulty caculation")
{
  uint64_t target8K = 0x0000000000083126;
  uint32_t difficulty8K = 8000;

  REQUIRE(ses::util::difficultyToTarget(difficulty8K) == target8K);
  REQUIRE(ses::util::targetToDifficulty(target8K) == difficulty8K);

  std::string hash = "8d962fb8adc880ab6b7297c0dbb3f62ae4c26b7dd51f68ce1acbd89569dd0400";
  uint32_t hashDifficulty = 13471;
  std::vector<uint8_t> hashData = ses::util::fromHex(hash);
  REQUIRE(ses::util::difficultyFromHashBuffer(hashData.data(), hashData.size()) == hashDifficulty);
}