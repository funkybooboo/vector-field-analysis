# Analyzer Docs

Documentation for the `analyzer` binary (`bins/analyzer`).

## What It Does

Reads a time-series vector field from an HDF5 file and, for each time step, constructs a
`Field::Grid` and traces streamlines across every cell using a selected parallel implementation.
All implementations share a two-pass design: a parallelisable read pass followed by a
sequential merge pass.

## Quick Start

```sh
# Recommended: build, generate field data, and benchmark all impls in one step
mise run run:analyzer         # uses MPI_RANKS for a fair apples-to-apples comparison

# MPI solver only with a custom rank count
MPI_RANKS=8 mise run run:analyzer:mpi

# Or drive manually:
simulator configs/vortex_128x128.toml                        # generate field.h5
analyzer configs/vortex_128x128.toml                         # benchmark all impls (single-process)
mpirun -n 4 analyzer configs/vortex_128x128.toml             # benchmark with MPI active
```

## Input Format

The analyzer reads HDF5 files in the format produced by the simulator:

```
field/               (group)
  vx                 (float dataset, shape [steps][height][width])
  vy                 (float dataset, shape [steps][height][width])
  xMin               (float attribute)
  xMax               (float attribute)
  yMin               (float attribute)
  yMax               (float attribute)
  steps              (int attribute)
  dt                 (float attribute)
  viscosity          (float attribute)
  type               (string attribute)
```

## Contents

- [pipeline.md](pipeline.md) -- data flow, algorithm details, and source file map
- [config-guide.md](config-guide.md) -- all TOML config keys and examples
- [adding-a-solver.md](adding-a-solver.md) -- how to implement and register a new solver
