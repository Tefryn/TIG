#include "objects/blob.hpp"

#include "objects/object_store.hpp"
#include "utilities/hex.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class BlobTest : public ::testing::Test {
protected:
  void SetUp() override {
    tigDir = fs::temp_directory_path() / "tig_test_blob";
    fs::remove_all(tigDir);
    fs::create_directories(tigDir / "objects");
  }

  void TearDown() override { fs::remove_all(tigDir); }

  fs::path tigDir;

  static std::span<const uint8_t> bytes(std::string_view s) {
    return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
  }
};

// ---------------------------------------------------------------------------
// serializeBlob
// ---------------------------------------------------------------------------

TEST_F(BlobTest, SerializeProducesIdenticalBytes) {
  std::string content = "hello, world";
  Blob result = serializeBlob(bytes(content));
  EXPECT_EQ(std::string(result.begin(), result.end()), content);
}

TEST_F(BlobTest, SerializeEmptyContent) {
  Blob result = serializeBlob({});
  EXPECT_TRUE(result.empty());
}

TEST_F(BlobTest, SerializeBinaryContent) {
  std::vector<uint8_t> content(256);
  for (size_t i = 0; i < 256; ++i)
    content[i] = static_cast<uint8_t>(i);
  Blob result = serializeBlob(content);
  EXPECT_EQ(result, content);
}

// ---------------------------------------------------------------------------
// parseBlob
// ---------------------------------------------------------------------------

TEST_F(BlobTest, ParseProducesIdenticalBytes) {
  std::string content = "hello, world";
  Blob result = parseBlob(bytes(content));
  EXPECT_EQ(std::string(result.begin(), result.end()), content);
}

TEST_F(BlobTest, ParseEmptyRaw) {
  Blob result = parseBlob({});
  EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// serialize / parse round-trip
// ---------------------------------------------------------------------------

TEST_F(BlobTest, SerializeParseRoundTrip) {
  std::string content = "round-trip content";
  Blob serialized = serializeBlob(bytes(content));
  Blob parsed = parseBlob(serialized);
  EXPECT_EQ(std::string(parsed.begin(), parsed.end()), content);
}

// ---------------------------------------------------------------------------
// writeBlob / readBlob
// ---------------------------------------------------------------------------

TEST_F(BlobTest, WriteThenReadRoundTrip) {
  std::string content = "hello, world";
  auto hash = writeBlob(bytes(content), tigDir);
  Blob result = readBlob(hash, tigDir);
  EXPECT_EQ(std::string(result.begin(), result.end()), content);
}

TEST_F(BlobTest, WriteEmptyBlob) {
  auto hash = writeBlob({}, tigDir);
  Blob result = readBlob(hash, tigDir);
  EXPECT_TRUE(result.empty());
}

TEST_F(BlobTest, WriteKnownHash) {
  // printf 'hello\n' | git hash-object --stdin
  auto hash = writeBlob(bytes("hello\n"), tigDir);
  EXPECT_EQ(hashToHex(hash), "ce013625030ba8dba906f756967f9e9ca394464a");
}

TEST_F(BlobTest, ReadNonBlobThrows) {
  // Write a commit-typed object and try to read it as a blob.
  std::string fakeCommit = "tree 0000000000000000000000000000000000000000\n";
  auto hash = writeObject(ObjectType::Commit, bytes(fakeCommit), tigDir);
  EXPECT_THROW(readBlob(hash, tigDir), std::runtime_error);
}

TEST_F(BlobTest, ReadMissingObjectThrows) {
  std::array<uint8_t, 20> bogus{};
  EXPECT_THROW(readBlob(bogus, tigDir), std::runtime_error);
}
