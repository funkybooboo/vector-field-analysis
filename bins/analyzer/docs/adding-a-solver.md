# Adding a Solver

This guide walks through adding a new streamline solver implementation to the analyzer.

---

## Overview

Every solver implements one method:

```cpp
// streamlineSolver.hpp
class StreamlineSolver {
  public:
    virtual ~StreamlineSolver() = default;
    StreamlineSolver(const StreamlineSolver&) = delete;
    StreamlineSolver& operator=(const StreamlineSolver&) = delete;
    StreamlineSolver(StreamlineSolver&&) = delete;
    StreamlineSolver& operator=(StreamlineSolver&&) = delete;

    virtual void computeTimeStep(Field::Grid& grid) = 0;
};
```

`computeTimeStep` is called once per time step. The `Field::Grid` it receives exposes two operations:

| Method | Thread-safe | Description |
|--------|-------------|-------------|
| `grid.rows()` / `grid.cols()` | Yes | Grid dimensions |
| `grid.downstreamCell(row, col)` | Yes | Returns the neighbor cell in the vector direction |
| `grid.traceStreamlineStep(src, dest)` | **No** | Merges src and dest into one streamline |

All existing solvers use a **two-pass design** to exploit this split:

- **Pass 1 (parallel)** -- call `downstreamCell()` for every cell, collecting `(src, dest)` pairs
- **Pass 2 (sequential)** -- iterate the pairs and call `traceStreamlineStep()` on each

---

## Checklist

| Step | File | Action |
|------|------|--------|
| 1 | `src/mysolver.hpp` (new) | Declare class inheriting `StreamlineSolver` |
| 2 | `src/mysolver.cpp` (new) | Implement `computeTimeStep()` |
| 3 | `src/analyzerConfig.hpp` | Add name to `kValidSolvers`; update array size |
| 4 | `src/solverFactory.cpp` | Add case in `makeSolver()`; update `static_assert` |
| 5 | `CMakeLists.txt` | Add `.cpp` to `analyzer_lib` sources |
| 6 | `configs/mysolver.toml` (new) | Config file for running the solver in isolation |

---

## Step-by-Step

### 1. Header

```cpp
// bins/analyzer/src/mysolver.hpp
#pragma once
#include "streamlineSolver.hpp"

class MySolver : public StreamlineSolver {
    unsigned int threadCount_;
  public:
    explicit MySolver(unsigned int threadCount);
    void computeTimeStep(Field::Grid& grid) override;
};
```

Omit `threadCount_` if your solver does not use threads.

### 2. Implementation

```cpp
// bins/analyzer/src/mysolver.cpp
#include "mysolver.hpp"
#include <vector>

MySolver::MySolver(unsigned int threadCount) : threadCount_(threadCount) {}

void MySolver::computeTimeStep(Field::Grid& grid) {
    const auto numRow = grid.rows();
    const auto numCol = grid.cols();
    if (numRow == 0 || numCol == 0) return;

    // Pass 1: read-only, parallel-safe
    std::vector<Field::GridCell> neighbors(numRow * numCol);
    for (std::size_t row = 0; row < numRow; row++) {
        for (std::size_t col = 0; col < numCol; col++) {
            neighbors[row * numCol + col] =
                grid.downstreamCell(static_cast<int>(row), static_cast<int>(col));
        }
    }

    // Pass 2: sequential, writes shared streamline state
    for (std::size_t row = 0; row < numRow; row++) {
        for (std::size_t col = 0; col < numCol; col++) {
            grid.traceStreamlineStep(
                {static_cast<int>(row), static_cast<int>(col)},
                neighbors[row * numCol + col]);
        }
    }
}
```

### 3. Register the Name

```cpp
// bins/analyzer/src/analyzerConfig.hpp
inline constexpr std::array<std::string_view, 7> kValidSolvers = {   // was 6
    "sequential", "openmp", "pthreads", "mpi", "cuda", "mysolver", "all"
};
```

The config parser validates against `kValidSolvers` automatically -- no parser changes needed.

### 4. Wire the Factory

```cpp
// bins/analyzer/src/solverFactory.cpp
#include "mysolver.hpp"

// inside makeSolver():
if (name == "mysolver") {
    return std::make_unique<MySolver>(threadCount);
}

// update the guard:
static_assert(kValidSolvers.size() == 7,   // was 6
              "kValidSolvers changed -- update makeSolver() to match");
```

### 5. CMake

```cmake
# bins/analyzer/CMakeLists.txt
add_library(analyzer_lib STATIC
    ...
    src/mysolver.cpp    # add this
)
```

#### Optional: external library dependency

If your solver requires an optional library (e.g. CUDA, TBB), follow the pattern used by MPI:

```cmake
find_package(MyLib)
if(MyLib_FOUND)
    target_compile_definitions(analyzer_lib PUBLIC USE_MYSOLVER)
    target_link_libraries(analyzer_lib PUBLIC MyLib::MyLib)
endif()
```

Then guard the implementation:

```cpp
// mysolver.cpp
#ifdef USE_MYSOLVER
#include <mylib.h>
// real implementation
#else
#include "sequentialStreamlineSolver.hpp"
void MySolver::computeTimeStep(Field::Grid& grid) {
    SequentialStreamlineSolver fallback;
    fallback.computeTimeStep(grid);
}
#endif
```

### 6. Config File

```toml
# configs/mysolver.toml
# Brief description of the implementation.
#
# Run: analyzer configs/mysolver.toml

[analyzer]
solver = "mysolver"
```

Thread count is controlled by the `ANALYZER_THREADS` env var (see `.env.example`).

---

## Including in `solver = "all"` Mode

`all` mode runs every solver on rank 0 and prints side-by-side timings. To include yours:

1. Add it to the `runAll()` function in `main.cpp` following the pattern of the existing solvers.
2. Call `verify()` against the sequential reference result to confirm correctness.

If your solver is MPI-aware (collective calls required), it must participate across all ranks -- see `MpiStreamlineSolver` as the reference.

---

## Thread Count

`makeSolver(name, threadCount)` receives the resolved thread count from `main.cpp`.
Resolution order:

1. `threads` key in the TOML config (if non-zero)
2. `ANALYZER_THREADS` environment variable
3. `std::thread::hardware_concurrency()`
4. 1 (final fallback)

In `solver = "all"` mode with an active MPI run, `threadCount` is automatically aligned to
`mpiSize` so all parallel solvers use the same number of workers.
