# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Rack is a learning-stage, git-like content-addressable store written in C++ using OpenSSL for SHA-256 hashing. Git concepts map as: `Blob` = file contents, `tree` = directory listing of branches, `Plate` = commit. The `.rack/` directory (created in the process CWD on first run) holds the store: `.rack/objects/<hash>` for all objects and `.rack/init` as the single HEAD pointer to the latest `Plate`.

## Build & Run

Build system is [xmake](https://xmake.io). OpenSSL is fetched via `add_requires("openssl")`.

```
xmake                 # build all targets
xmake f -m debug      # switch to debug mode (release is default)
xmake run client      # run client binary
xmake run server      # run server binary (currently empty)
xmake run -d client   # run under debugger
```

Targets defined in `xmake.lua`:
- `client` — `src/Client/main.cpp` + `src/Common/*.cpp`, links OpenSSL.
- `server` — `src/Server/main.cpp` + `src/Common/*.cpp`, links OpenSSL. `main.cpp` is currently empty.
- `Rack` — legacy target over `src/*.cpp`; also currently empty. Prefer `client`/`server`.

No test target, linter, or formatter configured yet.

## Architecture

Three object types, all content-addressed by SHA-256 hex digest (see `src/Common/hash.cpp`):

- **`Blob`** (`model.h`) — wraps raw file content; hash is `SHA256(content)`.
- **`branch`** (`model.h`) — one entry in a tree: `{isDirectory, name, hash}`. Points at either a sub-`tree` or a `Blob`.
- **`tree`** (`model.h`) — ordered list of `branch`es. Serialized as `[Dir|File][Hash]<h> [Name]<n>\n` per line; hash is SHA-256 of that serialization.
- **`Plate`** (`model.h`) — commit record with `parent_hash`, `init_tree_hash`, `timeStamp`. NOTE: hash is currently a string concatenation, not a SHA; `seralize()` omits the timestamp — both are known rough edges (see `main.cpp:60-61` for UB ordering).

Client flow in `src/Client/main.cpp` (`Rack` class):
1. Constructor ensures `.rack/objects/` and `.rack/init` exist in CWD.
2. `createBlobFile(content)` — hash → write `objects/<hash>`.
3. `createTreeFile(path)` — recursive `fs::directory_iterator` over `path`, builds `branch`es, writes tree object. Walks CWD including `.rack/` itself, which is a known bug.
4. `createPlateFileInitFile()` — reads current HEAD from `.rack/init`, builds a new `Plate` over `fs::current_path()`, writes the plate object, rewrites `.rack/init` with the new plate hash.
5. `readFile(hash)` — reads `.rack/init` when `hash == "init"`, otherwise `objects/<hash>`.

Hashing is via `EVP_Digest*` in `src/Common/hash.cpp`, returning a 64-char lowercase hex string.

## Conventions / Gotchas

- `src/Common/` is the shared header+impl area; both `client` and `server` targets include it explicitly in `xmake.lua`. Anything cross-binary goes here.
- Object files are opened `std::ios::binary`, but `createTreeFile` opens source files in text mode — a real bug for cross-platform reproducibility.
- `fs::directory_iterator` order is filesystem-defined; tree hashes are therefore not yet deterministic across machines.
- Spelling inside the code: `seralize` (not `serialize`), `branchs` (not `branches`). Match existing spelling when editing until a rename lands.
- The store lives relative to the process CWD (`.rack/` under wherever you run from), not relative to the binary or project root.
