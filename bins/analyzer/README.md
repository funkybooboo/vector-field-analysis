# Analyzer

Reads a time-series vector field from HDF5 and traces streamlines through every cell in
each time step's field. Supports five solver implementations that can be benchmarked
side-by-side.

## Usage

```sh
analyzer <config.toml>                              # run with a config file
mpirun -n 4 analyzer configs/source_grid_divergent_512x512.toml  # MPI solver, 4 ranks
mise run run:analyzer                               # build + simulate + benchmark (recommended)
```

The `[analyzer]` table in the config is optional -- defaults to `solver = "all"`, which
benchmarks all four implementations and prints timings side-by-side.

## What It Does

1. Reads `vx[steps][height][width]` and `vy[steps][height][width]` from an HDF5 file
2. For each time step, constructs a `Field::Grid` from that step's 2D vector slice
3. Seeds every grid cell and traces streamlines using the selected solver (sequential,
   openmp, pthreads, mpi, or cuda); merges converging paths across the entire field
4. Writes the traced streamlines to `streams.h5` (name derived from config filename stem)

## Dependencies

- [`libs/field`](../../libs/field) - `Vec2`, `Grid`, `Streamline`, and related types
- HighFive v2.10.0 - HDF5 input/output
- OpenMPI - MPI runtime for the distributed-memory solver

## Tasks

```sh
mise run build:analyzer       # build
mise run test:analyzer        # test
mise run run:analyzer         # run simulator then benchmark all impls under mpirun -n $MPI_RANKS
mise run run:analyzer:mpi     # MPI solver only (MPI_RANKS defaults to 4; override with MPI_RANKS=N)
```
