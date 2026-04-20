# Analyzer Config Guide

An analyzer config is a TOML file with an optional `[analyzer]` table. When the
`[analyzer]` table is absent, struct defaults apply (`solver = "sequential"`, `threads = 0`).
Any simulator config in `configs/` can be passed directly to the analyzer.

```
analyzer <config.toml>
```

---

## `[analyzer]` Reference

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `solver` | string | `"sequential"` | Solver to run (see below) |
| `threads` | int | `0` | Thread count for `openmp`, `pthreads`. `0` = `ANALYZER_THREADS` env var, then `hardware_concurrency`. |
| `cuda_block_size` | int | `256` | CUDA threads per block (1–1024). Only used when `solver = "cuda"`. |
| `output` | string | `""` | Path for output `streams.h5`. Empty = `data/<stem>/streams.h5`. |
| `timing_output` | string | `""` | Path to write timing results. Empty = no timing file written. |

I/O paths are derived from the config filename stem at runtime when not explicitly set.
The analyzer reads `data/<stem>/field.h5` and writes `data/<stem>/streams.h5`.

---

## `solver` Values

| Value | Description |
|-------|-------------|
| `"sequential"` | Single-threaded reference implementation (default) |
| `"openmp"` | Shared memory, OpenMP parallelism |
| `"pthreads"` | Shared memory, manual pthread parallelism |
| `"mpi"` | Distributed memory CPU via MPI (see MPI usage below) |
| `"cuda"` | Full GPU solver using union-find on CUDA (requires `-DENABLE_CUDA=ON` at build time) |
| `"cudaMpi"` | Hybrid GPU + distributed memory (requires `-DENABLE_CUDA=ON` and `mpirun`) |

The pipeline scripts (`scripts/local/pipeline.sh`, `scripts/chpc/pipeline-job.sh`) run all
implementations automatically by injecting a fresh `[analyzer]` section for each solver
variant. Use `timings.sh` (project root) to read the resulting timing files and print a
speedup report.

---

## MPI Usage

The `mpi` and `cudaMpi` solvers require all ranks to call `computeTimeStep` together.
Run with `mpirun` (local) or `srun` (CHPC/Slurm):

```bash
# Local
mpirun -n 4 analyzer configs/source_grid_divergent_512x512.toml  # with solver = "mpi" in config

# CHPC via Slurm
srun -n 4 analyzer configs/source_grid_divergent_512x512.toml
```

Each rank reads the field file independently (works on shared filesystems like Lustre).
Rank 0 performs the sequential merge pass and prints results.

---

## Examples

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

### Custom output paths

```toml
[analyzer]
solver         = "openmp"
threads        = 4
output         = "data/vortex_256x256/streams_openmp.h5"
timing_output  = "data/vortex_256x256/timing_openmp_4t.txt"
```
