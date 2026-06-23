#pragma once

#include <array>
#include <cstdint>
#include <span>

std::array<uint8_t, 20> computeSha1(std::span<const uint8_t> data);
