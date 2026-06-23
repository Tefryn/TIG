#include "hex.hpp"

#include <stdexcept>

std::string hashToHex(const std::array<uint8_t, kSha1HashSize> &hash) {
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string out;
  out.reserve(kSha1HexSize);
  for (uint8_t b : hash) {
    out += kDigits[b >> 4];
    out += kDigits[b & 0xf];
  }
  return out;
}

std::array<uint8_t, kSha1HashSize> hexToHash(std::string_view hex) {
  if (hex.size() != kSha1HexSize)
    throw std::invalid_argument("SHA-1 hex string must be exactly " +
                                std::to_string(kSha1HexSize) + " characters");

  auto nibble = [](char c) -> uint8_t {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    throw std::invalid_argument("invalid hex character");
  };

  std::array<uint8_t, kSha1HashSize> out;
  for (size_t i = 0; i < kSha1HashSize; ++i)
    out[i] = static_cast<uint8_t>((nibble(hex[2 * i]) << 4) |
                                  nibble(hex[2 * i + 1]));
  return out;
}
