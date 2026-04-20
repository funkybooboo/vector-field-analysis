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

## `libs/field`

**Path:** `libs/field/src/`

A shared library providing the core vector types used by both binaries.

| Type / Function | Namespace | Purpose |
|-----------------|-----------|---------|
| `Vec2` | `Vector` | 2D float vector with `x` and `y` components and arithmetic operators |
| `GridCell` | `Field` | Grid position identified by `(row, col)` indices |
| `Path` | `Field` | Type alias for `vector<GridCell>` -- an ordered sequence of grid positions |
| `Bounds` | `Field` | Physical-space domain boundaries (`xMin`, `xMax`, `yMin`, `yMax`) |
| `GridSize` | `Field` | Integer grid dimensions (`width`, `height`) |
| `Slice` | `Field` | Type alias for one time-step snapshot: `vector<vector<Vec2>>` indexed `[row][col]` |
| `TimeSeries` | `Field` | Struct holding `frames` (vector of `Slice`) and `bounds` (`Bounds`) |
| `Streamline` | `Field` | Ordered sequence of `GridCell` values tracing a path through the field |
| `Grid` | `Field` | Owns a single-frame field and drives the streamline tracing algorithm |
| `indexToCoord` | `Field` | Maps a grid index linearly onto a physical-space coordinate range |

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
| toml++ v3.4.0 | TOML config file parsing |
| exprtk 0.0.3 | Runtime evaluation of user-supplied math expressions |
| stb_perlin (master) | Perlin noise for the `noise` field type |
| HighFive v2.10.0 | C++ wrapper for HDF5 output |

---

## `bins/analyzer`

**Path:** `bins/analyzer/src/`

Reads the HDF5 output and traces streamlines through every cell in each time step's field.
Supports five parallel solver implementations that can be benchmarked side-by-side.

| Source file | Role |
|-------------|------|
| `main.cpp` | Entry point; orchestrates config, I/O, solver selection, and timing |
| `configParser.hpp/.cpp` | Parses `[analyzer]` TOML table into `AnalyzerConfig` |
| `fieldReader.hpp/.cpp` | Reads the HDF5 `field` group into `Field::TimeSeries` |
| `streamlineSolver.hpp` | Abstract `StreamlineSolver` base class |
| `solverFactory.hpp/.cpp` | Factory: solver name → `std::unique_ptr<StreamlineSolver>` |
| `sequentialStreamlineSolver.hpp/.cpp` | Single-threaded reference implementation |
| `openMpStreamlineSolver.hpp/.cpp` | Shared-memory parallelism via OpenMP |
| `pthreadsStreamlineSolver.hpp/.cpp` | Shared-memory parallelism via pthreads |
| `mpiStreamlineSolver.hpp/.cpp` | Distributed-memory parallelism via MPI |
| `streamWriter.hpp/.cpp` | Writes traced streamlines to `streams.h5` |

**Streamline tracing algorithm** (in `Field::Grid`):

All solver implementations follow the same two-pass design per time step:

- **Pass 1 (parallelisable)** -- each cell calls `downstreamCell(row, col)`, which returns
  the grid index of the nearest cell in the direction the vector points. This method is
  `const` and reads no mutable state, so it is safe to call concurrently.

- **Pass 2 (sequential)** -- `traceStreamlineStep(src, dest)` is called for each
  `(src, dest)` pair. It writes to the internal streamline grid (shared mutable state) and
  is not thread-safe. Each call either extends the current streamline (destination
  unclaimed) or merges two converging paths via `joinStreamlines`.

The pipeline scripts (`scripts/local/pipeline.sh`, `scripts/chpc/pipeline-job.sh`) run all
implementations in sequence. Each writes its elapsed time to `data/<stem>/timing_<name>.txt`
via the `timing_output` config key. Output correctness is verified against the sequential
reference using hash or `h5diff` comparison. Run `timings.sh` (project root) to read the
timing files and print a speedup report.

**Third-party dependencies:**

| Library | Purpose |
|---------|---------|
| HighFive v2.10.0 | C++ wrapper for HDF5 input/output |
| OpenMPI | MPI runtime for the distributed-memory solver |

---

## `tools/`

**Path:** `tools/`

Python scripts that operate on HDF5 output. Each script uses a `uv run` shebang so
dependencies are fetched automatically -- no manual `pip install` needed.

| Script | Purpose |
|--------|---------|
| `visualize.py` | Animates or plots a single step as a quiver (arrow) plot; supports streamline overlay |
| `field_math.py` | Adds or subtracts two `.h5` files element-wise |
| `stats.py` | Prints statistics for field and streamline `.h5` files |

Both scripts expect the same HDF5 schema written by the simulator and preserved by
`field_math.py`, so the output of `field_math.py` is a drop-in input for `visualize.py`.

---

## Dependency Overview

| Dependency | Managed by | Used by |
|------------|-----------|---------|
| HDF5 (system) | `mise run deps` | simulator, analyzer |
| HighFive v2.10.0 | CMake FetchContent | simulator, analyzer |
| Catch2 v3.5.3 | CMake FetchContent | all tests |
| toml++ v3.4.0 | CMake FetchContent | simulator |
| exprtk 0.0.3 | CMake FetchContent | simulator |
| stb_perlin (master) | CMake FetchContent | simulator |
| h5py, numpy, matplotlib | uv (per-script) | visualize.py |
| h5py, numpy | uv (per-script) | field_math.py |
