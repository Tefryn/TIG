#pragma once

#include <array>
#include <cstdint>
#include <span>

inline constexpr size_t kSha1HashSize = 20;

std::array<uint8_t, kSha1HashSize> computeSha1(std::span<const uint8_t> data);
