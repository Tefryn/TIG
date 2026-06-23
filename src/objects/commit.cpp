#include "objects/commit.hpp"

#include "objects/object_store.hpp"
#include "utilities/hex.hpp"

#include <stdexcept>

// Serializes an Identity to git's "name <email> timestamp timezone" format.
static std::string serializeIdentity(const Identity &id) {
  return id.name + " <" + id.email + "> " +
         std::to_string(id.timestamp) + " " + id.timezone;
}

// Parses git's "name <email> timestamp timezone" format.
// Parses right-to-left so names containing spaces are handled correctly.
static Identity parseIdentity(std::string_view s) {
  auto tzSep = s.rfind(' ');
  if (tzSep == std::string_view::npos)
    throw std::runtime_error("corrupt identity: missing timezone");
  std::string timezone(s.substr(tzSep + 1));
  s = s.substr(0, tzSep);

  auto tsSep = s.rfind(' ');
  if (tsSep == std::string_view::npos)
    throw std::runtime_error("corrupt identity: missing timestamp");
  int64_t timestamp = std::stoll(std::string(s.substr(tsSep + 1)));
  s = s.substr(0, tsSep);

  // s is now "name <email>"
  auto gt = s.rfind('>');
  auto lt = s.rfind('<');
  if (gt == std::string_view::npos || lt == std::string_view::npos || lt > gt)
    throw std::runtime_error("corrupt identity: malformed email");

  std::string email(s.substr(lt + 1, gt - lt - 1));
  std::string name(lt > 0 ? s.substr(0, lt - 1) : "");

  return {std::move(name), std::move(email), timestamp, std::move(timezone)};
}

std::vector<uint8_t> serializeCommit(const Commit &commit) {
  std::string out;
  out += "tree " + hashToHex(commit.tree) + '\n';
  for (const auto &parent : commit.parents)
    out += "parent " + hashToHex(parent) + '\n';
  out += "author " + serializeIdentity(commit.author) + '\n';
  out += "committer " + serializeIdentity(commit.committer) + '\n';
  out += '\n';
  out += commit.message;
  return {out.begin(), out.end()};
}

Commit parseCommit(std::span<const uint8_t> raw) {
  std::string text(raw.begin(), raw.end());
  Commit commit;
  size_t pos = 0;

  while (pos < text.size()) {
    size_t end = text.find('\n', pos);
    std::string line = (end == std::string::npos) ? text.substr(pos)
                                                   : text.substr(pos, end - pos);
    pos = (end == std::string::npos) ? text.size() : end + 1;

    if (line.empty()) break; // blank line — message follows

    if (line.starts_with("tree "))
      commit.tree = hexToHash(line.substr(5));
    else if (line.starts_with("parent "))
      commit.parents.push_back(hexToHash(line.substr(7)));
    else if (line.starts_with("author "))
      commit.author = parseIdentity(line.substr(7));
    else if (line.starts_with("committer "))
      commit.committer = parseIdentity(line.substr(10));
    // unknown headers silently skipped for forward compatibility
  }

  commit.message = text.substr(pos);
  return commit;
}

std::array<uint8_t, kSha1HashSize> writeCommit(const Commit &commit,
                                               const std::filesystem::path &tigDir) {
  return writeObject(ObjectType::Commit, serializeCommit(commit), tigDir);
}

Commit readCommit(const std::array<uint8_t, kSha1HashSize> &hash,
                  const std::filesystem::path &tigDir) {
  ObjectType type;
  auto raw = readObject(hash, tigDir, type);
  if (type != ObjectType::Commit)
    throw std::runtime_error("object is not a commit: " +
                             std::string(typeName(type)));
  return parseCommit(raw);
}
