#include "hex.hpp"

#include <stdexcept>

std::string hashToHex(const std::array<uint8_t, 20> &hash) {
  static constexpr char kDigits[] = "0123456789abcdef";
  std::string out;
  out.reserve(40);
  for (uint8_t b : hash) {
    out += kDigits[b >> 4];
    out += kDigits[b & 0xf];
  }
  return out;
}

std::array<uint8_t, 20> hexToHash(std::string_view hex) {
  if (hex.size() != 40)
    throw std::invalid_argument(
        "SHA-1 hex string must be exactly 40 characters");

  auto nibble = [](char c) -> uint8_t {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    throw std::invalid_argument("invalid hex character");
  };

  std::array<uint8_t, 20> out;
  for (size_t i = 0; i < 20; ++i)
    out[i] = static_cast<uint8_t>((nibble(hex[2 * i]) << 4) |
                                  nibble(hex[2 * i + 1]));
  return out;
}
