#pragma once

#include "objects/types.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>

// Content-level: convert between raw file bytes and the Blob type.
Blob serializeBlob(std::span<const uint8_t> fileBytes);
Blob parseBlob(std::span<const uint8_t> raw);

// Store-level: hash/compress/write and read/decompress from the object store.
std::array<uint8_t, kSha1HashSize>
writeBlob(std::span<const uint8_t> content,
          const std::filesystem::path &tigDir);
Blob readBlob(const std::array<uint8_t, kSha1HashSize> &hash,
              const std::filesystem::path &tigDir);
