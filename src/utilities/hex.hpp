#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

std::string hashToHex(const std::array<uint8_t, 20>& hash);
std::array<uint8_t, 20> hexToHash(std::string_view hex);
