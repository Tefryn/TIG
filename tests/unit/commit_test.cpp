#include "objects/commit.hpp"

#include "objects/object_store.hpp"
#include "utilities/hex.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class CommitTest : public ::testing::Test {
protected:
  void SetUp() override {
    tigDir = fs::temp_directory_path() / "tig_test_commit";
    fs::remove_all(tigDir);
    fs::create_directories(tigDir / "objects");
  }

  void TearDown() override { fs::remove_all(tigDir); }

  fs::path tigDir;

  static std::array<uint8_t, kSha1HashSize> fakeHash(uint8_t fill) {
    std::array<uint8_t, kSha1HashSize> h;
    h.fill(fill);
    return h;
  }

  static std::span<const uint8_t> bytes(std::string_view s) {
    return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
  }

  // Returns a minimal valid commit for use in store-level tests.
  static Commit makeCommit() {
    Commit c;
    c.tree       = fakeHash(0x01);
    c.author     = {"A U Thor", "author@example.com", 1234567890, "+0000"};
    c.committer  = {"C O Mitter", "committer@example.com", 1234567890, "+0000"};
    c.message    = "Initial commit\n";
    return c;
  }
};

// ---------------------------------------------------------------------------
// serializeCommit — byte format
// ---------------------------------------------------------------------------

TEST_F(CommitTest, SerializeTreeLine) {
  Commit c = makeCommit();
  c.tree = fakeHash(0xab);
  auto v = serializeCommit(c);
  std::string raw(v.begin(), v.end());
  EXPECT_EQ(raw.substr(0, 46),
            "tree abababababababababababababababababababab\n");
}

TEST_F(CommitTest, SerializeNoParentLinesForRootCommit) {
  auto v = serializeCommit(makeCommit());
  std::string raw(v.begin(), v.end());
  EXPECT_EQ(raw.find("parent"), std::string::npos);
}

TEST_F(CommitTest, SerializeOneParent) {
  Commit c = makeCommit();
  c.parents.push_back(fakeHash(0xcc));
  std::string raw(serializeCommit(c).begin(), serializeCommit(c).end());
  EXPECT_NE(raw.find("parent cccccccccccccccccccccccccccccccccccccccc\n"),
            std::string::npos);
}

TEST_F(CommitTest, SerializeMultipleParents) {
  Commit c = makeCommit();
  c.parents.push_back(fakeHash(0x11));
  c.parents.push_back(fakeHash(0x22));
  std::string raw(serializeCommit(c).begin(), serializeCommit(c).end());
  EXPECT_NE(raw.find("parent 1111111111111111111111111111111111111111\n"),
            std::string::npos);
  EXPECT_NE(raw.find("parent 2222222222222222222222222222222222222222\n"),
            std::string::npos);
}

TEST_F(CommitTest, SerializeAuthorAndCommitterLines) {
  auto v = serializeCommit(makeCommit());
  std::string raw(v.begin(), v.end());
  EXPECT_NE(raw.find("author A U Thor <author@example.com> 1234567890 +0000\n"),
            std::string::npos);
  EXPECT_NE(raw.find("committer C O Mitter <committer@example.com> 1234567890 +0000\n"),
            std::string::npos);
}

TEST_F(CommitTest, SerializeBlankLineSeparatesHeaderFromMessage) {
  auto v = serializeCommit(makeCommit());
  std::string raw(v.begin(), v.end());
  EXPECT_NE(raw.find("\n\n"), std::string::npos);
}

TEST_F(CommitTest, SerializeMessageAppearsAfterBlankLine) {
  Commit c = makeCommit();
  c.message = "My commit message\n";
  auto v = serializeCommit(c);
  std::string raw(v.begin(), v.end());
  auto sep = raw.find("\n\n");
  ASSERT_NE(sep, std::string::npos);
  EXPECT_EQ(raw.substr(sep + 2), "My commit message\n");
}

TEST_F(CommitTest, SerializeMultiLineMessage) {
  Commit c = makeCommit();
  c.message = "Subject line\n\nBody paragraph.\n";
  auto v = serializeCommit(c);
  std::string raw(v.begin(), v.end());
  auto sep = raw.find("\n\n");
  ASSERT_NE(sep, std::string::npos);
  EXPECT_EQ(raw.substr(sep + 2), "Subject line\n\nBody paragraph.\n");
}

// ---------------------------------------------------------------------------
// parseCommit — known byte sequence
// ---------------------------------------------------------------------------

TEST_F(CommitTest, ParseKnownText) {
  std::string text =
      "tree 0101010101010101010101010101010101010101\n"
      "author A U Thor <author@example.com> 1234567890 +0000\n"
      "committer C O Mitter <committer@example.com> 1234567890 +0000\n"
      "\n"
      "Initial commit\n";

  Commit c = parseCommit(bytes(text));

  EXPECT_EQ(c.tree, fakeHash(0x01));
  EXPECT_TRUE(c.parents.empty());
  EXPECT_EQ(c.author.name, "A U Thor");
  EXPECT_EQ(c.author.email, "author@example.com");
  EXPECT_EQ(c.author.timestamp, 1234567890);
  EXPECT_EQ(c.author.timezone, "+0000");
  EXPECT_EQ(c.committer.name, "C O Mitter");
  EXPECT_EQ(c.committer.email, "committer@example.com");
  EXPECT_EQ(c.message, "Initial commit\n");
}

TEST_F(CommitTest, ParseMultipleParents) {
  std::string text =
      "tree 0101010101010101010101010101010101010101\n"
      "parent 1111111111111111111111111111111111111111\n"
      "parent 2222222222222222222222222222222222222222\n"
      "author A U Thor <author@example.com> 0 +0000\n"
      "committer A U Thor <author@example.com> 0 +0000\n"
      "\n"
      "Merge commit\n";

  Commit c = parseCommit(bytes(text));

  ASSERT_EQ(c.parents.size(), 2u);
  EXPECT_EQ(c.parents[0], fakeHash(0x11));
  EXPECT_EQ(c.parents[1], fakeHash(0x22));
}

TEST_F(CommitTest, ParseUnknownHeadersAreSkipped) {
  // Headers like gpgsig appear in real git repos; tig should ignore them.
  std::string text =
      "tree 0101010101010101010101010101010101010101\n"
      "unknown-header some value\n"
      "author A U Thor <author@example.com> 0 +0000\n"
      "committer A U Thor <author@example.com> 0 +0000\n"
      "\n"
      "message\n";

  EXPECT_NO_THROW(parseCommit(bytes(text)));
  Commit c = parseCommit(bytes(text));
  EXPECT_EQ(c.message, "message\n");
}

TEST_F(CommitTest, ParseEmptyMessage) {
  std::string text =
      "tree 0101010101010101010101010101010101010101\n"
      "author A U Thor <author@example.com> 0 +0000\n"
      "committer A U Thor <author@example.com> 0 +0000\n"
      "\n";

  Commit c = parseCommit(bytes(text));
  EXPECT_EQ(c.message, "");
}

TEST_F(CommitTest, ParseNameWithSpaces) {
  std::string text =
      "tree 0101010101010101010101010101010101010101\n"
      "author First Middle Last <f@example.com> 0 +0000\n"
      "committer First Middle Last <f@example.com> 0 +0000\n"
      "\n";

  Commit c = parseCommit(bytes(text));
  EXPECT_EQ(c.author.name, "First Middle Last");
}

TEST_F(CommitTest, ParseNegativeTimezone) {
  std::string text =
      "tree 0101010101010101010101010101010101010101\n"
      "author A U Thor <a@b.com> 999 -0500\n"
      "committer A U Thor <a@b.com> 999 -0500\n"
      "\n";

  Commit c = parseCommit(bytes(text));
  EXPECT_EQ(c.author.timezone, "-0500");
  EXPECT_EQ(c.author.timestamp, 999);
}

// ---------------------------------------------------------------------------
// Round-trip
// ---------------------------------------------------------------------------

TEST_F(CommitTest, SerializeParseRoundTripRootCommit) {
  Commit original = makeCommit();
  Commit result   = parseCommit(serializeCommit(original));

  EXPECT_EQ(result.tree, original.tree);
  EXPECT_EQ(result.parents, original.parents);
  EXPECT_EQ(result.author.name,      original.author.name);
  EXPECT_EQ(result.author.email,     original.author.email);
  EXPECT_EQ(result.author.timestamp, original.author.timestamp);
  EXPECT_EQ(result.author.timezone,  original.author.timezone);
  EXPECT_EQ(result.committer.name,      original.committer.name);
  EXPECT_EQ(result.committer.email,     original.committer.email);
  EXPECT_EQ(result.committer.timestamp, original.committer.timestamp);
  EXPECT_EQ(result.committer.timezone,  original.committer.timezone);
  EXPECT_EQ(result.message, original.message);
}

TEST_F(CommitTest, SerializeParseRoundTripWithParents) {
  Commit original = makeCommit();
  original.parents = {fakeHash(0x10), fakeHash(0x20), fakeHash(0x30)};

  Commit result = parseCommit(serializeCommit(original));
  EXPECT_EQ(result.parents, original.parents);
}

// ---------------------------------------------------------------------------
// writeCommit / readCommit
// ---------------------------------------------------------------------------

TEST_F(CommitTest, WriteThenReadRoundTrip) {
  Commit original = makeCommit();
  auto hash       = writeCommit(original, tigDir);
  Commit result   = readCommit(hash, tigDir);

  EXPECT_EQ(result.tree, original.tree);
  EXPECT_EQ(result.author.name, original.author.name);
  EXPECT_EQ(result.message, original.message);
}

TEST_F(CommitTest, WriteIsIdempotent) {
  Commit c = makeCommit();
  auto h1  = writeCommit(c, tigDir);
  auto h2  = writeCommit(c, tigDir);
  EXPECT_EQ(h1, h2);
}

TEST_F(CommitTest, ReadNonCommitThrows) {
  // Write a blob and try to read it as a commit.
  auto hash = writeObject(ObjectType::Blob, bytes("data"), tigDir);
  EXPECT_THROW(readCommit(hash, tigDir), std::runtime_error);
}

TEST_F(CommitTest, ReadMissingObjectThrows) {
  std::array<uint8_t, kSha1HashSize> bogus{};
  EXPECT_THROW(readCommit(bogus, tigDir), std::runtime_error);
}

TEST_F(CommitTest, WriteKnownHash) {
  // Verified with: echo -n "..." | git hash-object --stdin -t commit
  // Commit text (177 bytes):
  //   tree 0000000000000000000000000000000000000000\n
  //   author A U Thor <author@example.com> 1234567890 +0000\n
  //   committer C O Mitter <committer@example.com> 1234567890 +0000\n
  //   \n
  //   Initial commit\n
  Commit c;
  c.tree      = fakeHash(0x00);
  c.author    = {"A U Thor", "author@example.com", 1234567890, "+0000"};
  c.committer = {"C O Mitter", "committer@example.com", 1234567890, "+0000"};
  c.message   = "Initial commit\n";

  auto hash = writeCommit(c, tigDir);
  EXPECT_EQ(hashToHex(hash), "272f9012d1252d147bae73b92971a2a3e7486202");
}
