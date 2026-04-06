# Config Guide

A simulator config is a TOML file with two sections: a `[simulation]` table for grid and
time settings, and one or more `[[layers]]` blocks that define the field.

The final vector at each grid cell is the **weighted sum** of all layer contributions,
multiplied by the viscous decay scalar. Layer order does not matter.

---

## `[simulation]` Reference

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `steps` | int | `100` | Number of time steps to generate |
| `dt` | float | `0.01` | Time increment per step. Advances `t` in custom expressions and scales viscous decay. Not an ODE step size. |
| `viscosity` | float | `0.0` | Decay rate: `field *= exp(-viscosity * t)` each step. `0` = no decay. |
| `output` | string | `"field.h5"` | Output file path |
| `width` | int | `64` | Grid columns (x-axis resolution) |
| `height` | int | `64` | Grid rows (y-axis resolution) |
| `xmin` | float | `-1.0` | Left physical bound |
| `xmax` | float | `1.0` | Right physical bound |
| `ymin` | float | `-1.0` | Bottom physical bound |
| `ymax` | float | `1.0` | Top physical bound |

---

## `[[layers]]` Semantics

Each `[[layers]]` block is a TOML array-of-tables entry. You can have any number of them.
All layers share the same set of keys; keys that are not relevant to a given type are
silently ignored.

**Common keys (all types):**

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `type` | string | `"vortex"` | Field type (see below) |
| `strength` | float | `1.0` | Scalar multiplier for this layer's contribution |
| `center_x` | float | `0.0` | x-coordinate of the layer's center point |
| `center_y` | float | `0.0` | y-coordinate of the layer's center point |

---

## Field Types

### `vortex`

Pure counter-clockwise rotation. Vectors are unit length everywhere except at the center
singularity (zero vector). Strength controls how strongly it dominates in a superposition.

```
r = sqrt((x - cx)^2 + (y - cy)^2)
v = (-(y - cy) / r,  (x - cx) / r)
```

**Additional parameters:** none beyond `strength`, `center_x`, `center_y`.

```toml
[[layers]]
type     = "vortex"
strength = 1.0
center_x = 0.0
center_y = 0.0
```

---

### `uniform`

Constant direction and magnitude across the entire grid. Good as a background flow or
steering current.

```
v = (magnitude * cos(angle_rad),  magnitude * sin(angle_rad))
```

| Key | Default | Description |
|-----|---------|-------------|
| `angle` | `0.0` | Direction in degrees from the positive x-axis |
| `magnitude` | `1.0` | Vector length |

```toml
[[layers]]
type      = "uniform"
angle     = 45.0     # upper-right diagonal
magnitude = 0.5
```

---

### `source`

Vectors point outward from the center. Unit length everywhere except the singularity.
Use to model outflow, divergence, or emission.

```
v = ((x - cx) / r,  (y - cy) / r)
```

```toml
[[layers]]
type     = "source"
center_x = -1.0
center_y =  0.0
strength =  1.0
```

---

### `sink`

Vectors point inward toward the center (negation of source). Use to model inflow,
convergence, or attraction.

```
v = (-(x - cx) / r,  -(y - cy) / r)
```

```toml
[[layers]]
type     = "sink"
center_x =  1.0
center_y =  0.0
strength =  1.0
```

---

### `saddle`

Hyperbolic flow: stretches along the x-axis, compresses along the y-axis, with two
attracting and two repelling sectors separated by the axes.

```
v = ((x - cx) / r,  -(y - cy) / r)
```

```toml
[[layers]]
type     = "saddle"
center_x = 0.0
center_y = 0.0
```

---

### `spiral`

A convex blend of vortex rotation and sink attraction. Controls the tightness of an
inward spiral.

```
v = (1 - sink_blend) * vortex(x, y) + sink_blend * sink(x, y)
```

| Key | Default | Description |
|-----|---------|-------------|
| `sink_blend` | `0.5` | `0.0` = pure circular vortex, `1.0` = pure inward sink |

```toml
[[layers]]
type       = "spiral"
sink_blend = 0.15    # mostly rotation, slight inward drift
center_x   = 0.0
center_y   = 0.0
```

---

### `noise`

Spatially varying turbulent flow using Perlin noise (stb_perlin). The x and y components
are sampled at different spatial offsets so they are uncorrelated.

| Key | Default | Description |
|-----|---------|-------------|
| `scale` | `1.0` | Spatial frequency multiplier. Higher = finer detail. |
| `seed` | `0` | Integer pattern offset. Different seeds give different noise fields. |

```toml
[[layers]]
type     = "noise"
scale    = 3.0
seed     = 7
strength = 0.3
```

---

### `custom`

Math expressions evaluated with exprtk. Gives full control over the field shape and
time evolution.

| Key | Default | Description |
|-----|---------|-------------|
| `x_expression` | `""` | Expression for the x velocity component |
| `y_expression` | `""` | Expression for the y velocity component |

**Available variables:** `x`, `y`, `t` (physical coordinates and current time).

**Available constants and functions:** all standard exprtk built-ins including `sin`, `cos`,
`exp`, `sqrt`, `abs`, `pi`, `e`, and arithmetic operators.

```toml
[[layers]]
type         = "custom"
x_expression = "-y / (x*x + y*y + 0.1)"   # vortex-like, regularised at origin
y_expression = " x / (x*x + y*y + 0.1)"
```

---

## Layer Composition

### Weighting with `strength`

`strength` is a plain scalar multiplier. To make one layer dominate, give it a higher
strength. To suppress a layer without removing it, set `strength` to a small value.

```toml
[[layers]]
type     = "vortex"
strength = 1.0      # dominant

[[layers]]
type     = "noise"
strength = 0.1      # subtle perturbation
```

### Cancellation

Layers with opposing contributions cancel. A source and a sink of equal strength at the
same location produce a zero field. This is useful for masking regions.

### Source-sink channels

A source on the left and a sink on the right drive flow across the domain in a channel.
Move the center points to position the channel; adjust `strength` to control flow speed.

```toml
[[layers]]
type     = "source"
center_x = -1.5
center_y =  0.5
strength =  1.0

[[layers]]
type     = "sink"
center_x =  1.5
center_y =  0.5
strength =  1.0
```

### Background flow with a feature

Combine a uniform layer for background drift with a vortex or spiral for a localised feature.

```toml
[[layers]]
type      = "uniform"
angle     = 0.0
magnitude = 0.3

[[layers]]
type     = "vortex"
center_x = 0.3
strength = 1.0
```

---

## Time-Varying Fields

`t` advances by `dt` each step: `t = step * dt`. Use it in `custom` expressions to
animate the field.

**Common patterns:**

```toml
# Oscillating speed
x_expression = "cos(t) * exp(-((y)*(y)) / 0.1)"

# Rotating direction
x_expression = "cos(t)"
y_expression = "sin(t)"

# Exponential growth (instability)
y_expression = "0.1 * sin(2.5 * x) * exp(0.1 * t)"

# Pulsing on/off (sin ranges -1 to 1; offset to keep positive)
x_expression = "exp(-((y)*(y)) / 0.1) * (0.5 + 0.5 * sin(t))"
```

---

## Viscosity Guide

Viscosity applies a global `exp(-viscosity * t)` decay, reducing all vectors toward zero
over time. It does not interact with individual layer strengths.

| Value | Effect |
|-------|--------|
| `0.0` | No decay -- field is fully steady (or animated only by custom `t` expressions) |
| `0.01-0.05` | Gentle fade over many steps -- good for long runs |
| `0.1-0.5` | Noticeable decay within the first 50-100 steps |
| `> 1.0` | Field collapses quickly -- meaningful only for very short runs or small `dt` |

At `viscosity = 0.5` and `dt = 0.01`, the field reaches ~37% of its initial magnitude
at step 200 (`t = 2.0`).

---

## Custom Expression Tips

**Division by zero at the origin** -- expressions like `x / (x*x + y*y)` blow up at
`(0, 0)`. Add a small regularisation constant: `x / (x*x + y*y + 0.001)`.

**Gaussian confinement** -- multiply any expression by `exp(-((y - c)*(y - c)) / w)` to
confine it to a horizontal band centred at `c` with width `~sqrt(w)`.

**Differential rotation** -- `omega ~ 1/r` produces faster inner orbits than outer ones
(like a galaxy), unlike a vortex where all radii rotate at the same angular speed.
Use `(-y) / (x*x + y*y + eps)` and `x / (x*x + y*y + eps)`.

**Compiled once per run** -- expressions are compiled before the time loop, not per cell.
Complex expressions are fine; compilation cost is paid once.
