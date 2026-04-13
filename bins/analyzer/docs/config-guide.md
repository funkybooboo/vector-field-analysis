# Analyzer Config Guide

An analyzer config is a TOML file with a single `[analyzer]` table.

```
analyzer <config.toml>
```

## Ready-to-Run Configs

| Config | Solver | Notes |
|--------|--------|-------|
| `bins/analyzer/configs/all.toml` | `all` | Benchmark all impls side-by-side (default) |
| `bins/analyzer/configs/sequential.toml` | `sequential` | Single-threaded reference |
| `bins/analyzer/configs/openmp.toml` | `openmp` | Shared memory, OpenMP |
| `bins/analyzer/configs/pthreads.toml` | `pthreads` | Shared memory, pthreads |
| `bins/analyzer/configs/mpi.toml` | `mpi` | Distributed memory, MPI |

---

## `[analyzer]` Reference

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `input` | string | `"field.h5"` | HDF5 field file produced by the simulator |
| `solver` | string | `"all"` | Solver to run (see below) |
| `threads` | int | `0` | Thread count for `openmp` and `pthreads`. `0` = `ANALYZER_THREADS` env var, then `hardware_concurrency`. |

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
# Local -- MPI solver only
mpirun -n 4 analyzer bins/analyzer/configs/mpi.toml

# Local -- all solvers with fair comparison (thread count adapts to rank count)
mpirun -n 4 analyzer bins/analyzer/configs/all.toml

# CHPC via Slurm
srun -n 4 analyzer bins/analyzer/configs/mpi.toml
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
input  = "field.h5"
solver = "all"
```

### Run pthreads with a fixed thread count

```toml
[analyzer]
input   = "field.h5"
solver  = "pthreads"
threads = 8    # overrides ANALYZER_THREADS env var
```

### MPI run

```toml
[analyzer]
input  = "field.h5"
solver = "mpi"
```
