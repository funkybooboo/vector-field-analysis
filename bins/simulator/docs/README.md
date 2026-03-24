# Simulator Docs

Documentation for the `simulator` binary (`bins/simulator`).

## What It Does

Generates a 64x64 vortex vector field over `[-1, 1] x [-1, 1]` and writes it to an HDF5 file (`field.h5` by default).

## Generation Algorithm

At each grid point `(x, y)` the vortex function computes a unit vector perpendicular to the radius vector:

```
r = sqrt(x^2 + y^2)
v = (-y/r, x/r)   # rotates counter-clockwise
v = (0, 0)        # at the origin (r ~ 0)
```

This produces a purely rotational field with unit-magnitude vectors everywhere except the origin.

## Output Format

Writes an HDF5 file with the following layout:

```
field/          (group)
  vx            (2D float dataset, shape [height][width])
  vy            (2D float dataset, shape [height][width])
  width         (int attribute)
  height        (int attribute)
  xMin          (float attribute)
  xMax          (float attribute)
  yMin          (float attribute)
  yMax          (float attribute)
```

## Usage

```sh
mise run run:bin:simulator               # write field.h5 in the current directory
./build/bins/simulator/simulator my.h5  # write to a specific file
```

## Contents

- **Generation algorithms** -> how vector fields are procedurally constructed
- **Configuration** -> parameters controlling field dimensions and bounds
- **Output format** -> HDF5 layout consumed by the analyzer
- **Usage examples** -> example invocations and generated field walkthroughs
