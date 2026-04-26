# Rack

A git-like content-addressable version control system written in C++.

Files → Blobs, Directories → Trees, Commits → Plates. Everything stored by SHA-256 hash under `.rack/objects/`.

## Build

Requires [xmake](https://xmake.io) and OpenSSL.

```sh
xmake              # build client + server
xmake run client   # run client
xmake run server   # run server (default: http://0.0.0.0:8080)
```

## Client Commands

Most commands accept an optional `[project]` to target a project other than the active one.

```
rack commit [-n <name>] [-f <flag>]      Hash & store current files, auto-push if server up
rack push   [project]                    Upload local HEAD to server
rack pull   [project] [-o]               Sync local files to server HEAD (-o: no deletion)

rack log    [project]                    Show plate history (HEAD → root)
rack files  [project]                    List files in latest plate
rack status [project]                    Compare local HEAD vs server HEAD
rack restore <plate-id> [project]        Restore files from a specific plate

rack projects                            List all projects on server
rack init <project>                      Create/activate project on server
rack domain <url>                        Set server URL (saved globally in ~/.rack/config)
rack delete-project                      Delete active project from server (requires confirmation)
  rack delete-project            # deletes active project              
  rack delete-project myproject  # deletes named project               
  # prompts: Delete 'myproject' from server? [y/N]   


rack cat <hash>                          Print object contents by hash
rack ls                                  List all local object hashes
rack reconstruct                         Rebuild files from local HEAD plate
rack serverCheck                         Exit 0 if server reachable
```

### Commit flags

| Flag | Meaning |
|------|---------|
| `-n <name>` | Label for this plate (e.g. `"fix login bug"`) |
| `-f <flag>` | Plate flag, default `Normal` (e.g. `Hotfix`, `Knot`) |

## Architecture

```
.rack/
  objects/<sha256>   content-addressed blobs and trees
  init               HEAD pointer (latest plate hash)
  config             active project name + last server plate ID

~/.rack/
  config             server domain (global, not committed)
```

Server stores data under `data/<project>/` with `blobs/`, `plates/`, and `HEAD`.

## Object Types

| Type | Description |
|------|-------------|
| Blob | Raw file contents, hash = SHA-256(content) |
| Tree | Directory listing of branches (file/dir name + hash pairs) |
| Plate | Commit — parent, tree hash, name, flag |
