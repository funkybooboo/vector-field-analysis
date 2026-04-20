# Analyzer Docs

Documentation for the `analyzer` binary (`bins/analyzer`).

## What It Does

Reads a time-series vector field from an HDF5 file and, for each time step, constructs a
`Field::Grid` and traces streamlines across every cell using a selected parallel implementation.
All implementations share a two-pass design: a parallelisable read pass followed by a
sequential merge pass.

## Quick Start

```sh
# Recommended: build, generate field data, and run
mise run run:analyzer

# MPI solver with a custom rank count
mpirun -n 8 ./build/bins/analyzer/analyzer configs/vortex_256x256.toml  # with solver = "mpi"

# Or drive manually:
simulator configs/vortex_256x256.toml            # generate field.h5
analyzer configs/vortex_256x256.toml             # run sequential solver

# Run all implementations and compare (pipeline script):
./scripts/local/pipeline.sh vortex_256x256

# Print timing speedup report:
./timings.sh vortex_256x256
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
