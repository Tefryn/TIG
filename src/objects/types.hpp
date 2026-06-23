#pragma once

#include "utilities/sha1.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

enum class ObjectType { Blob, Tree, Commit, Tag };

using Blob = std::vector<uint8_t>;

std::string_view typeName(ObjectType type);
ObjectType parseTypeName(std::string_view s);

namespace FileMode {
inline constexpr uint32_t RegularFile = 0100644;
inline constexpr uint32_t ExecutableFile = 0100755;
inline constexpr uint32_t SymbolicLink = 0120000;
inline constexpr uint32_t Directory = 0040000;
} // namespace FileMode

struct TreeEntry {
  uint32_t mode;
  std::string name;
  std::array<uint8_t, kSha1HashSize> hash;
};

struct Identity {
  std::string name;
  std::string email;
  int64_t timestamp;    // Unix seconds
  std::string timezone; // e.g. "+0000"
};

struct Commit {
  std::array<uint8_t, kSha1HashSize> tree;
  std::vector<std::array<uint8_t, kSha1HashSize>>
      parents; // empty for root commit
  Identity author;
  Identity committer;
  std::string message;
};
