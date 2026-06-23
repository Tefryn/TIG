# Phase 1 — Object Store

Goal: read and write the four git object types (blob, tree, commit, tag) to disk, using the same on-disk format as real git so that your repository stays compatible with the `git` CLI.

---

## On-Disk Format

Every object is stored at:

```
.git/objects/<xx>/<remaining-38-chars>
```

where `<xx>` is the first two hex characters of the SHA-1 hash and the remaining 38 are the rest. The file content is:

```
zlib( "<type> <size>\0<raw-content>" )
```

For example, a blob containing `hello\n` (6 bytes) is stored as:

```
zlib( "blob 6\0hello\n" )
```

This header-plus-content string is also exactly what you SHA-1 hash to get the object's key.

---

## Hashing

Use **SHA-1** to match git's default. The hash input is always:

```
"<type> <size>\0<raw-content>"
```

Do not hash the zlib-compressed bytes — hash the plaintext header+content.

For Phase 1, link against **OpenSSL** (`libcrypto`) or **libgcrypt** for SHA-1. Alternatively, bundle a single-file public-domain SHA-1 implementation (e.g. the one from git's own source). Avoid rolling your own.

---

## Compression

Use **zlib** (`libz`) for both compression (write) and decompression (read). The default compression level (`Z_DEFAULT_COMPRESSION`) matches what git uses.

---

## Object Types

### Blob

The simplest object — raw file bytes with no additional structure.

```
blob <size>\0<file-bytes>
```

No filename, no permissions. Those live in the tree that references this blob.

### Tree

A sorted list of entries. Each entry is:

```
<mode> <name>\0<20-byte-raw-hash>
```

- **mode** is an ASCII octal string:
  - `100644` — regular file
  - `100755` — executable file
  - `120000` — symbolic link
  - `040000` — subdirectory (points to another tree)
- **name** is the filename (no path separators)
- The null byte terminates the name; the 20 raw bytes follow immediately (not hex-encoded)
- Entries must be sorted: files by name, but directories sort as if their name ends with `/`

### Commit

A text object with a fixed header block followed by a blank line and the message body:

```
tree <tree-sha-hex>\n
parent <parent-sha-hex>\n        ← zero or more; merge commits have two
author <name> <email> <unix-timestamp> <timezone>\n
committer <name> <email> <unix-timestamp> <timezone>\n
\n
<message>\n
```

Example timezone: `+0000`, `-0500`.

### Tag

Similar structure to a commit header:

```
object <sha-hex>\n
type <blob|tree|commit|tag>\n
tag <name>\n
tagger <name> <email> <unix-timestamp> <timezone>\n
\n
<message>\n
```

Annotated tags are full objects; lightweight tags are just refs and have no object. Only implement annotated tags here if you plan to support `tag -a` early.

---

## Core API to Implement

```cpp
// Hash + store an object. Returns the 20-byte raw hash.
std::array<uint8_t, 20> write_object(ObjectType type, std::span<const uint8_t> data,
                                     const std::filesystem::path& git_dir);

// Read an object by hash. Fills type and returns the raw content bytes.
std::vector<uint8_t> read_object(const std::array<uint8_t, 20>& hash, ObjectType type_hint,
                                 const std::filesystem::path& git_dir,
                                 ObjectType& out_type);

// Compute SHA-1 of the header+content without writing to disk.
std::array<uint8_t, 20> hash_object(ObjectType type, std::span<const uint8_t> data);

// Serialize/deserialize each object type.
std::vector<uint8_t>  serialize_blob(std::span<const uint8_t> file_bytes);
Blob                  parse_blob(std::span<const uint8_t> raw);

std::vector<uint8_t>  serialize_tree(const std::vector<TreeEntry>& entries);
std::vector<TreeEntry> parse_tree(std::span<const uint8_t> raw);

std::vector<uint8_t>  serialize_commit(const Commit& c);
Commit                parse_commit(std::span<const uint8_t> raw);
```

---

## Data Structures

```cpp
enum class ObjectType { Blob, Tree, Commit, Tag };

struct TreeEntry {
    uint32_t    mode;   // 0100644, 0100755, 0120000, 0040000
    std::string name;
    std::array<uint8_t, 20> hash;
};

struct Identity {
    std::string name;
    std::string email;
    int64_t     timestamp;  // Unix seconds
    std::string timezone;   // e.g. "+0000"
};

struct Commit {
    std::array<uint8_t, 20>              tree;
    std::vector<std::array<uint8_t, 20>> parents;  // empty for root commit
    Identity                             author;
    Identity                             committer;
    std::string                          message;
};
```

---

## Utility Helpers

These are small but used everywhere — build them first:

| Helper | Notes |
|--------|-------|
| `hex_to_hash(string_view)` | Parse 40-char hex string → 20-byte array |
| `hash_to_hex(array)` | 20-byte array → 40-char lowercase hex string |
| `object_path(git_dir, hash)` | Build the `.git/objects/xx/…` path |
| `zlib_compress(span)` | Wrap `deflate` — returns `vector<uint8_t>` |
| `zlib_decompress(span)` | Wrap `inflate` — returns `vector<uint8_t>` |

---

## File Layout for This Phase

```
src/
  objects/
    object_store.hpp   ← write_object / read_object / hash_object
    object_store.cpp
    types.hpp          ← ObjectType, TreeEntry, Commit, Identity
    blob.hpp / .cpp    ← serialize_blob / parse_blob
    tree.hpp / .cpp    ← serialize_tree / parse_tree
    commit.hpp / .cpp  ← serialize_commit / parse_commit
  util/
    sha1.hpp / .cpp    ← thin wrapper around your SHA-1 backend
    zlib.hpp / .cpp    ← thin wrapper around zlib
    hex.hpp / .cpp     ← hex_to_hash / hash_to_hex
```

---

## Verification

After implementing, verify compatibility with the real `git` CLI:

1. Run `git init test-repo && cd test-repo`
2. Use your `hash_object` tool to hash a file and compare its output to `git hash-object <file>`
3. Write a blob with your `write_object`, then read it back with `git cat-file -p <hash>` — output should match the original file
4. Build a tree and commit manually, then run `git log` in the same repo — the commit should appear with correct metadata
5. Conversely, create a commit with real git, then parse it with your `parse_commit` and confirm all fields match

---

## Dependencies

| Library | Purpose | How to pull in |
|---------|---------|----------------|
| `zlib` | Compression | System package (`libz-dev`) or vcpkg/conan |
| `openssl` (libcrypto) | SHA-1 | System package or vcpkg; or bundle a single-file SHA-1 impl |

Build system recommendation: **CMake** with `find_package(ZLIB)` and `find_package(OpenSSL)`.

---

## What Is Deferred

- **Loose object → pack file compaction** (Phase 7)
- **SHA-256 object format** (git's newer default; sha1 is fine for now)
- **Tag objects** (can stub out; not needed until `tag -a`)
- **Symlink blobs** (mode `120000`) — can skip until the full working-tree layer is in place
