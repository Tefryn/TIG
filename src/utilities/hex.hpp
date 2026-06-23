#pragma once

#include "utilities/sha1.hpp"

#include <string>
#include <string_view>

inline constexpr size_t kSha1HexSize = kSha1HashSize * 2;

std::string hashToHex(const std::array<uint8_t, kSha1HashSize> &hash);
std::array<uint8_t, kSha1HashSize> hexToHash(std::string_view hex);
