# Analyzer Pipeline

The analyzer runs three stages: read the config, read the field, compute streamlines.

```
config.toml
    |
    v
+--------------------+     AnalyzerConfig
| ConfigParser       | --------------------+
+--------------------+                     |
                                           v
                           +--------------------+     Field::TimeSeries
                           |    FieldReader     | --------------------+
                           +--------------------+                     |
                                                                      v
                                                        +---------------------------+
                                                        | StreamlineSolver::        |
                                                        |   computeTimeStep(grid)   |
                                                        | (per time step)           |
                                                        +---------------------------+
                                                                      |
                                                                      v
                                                              Field::Grid (streamlines)
                                                                      |
                                                                      v
                                                              StreamWriter -> streams.h5
```

---

## Stage 1: ConfigParser

**Source:** `configParser.hpp`, `configParser.cpp`

Reads a TOML file using toml++ and produces an `AnalyzerConfig` struct (defined in
`libs/config/src/analyzerConfig.hpp`). The `[analyzer]` table is optional -- struct
defaults apply when absent.

```
AnalyzerConfig
  +-- solver      ("sequential" | "openmp" | "pthreads" | "mpi" | "all")
  +-- threadCount (pthreads/openmp thread count; 0 = hardware_concurrency)
```

I/O paths are derived from the config filename stem at runtime and are not stored in
`AnalyzerConfig`.

---

## Stage 2: FieldReader

**Source:** `fieldReader.hpp`, `fieldReader.cpp`

Reads the HDF5 file produced by the simulator into a `Field::TimeSeries`.

```
Field::TimeSeries
  +-- bounds: Bounds        (xMin, xMax, yMin, yMax -- physical domain)
  +-- frames: []Slice       (one slice per time step)
        +-- [row][col]: Vec2  (vx, vy at each grid cell)
```

---

## Stage 3: Streamline Computation

**Source:** solver implementation files + `libs/field/src/grid.hpp` (`Field::Grid`)

For each time step:
1. A `Field::Grid` is constructed from the slice and domain bounds.
2. The selected `StreamlineSolver` calls `computeTimeStep(grid)`.
3. Two-pass design shared by all implementations:

   **Pass 1 (parallelisable)** -- each cell calls `downstreamCell(row, col)`,
   which returns the grid index of the nearest cell in the direction the vector points.
   This method is `const` and reads no mutable state, so it is safe to call concurrently.

   **Pass 2 (sequential)** -- `traceStreamlineStep(src, dest)` is called for each
   `(src, dest)` pair. It writes to the internal streamline grid (shared mutable state)
   and is not thread-safe.

### `downstreamCell(row, col)`

Maps the vector at `(row, col)` to a grid offset: the x-component advances the column
index, the y-component advances the row index. The result is clamped to grid bounds.

### `traceStreamlineStep(src, dest)`

1. If `src` has no streamline, creates one.
2. If `dest` has no streamline, assigns it to `src`'s streamline (extend the path).
3. If `dest` already belongs to a different streamline, merges via `joinStreamlines`.

### `joinStreamlines(start, end)`

Absorbs `end`'s path into `start`. All grid entries that pointed to `end` are
redirected to `start`.

---

## Implementations

### Sequential

Single loop over all `(row, col)` pairs; both passes merged into one sequential scan.

### OpenMP

- Accepts `threadCount` in its constructor; calls `omp_set_num_threads(threadCount)` before Pass 1 so the thread count matches pthreads exactly.
- Pass 1: `#pragma omp parallel for schedule(static) collapse(2)` over all cells.
- Pass 2: sequential.

### Pthreads

- Pass 1: row range partitioned across `threadCount` pthreads.
- Pass 2: sequential on the calling thread after `pthread_join`.

### MPI

- All ranks read the field file independently (shared filesystem on CHPC/Lustre).
- Pass 1: each rank computes neighbor pairs for its assigned row range.
- `MPI_Gatherv` collects all pairs on rank 0.
- Pass 2: sequential on rank 0 only.
- Single-rank run falls back to Sequential (no communication overhead).

---

## `all` Mode: Fair Comparison

When `solver = "all"`, `main.cpp` runs every implementation and prints timings side-by-side.
To keep the comparison apples-to-apples:

- **Thread adaptation** -- if MPI is active (`mpiSize > 1`), `threadCount` is set to `mpiSize`
  before any solver runs. This ensures openmp, pthreads, and mpi all use the same number of
  workers regardless of what the config's `threads` key says.

- **MPI skip** -- if the binary is invoked without `mpirun` (single-rank), the MPI solver is
  skipped and a hint is printed. A single-rank MPI run falls back internally to sequential,
  so including it would just duplicate the sequential result and mislead.

- **`mise run run:analyzer`** launches with `mpirun -n $MPI_RANKS` (see `.env.example`) so MPI
  always participates with a consistent rank count.

---

## Source File Map

| File | Role |
|------|------|
| `main.cpp` | Entry point; orchestrates all stages |
| `configParser.hpp/.cpp` | Parses TOML into `AnalyzerConfig` |
| `streamlineSolver.hpp` | Abstract `StreamlineSolver` base class |
| `solverFactory.hpp/.cpp` | Factory: name -> `std::unique_ptr<StreamlineSolver>` |
| `fieldReader.hpp/.cpp` | Reads HDF5 into `Field::TimeSeries` |
| `streamWriter.hpp/.cpp` | Writes traced streamlines to `streams.h5` |
| `sequentialStreamlineSolver.hpp/.cpp` | Single-threaded reference implementation |
| `openMpStreamlineSolver.hpp/.cpp` | OpenMP shared-memory implementation |
| `pthreadsStreamlineSolver.hpp/.cpp` | Pthreads shared-memory implementation |
| `mpiStreamlineSolver.hpp/.cpp` | MPI distributed-memory CPU implementation |
| `libs/config/src/analyzerConfig.hpp` | `AnalyzerConfig` struct and `kValidSolvers` array |
| `libs/field/src/grid.hpp` | `Field::Grid` -- owns the streamline state per time step |
