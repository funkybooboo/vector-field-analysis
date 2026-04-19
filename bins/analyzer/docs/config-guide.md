# Analyzer Config Guide

An analyzer config is a TOML file with an optional `[analyzer]` table. When the
`[analyzer]` table is absent, struct defaults apply (`solver = "benchmark"`, `threads = 0`).
Any simulator config in `configs/` can be passed directly to the analyzer.

```
analyzer <config.toml>
```

---

## `[analyzer]` Reference

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `solver` | string | `"benchmark"` | Solver to run (see below) |
| `threads` | int | `0` | Thread count for `openmp`, `pthreads`, `mpi` in single-solver mode. `0` = `ANALYZER_THREADS` env var, then `hardware_concurrency`. |
| `cuda_block_size` | int | `256` | CUDA threads per block (1–1024). Only used when `solver = "cuda"`. |
| `benchmark_threads` | int[] | `[2, 4, 8]` | Thread counts to test in `benchmark` mode for `pthreads` and `openmp`. |
| `benchmark_cuda_block_sizes` | int[] | `[64, 128, 256, 512]` | CUDA block sizes to test in `benchmark` mode. |

I/O paths are derived from the config filename stem at runtime -- they are not stored in
the config. The analyzer reads `data/<stem>/field.h5` and writes `data/<stem>/streams.h5`.

---

## `solver` Values

| Value | Description |
|-------|-------------|
| `"benchmark"` | Run all available implementations, time each, verify vs sequential, print speedup table (default) |
| `"sequential"` | Single-threaded reference implementation |
| `"openmp"` | Shared memory, OpenMP parallelism |
| `"pthreads"` | Shared memory, manual pthread parallelism |
| `"mpi"` | Distributed memory CPU via MPI (see MPI usage below) |
| `"cuda"` | Full GPU solver using union-find on CUDA (requires `-DENABLE_CUDA=ON` at build time) |

---

## MPI Usage

The `mpi` solver requires all ranks to call `computeTimeStep` together.
Run with `mpirun` (local) or `srun` (CHPC/Slurm):

```bash
# Local -- benchmark mode includes MPI when launched with multiple ranks
mpirun -n 4 analyzer configs/source_grid_divergent_512x512.toml

# CHPC via Slurm
srun -n 4 analyzer configs/source_grid_divergent_512x512.toml

# MPI solver only (single-solver mode)
mpirun -n 4 analyzer configs/source_grid_divergent_512x512.toml  # with solver = "mpi" in config
```

Each rank reads the field file independently (works on shared filesystems like Lustre).
Rank 0 performs the sequential merge pass and prints results.

### MPI in `benchmark` Mode

When the binary is launched under `mpirun` with N ranks, all ranks run the MPI benchmark
first (collective operation), then rank 0 runs sequential/pthreads/openmp/cuda.
If launched without `mpirun` (single rank), the MPI benchmark is skipped.

`mise run run:analyzer` handles this automatically using `MPI_RANKS` (see `.env.example`).

---

## Examples

### Benchmark all solvers (default)

```toml
[analyzer]
solver = "benchmark"
benchmark_threads = [2, 4, 8]
benchmark_cuda_block_sizes = [64, 128, 256, 512]
```

### Run pthreads with a fixed thread count

```toml
[analyzer]
solver  = "pthreads"
threads = 8    # overrides ANALYZER_THREADS env var
```

### MPI run

```toml
[analyzer]
solver = "mpi"
```
