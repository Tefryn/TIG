#include "objects/tree.hpp"

#include "objects/object_store.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

// Git sorts tree entries by name, but treats directory names as if they end
// with '/' so they interleave correctly with files of similar names.
// e.g. dir "foo" sorts after file "foo" but before file "foobar".
static std::string sortKey(const TreeEntry &e) {
  return e.mode == FileMode::Directory ? e.name + '/' : e.name;
}

std::vector<uint8_t> serializeTree(const std::vector<TreeEntry> &entries) {
  std::vector<TreeEntry> sorted = entries;
  std::sort(
      sorted.begin(), sorted.end(), [](const TreeEntry &a, const TreeEntry &b) {
        return sortKey(a) < sortKey(b);
      });

  std::vector<uint8_t> out;
  for (const auto &e : sorted) {
    // "<mode_octal> <name>\0<kSha1HashSize raw bytes>"
    std::ostringstream header;
    header << std::oct << e.mode << ' ' << e.name << '\0';
    std::string h = header.str();
    out.insert(out.end(), h.begin(), h.end());
    out.insert(out.end(), e.hash.begin(), e.hash.end());
  }
  return out;
}

std::vector<TreeEntry> parseTree(std::span<const uint8_t> raw) {
  std::vector<TreeEntry> entries;
  size_t pos = 0;

  while (pos < raw.size()) {
    // Read mode (octal string up to the first space).
    size_t spacePos = pos;
    while (spacePos < raw.size() && raw[spacePos] != ' ') ++spacePos;
    if (spacePos >= raw.size())
      throw std::runtime_error("corrupt tree: missing space after mode");

    std::string modeStr(raw.begin() + pos, raw.begin() + spacePos);
    uint32_t mode = static_cast<uint32_t>(std::stoul(modeStr, nullptr, 8));
    pos = spacePos + 1;

    // Read name (up to the null terminator).
    size_t nullPos = pos;
    while (nullPos < raw.size() && raw[nullPos] != '\0') ++nullPos;
    if (nullPos >= raw.size())
      throw std::runtime_error(
          "corrupt tree: missing null terminator after name");

    std::string name(raw.begin() + pos, raw.begin() + nullPos);
    pos = nullPos + 1;

    // Read the raw hash.
    if (pos + kSha1HashSize > raw.size())
      throw std::runtime_error("corrupt tree: truncated entry hash");

    std::array<uint8_t, kSha1HashSize> hash;
    std::copy(
        raw.begin() + pos, raw.begin() + pos + kSha1HashSize, hash.begin());
    pos += kSha1HashSize;

    entries.push_back({mode, std::move(name), hash});
  }

  return entries;
}

std::array<uint8_t, kSha1HashSize>
writeTree(const std::vector<TreeEntry> &entries,
          const std::filesystem::path &tigDir) {
  return writeObject(ObjectType::Tree, serializeTree(entries), tigDir);
}

std::vector<TreeEntry> readTree(const std::array<uint8_t, kSha1HashSize> &hash,
                                const std::filesystem::path &tigDir) {
  ObjectType type;
  auto raw = readObject(hash, tigDir, type);
  if (type != ObjectType::Tree)
    throw std::runtime_error("object is not a tree: " +
                             std::string(typeName(type)));
  return parseTree(raw);
}
