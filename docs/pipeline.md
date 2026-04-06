# Simulator Pipeline

The simulator runs three sequential stages: `ConfigParser` -> `FieldGenerator` -> `FieldWriter`.

---

## Stage 1: ConfigParser

**Entry point:** `ConfigParser::parseFile(path)` -> `SimulatorConfig`

Parses a TOML file into a `SimulatorConfig` struct. All keys are optional and fall back to
defaults if absent.

**`[simulation]` keys:**

| Key | Default | Description |
|-----|---------|-------------|
| `steps` | `100` | Number of time steps to generate |
| `dt` | `0.01` | Time increment per step (advances `t` in expressions and decay) |
| `viscosity` | `0.0` | Exponential decay coefficient (0 = no damping) |
| `output` | `"field.h5"` | Output HDF5 file path |
| `width` / `height` | `64` | Grid dimensions in cells |
| `xmin`/`xmax`/`ymin`/`ymax` | `+/-1.0` | Physical coordinate bounds |

**`[[layers]]` keys:**

| Key | Default | Applies to |
|-----|---------|------------|
| `type` | `"vortex"` | All types |
| `strength` | `1.0` | All types (per-layer weight) |
| `center_x` / `center_y` | `0.0` | Radial types |
| `angle` | `0.0` | `uniform` |
| `magnitude` | `1.0` | `uniform` |
| `sink_blend` | `0.5` | `spiral` (0 = pure rotation, 1 = pure sink) |
| `scale` | `1.0` | `noise` (spatial frequency) |
| `seed` | `0` | `noise` |
| `x_expression` / `y_expression` | `""` | `custom` (exprtk math strings) |

If no `[[layers]]` are defined, a single default vortex layer is used.

---

## Stage 2: FieldGenerator

**Entry point:** `FieldGenerator::generateTimeSeries(config)` -> `FieldTimeSeries`

Produces two 3-D arrays, `vx` and `vy`, each shaped `[steps][height][width]`.

### Coordinate mapping

Grid indices are mapped to physical coordinates once before the main loop:

```
px = xMin + (xMax - xMin) * col / (width - 1)
py = yMin + (yMax - yMin) * row / (height - 1)
```

### Custom expression compilation

`custom` layers use [exprtk](https://github.com/ArashPartow/exprtk) to evaluate
user-supplied math strings at runtime. Expressions are compiled **once** before the
time-step loop (compilation is expensive) and then evaluated per cell by updating the
bound `x`, `y`, `t` variables.

### Main loop

For each `step` in `[0, steps)`:

1. Compute `time = step * dt`.
2. Compute decay scalar: `decay = exp(-viscosity * time)`.
3. For each grid cell `(row, col)`, sum contributions from all layers:
   ```
   sum = sum(layer.strength * eval_layer(px, py, time))
   ```
4. Write `sum * decay` into `vx[step][row][col]` and `vy[step][row][col]`.

Layer order does not affect the result (linear superposition).

### Layer evaluation functions

| Type | Formula |
|------|---------|
| `vortex` | Unit tangent perpendicular to radius: `(-dy/r, dx/r)` |
| `uniform` | `(magnitude*cos(angle), magnitude*sin(angle))` |
| `source` | Outward radial unit vector: `(dx/r, dy/r)` |
| `sink` | Inward radial unit vector: `(-dx/r, -dy/r)` |
| `saddle` | Hyperbolic: `(dx/r, -dy/r)` |
| `spiral` | Blend of vortex and sink: `(1-sinkBlend)*vortex + sinkBlend*sink` |
| `noise` | Stb Perlin noise sampled at `(px*scale, py*scale, time)` for each component |
| `custom` | exprtk evaluation of `x_expression` and `y_expression` |

All radial types (`vortex`, `source`, `sink`, `saddle`, `spiral`) zero out vectors within
radius `1e-6` of the center to avoid the singularity at `r = 0`.

---

## Stage 3: FieldWriter

**Entry point:** `FieldWriter::write(field, config)`

Writes the output HDF5 file using HighFive. The file layout is:

```
field.h5
field/               (HDF5 group)
    vx               (dataset: float32 [steps][height][width])
    vy               (dataset: float32 [steps][height][width])
    attributes:
        type         string  e.g. "vortex+noise"
        steps        int
        dt           float
        viscosity    float
        width        int
        height       int
        xMin/xMax    float
        yMin/yMax    float
```

All grid geometry and simulation parameters are stored as attributes alongside the data,
so the HDF5 file is fully self-contained -- downstream tools (`analyzer`, `visualize.py`,
`field_math.py`) need only the `.h5` file, not the original TOML config.
