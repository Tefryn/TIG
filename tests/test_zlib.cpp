#include "utilities/zlib.hpp"

#include <gtest/gtest.h>

#include <numeric>
#include <stdexcept>

namespace {

// Helper: turn a string literal into a byte span for compress input.
std::span<const uint8_t> asBytes(std::string_view s) {
  return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
}

} // namespace

// ---------------------------------------------------------------------------
// zlibCompress + zlibDecompress round-trips
// ---------------------------------------------------------------------------

TEST(ZlibRoundTrip, ShortString) {
  std::string original = "hello, world";
  auto compressed = zlibCompress(asBytes(original));
  auto decompressed = zlibDecompress(compressed);
  std::string result(decompressed.begin(), decompressed.end());
  EXPECT_EQ(result, original);
}

TEST(ZlibRoundTrip, EmptyInput) {
  std::span<const uint8_t> empty;
  auto compressed = zlibCompress(empty);
  auto decompressed = zlibDecompress(compressed);
  EXPECT_TRUE(decompressed.empty());
}

TEST(ZlibRoundTrip, SingleByte) {
  std::array<uint8_t, 1> input = {0x42};
  auto compressed = zlibCompress(input);
  auto decompressed = zlibDecompress(compressed);
  EXPECT_EQ(decompressed, std::vector<uint8_t>({0x42}));
}

TEST(ZlibRoundTrip, AllZeroBytes) {
  std::vector<uint8_t> input(1024, 0x00);
  auto compressed = zlibCompress(input);
  auto decompressed = zlibDecompress(compressed);
  EXPECT_EQ(decompressed, input);
}

TEST(ZlibRoundTrip, LargeRepetitiveData) {
  // 256 KB of repeating pattern — highly compressible.
  std::vector<uint8_t> input(256 * 1024);
  for (size_t i = 0; i < input.size(); ++i)
    input[i] = static_cast<uint8_t>(i % 64);
  auto compressed = zlibCompress(input);
  auto decompressed = zlibDecompress(compressed);
  EXPECT_EQ(decompressed, input);
}

TEST(ZlibRoundTrip, BinaryData) {
  // All 256 byte values in sequence.
  std::vector<uint8_t> input(256);
  std::iota(input.begin(), input.end(), 0);
  auto compressed = zlibCompress(input);
  auto decompressed = zlibDecompress(compressed);
  EXPECT_EQ(decompressed, input);
}

TEST(ZlibRoundTrip, PreservesExactBytes) {
  std::vector<uint8_t> input = {0xde, 0xad, 0xbe, 0xef, 0x00, 0xff};
  auto compressed = zlibCompress(input);
  auto decompressed = zlibDecompress(compressed);
  EXPECT_EQ(decompressed, input);
}

// ---------------------------------------------------------------------------
// Compression properties
// ---------------------------------------------------------------------------

TEST(ZlibCompress, ReducesSizeForRepetitiveData) {
  std::vector<uint8_t> input(4096, 'A');
  auto compressed = zlibCompress(input);
  EXPECT_LT(compressed.size(), input.size());
}

TEST(ZlibCompress, OutputIsNonEmpty) {
  std::string original = "git";
  auto compressed = zlibCompress(asBytes(original));
  EXPECT_FALSE(compressed.empty());
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST(ZlibDecompress, InvalidDataThrows) {
  std::vector<uint8_t> garbage = {0x00, 0x01, 0x02, 0x03, 0xff, 0xfe};
  EXPECT_THROW(zlibDecompress(garbage), std::runtime_error);
}
