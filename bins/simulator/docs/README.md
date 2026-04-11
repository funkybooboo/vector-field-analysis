# Simulator Docs

Documentation for the `simulator` binary (`bins/simulator`).

## What It Does

Reads a TOML config file, generates a time-series vector field by superposing one or more
named field layers, and writes the result to an HDF5 file. Each time step applies a global
viscous decay to all vectors.

## Field Generation Algorithms

All types return a `Vector::Vec2` for a given physical coordinate `(px, py)` and time `t`.
Contributions from all layers are summed, then multiplied by the viscous decay scalar.

### Vortex

Rotational field, counter-clockwise. Vectors are unit length everywhere except at the center
singularity, which produces a zero vector.

```
r = sqrt((px - cx)^2 + (py - cy)^2)
v = (-(py - cy) / r,  (px - cx) / r)   if r > kSingularityRadius
v = (0, 0)                               otherwise
```

### Uniform

Constant direction field across the entire grid.

```
v = (magnitude * cos(angle), magnitude * sin(angle))
```

`angle` is in degrees from the positive x-axis.

### Source

Vectors point outward from the center. Unit length everywhere except the singularity.

```
v = ((px - cx) / r,  (py - cy) / r)
```

### Sink

Vectors point inward toward the center. Defined as the negation of Source.

### Saddle

Vectors diverge along x and converge along y (or vice versa), creating a saddle point.

```
v = ((px - cx) / r, -(py - cy) / r)
```

### Spiral

Linear blend between Vortex and Sink, controlled by `sink_blend`:

```
v = (1 - sink_blend) * vortex(px, py) + sink_blend * sink(px, py)
```

`sink_blend = 0` gives a pure vortex; `sink_blend = 1` gives a pure sink.

### Noise

Perlin noise (via stb_perlin), using independent noise samples for x and y components.
The `scale` parameter controls spatial frequency; `seed` shifts the noise pattern.

### Custom

Math expressions evaluated with exprtk. Variables available in expressions: `x`, `y`, `t`.

Example: `x_expression = "-y"`, `y_expression = "x"` produces a vortex-like field.

## Viscous Decay

After all layer contributions are summed, the result is scaled by:

```
decay = exp(-viscosity * t)
```

where `t = step * dt`. At `viscosity = 0`, no decay occurs.

## Output Format

```
field/               (group)
  vx                 (float dataset, shape [steps][height][width])
  vy                 (float dataset, shape [steps][height][width])
  type               (string attribute - '+'-separated layer type names)
  steps              (int attribute)
  dt                 (float attribute)
  viscosity          (float attribute)
  width              (int attribute)
  height             (int attribute)
  xMin               (float attribute)
  xMax               (float attribute)
  yMin               (float attribute)
  yMax               (float attribute)
```

## Usage

```sh
mise run run:bin:simulator                      # run with karman_street.toml (default)
./build/bins/simulator/simulator <config.toml>  # run with a specific config
```

## Contents

- [`pipeline.md`](pipeline.md) -- three-stage internal pipeline, data structures, source file map
- [`config-guide.md`](config-guide.md) -- full config reference, all field types, composition patterns, custom expression tips
- This file -- field generation algorithms, viscous decay formula, output schema
