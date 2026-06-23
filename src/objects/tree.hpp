#pragma once

#include "objects/types.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

// Content-level: convert between a list of TreeEntry and raw bytes.
// serializeTree sorts entries into git's required order before encoding.
std::vector<uint8_t> serializeTree(const std::vector<TreeEntry> &entries);
std::vector<TreeEntry> parseTree(std::span<const uint8_t> raw);

// Store-level: write/read a tree object via the object store.
std::array<uint8_t, kSha1HashSize>
writeTree(const std::vector<TreeEntry> &entries,
          const std::filesystem::path &tigDir);
std::vector<TreeEntry> readTree(const std::array<uint8_t, kSha1HashSize> &hash,
                                const std::filesystem::path &tigDir);
