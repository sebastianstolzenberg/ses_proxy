#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "util/difficulty.hpp"
#include "util/target.hpp"
#include "util/hex.hpp"

SCENARIO("target conversions")
{
  uint64_t target64             = 0x00123456789abcde;
  std::string target64Hex       =  "debc9a7856341200";
  uint32_t target32             = 0x00123456;
  std::string target32Hex       =  "56341200";
  uint64_t target64From32       = 0x0012345600000000;
  std::string target64HexFrom32 =  "0000000056341200";
  uint64_t target64Trim7        = 0x00123456789abc00;
  uint64_t target64Trim6        = 0x00123456789a0000;
  uint64_t target64Trim2        = 0x0012000000000000;


  REQUIRE(ses::util::Target(target64).getRaw() == target64);
  REQUIRE(ses::util::Target(target32).getRaw() == target64From32);
  REQUIRE(ses::util::Target(target64Hex).getRaw() == target64);
  REQUIRE(ses::util::Target(target32Hex).getRaw() == target64From32);

  REQUIRE(ses::util::Target(target64).toHexString() == target32Hex);
  REQUIRE(ses::util::Target(target32).toHexString() == target32Hex);
  REQUIRE(ses::util::Target(target64Hex).toHexString() == target32Hex);
  REQUIRE(ses::util::Target(target32Hex).toHexString() == target32Hex);

  REQUIRE(ses::util::Target(target64).toHexString(8) == target64Hex);
  REQUIRE(ses::util::Target(target32).toHexString(8) == target64HexFrom32);
  REQUIRE(ses::util::Target(target64Hex).toHexString(8) == target64Hex);
  REQUIRE(ses::util::Target(target32Hex).toHexString(8) == target64HexFrom32);

  REQUIRE(ses::util::Target(target64).trim(7).getRaw() == target64Trim7);
  REQUIRE(ses::util::Target(target64).trim(6).getRaw() == target64Trim6);
  REQUIRE(ses::util::Target(target64).trim().getRaw() == target64From32);
  REQUIRE(ses::util::Target(target64).trim(2).getRaw() == target64Trim2);
}

SCENARIO("hex string conversions")
{
  std::string test1Hex = "0abcde";
  uint32_t test1Uint32 = 0xabcde;

//  REQUIRE(ses::util::fromHex<uint32_t>(test1Hex) == test1Uint32);
}

SCENARIO("difficulty calculation")
{
  uint32_t target8k        = 0x00083126;
  uint32_t difficulty8k    = 8000;
  std::string target8kHex  = "26310800";

  uint32_t target50k       = 0x00014f8b;
  uint32_t difficulty50k   = 50000;
  std::string target50kHex = "8b4f0100";


  REQUIRE(ses::util::difficultyToTarget(difficulty8k).toHexString() == target8kHex);
  REQUIRE(ses::util::difficultyToTarget(difficulty8k).trim().getRaw() == ses::util::Target(target8k).getRaw());
  REQUIRE(ses::util::targetToDifficulty(ses::util::Target(target8k)) == difficulty8k);

  REQUIRE(ses::util::difficultyToTarget(difficulty50k).toHexString() == target50kHex);
  REQUIRE(ses::util::difficultyToTarget(difficulty50k).trim().getRaw() == ses::util::Target(target50k).getRaw());
  REQUIRE(ses::util::targetToDifficulty(ses::util::Target(target50k)) == difficulty50k);

  std::string hash = "8d962fb8adc880ab6b7297c0dbb3f62ae4c26b7dd51f68ce1acbd89569dd0400";
  uint32_t hashDifficulty = 13471;
  std::vector<uint8_t> hashData = ses::util::fromHex(hash);
  REQUIRE(ses::util::difficultyFromHashBuffer(hashData.data(), hashData.size()) == hashDifficulty);
}