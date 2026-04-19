# scripts/local

Scripts for running the pipeline locally and syncing files with CHPC. Run these from your local machine.

| Script | Description |
|--------|-------------|
| [`pipeline.sh`](pipeline.sh) | Run all (or named) configs through the full pipeline: simulator → analyzer → stats → visualizer |
| [`login.sh`](login.sh) | SSH into the CHPC login node |
| [`push.sh`](push.sh) | Rsync local project files to CHPC (excludes `build/`, `output/`, `.env`) |
| [`pull.sh`](pull.sh) | Rsync `data/` from CHPC back to local |

## Configuration

Requires `CHPC_USER`, `CHPC_HOST`, and `CHPC_PROJECT` in `.env` at the project root.
