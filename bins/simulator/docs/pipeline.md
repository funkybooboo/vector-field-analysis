# Simulator Pipeline

The simulator runs three sequential stages: parse the config, generate the field, write the output.

```
config.toml
    |
    v
+-------------+     SimulatorConfig
| ConfigParser| -----------------------------------+
+-------------+                                   |
                                                  v
                                        +------------------+     Field::TimeSeries
                                        |  FieldGenerator  | ------------------+
                                        +------------------+                   |
                                                                               v
                                                                     +-----------------+
                                                                     |   FieldWriter   |
                                                                     +-----------------+
                                                                              |
                                                                              v
                                                                          field.h5
```

---

## Stage 1: ConfigParser

**Source:** `configParser.hpp`, `configParser.cpp`

Reads a TOML file using toml++ and produces a `SimulatorConfig` struct.

The TOML file has two sections:

- `[simulation]` -- grid dimensions, time parameters, output path
- `[[layers]]` -- one entry per field layer (TOML array-of-tables)

Each `[[layers]]` block is parsed into a `FieldLayerConfig` struct. If no layers are present
in the file, a single default vortex layer is inserted so the simulator always produces
non-trivial output.

**Key types** (defined in `simulatorConfig.hpp`):

```
SimulatorConfig
  +-- steps, dt, viscosity, output
  +-- width, height, xMin, xMax, yMin, yMax
  +-- layers: []FieldLayerConfig
        +-- type (FieldType enum)
        +-- strength, centerX, centerY
        +-- angle, magnitude          (uniform)
        +-- sinkBlend                 (spiral)
        +-- scale, seed               (noise)
        +-- xExpression, yExpression  (custom)
```

---

## Stage 2: FieldGenerator

**Source:** `fieldGenerator.hpp`, `fieldGenerator.cpp`

Takes a `SimulatorConfig` and produces a `Field::TimeSeries` -- a `frames` vector of
`Field::Slice` grids, where each slice is indexed `[row][col]` and holds a `Vector::Vec2`.
The HDF5 writer splits the Vec2 components into separate `vx` and `vy` datasets on output.

### Setup (runs once before the main loop)

**Coordinate pre-computation** -- physical x/y coordinates for each column and row index
are computed once and stored in `xCoords[]` and `yCoords[]`. This avoids recomputing
`xMin + (xMax - xMin) * col / (width - 1)` inside the inner loop.

**Expression compilation** -- for any layer of type `Custom`, the `x_expression` and
`y_expression` strings are compiled into an `exprtk` expression object. Compilation is
expensive, so it happens here rather than per cell. The variables `x`, `y`, `t` are bound
by reference into the symbol table; updating them before each `eval()` call makes the
compiled expression evaluate at the new point.

### Main loop

```
for each step:
    t = step * dt
    decay = exp(-viscosity * t)          # computed once per step (spatially uniform)

    for each row:
        for each col:
            sum = (0, 0)
            for each layer:
                contribution = eval_layer(px, py, t, layer)
                sum += layer.strength * contribution
            vx[step][row][col] = sum.x * decay
            vy[step][row][col] = sum.y * decay
```

**Layer evaluation** -- each layer type has a dedicated `eval*` function:

| Function | Returns |
|----------|---------|
| `evalVortex` | unit tangent vector (CCW rotation) |
| `evalUniform` | constant vector at fixed angle/magnitude |
| `evalSource` | unit radial vector (outward) |
| `evalSink` | negated source (inward) |
| `evalSaddle` | hyperbolic flow (stretch x, compress y) |
| `evalSpiral` | weighted blend of vortex + sink |
| `evalNoise` | Perlin noise (stb_perlin), independent x/y samples |
| `CustomExpressionEvaluator::eval` | user expression via exprtk |

All radial types guard against division by zero at the center point with
`kSingularityRadius = 1e-6f`, returning a zero vector there.

**Linear superposition** -- contributions from all layers are summed. There is no blending
or normalisation; `strength` is a plain scalar multiplier. Layers with opposing contributions
can cancel each other.

**Viscous decay** -- after summation, both components are multiplied by
`exp(-viscosity * t)`. At `viscosity = 0` the decay factor is always 1.0.

---

## Stage 3: FieldWriter

**Source:** `fieldWriter.hpp`, `fieldWriter.cpp`

Writes the `Field::TimeSeries` and `SimulatorConfig` metadata to an HDF5 file using HighFive.

The file is always opened with `HighFive::File::Overwrite`, so existing output is silently
replaced. All data lives under a single `field` group:

```
field/
  vx         float dataset  [steps][height][width]
  vy         float dataset  [steps][height][width]
  type       string attr    '+'-joined layer type names (informational only)
  steps      int attr
  dt         float attr
  viscosity  float attr
  width      int attr
  height     int attr
  xMin       float attr
  xMax       float attr
  yMin       float attr
  yMax       float attr
```

Metadata is stored as attributes alongside the data so the file is self-contained --
downstream readers (`fieldReader.cpp`, `visualize.py`) do not need the original config file.

---

## Source File Map

| File | Role |
|------|------|
| `main.cpp` | Entry point; orchestrates the three stages |
| `simulatorConfig.hpp` | `SimulatorConfig` and `FieldLayerConfig` struct definitions |
| `configParser.hpp/.cpp` | Parses TOML into `SimulatorConfig` |
| `fieldGenerator.hpp/.cpp` | Evaluates field layers and assembles the time series |
| `fieldWriter.hpp/.cpp` | Writes HDF5 output |
