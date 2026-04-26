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

```
rack commit                  Hash & store current files, auto-push if server up
rack push                    Upload local HEAD to server
rack pull                    Sync local files to server HEAD (deletes extras)
rack pull -o                 Pull without deleting local-only files

rack log                     Show plate history (HEAD → root)
rack files                   List files in latest server plate
rack status                  Compare local HEAD vs server HEAD
rack restore <plate-id>      Restore files from a specific plate

rack init <project>          Create/activate project on server
rack domain <url>            Set server URL (saved globally in ~/.rack/config)
rack delete-project          Delete active project from server (requires confirmation)

rack cat <hash>              Print object contents by hash
rack ls                      List all local object hashes
rack reconstruct             Rebuild files from local HEAD plate
rack serverCheck             Exit 0 if server reachable
```

## Architecture

```
.rack/
  objects/<sha256>   content-addressed blobs and trees
  init               HEAD pointer (latest plate hash)
  config             active project name

~/.rack/
  config             server domain (global, not committed)
```

Server stores data under `data/<project>/` with `blobs/`, `plates/`, and `HEAD`.

## Object Types

| Type | Description |
|------|-------------|
| Blob | Raw file contents, hash = SHA-256(content) |
| Tree | Directory listing of branches (file/dir name + hash pairs) |
| Plate | Commit record — parent hash + tree hash + timestamp |
