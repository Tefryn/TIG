#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

enum class ObjectType { Blob, Tree, Commit, Tag };

std::string_view typeName(ObjectType type);
ObjectType parseTypeName(std::string_view s);

struct TreeEntry {
  uint32_t mode; // 0100644 - regular, 0100755 - executable, 0120000 - symbolic,
                 // 0040000 - subdir
  std::string name;
  std::array<uint8_t, 20> hash;
};

struct Identity {
  std::string name;
  std::string email;
  int64_t timestamp;    // Unix seconds
  std::string timezone; // e.g. "+0000"
};

struct Commit {
  std::array<uint8_t, 20> tree;
  std::vector<std::array<uint8_t, 20>> parents; // empty for root commit
  Identity author;
  Identity committer;
  std::string message;
};
