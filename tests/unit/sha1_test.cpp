#include "utilities/sha1.hpp"

#include "utilities/hex.hpp"

#include <gtest/gtest.h>

// SHA-1 test vectors from NIST / RFC 3174.

TEST(Sha1, EmptyString) {
  std::string in = "";
  auto hash = computeSha1({reinterpret_cast<const uint8_t *>(in.data()), in.size()});
  EXPECT_EQ(hashToHex(hash), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

TEST(Sha1, Abc) {
  std::string in = "abc";
  auto hash = computeSha1({reinterpret_cast<const uint8_t *>(in.data()), in.size()});
  EXPECT_EQ(hashToHex(hash), "a9993e364706816aba3e25717850c26c9cd0d89d");
}

TEST(Sha1, LongerMessage) {
  std::string in = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
  auto hash = computeSha1({reinterpret_cast<const uint8_t *>(in.data()), in.size()});
  EXPECT_EQ(hashToHex(hash), "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST(Sha1, OutputIsAlways20Bytes) {
  std::string in = "hello";
  auto hash = computeSha1({reinterpret_cast<const uint8_t *>(in.data()), in.size()});
  EXPECT_EQ(hash.size(), 20u);
}

TEST(Sha1, DifferentInputsDifferentHashes) {
  std::string a = "foo";
  std::string b = "bar";
  auto ha = computeSha1({reinterpret_cast<const uint8_t *>(a.data()), a.size()});
  auto hb = computeSha1({reinterpret_cast<const uint8_t *>(b.data()), b.size()});
  EXPECT_NE(ha, hb);
}

TEST(Sha1, DeterministicSameInputSameHash) {
  std::string in = "hello, world";
  auto h1 = computeSha1({reinterpret_cast<const uint8_t *>(in.data()), in.size()});
  auto h2 = computeSha1({reinterpret_cast<const uint8_t *>(in.data()), in.size()});
  EXPECT_EQ(h1, h2);
}
