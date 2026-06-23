#include "objects/types.hpp"

#include <stdexcept>

std::string_view typeName(ObjectType type) {
  switch (type) {
  case ObjectType::Blob:
    return "blob";
  case ObjectType::Tree:
    return "tree";
  case ObjectType::Commit:
    return "commit";
  case ObjectType::Tag:
    return "tag";
  }
  throw std::runtime_error("unknown ObjectType");
}

ObjectType parseTypeName(std::string_view s) {
  if (s == "blob")
    return ObjectType::Blob;
  if (s == "tree")
    return ObjectType::Tree;
  if (s == "commit")
    return ObjectType::Commit;
  if (s == "tag")
    return ObjectType::Tag;
  throw std::runtime_error("unknown object type: " + std::string(s));
}
