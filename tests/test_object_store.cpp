#include "objects/object_store.hpp"

#include "utilities/hex.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// Creates a temporary .git/objects directory tree for each test and removes
// it on teardown.
class ObjectStoreTest : public ::testing::Test {
protected:
  void SetUp() override {
    gitDir = fs::temp_directory_path() / "tig_test_object_store";
    fs::remove_all(gitDir);
    fs::create_directories(gitDir / "objects");
  }

  void TearDown() override { fs::remove_all(gitDir); }

  fs::path gitDir;

  static std::span<const uint8_t> bytes(std::string_view s) {
    return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
  }
};

// ---------------------------------------------------------------------------
// objectPath
// ---------------------------------------------------------------------------

TEST_F(ObjectStoreTest, ObjectPathLayout) {
  std::array<uint8_t, 20> hash = {0xce, 0x01, 0x36, 0x25, 0x03, 0x0b,
                                   0xa8, 0xdb, 0xa9, 0x06, 0xf7, 0x56,
                                   0x96, 0x7f, 0x9e, 0x9c, 0xa3, 0x94,
                                   0x46, 0x4a};
  auto path = objectPath(gitDir, hash);
  EXPECT_EQ(path.parent_path().filename(), "ce");
  EXPECT_EQ(path.filename(),
            "013625030ba8dba906f756967f9e9ca394464a");
}

// ---------------------------------------------------------------------------
// hashObject — verified against `git hash-object`
// ---------------------------------------------------------------------------

TEST_F(ObjectStoreTest, HashObjectEmptyBlob) {
  // git hash-object /dev/null → e69de29bb2d1d6434b8b29ae775ad8c2e48c5391
  auto hash = hashObject(ObjectType::Blob, {});
  EXPECT_EQ(hashToHex(hash), "e69de29bb2d1d6434b8b29ae775ad8c2e48c5391");
}

TEST_F(ObjectStoreTest, HashObjectHelloNewline) {
  // printf 'hello\n' | git hash-object --stdin
  // → ce013625030ba8dba906f756967f9e9ca394464a
  auto hash = hashObject(ObjectType::Blob, bytes("hello\n"));
  EXPECT_EQ(hashToHex(hash), "ce013625030ba8dba906f756967f9e9ca394464a");
}

// ---------------------------------------------------------------------------
// writeObject + readObject round-trips
// ---------------------------------------------------------------------------

TEST_F(ObjectStoreTest, WriteCreatesFileAtCorrectPath) {
  auto hash = writeObject(ObjectType::Blob, bytes("hello\n"), gitDir);
  EXPECT_TRUE(fs::exists(objectPath(gitDir, hash)));
}

TEST_F(ObjectStoreTest, RoundTripBlob) {
  std::string content = "hello, world";
  auto hash = writeObject(ObjectType::Blob, bytes(content), gitDir);

  ObjectType type;
  auto result = readObject(hash, gitDir, type);

  EXPECT_EQ(type, ObjectType::Blob);
  EXPECT_EQ(std::string(result.begin(), result.end()), content);
}

TEST_F(ObjectStoreTest, RoundTripEmptyBlob) {
  auto hash = writeObject(ObjectType::Blob, {}, gitDir);

  ObjectType type;
  auto result = readObject(hash, gitDir, type);

  EXPECT_EQ(type, ObjectType::Blob);
  EXPECT_TRUE(result.empty());
}

TEST_F(ObjectStoreTest, RoundTripBinaryContent) {
  std::vector<uint8_t> content(256);
  for (size_t i = 0; i < 256; ++i)
    content[i] = static_cast<uint8_t>(i);

  auto hash = writeObject(ObjectType::Blob, content, gitDir);

  ObjectType type;
  auto result = readObject(hash, gitDir, type);

  EXPECT_EQ(type, ObjectType::Blob);
  EXPECT_EQ(result, content);
}

TEST_F(ObjectStoreTest, WriteIsIdempotent) {
  auto h1 = writeObject(ObjectType::Blob, bytes("hello\n"), gitDir);
  auto h2 = writeObject(ObjectType::Blob, bytes("hello\n"), gitDir);
  EXPECT_EQ(h1, h2);
}

TEST_F(ObjectStoreTest, DifferentContentDifferentHash) {
  auto h1 = writeObject(ObjectType::Blob, bytes("foo"), gitDir);
  auto h2 = writeObject(ObjectType::Blob, bytes("bar"), gitDir);
  EXPECT_NE(h1, h2);
}

TEST_F(ObjectStoreTest, HashObjectMatchesWriteObject) {
  std::string content = "some content";
  auto precomputed = hashObject(ObjectType::Blob, bytes(content));
  auto written = writeObject(ObjectType::Blob, bytes(content), gitDir);
  EXPECT_EQ(precomputed, written);
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

TEST_F(ObjectStoreTest, ReadMissingObjectThrows) {
  std::array<uint8_t, 20> bogus{};
  ObjectType type;
  EXPECT_THROW(readObject(bogus, gitDir, type), std::runtime_error);
}
