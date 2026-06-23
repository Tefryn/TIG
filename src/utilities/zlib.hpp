#pragma once

#include <cstdint>
#include <span>
#include <vector>

std::vector<uint8_t> zlibCompress(std::span<const uint8_t> data);
std::vector<uint8_t> zlibDecompress(std::span<const uint8_t> data);
