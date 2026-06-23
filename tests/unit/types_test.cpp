#include "objects/types.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

// ---------------------------------------------------------------------------
// typeName
// ---------------------------------------------------------------------------

TEST(TypeName, Blob) { EXPECT_EQ(typeName(ObjectType::Blob), "blob"); }
TEST(TypeName, Tree) { EXPECT_EQ(typeName(ObjectType::Tree), "tree"); }
TEST(TypeName, Commit) { EXPECT_EQ(typeName(ObjectType::Commit), "commit"); }
TEST(TypeName, Tag) { EXPECT_EQ(typeName(ObjectType::Tag), "tag"); }

// ---------------------------------------------------------------------------
// parseTypeName
// ---------------------------------------------------------------------------

TEST(ParseTypeName, Blob) {
  EXPECT_EQ(parseTypeName("blob"), ObjectType::Blob);
}
TEST(ParseTypeName, Tree) {
  EXPECT_EQ(parseTypeName("tree"), ObjectType::Tree);
}
TEST(ParseTypeName, Commit) {
  EXPECT_EQ(parseTypeName("commit"), ObjectType::Commit);
}
TEST(ParseTypeName, Tag) {
  EXPECT_EQ(parseTypeName("tag"), ObjectType::Tag);
}

TEST(ParseTypeName, UnknownThrows) {
  EXPECT_THROW(parseTypeName("unknown"), std::runtime_error);
}

TEST(ParseTypeName, EmptyStringThrows) {
  EXPECT_THROW(parseTypeName(""), std::runtime_error);
}

TEST(ParseTypeName, CaseSensitive) {
  EXPECT_THROW(parseTypeName("Blob"), std::runtime_error);
  EXPECT_THROW(parseTypeName("COMMIT"), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Round-trip
// ---------------------------------------------------------------------------

TEST(TypeNameRoundTrip, AllTypes) {
  for (auto type :
       {ObjectType::Blob, ObjectType::Tree, ObjectType::Commit, ObjectType::Tag})
    EXPECT_EQ(parseTypeName(typeName(type)), type);
}
