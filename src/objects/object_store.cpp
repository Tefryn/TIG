#include "objects/object_store.hpp"

#include "utilities/hex.hpp"
#include "utilities/sha1.hpp"
#include "utilities/zlib.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

// Builds the header+content byte sequence that tig hashes and stores.
static std::vector<uint8_t> buildStore(ObjectType type,
                                       std::span<const uint8_t> data) {
  std::string header =
      std::string(typeName(type)) + " " + std::to_string(data.size()) + '\0';
  std::vector<uint8_t> store;
  store.reserve(header.size() + data.size());
  store.insert(store.end(), header.begin(), header.end());
  store.insert(store.end(), data.begin(), data.end());
  return store;
}

std::filesystem::path objectPath(const std::filesystem::path &tigDir,
                                 const std::array<uint8_t, 20> &hash) {
  std::string hex = hashToHex(hash);
  return tigDir / "objects" / hex.substr(0, 2) / hex.substr(2);
}

std::array<uint8_t, 20> hashObject(ObjectType type,
                                   std::span<const uint8_t> data) {
  auto store = buildStore(type, data);
  return computeSha1(store);
}

std::array<uint8_t, 20> writeObject(ObjectType type,
                                    std::span<const uint8_t> data,
                                    const std::filesystem::path &tigDir) {
  auto store = buildStore(type, data);
  auto hash = computeSha1(store);
  auto path = objectPath(tigDir, hash);

  if (std::filesystem::exists(path)) return hash;

  std::filesystem::create_directories(path.parent_path());

  auto compressed = zlibCompress(store);
  std::ofstream file(path, std::ios::binary);
  if (!file) throw std::runtime_error("cannot write object: " + path.string());
  file.write(reinterpret_cast<const char *>(compressed.data()),
             static_cast<std::streamsize>(compressed.size()));

  return hash;
}

std::vector<uint8_t> readObject(const std::array<uint8_t, 20> &hash,
                                const std::filesystem::path &tigDir,
                                ObjectType &outType) {
  auto path = objectPath(tigDir, hash);
  std::ifstream file(path, std::ios::binary);
  if (!file) throw std::runtime_error("object not found: " + hashToHex(hash));

  std::vector<uint8_t> compressed(std::istreambuf_iterator<char>(file), {});
  auto raw = zlibDecompress(compressed);

  // Header format: "<type> <size>\0<content>"
  auto nullIt = std::find(raw.begin(), raw.end(), uint8_t{0});
  if (nullIt == raw.end())
    throw std::runtime_error("corrupt object: no null byte in header");

  std::string header(raw.begin(), nullIt);
  auto spacePos = header.find(' ');
  if (spacePos == std::string::npos)
    throw std::runtime_error("corrupt object: malformed header");

  outType = parseTypeName(header.substr(0, spacePos));
  return std::vector<uint8_t>(nullIt + 1, raw.end());
}
