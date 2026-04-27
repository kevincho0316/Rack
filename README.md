# Rack

A git-like content-addressable version control system written in C++.

Files → Blobs, Directories → Trees, Commits → Plates. Everything stored by SHA-256 hash under `.rack/objects/`.

## Build

Requires [xmake](https://xmake.io) and OpenSSL.

```sh
xmake              # build client + server
xmake run client   # run client
xmake run server   # run server (default: http://0.0.0.0:8080)
xmake f -m debug   # switch to debug mode
xmake f -m release # switch to release mode
```

## Running the Server

```sh
xmake run server
```

Server listens on `http://0.0.0.0:8080` by default. To enable API key auth (required for a public server):

```sh
RACK_API_KEY=mysecretkey xmake run server
```

When a key is set, all routes except `/hello` require `Authorization: Bearer <key>`. The `/hello` endpoint remains public so `rack serverCheck` works without credentials.

## Client Commands

Most commands accept an optional `[project]` to target a project other than the active one.

```
rack commit [-n <name>] [-f <flag>]      Hash & store current files, auto-push if server up
rack push   [project]                    Upload local HEAD to server
rack pull   [project] [-o]               Sync local files to server HEAD (-o: no deletion)

rack log    [project]                    Show plate history with upload timestamps
rack files  [project]                    List files in latest plate
rack status [project]                    Compare local HEAD vs server HEAD
rack restore <plate-id> [project]        Restore files from a specific plate

rack diff                                Diff local HEAD vs server HEAD
rack diff <plate-id>                     Diff local HEAD vs specific plate
rack diff <plate-id-a> <plate-id-b>      Diff two server plates

rack projects                            List all projects on server
rack init <project>                      Create/activate project on server
rack domain <url>                        Set server URL (saved globally in ~/.rack/config)
rack auth <key>                          Set API key (saved globally in ~/.rack/config)
rack delete-project [project] [-y]       Delete project from server (-y skips confirmation)

rack checkout <url> <project>            Set domain + init + pull in one step (fresh device setup)
rack cat <hash>                          Print object contents by hash
rack ls                                  List all local object hashes
rack reconstruct                         Rebuild files from local HEAD plate
rack serverCheck                         Exit 0 if server reachable
```

Plate IDs can be shortened to any unique prefix (e.g. `931121fa8fb5`).

### Commit flags

| Flag | Meaning |
|------|---------|
| `-n <name>` | Label for this plate (e.g. `"fix login bug"`) |
| `-f <flag>` | Plate flag, default `Normal` (e.g. `Hotfix`, `Knot`) |

Name/flag-only commits (no file changes) are allowed — useful for relabeling.

## Authentication

The server uses a single shared API key loaded from the `RACK_API_KEY` environment variable. If the variable is not set, the server runs open with no auth.

**Server setup:**
```sh
export RACK_API_KEY=mysecretkey
xmake run server
```

**Client setup:**
```sh
rack auth mysecretkey
```

The key is saved to `~/.rack/config` (global, never inside any repository). All HTTP calls except `serverCheck` automatically include `Authorization: Bearer <key>`.

**Key never leaks to GitHub because:**
- Server key lives only in your environment (`RACK_API_KEY`), never written to disk
- Client key lives in `~/.rack/config` (home directory, outside any repo)
- `data/` (server storage) and `.rack/` (local store) are both in `.gitignore`

## Fresh Device Setup

On a new machine, a single command sets the domain, activates the project on the server, and pulls all files:

```sh
rack auth mysecretkey
rack checkout http://yourserver.com:8080 myproject
```

Or with the Neovim plugin: `:RackAuth` then `:RackCheckout`.

## Color Output

Client and server output use ANSI colors automatically when connected to a terminal. Colors are suppressed when output is piped (e.g. when called from the Neovim plugin), so parsing is unaffected.

| Output | Color |
|--------|-------|
| `[Hotfix]` flag | Red |
| `[Knot]` flag | Magenta |
| Named plate | Yellow |
| `<- HEAD` marker | Bold green |
| Diff `+` lines | Green |
| Diff `-` lines | Red |
| `@@ ... @@` hunks | Cyan |
| Server `GET` | Cyan |
| Server `POST` | Green |
| Server `DELETE` | Red |
| Server 4xx status | Yellow |
| Server 5xx status | Bold red |

Requires a terminal with ANSI support (Windows Terminal, any modern Linux terminal). On Windows, VT processing is enabled automatically.

## Architecture

```
.rack/
  objects/<sha256>   content-addressed blobs and trees
  init               HEAD pointer (latest plate hash)
  config             active project name + last server plate ID

~/.rack/
  config             server domain + API key (global, never committed)
```

Server stores data under `data/<project>/` with `blobs/`, `plates/`, and `HEAD`.

## Object Types

| Type | Description |
|------|-------------|
| Blob | Raw file contents, hash = SHA-256(content) |
| Tree | Directory listing of branches (file/dir name + hash pairs) |
| Plate | Commit — parent, tree hash, name, flag, upload timestamp |
