# Vector Field Analysis

C++17 tool for generating and analyzing 2D vector fields. The simulator produces procedural vector fields written to HDF5 files; the analyzer reads those files and traces streamlines using parallel solvers.

Six solver implementations are included: sequential, pthreads, OpenMP, MPI, CUDA, and a hybrid CUDA+MPI solver.

See the [report](report/README.md) for full analysis and results.

## Dependencies

**Tools** — managed by [mise](https://mise.jdx.dev):

```sh
mise install   # installs cmake, ninja, uv, clang-format, lychee, shellcheck, shfmt
```

**System packages:**

```sh
sudo apt-get install libhdf5-dev libopenmpi-dev openmpi-bin cppcheck nvidia-cuda-toolkit clang-tidy
```

**C++ libraries** — fetched automatically by CMake at configure time:

| Library | Purpose |
|---|---|
| HighFive v2.10.0 | HDF5 C++ wrapper |
| Catch2 v3.5.3 | Test framework |
| toml++ v3.4.0 | Config file parsing |
| exprtk 0.0.3 | Math expression parsing |
| stb_perlin | Perlin noise |

## Build

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Or via mise: `mise run build`

## Configs

Three field types are provided, each at three grid sizes:

| Config | Description |
|---|---|
| `vortex_<size>` | Single counter-clockwise vortex with viscous decay |
| `karman_street_<size>` | Von Kármán vortex street — alternating vortices downstream of a bluff body |
| `source_grid_divergent_<size>` | Five uniform background flows at varied angles overlaid with a 3×3 grid of divergent point sources |

Sizes: `128x128` (small/test), `1024x1024` (medium), `65536x65536` (large).

The `[analyzer]` table in each config controls the solver and thread count. See [`configs/`](configs/) for the full list and [`bins/analyzer/docs/config-guide.md`](bins/analyzer/docs/config-guide.md) for all options.

## Run

Pass any config from `configs/` as the argument — the stem (filename without `.toml`) determines the input/output paths under `data/<stem>/`.

```sh
# Generate a vector field → data/<stem>/field.h5
./build/bins/simulator/simulator configs/<stem>.toml
```

The analyzer solver is set in the config's `[analyzer]` table (`solver = "..."`). Default is `benchmark`, which runs all solvers and prints a speedup table.

```sh
# Benchmark all solvers (default)
mpirun -n $(nproc) ./build/bins/analyzer/analyzer configs/<stem>.toml

# Sequential
./build/bins/analyzer/analyzer configs/<stem>.toml

# OpenMP (shared memory)
./build/bins/analyzer/analyzer configs/<stem>.toml   # solver = "openmp" in config

# Pthreads (shared memory)
./build/bins/analyzer/analyzer configs/<stem>.toml   # solver = "pthreads" in config

# MPI (distributed memory) — must be launched with mpirun/srun
mpirun -n $(nproc) ./build/bins/analyzer/analyzer configs/<stem>.toml   # solver = "mpi" in config

# CUDA (GPU) — requires -DENABLE_CUDA=ON at build time
./build/bins/analyzer/analyzer configs/<stem>.toml   # solver = "cuda" in config

# Hybrid CUDA+MPI — requires -DENABLE_CUDA=ON and mpirun
mpirun -n $(nproc) ./build/bins/analyzer/analyzer configs/<stem>.toml   # solver = "hybrid" in config
```

Output: `data/<stem>/streams.h5`

```sh
# Visualize
uv run tools/visualize.py data/<stem>/field.h5
uv run tools/visualize.py data/<stem>/field.h5 --streams data/<stem>/streams.h5
```

Or via mise: `mise run run:simulator`, `mise run run:analyzer`, `mise run visualize`

## Test

```sh
ctest --test-dir build --output-on-failure
```

Or via mise: `mise run test`

## Other tasks (via mise)

| Task | What it runs |
|---|---|
| `mise run format` | `clang-format -i` on all sources |
| `mise run lint` | `clang-tidy` on all sources |
| `mise run cppcheck` | `cppcheck` static analysis |
| `mise run test:sanitize` | Tests under ASan + UBSan |
| `mise run test:coverage` | `gcovr` coverage report |
| `mise run ci` | Full CI pipeline |
| `mise run clean` | Remove build artifacts |

## CHPC cluster

See [`scripts/CHPC.md`](scripts/CHPC.md) for cluster setup and workflow.
Scripts in [`scripts/local/`](scripts/local/) sync code and data between local and cluster.
