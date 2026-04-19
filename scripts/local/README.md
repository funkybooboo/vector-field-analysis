# scripts/local

Scripts for syncing files between your local machine and CHPC. Run these from your local machine.

| Script | Description |
|--------|-------------|
| [`login.sh`](login.sh) | SSH into the CHPC login node |
| [`push.sh`](push.sh) | Rsync local project files to CHPC (excludes `build/`, `output/`, `.env`) |
| [`pull.sh`](pull.sh) | Rsync `output/` and `logs/` from CHPC back to local |

## Configuration

Requires `CHPC_USER`, `CHPC_HOST`, and `CHPC_PROJECT` in `.env` at the project root.
