# Dependencies

All dependencies, why they are used, where they appear, and what version is pinned.

---

## Build Tools (mise-managed)

Installed by `mise install`. Versions are pinned in `.mise.toml`.

| Tool | Version | Purpose |
|------|---------|---------|
| CMake | 4.2.3 | Build system generator |
| Ninja | 1.13.2 | Fast build executor (backend for CMake) |
| uv | 0.11.2 | Python tool runner for visualization and stats scripts |
| clang-format | 22.1.3 | Source code formatter (enforced in CI) |
| clang-tidy | system (apt/dnf/pacman) | C++ linter and static analyzer (enforced in CI) |
| lychee | 0.15.1 | Broken link checker for markdown files |

---

## System Dependencies (apt/dnf/pacman)

Installed by `mise run deps`. Cannot be fetched by CMake because they provide compiled
native libraries that must match the system ABI.

### HDF5 (`libhdf5-dev`)

**Version (Ubuntu 22.04):** `1.10.7+repack-4ubuntu2`

HDF5 is a binary file format designed for large scientific datasets. It is the storage
format for both simulator output (`field.h5`) and analyzer output (`streams.h5`). The
project accesses HDF5 through the HighFive C++ wrapper rather than the raw C API.

### OpenMPI (`libopenmpi-dev`, `openmpi-bin`)

**Version (Ubuntu 22.04):** `4.1.2-2ubuntu1`

MPI (Message Passing Interface) is a standard for distributed-memory parallelism. This
project implements an MPI-based streamline solver (`mpiStreamlineSolver`) that distributes
field slices across multiple processes. `openmpi-bin` provides the `mpirun` launcher used
to start multi-process analyzer runs.

### CUDA Toolkit (`nvidia-cuda-toolkit`)

**Version (Ubuntu 22.04):** `11.5.1-1ubuntu1`

NVIDIA's parallel computing platform. Used to compile the GPU-accelerated streamline
solver (`cudaStreamlineSolver`) and the GPU union-find connected-component kernel (`cuda`).
Enabled automatically when a CUDA toolkit is
detected; disabled gracefully when absent (e.g. on CPU-only CI runners).

> **Note:** CUDA 11.5.1 does not officially support GCC 11. The build passes
> `-allow-unsupported-compiler` to suppress the version check. The code itself
> is valid C++17.
>
> **Note:** On glibc 2.34+ (Ubuntu 22.04+), `libpthread.a`, `libdl.a`, and
> `librt.a` are linker-script stubs that nvlink cannot process during CUDA
> separable compilation. HDF5 (via HighFive), MPI, and OpenMP all inject
> full-path references to these stubs into their CMake targets. The fix has
> two parts: (1) `analyzer_cuda_kernels` is a separate static library
> containing only the `.cu` sources, with `CUDA_RESOLVE_DEVICE_SYMBOLS ON`
> so device linking happens before HDF5/MPI/OpenMP enter the closure; (2)
> the stub paths are stripped entirely from `CUDA::cudart_static_deps` (an
> internal target that even dynamic `CUDA::cudart` pulls in) -- since we
> link the dynamic cudart, these host-only deps are already baked into
> `libcudart.so` and need not appear on the link line at all.

### cppcheck

**Version (Ubuntu 22.04):** `2.7-1`

Secondary static analysis tool run as part of CI (`mise run cppcheck`). Catches a
different class of bugs than clang-tidy and is run with `--enable=all --error-exitcode=1`.

---

## CMake FetchContent Dependencies

Fetched automatically at configure time by CMake. No manual installation needed.
Versions are pinned in `cmake/Dependencies.cmake`.

### HighFive `v2.10.0`

**Repository:** https://github.com/BlueBrain/HighFive

A header-only C++ wrapper around the HDF5 C library. Provides a modern, type-safe API
for reading and writing datasets. Used throughout `libs/field_io` to serialize and
deserialize `Field::TimeSeries` and `Field::Path` objects to disk.

### Catch2 `v3.5.3`

**Repository:** https://github.com/catchorg/Catch2

The test framework used for all unit and integration tests across the project. Every
`test_*.cpp` file links against `Catch2::Catch2WithMain`.

### toml++ `v3.4.0`

**Repository:** https://github.com/marzer/tomlplusplus

A header-only TOML parser. Used in `bins/analyzer/src/configParser.cpp` and
`bins/simulator/src/simulatorConfig.*` to read the `.toml` config files that control
simulation parameters, field type, grid dimensions, and solver selection.

### exprtk `0.0.3`

**Repository:** https://github.com/ArashPartow/exprtk

A header-only C++ math expression parser. Used by the simulator's `expression` field
type to evaluate user-supplied mathematical expressions (e.g. `"sin(x) * cos(y)"`) at
each grid point, allowing arbitrary custom vector fields to be defined in config files
without recompiling.

### stb (stb_perlin)

**Commit:** `31c1ad37456438565541f4919958214b6e762fb4`

**Repository:** https://github.com/nothings/stb

A collection of single-file public-domain libraries. This project uses only
`stb_perlin.h` for coherent Perlin noise generation. The noise is used by the
simulator's `perlin` field type to produce smooth, spatially-varying vector fields.

> **Note:** stb has no release tags. The build is pinned to a specific commit SHA
> instead of a version string.

---

## Compiler-Provided Dependencies

These are supplied by the compiler toolchain and require no installation steps.

### OpenMP

Provided by GCC (`-fopenmp`) or Clang (`-fopenmp`). Used by `openMpStreamlineSolver`
to parallelize streamline tracing across CPU threads with `#pragma omp parallel for`.
The build detects OpenMP with `find_package(OpenMP)` and enables it when present.

### pthreads

POSIX threads from the system libc. Used by `pthreadsStreamlineSolver` as a lower-level
alternative to OpenMP, explicitly managing a thread pool and work queue for comparison
against the OpenMP implementation.
