# tig — A Git Reimplementation

## Language

**C++** (primary recommendation). Rationale: direct control over binary I/O, SHA-1/SHA-256 hashing, and file system operations with no runtime overhead. Standard alternatives:
- **C#** — easier string/stream handling, good cross-platform support via .NET 8
- **Java** — mature ecosystem, but heavyweight for a CLI tool

---

## How Git Works (Internals Overview)

### Object Store

Git is fundamentally a content-addressable key-value store. Everything is an **object**, stored under `.git/objects/<first-2-bytes-of-hash>/<remaining-38-bytes>`. Objects are zlib-compressed blobs whose SHA-1 (or SHA-256) hash is their key.

There are four object types:

| Type | Description |
|------|-------------|
| **blob** | Raw file content. No filename, no metadata — just bytes. |
| **tree** | A directory listing. Each entry is a `(mode, name, hash)` triple pointing to a blob or another tree. |
| **commit** | A snapshot. Points to one root tree, zero or more parent commits, and carries author/committer metadata plus a message. |
| **tag** | An annotated pointer to another object (usually a commit). |

### The DAG

Commits form a directed acyclic graph (DAG). Each commit records the hash of its parent(s). This makes history immutable: changing any commit changes its hash, which invalidates all descendants.

```
[commit C] ──parent──▶ [commit B] ──parent──▶ [commit A]
     │                      │                      │
  [tree]                 [tree]                 [tree]
  /    \                 /    \                 /    \
blob  blob            blob  blob            blob  blob
```

### Refs

Named pointers into the DAG, stored as text files under `.git/refs/`:
- `refs/heads/<name>` — a branch (mutable pointer to a commit)
- `refs/tags/<name>` — a tag
- `HEAD` — points to the current branch (or directly to a commit in detached-HEAD state)

### Index (Staging Area)

`.git/index` is a binary file that caches the state of the working tree. It maps `(path, mode)` → `(blob hash, stat data)`. `git add` writes blobs to the object store and updates the index; `git commit` turns the index into a tree object.

### Pack Files

For efficiency, git can pack many loose objects into a single `.pack` file with a `.idx` index. Out of scope for an initial implementation.

---

## Command Reference

### Repository Lifecycle

| Command | Purpose |
|---------|---------|
| `init` | Create a new repository. Writes `.git/` with subdirectories `objects/`, `refs/heads/`, `refs/tags/`, and seed files `HEAD`, `config`, `description`. |
| `clone` | Copy a remote repository locally. Fetches all objects and refs, checks out HEAD. Requires network/transfer layer. |

### Staging & Committing

| Command | Purpose |
|---------|---------|
| `add <path>` | Hash file content into a blob object, then write/update the index entry for that path. Supports globbing and `-A` for all changes. |
| `rm <path>` | Remove a path from the index (and optionally the working tree). |
| `commit` | Turn the current index into a tree object, then create a commit object pointing to that tree and the current HEAD commit. Advance HEAD (and the current branch ref) to the new commit. |
| `commit --amend` | Replace the most recent commit with a new one (same tree or modified). Rewrites history — only safe before pushing. |

### Inspection

| Command | Purpose |
|---------|---------|
| `status` | Compare HEAD commit → index → working tree. Report staged changes, unstaged changes, and untracked files. |
| `diff` | Show line-level differences. `git diff` = index vs. working tree. `git diff --staged` = HEAD vs. index. `git diff <a> <b>` = between two commits/trees. |
| `log` | Walk the commit DAG from HEAD (or a given ref), printing commit metadata. Flags like `--oneline`, `--graph`, `--decorate` control output format. |
| `show <object>` | Pretty-print any object: commit with diff, tree listing, raw blob, or tag. |
| `cat-file` | Low-level object inspection: `-t` prints type, `-p` prints content, `-s` prints size. |
| `ls-files` | List paths tracked in the index, optionally with flags for untracked or ignored files. |

### Branching & Merging

| Command | Purpose |
|---------|---------|
| `branch` | List, create, rename, or delete branch refs. Does not switch the working tree. |
| `switch <branch>` / `checkout <branch>` | Move HEAD to a branch (or commit), update the index and working tree to match. |
| `merge <branch>` | Integrate another branch into the current one. Fast-forward if possible; otherwise create a merge commit. Must detect and surface conflicts. |
| `rebase <upstream>` | Replay commits from the current branch on top of upstream, producing a linear history. Each replayed commit gets a new hash. |
| `cherry-pick <commit>` | Apply the diff of a single commit onto HEAD as a new commit. |

### Remote Collaboration

| Command | Purpose |
|---------|---------|
| `remote` | Manage named remote URLs (add, remove, list). Stored in `.git/config`. |
| `fetch <remote>` | Download objects and refs from a remote without merging. Updates `refs/remotes/<remote>/`. |
| `pull` | `fetch` + `merge` (or `rebase` with `--rebase`). |
| `push <remote> <branch>` | Upload local objects and advance a remote ref. Checks for non-fast-forward and rejects unless `--force`. |

### History Rewriting

| Command | Purpose |
|---------|---------|
| `reset <commit>` | Move the current branch pointer (and optionally the index/working tree) to a given commit. `--soft` / `--mixed` / `--hard` control how much state is reset. |
| `revert <commit>` | Create a new commit that undoes the changes introduced by the given commit. Safe for shared history. |
| `stash` | Save dirty working tree state to a temporary ref stack, restore a clean working tree. `stash pop` reapplies. |

### Tagging

| Command | Purpose |
|---------|---------|
| `tag <name>` | Create a lightweight tag (just a ref pointing to a commit). `tag -a` creates an annotated tag object. |

### Plumbing (Low-Level)

| Command | Purpose |
|---------|---------|
| `hash-object` | Compute (and optionally store) the SHA hash of a file as a blob. |
| `write-tree` | Construct a tree object from the current index. |
| `read-tree` | Load a tree object into the index. |
| `commit-tree` | Create a commit object given a tree hash and optional parent(s). |
| `update-ref` | Safely set a ref to a new value. |

---

## Suggested Implementation Phases

1. **Object store** — blob/tree/commit read+write, SHA hashing, zlib compression
2. **Index** — binary format parsing and writing
3. **Plumbing commands** — `hash-object`, `write-tree`, `commit-tree`, `cat-file`
4. **Core porcelain** — `init`, `add`, `status`, `commit`, `log`
5. **Branching** — `branch`, `switch`, `merge` (fast-forward first)
6. **Diff engine** — `diff`, `show`
7. **Remotes** — `fetch`, `push`, `pull` (requires implementing the pack protocol)
8. **Advanced** — `rebase`, `stash`, `cherry-pick`, annotated tags, conflict resolution UI

---

## Key Data Structures

- **Object**: `enum Type { BLOB, TREE, COMMIT, TAG }` + raw byte buffer + SHA hash
- **TreeEntry**: `uint32_t mode`, `std::string name`, `std::array<uint8_t,20> hash`
- **IndexEntry**: path, blob hash, stat cache (`ctime`, `mtime`, `dev`, `ino`, `uid`, `gid`, `size`)
- **Commit**: tree hash, parent hash(es), author/committer identity + timestamp, message
- **Ref**: named pointer — just a 40-char hex string on disk

---

## Deferred / Out of Scope (Initially)

- Pack files and the pack transfer protocol (needed for real `push`/`fetch`)
- Signed commits (GPG/SSH)
- Submodules
- Worktrees
- `.gitattributes` / line-ending normalization
- Sparse checkout
