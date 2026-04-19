# scripts

Helper scripts for local development and CHPC cluster workflows.

## Subdirectories

| Directory | Description |
|-----------|-------------|
| [`local/`](local) | Run the pipeline locally and sync files between local machine and CHPC |
| [`chpc/`](chpc) | Build, submit, and monitor jobs on the CHPC cluster |

## Configuration

All scripts read from a `.env` file at the project root. Copy `.env.example` if it exists, then fill in your values. Variables can also be exported before running any script.
