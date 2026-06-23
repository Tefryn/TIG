#pragma once

#include "objects/types.hpp"

#include <filesystem>
#include <span>
#include <vector>

// Content-level: convert between a Commit struct and the raw bytes git stores.
std::vector<uint8_t> serializeCommit(const Commit &commit);
Commit parseCommit(std::span<const uint8_t> raw);

// Store-level: write/read a commit object via the object store.
std::array<uint8_t, kSha1HashSize> writeCommit(const Commit &commit,
                                               const std::filesystem::path &tigDir);
Commit readCommit(const std::array<uint8_t, kSha1HashSize> &hash,
                  const std::filesystem::path &tigDir);
