#pragma once

#include "objects/types.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

// Returns the path where an object with the given hash is stored.
std::filesystem::path
objectPath(const std::filesystem::path &tigDir,
           const std::array<uint8_t, kSha1HashSize> &hash);

// Computes the SHA-1 hash of an object without writing it to disk.
std::array<uint8_t, kSha1HashSize> hashObject(ObjectType type,
                                              std::span<const uint8_t> data);

// Hashes, compresses, and writes an object under tigDir/objects/.
// Returns the object's hash. No-ops if the object already exists.
std::array<uint8_t, kSha1HashSize>
writeObject(ObjectType type,
            std::span<const uint8_t> data,
            const std::filesystem::path &tigDir);

// Reads and decompresses an object by hash. Fills outType and returns
// the raw content bytes (everything after the header null byte).
std::vector<uint8_t> readObject(const std::array<uint8_t, kSha1HashSize> &hash,
                                const std::filesystem::path &tigDir,
                                ObjectType &outType);
