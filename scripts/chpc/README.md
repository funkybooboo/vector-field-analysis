# scripts/chpc

Scripts for running jobs on the University of Utah CHPC cluster. Run all of these from a CHPC login node unless noted otherwise.

See [`docs/chpc-reference.md`](../../docs/chpc-reference.md) for cluster setup, partitions, and GPU availability.

## Scripts

| Script | Where to run | Description |
|--------|-------------|-------------|
| [`check.sh`](check.sh) | Login node | Format, lint, and build check before requesting a GPU allocation |
| [`gpu-shell.sh`](gpu-shell.sh) | Login node | Request an interactive GPU session via `srun` |
| [`cpu-shell.sh`](cpu-shell.sh) | Login node | Request an interactive CPU session via `srun` |
| [`pipeline.sh`](pipeline.sh) | Login node | Build and submit batch jobs for all (or named) config stems |
| [`pipeline-job.sh`](pipeline-job.sh) | Submitted by SLURM | Batch worker — do not run directly |
| [`monitor.sh`](monitor.sh) | Login node | Show job queue and recent log output; tail a specific stem live |
| [`stop.sh`](stop.sh) | Login node | Cancel all (or a named stem's) running pipeline jobs |

## Typical workflow

```sh
# 1. Push local changes to CHPC
./scripts/local/push.sh

# 2. SSH in and run pre-flight checks
./scripts/chpc/check.sh

# 3a. Interactive GPU session (for debugging)
./scripts/chpc/gpu-shell.sh

# 3b. Submit all configs through the full pipeline
./scripts/chpc/pipeline.sh

# 3c. Or submit only specific stems
./scripts/chpc/pipeline.sh karman_street_128x64 source_grid_divergent_512x512

# 4. Pull results back to local
./scripts/local/pull.sh
```

## Configuration

All scripts read from `.env` at the project root. Required variables depend on the script — each script validates its own requirements and prints a clear error if something is missing.
