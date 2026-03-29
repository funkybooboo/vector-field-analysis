# Analyzer Docs

Documentation for the `analyzer` binary (`bins/analyzer`).

## What It Does

Reads a time-series vector field from an HDF5 file, then for each time step constructs a
`FieldGrid` and traces a streamline starting from grid position `(0, 0)`.

## Input Format

The analyzer expects an HDF5 file in the format produced by the simulator:

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

## Algorithms

### `neighborInVectorDirection(row, col)`

Given a grid index `(row, col)`, returns the index of the nearest grid cell in the direction
the vector at that position points. The vector's x-component advances the column index;
the y-component advances the row index. The result is clamped to grid bounds.

### `traceStreamlineStep(startCoords)`

Follows the vector at `startCoords` one step:

1. If the source cell has no streamline, creates one starting at `startCoords`.
2. Finds the destination cell via `neighborInVectorDirection`.
3. If the destination has no streamline, assigns it to the source's streamline and appends
   its coordinates to the path.
4. If the destination already belongs to a different streamline, merges the two via
   `joinStreamlines`.

### `joinStreamlines(start, end)`

Absorbs `end`'s path into `start`. All field vectors that belonged to `end` are redirected
to point to `start`.

## Usage

```sh
mise run run:bin:analyzer                # read field.h5 in the working directory
./build/bins/analyzer/analyzer my.h5    # read a specific file
```

## Contents

- **Input formats** - supported vector field data formats
- **Algorithms** - streamline computation, flow tracing, and streamline merging
- **Usage examples** - example invocations and walkthroughs
