# Vector Field Analysis

A C++17 research tool for generating and analyzing 2D vector fields. The simulator produces vector fields and writes them to HDF5 files; the analyzer reads those files and traces streamlines through the field.

## Structure

**Libraries** (`libs/`)

| Library | Description |
|---|---|
| [`libs/field`](libs/field) | Core vector types, grid, and streamline math shared across binaries and libraries |
| [`libs/field_io`](libs/field_io) | HDF5 read/write for field and streamline data |
| [`libs/utils`](libs/utils) | Shared utilities (byte formatting, TOML helpers) |

**Binaries** (`bins/`)

| Binary | Description |
|---|---|
| [`bins/analyzer`](bins/analyzer) | Reads vector field data from HDF5 and computes streamlines |
| [`bins/simulator`](bins/simulator) | Generates procedural vector fields and writes them to HDF5 |

**Tools** (`tools/`)

| Tool | Description |
|---|---|
| [`tools/visualize.py`](tools/README.md) | Animate or plot simulator `.h5` output as a quiver (arrow) plot |

## Quick Start

```sh
mise install     # install pinned tools (cmake, ninja, uv, clang-format, clang-tidy, lychee)
mise run deps    # install system dependencies (HDF5, OpenMPI, cppcheck)
mise run build   # configure and build everything
mise run test    # run all tests
```

## Tasks

| Task | Description |
|---|---|
| `mise run deps` | Install system dependencies |
| `mise run build` | Build everything |
| `mise run test` | Run all tests |
| `mise run format` | Format all source files in place |
| `mise run format:check` | Check formatting without modifying files |
| `mise run lint` | Run clang-tidy on all source files |
| `mise run ascii-check` | Fail if any project source file contains non-ASCII characters |
| `mise run spell` | Check source files and docs for spelling errors |
| `mise run cppcheck` | Run cppcheck static analysis |
| `mise run test:sanitize` | Run tests under AddressSanitizer + UBSanitizer |
| `mise run test:coverage` | Run tests and generate coverage report |
| `mise run links` | Check for broken links in markdown files |
| `mise run ci` | Full pipeline -- mirrors all GitHub Actions jobs |
| `mise run run:simulator` | Build and run the simulator with `karman_street.toml` (writes `field.h5`) |
| `mise run run:analyzer` | Run simulator then benchmark all solver impls under `mpirun -n $(nproc)` |
| `mise run run:analyzer:mpi` | Run MPI solver only under mpirun (default 4 ranks; override with `NRANKS=N`) |
| `mise run visualize` | Animate `field.h5` as a quiver plot |
| `mise run visualize:streams` | Animate `field.h5` with streamlines overlaid from `field.streams.h5` |
| `mise run clean` | Remove build artifacts |

Individual targets:

| Task | Description |
|---|---|
| `mise run build` | Build vector library only |
| `mise run build:analyzer` | Build analyzer only |
| `mise run build:simulator` | Build simulator only |
| `mise run test:vector` | Test vector library only |
| `mise run test:analyzer` | Test analyzer only |
| `mise run test:simulator` | Test simulator only |

## Dependencies

**Managed by `mise install`**

| Dependency | Version | Notes |
|---|---|---|
| cmake | latest | Build system |
| ninja | latest | Build backend |
| uv | latest | Python package manager (visualize, codespell, gcovr -- tools fetched on demand via `uvx`) |
| clang-format | 22.1.1 | Code formatter |
| clang-tidy | 22.1.0 | Static analysis / linter |
| lychee | latest | Markdown link checker |

**Installed by `mise run deps`** (system package manager)

| Dependency | Notes |
|---|---|
| HDF5 | Binary data format used by simulator and analyzer |
| OpenMPI | MPI runtime and headers for the MPI solver |
| cppcheck | Secondary static analyzer |

**Fetched automatically by CMake** (`FetchContent`)

| Dependency | Version | Notes |
|---|---|---|
| HighFive | v2.10.0 | HDF5 C++ wrapper |
| Catch2 | v3.5.3 | Testing framework |
| toml++ | v3.4.0 | TOML config file parsing |
| exprtk | 0.0.3 | Math expression parsing for custom fields |
| stb_perlin | master | Perlin noise for noise field type |

## Code Quality

- **Compiler flags:** `-Wall -Wextra -Wpedantic -Werror` -- warnings are errors
- **Formatting:** clang-format 22.1.1 (LLVM style, indent 4, column limit 100)
- **Linting:** clang-tidy with bugprone, modernize, performance, and readability checks; all warnings treated as errors

## CI

GitHub Actions runs the following jobs on every push and pull request to `main`:

| Job | What it checks |
|---|---|
| Build | Compiles with `-Werror` |
| Test | Runs all CTest cases |
| Format | `clang-format --dry-run --Werror` |
| ASCII Check | No non-ASCII characters in project source files |
| Lint | `clang-tidy --warnings-as-errors=*` |
| Spell Check | `codespell` on source files and docs |
| Coverage | `gcovr` coverage report |
| Cppcheck | `cppcheck` static analysis |
| Link Check | `lychee` broken link check on markdown files |
