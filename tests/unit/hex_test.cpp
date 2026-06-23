#include "utilities/hex.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

// ---------------------------------------------------------------------------
// hashToHex
// ---------------------------------------------------------------------------

TEST(HashToHex, AllZeros) {
  std::array<uint8_t, 20> hash{};
  EXPECT_EQ(hashToHex(hash), "0000000000000000000000000000000000000000");
}

TEST(HashToHex, AllOnes) {
  std::array<uint8_t, 20> hash;
  hash.fill(0xff);
  EXPECT_EQ(hashToHex(hash), "ffffffffffffffffffffffffffffffffffffffff");
}

TEST(HashToHex, KnownValue) {
  std::array<uint8_t, 20> hash = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd,
                                   0xef, 0x00, 0xff, 0x10, 0x20, 0x30, 0x40,
                                   0x50, 0x60, 0x70, 0x80, 0x90, 0xa0};
  EXPECT_EQ(hashToHex(hash), "0123456789abcdef00ff102030405060708090a0");
}

TEST(HashToHex, OutputIsAlwaysLowercase) {
  std::array<uint8_t, 20> hash;
  hash.fill(0xaf);
  std::string hex = hashToHex(hash);
  for (char c : hex)
    EXPECT_TRUE(c >= '0' && c <= '9' || c >= 'a' && c <= 'f')
        << "unexpected character: " << c;
}

TEST(HashToHex, OutputIsFortyChars) {
  std::array<uint8_t, 20> hash{};
  EXPECT_EQ(hashToHex(hash).size(), 40u);
}

// ---------------------------------------------------------------------------
// hexToHash
// ---------------------------------------------------------------------------

TEST(HexToHash, AllZeros) {
  auto hash = hexToHash("0000000000000000000000000000000000000000");
  std::array<uint8_t, 20> expected{};
  EXPECT_EQ(hash, expected);
}

TEST(HexToHash, AllOnes) {
  auto hash = hexToHash("ffffffffffffffffffffffffffffffffffffffff");
  std::array<uint8_t, 20> expected;
  expected.fill(0xff);
  EXPECT_EQ(hash, expected);
}

TEST(HexToHash, KnownValue) {
  auto hash = hexToHash("0123456789abcdef00ff102030405060708090a0");
  std::array<uint8_t, 20> expected = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab,
                                       0xcd, 0xef, 0x00, 0xff, 0x10, 0x20,
                                       0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                                       0x90, 0xa0};
  EXPECT_EQ(hash, expected);
}

TEST(HexToHash, AcceptsUppercase) {
  auto lower = hexToHash("aabbccddeeff00112233445566778899aabbccdd");
  auto upper = hexToHash("AABBCCDDEEFF00112233445566778899AABBCCDD");
  EXPECT_EQ(lower, upper);
}

TEST(HexToHash, AcceptsMixedCase) {
  auto a = hexToHash("aAbBcCdDeEfF00112233445566778899aabbccdd");
  auto b = hexToHash("aabbccddeeff00112233445566778899aabbccdd");
  EXPECT_EQ(a, b);
}

TEST(HexToHash, TooShortThrows) {
  EXPECT_THROW(hexToHash("abc"), std::invalid_argument);
}

TEST(HexToHash, TooLongThrows) {
  EXPECT_THROW(hexToHash("0000000000000000000000000000000000000000ff"),
               std::invalid_argument);
}

TEST(HexToHash, InvalidCharThrows) {
  EXPECT_THROW(hexToHash("gggggggggggggggggggggggggggggggggggggggg"),
               std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Round-trip
// ---------------------------------------------------------------------------

TEST(HexRoundTrip, HashToHexToHash) {
  std::array<uint8_t, 20> original = {0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe,
                                       0xba, 0xbe, 0x00, 0x11, 0x22, 0x33,
                                       0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
                                       0xaa, 0xbb};
  EXPECT_EQ(hexToHash(hashToHex(original)), original);
}

TEST(HexRoundTrip, HexToHashToHex) {
  std::string original = "deadbeefcafebabe00112233445566778899aabb";
  EXPECT_EQ(hashToHex(hexToHash(original)), original);
}
