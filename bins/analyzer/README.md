# Analyzer

Reads a time-series vector field from HDF5 and traces streamlines through every cell in
each time step's field. Supports six solver implementations: sequential, openmp, pthreads,
mpi, cuda, and cudaMpi.

## Usage

```sh
analyzer <config.toml>                                               # run with a config file
mpirun -n 4 analyzer configs/source_grid_divergent_512x512.toml     # run with MPI solver
mise run run:analyzer                                                # build + simulate + run (recommended)
```

The `[analyzer]` table in the config is optional -- defaults to `solver = "sequential"`.
The pipeline scripts run all implementations automatically and write per-solver timing files
to `data/<stem>/`. Run `./timings.sh` to print a speedup report from those files.

## What It Does

1. Reads `vx[steps][height][width]` and `vy[steps][height][width]` from an HDF5 file
2. For each time step, constructs a `Field::Grid` from that step's 2D vector slice
3. Seeds every grid cell and traces streamlines using the selected solver (sequential,
   openmp, pthreads, mpi, cuda, or cudaMpi); merges converging paths across the entire field
4. Writes the traced streamlines to `streams.h5` (name derived from config filename stem)
5. If `timing_output` is set in the config, writes elapsed milliseconds to that file

## Dependencies

- [`libs/field`](../../libs/field) - `Vec2`, `Grid`, `Streamline`, and related types
- HighFive v2.10.0 - HDF5 input/output
- OpenMPI - MPI runtime for the distributed-memory solver

## Tasks

```sh
mise run build:analyzer       # build
mise run test:analyzer        # test
mise run run:analyzer         # run simulator then run analyzer with sequential solver
```
