#include "objects/blob.hpp"

#include "objects/object_store.hpp"

#include <stdexcept>

Blob serializeBlob(std::span<const uint8_t> fileBytes) {
  return {fileBytes.begin(), fileBytes.end()};
}

Blob parseBlob(std::span<const uint8_t> raw) {
  return {raw.begin(), raw.end()};
}

std::array<uint8_t, kSha1HashSize>
writeBlob(std::span<const uint8_t> content,
          const std::filesystem::path &tigDir) {
  return writeObject(ObjectType::Blob, serializeBlob(content), tigDir);
}

Blob readBlob(const std::array<uint8_t, kSha1HashSize> &hash,
              const std::filesystem::path &tigDir) {
  ObjectType type;
  auto raw = readObject(hash, tigDir, type);
  if (type != ObjectType::Blob)
    throw std::runtime_error("object is not a blob: " +
                             std::string(typeName(type)));
  return parseBlob(raw);
}
