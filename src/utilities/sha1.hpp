#pragma once

#include <array>
#include <cstdint>
#include <span>

std::array<uint8_t, 20> sha1(std::span<const uint8_t> data);
