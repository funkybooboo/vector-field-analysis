# Analyzer Docs

Documentation for the `analyzer` binary (`bins/analyzer`).

## What It Does

Reads a vector field from an HDF5 file (`field.h5` by default), constructs a `VectorField`, then calls `flowFromVector` to trace a streamline starting from grid position `(0, 0)`.

## Input Format

The analyzer expects an HDF5 file with the following layout:

```
field/          (group)
  vx            (2D float dataset, shape [height][width])
  vy            (2D float dataset, shape [height][width])
  xMin          (float attribute)
  xMax          (float attribute)
  yMin          (float attribute)
  yMax          (float attribute)
```

This is the format produced by the simulator.

## Algorithms

### `pointsTo(x, y)`

Given a grid index `(x, y)`, returns the index of the nearest neighbour in the direction the vector at that cell points. Accounts for the physical coordinate spacing derived from the field bounds.

### `flowFromVector(startCoords)`

Traces a streamline starting from `startCoords`:
1. If the source vector has no streamline, creates one.
2. Finds the destination cell via `pointsTo`.
3. If the destination has no streamline, adds it to the source's streamline.
4. If the destination already belongs to a different streamline, merges the two via `mergeStreamLines`.

## Usage

```sh
mise run run:bin:analyzer              # read field.h5 in the current directory
./build/bins/analyzer/analyzer my.h5  # read a specific file
```

## Contents

- **Input formats** -> supported vector field data formats
- **Algorithms** -> streamline computation, flow tracing, and streamline merging
- **Usage examples** -> example invocations and walkthroughs
