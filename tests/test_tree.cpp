#include "objects/tree.hpp"

#include "objects/blob.hpp"
#include "utilities/hex.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class TreeTest : public ::testing::Test {
protected:
  void SetUp() override {
    tigDir = fs::temp_directory_path() / "tig_test_tree";
    fs::remove_all(tigDir);
    fs::create_directories(tigDir / "objects");
  }

  void TearDown() override { fs::remove_all(tigDir); }

  fs::path tigDir;

  static std::array<uint8_t, 20> fakeHash(uint8_t fill) {
    std::array<uint8_t, 20> h;
    h.fill(fill);
    return h;
  }
};

// ---------------------------------------------------------------------------
// serializeTree — byte format
// ---------------------------------------------------------------------------

TEST_F(TreeTest, SerializeSingleEntry) {
  auto hash = fakeHash(0xab);
  TreeEntry e{0100644, "hello.txt", hash};
  auto raw = serializeTree({e});

  // Header: "100644 hello.txt\0" = 17 bytes, then 20 raw hash bytes.
  std::string expectedHeader = "100644 hello.txt";
  ASSERT_GE(raw.size(), expectedHeader.size() + 1 + 20);
  EXPECT_EQ(std::string(raw.begin(), raw.begin() + expectedHeader.size()),
            expectedHeader);
  EXPECT_EQ(raw[expectedHeader.size()], '\0');
  for (size_t i = 0; i < 20; ++i)
    EXPECT_EQ(raw[expectedHeader.size() + 1 + i], 0xab);
}

TEST_F(TreeTest, SerializeDirectoryMode) {
  TreeEntry e{040000, "src", fakeHash(0x01)};
  auto raw = serializeTree({e});
  // Git writes "40000" (5 chars) for directories, not "040000".
  std::string expectedHeader = "40000 src";
  EXPECT_EQ(std::string(raw.begin(), raw.begin() + expectedHeader.size()),
            expectedHeader);
}

TEST_F(TreeTest, SerializeEmptyList) {
  EXPECT_TRUE(serializeTree({}).empty());
}

// ---------------------------------------------------------------------------
// serializeTree — sort order
// ---------------------------------------------------------------------------

TEST_F(TreeTest, SortFilesAlphabetically) {
  std::vector<TreeEntry> entries = {
      {0100644, "zebra.txt", fakeHash(0x01)},
      {0100644, "apple.txt", fakeHash(0x02)},
      {0100644, "mango.txt", fakeHash(0x03)},
  };
  auto parsed = parseTree(serializeTree(entries));
  EXPECT_EQ(parsed[0].name, "apple.txt");
  EXPECT_EQ(parsed[1].name, "mango.txt");
  EXPECT_EQ(parsed[2].name, "zebra.txt");
}

TEST_F(TreeTest, FileBeforeDirectoryOfSameName) {
  // File "foo" must sort before directory "foo".
  std::vector<TreeEntry> entries = {
      {040000,  "foo", fakeHash(0x01)},  // dir
      {0100644, "foo", fakeHash(0x02)},  // file
  };
  auto parsed = parseTree(serializeTree(entries));
  EXPECT_EQ(parsed[0].mode, static_cast<uint32_t>(0100644)); // file first
  EXPECT_EQ(parsed[1].mode, static_cast<uint32_t>(040000));  // dir second
}

TEST_F(TreeTest, DirectoryInterleavesBySlash) {
  // dir "foo" sorts as "foo/" which is after file "foo" but before "foobar".
  std::vector<TreeEntry> entries = {
      {0100644, "foobar", fakeHash(0x03)},
      {040000,  "foo",    fakeHash(0x01)},
      {0100644, "foo",    fakeHash(0x02)},
  };
  auto parsed = parseTree(serializeTree(entries));
  EXPECT_EQ(parsed[0].name, "foo");
  EXPECT_EQ(parsed[0].mode, static_cast<uint32_t>(0100644)); // file "foo"
  EXPECT_EQ(parsed[1].name, "foo");
  EXPECT_EQ(parsed[1].mode, static_cast<uint32_t>(040000));  // dir "foo" ("foo/")
  EXPECT_EQ(parsed[2].name, "foobar");                        // file "foobar"
}

// ---------------------------------------------------------------------------
// parseTree — known byte sequence
// ---------------------------------------------------------------------------

TEST_F(TreeTest, ParseSingleKnownEntry) {
  // Manually build "100644 hello.txt\0" + 20 bytes of 0xab.
  std::vector<uint8_t> raw;
  std::string header = "100644 hello.txt";
  raw.insert(raw.end(), header.begin(), header.end());
  raw.push_back('\0');
  for (int i = 0; i < 20; ++i)
    raw.push_back(0xab);

  auto entries = parseTree(raw);
  ASSERT_EQ(entries.size(), 1u);
  EXPECT_EQ(entries[0].mode, static_cast<uint32_t>(0100644));
  EXPECT_EQ(entries[0].name, "hello.txt");
  EXPECT_EQ(entries[0].hash, fakeHash(0xab));
}

TEST_F(TreeTest, ParseEmptyRaw) {
  EXPECT_TRUE(parseTree({}).empty());
}

TEST_F(TreeTest, ParseMissingNullThrows) {
  std::vector<uint8_t> bad = {'1', '0', '0', '6', '4', '4', ' ', 'f'};
  EXPECT_THROW(parseTree(bad), std::runtime_error);
}

TEST_F(TreeTest, ParseTruncatedHashThrows) {
  std::vector<uint8_t> raw;
  std::string header = "100644 hello.txt";
  raw.insert(raw.end(), header.begin(), header.end());
  raw.push_back('\0');
  raw.push_back(0xab); // Only 1 byte of hash instead of 20.
  EXPECT_THROW(parseTree(raw), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Round-trip
// ---------------------------------------------------------------------------

TEST_F(TreeTest, SerializeParseRoundTrip) {
  std::vector<TreeEntry> entries = {
      {0100644, "file.txt",  fakeHash(0x01)},
      {040000,  "subdir",    fakeHash(0x02)},
      {0100755, "script.sh", fakeHash(0x03)},
  };
  auto parsed = parseTree(serializeTree(entries));
  ASSERT_EQ(parsed.size(), 3u);

  // After round-trip, entries come back sorted.
  // Alphabetical: file.txt < script.sh < subdir/
  EXPECT_EQ(parsed[0].name, "file.txt");
  EXPECT_EQ(parsed[1].name, "script.sh");
  EXPECT_EQ(parsed[2].name, "subdir");

  for (const auto &e : parsed) {
    auto it = std::find_if(entries.begin(), entries.end(),
                           [&](const TreeEntry &o) { return o.name == e.name; });
    ASSERT_NE(it, entries.end());
    EXPECT_EQ(e.mode, it->mode);
    EXPECT_EQ(e.hash, it->hash);
  }
}

// ---------------------------------------------------------------------------
// writeTree / readTree
// ---------------------------------------------------------------------------

TEST_F(TreeTest, WriteThenReadRoundTrip) {
  auto blobHash = writeBlob(
      {reinterpret_cast<const uint8_t *>("hello\n"), 6}, tigDir);

  std::vector<TreeEntry> entries = {{0100644, "hello.txt", blobHash}};
  auto treeHash = writeTree(entries, tigDir);
  auto readBack = readTree(treeHash, tigDir);

  ASSERT_EQ(readBack.size(), 1u);
  EXPECT_EQ(readBack[0].mode, static_cast<uint32_t>(0100644));
  EXPECT_EQ(readBack[0].name, "hello.txt");
  EXPECT_EQ(readBack[0].hash, blobHash);
}

TEST_F(TreeTest, ReadNonTreeThrows) {
  auto hash = writeBlob(
      {reinterpret_cast<const uint8_t *>("data"), 4}, tigDir);
  EXPECT_THROW(readTree(hash, tigDir), std::runtime_error);
}

TEST_F(TreeTest, WriteIsIdempotent) {
  std::vector<TreeEntry> entries = {{0100644, "a.txt", fakeHash(0x01)}};
  auto h1 = writeTree(entries, tigDir);
  auto h2 = writeTree(entries, tigDir);
  EXPECT_EQ(h1, h2);
}
