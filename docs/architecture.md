# Architecture

Vector Field Analysis is a C++17 research tool for generating and analysing 2D vector
fields. A TOML config drives the simulator, which writes a self-contained HDF5 file;
the analyzer and Python tools consume that file independently.

---

## Component Overview

```
bins/simulator/          bins/analyzer/
  config.toml  -->  [simulator]  -->  field.h5  -->  [analyzer]
                                          |
                                          +--> tools/visualize.py
                                          +--> tools/field_math.py
```

**`libs/`** provides shared types used by both binaries.

---

## Data Flow

1. The user writes a TOML config describing the field (grid dimensions, time parameters,
   one or more field layers).
2. The **simulator** parses the config, evaluates each layer at every grid cell for every
   time step, and writes the result to an HDF5 file.
3. The **analyzer** reads the HDF5 file and traces streamlines through the field.
4. **Python tools** read the same HDF5 file for visualisation or arithmetic operations.

The HDF5 file is the only data exchange format between components. It is self-contained --
it stores grid geometry and simulation parameters as attributes alongside the field data,
so no secondary config file is needed at read time.

---

## `libs/vector`

**Path:** `libs/vector/src/`

A shared library providing the core vector types used by both binaries.

| Type / Function | Purpose |
|-----------------|---------|
| `Vec2` | 2D float vector with `x`, `y`, and an optional `Streamline` reference |
| `Streamline` | Ordered list of grid-index pairs tracing a path through the field |
| `dotProduct` | Scalar dot product of two `Vec2`s |
| `almostParallel` | L1-distance test on near-unit vectors |

`Vec2` holds a `shared_ptr<Streamline>` so that multiple grid cells can reference the
same streamline after a merge without duplicating the path data.

The library is intentionally small -- it defines data structures and math primitives only,
with no I/O or field-generation logic.

---

## `bins/simulator`

**Path:** `bins/simulator/src/`

Generates a procedural vector field and writes it to HDF5. Runs as a three-stage pipeline:

```
ConfigParser  ->  FieldGenerator  ->  FieldWriter
```

| Source file | Role |
|-------------|------|
| `main.cpp` | Entry point; orchestrates the three stages |
| `simulatorConfig.hpp` | `SimulatorConfig` and `FieldLayerConfig` structs |
| `configParser.hpp/.cpp` | Parses TOML into `SimulatorConfig` |
| `fieldGenerator.hpp/.cpp` | Evaluates field layers, applies decay, assembles time series |
| `fieldWriter.hpp/.cpp` | Writes `vx`/`vy` datasets and metadata attributes to HDF5 |

The field is produced by linear superposition: for each cell, each layer's contribution is
computed, scaled by `strength`, and summed. A global viscous decay scalar is applied to
the sum before writing.

See [`pipeline.md`](pipeline.md) for a detailed walkthrough.

**Third-party dependencies used only by the simulator:**

| Library | Purpose |
|---------|---------|
| Eigen 3.4.0 | 2D vector arithmetic in field evaluation functions |
| toml++ v3.4.0 | TOML config file parsing |
| exprtk 0.0.3 | Runtime evaluation of user-supplied math expressions |
| stb_perlin (master) | Perlin noise for the `noise` field type |
| HighFive v2.10.0 | C++ wrapper for HDF5 output |

---

## `bins/analyzer`

**Path:** `bins/analyzer/src/`

Reads the HDF5 output and traces streamlines through each time step's field.

| Source file | Role |
|-------------|------|
| `main.cpp` | Entry point; loads data, traces from the origin cell each step |
| `fieldReader.hpp/.cpp` | Reads the HDF5 `field` group into `FieldTimeSeries` |
| `vectorField.hpp/.cpp` | `FieldGrid`: owns one step's field and drives streamline tracing |

The current implementation is a prototype -- it traces a single streamline step from
grid cell `(0, 0)` in each time step. A full analysis would seed every cell in the grid.

**Streamline tracing algorithm** (in `FieldGrid`):

Each `traceStreamlineStep` call advances one cell forward in the vector direction and
either extends the current streamline (if the destination is unclaimed) or merges with the
existing streamline at that cell (if the two paths converge). Merging is done by absorbing
the shorter path's cells into the longer path's `Streamline` object and redirecting all
field vector references to the surviving object.

**Third-party dependency:**

| Library | Purpose |
|---------|---------|
| HighFive v2.10.0 | C++ wrapper for HDF5 input |

---

## `tools/`

**Path:** `tools/`

Python scripts that operate on HDF5 output. Each script uses a `uv run` shebang so
dependencies are fetched automatically -- no manual `pip install` needed.

| Script | Purpose |
|--------|---------|
| `visualize.py` | Animates or plots a single step as a quiver (arrow) plot |
| `field_math.py` | Adds or subtracts two `.h5` files element-wise |

Both scripts expect the same HDF5 schema written by the simulator and preserved by
`field_math.py`, so the output of `field_math.py` is a drop-in input for `visualize.py`.

---

## Dependency Overview

| Dependency | Managed by | Used by |
|------------|-----------|---------|
| HDF5 (system) | `mise run deps` | simulator, analyzer |
| Eigen 3.4.0 | CMake FetchContent | simulator |
| HighFive v2.10.0 | CMake FetchContent | simulator, analyzer |
| Catch2 v3.5.3 | CMake FetchContent | all tests |
| toml++ v3.4.0 | CMake FetchContent | simulator |
| exprtk 0.0.3 | CMake FetchContent | simulator |
| stb_perlin (master) | CMake FetchContent | simulator |
| h5py, numpy, matplotlib | uv (per-script) | visualize.py |
| h5py, numpy | uv (per-script) | field_math.py |
