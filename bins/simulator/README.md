# Simulator

Generates procedural vector fields from a TOML config file and writes them to HDF5.

## Usage

```sh
simulator <config.toml>         # run with a config file
mise run run:simulator      # build and run (uses source_grid_divergent_512x512.toml by default)
```

To visualize the output:

```sh
mise run visualize        # animate field.h5
uv run tools/visualize.py field.h5 --step 0   # single step, more options
```

See [`configs/`](../../configs/) for ready-to-run config files.

## Config File

Simulation parameters live under a `[simulation]` table.
Field layers are `[[layers]]` array entries (TOML array-of-tables syntax).

```toml
[simulation]
steps     = 200      # number of time steps to generate
dt        = 0.01     # time delta between steps (used by viscous decay and custom expressions)
viscosity = 0.5      # decay rate: field magnitude *= exp(-viscosity * t) each step; 0 = no decay
output    = "field.h5"
width     = 64       # grid columns
height    = 64       # grid rows
xmin      = -1.0
xmax      =  1.0
ymin      = -1.0
ymax      =  1.0

# Each [[layers]] block adds one field. The final vector at each cell is the
# weighted sum of all layers, then multiplied by the viscous decay scalar.

[[layers]]
type     = "vortex"
strength = 1.0       # multiplier applied to this layer's contribution

[[layers]]
type      = "uniform"
angle     = 45.0     # degrees from the positive x-axis
magnitude = 0.5

[[layers]]
type     = "source"
center_x = 0.2
center_y = -0.3

[[layers]]
type     = "sink"
center_x = -0.2
center_y =  0.3

[[layers]]
type     = "saddle"
center_x = 0.0
center_y = 0.0

[[layers]]
type       = "spiral"
sink_blend = 0.4     # 0.0 = pure vortex rotation, 1.0 = pure sink attraction

[[layers]]
type  = "noise"
scale = 2.0          # spatial frequency multiplier
seed  = 42           # integer offset shifts the noise pattern

[[layers]]
type         = "custom"
x_expression = "-y"  # exprtk expression; variables: x, y, t
y_expression = "x"
```

See `bins/simulator/configs/` for ready-to-run config files.

## Field Types and Parameters

| Type | Active parameters |
|------|-------------------|
| `vortex` | `strength`, `center_x`, `center_y` |
| `uniform` | `strength`, `angle`, `magnitude` |
| `source` | `strength`, `center_x`, `center_y` |
| `sink` | `strength`, `center_x`, `center_y` |
| `saddle` | `strength`, `center_x`, `center_y` |
| `spiral` | `strength`, `center_x`, `center_y`, `sink_blend` |
| `noise` | `strength`, `scale`, `seed` |
| `custom` | `strength`, `x_expression`, `y_expression` |

Parameters not listed for a type are silently ignored.

## HDF5 Output Schema

```
field/               (group)
  vx                 (float dataset, shape [steps][height][width])
  vy                 (float dataset, shape [steps][height][width])
  type               (string attribute - '+'-separated layer type names, e.g. "vortex+uniform")
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

## Tasks

```sh
mise run build:simulator   # build
mise run test:simulator    # test
mise run run:simulator     # run source_grid_divergent_512x512.toml (writes field.h5)
```

## Dependencies

- [`libs/field`](../../libs/field) - `Vec2` and `Streamline` types
- toml++ v3.4.0 - config parsing
- exprtk 0.0.3 - custom field expression evaluation
- stb_perlin (stb master) - Perlin noise
- HighFive v2.10.0 - HDF5 output
