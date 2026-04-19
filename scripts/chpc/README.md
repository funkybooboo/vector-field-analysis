# scripts/chpc

Scripts for running jobs on the University of Utah CHPC cluster. Run all of these from a CHPC login node unless noted otherwise.

See [`docs/chpc-reference.md`](../../docs/chpc-reference.md) for cluster setup, partitions, and GPU availability.

## Scripts

| Script | Where to run | Description |
|--------|-------------|-------------|
| [`check.sh`](check.sh) | Login node | Format, lint, and build check before requesting a GPU allocation |
| [`gpu-shell.sh`](gpu-shell.sh) | Login node | Request an interactive GPU session via `srun` |
| [`cpu-shell.sh`](cpu-shell.sh) | Login node | Request an interactive CPU session via `srun` |
| [`submit.sh`](submit.sh) | Login node | Build and submit a single batch job for one GPU target |
| [`benchmark.sh`](benchmark.sh) | Login node | Build and submit batch jobs across all configured GPU types |
| [`job.sh`](job.sh) | Submitted by SLURM | Batch worker — do not run directly |

## Typical workflow

```sh
# 1. Push local changes to CHPC
./scripts/local/push.sh

# 2. SSH in and run pre-flight checks
./scripts/chpc/check.sh

# 3a. Interactive GPU session (for debugging)
./scripts/chpc/gpu-shell.sh

# 3b. Or submit a batch job
./scripts/chpc/submit.sh

# 3c. Or benchmark across all GPU types
./scripts/chpc/benchmark.sh

# 4. Pull results back to local
./scripts/local/pull.sh
```

## Configuration

All scripts read from `.env` at the project root. Required variables depend on the script — each script validates its own requirements and prints a clear error if something is missing.
