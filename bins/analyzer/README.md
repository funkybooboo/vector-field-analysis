# Analyzer

Reads a time-series vector field from HDF5 and traces streamlines through each step.

## Usage

```sh
analyzer field.h5            # run with a specific file
mise run run:bin:analyzer    # build and run (reads field.h5 in the working directory)
```

## What It Does

1. Reads `vx[steps][height][width]` and `vy[steps][height][width]` from an HDF5 file
2. For each time step, constructs a `FieldGrid` from that step's 2D vector slice
3. Calls `traceStreamlineStep` to follow one vector and connect it to a streamline

## Dependencies

- [`libs/vector`](../../libs/vector) - `Vec2` and `Streamline` types
- HighFive v2.10.0 - HDF5 input

## Tasks

```sh
mise run build:bin:analyzer   # build
mise run test:bin:analyzer    # test
mise run run:bin:analyzer     # run (reads field.h5)
```
