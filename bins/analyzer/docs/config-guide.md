# Analyzer Config Guide

An analyzer config is a TOML file with an optional `[analyzer]` table. When the
`[analyzer]` table is absent, struct defaults apply (`solver = "all"`, `threads = 0`).
Any simulator config in `configs/` can be passed directly to the analyzer.

```
analyzer <config.toml>
```

---

## `[analyzer]` Reference

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `solver` | string | `"all"` | Solver to run (see below) |
| `threads` | int | `0` | Thread count for `openmp` and `pthreads`. `0` = `ANALYZER_THREADS` env var, then `hardware_concurrency`. |

I/O paths are derived from the config filename stem at runtime -- they are not stored in
the config. The analyzer reads `data/<stem>/field.h5` and writes `data/<stem>/streams.h5`.

---

## `solver` Values

| Value | Description |
|-------|-------------|
| `"all"` | Benchmark all solvers and print timings side-by-side (default) |
| `"sequential"` | Single-threaded reference implementation |
| `"openmp"` | Shared memory, OpenMP parallelism |
| `"pthreads"` | Shared memory, manual pthread parallelism |
| `"mpi"` | Distributed memory CPU via MPI (see MPI usage below) |

---

## MPI Usage

The `mpi` solver requires all ranks to call `computeTimeStep` together.
Run with `mpirun` (local) or `srun` (CHPC/Slurm):

```bash
# Local -- all solvers with fair comparison (thread count adapts to rank count)
mpirun -n 4 analyzer configs/karman_street_128x64.toml

# CHPC via Slurm
srun -n 4 analyzer configs/karman_street_128x64.toml
```

Each rank reads the field file independently (works on shared filesystems like Lustre).
Rank 0 performs the sequential merge pass and prints results.

### Fair Comparison in `solver = "all"` Mode

When the binary is launched under `mpirun` with N ranks, the thread count for `openmp` and
`pthreads` is automatically aligned to N so all parallel solvers use the same number of workers.
If the binary is invoked without `mpirun` (single-rank), the MPI solver is skipped and a hint
is printed instead of producing a misleading result.

`mise run run:analyzer` handles this automatically using `MPI_RANKS` (see `.env.example`).

---

## Examples

### Benchmark all solvers

```toml
[analyzer]
solver = "all"
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
