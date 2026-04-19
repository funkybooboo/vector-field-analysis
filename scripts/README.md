# scripts

Helper scripts for local development and CHPC cluster workflows.

| Script | Description |
|--------|-------------|
| [`pipeline.sh`](pipeline.sh) | Run all configs through the full pipeline: simulator → analyzer → stats → visualizer |
| [`validate.sh`](validate.sh) | Shared validation helpers sourced by all other scripts |

## Subdirectories

| Directory | Description |
|-----------|-------------|
| [`chpc/`](chpc) | Scripts for submitting and monitoring jobs on the CHPC cluster |
| [`local/`](local) | Scripts for syncing files between local machine and CHPC |

## Configuration

All scripts read from a `.env` file at the project root. Copy `.env.example` if it exists, then fill in your values. Variables can also be exported before running any script.
