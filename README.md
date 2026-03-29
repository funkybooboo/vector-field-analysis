# Vector Field Analysis

A C++17 research tool for generating and analyzing 2D vector fields. The simulator produces vector fields and writes them to HDF5 files; the analyzer reads those files and traces streamlines through the field.

## Structure

**Libraries** (`libs/`)

| Library | Description |
|---|---|
| [`libs/vector`](libs/vector) | Core vector types and math utilities shared across binaries and libraries |

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
mise install     # install pinned tools (cmake, ninja, clang-format, clang-tidy)
mise run deps    # install system dependencies (HDF5)
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
| `mise run ci` | Full pipeline: format check, build, test, lint, ascii-check |
| `mise run run:bin:simulator` | Build and run the simulator with `karman_street.toml` (writes `field.h5`) |
| `mise run run:bin:analyzer` | Build and run the analyzer (reads `field.h5`) |
| `mise run visualize` | Animate `field.h5` as a quiver plot |
| `mise run clean` | Remove build artifacts |

Individual targets:

| Task | Description |
|---|---|
| `mise run build:lib:vector` | Build vector library only |
| `mise run build:bin:analyzer` | Build analyzer only |
| `mise run build:bin:simulator` | Build simulator only |
| `mise run test:lib:vector` | Test vector library only |
| `mise run test:bin:analyzer` | Test analyzer only |
| `mise run test:bin:simulator` | Test simulator only |

## Dependencies

| Dependency | Version | How |
|---|---|---|
| HDF5 | system | `mise run deps` |
| Eigen | 3.4.0 | FetchContent |
| HighFive | v2.10.0 | FetchContent |
| Catch2 | v3.5.3 | FetchContent |
| clang-format | 22.1.1 | `mise install` (pipx/PyPI) |
| clang-tidy | 22.1.0 | `mise install` (pipx/PyPI) |

## Code Quality

- **Compiler flags:** `-Wall -Wextra -Wpedantic -Werror` -> warnings are errors
- **Formatting:** clang-format 22.1.1 (LLVM style, indent 4, column limit 100)
- **Linting:** clang-tidy with bugprone, modernize, performance, and readability checks; all warnings treated as errors

## CI

GitHub Actions runs five jobs on every push and pull request to `main`:

| Job | What it checks |
|---|---|
| Build | Compiles with `-Werror` |
| Test | Runs all CTest cases |
| Format | `clang-format --dry-run --Werror` |
| Lint | `clang-tidy --warnings-as-errors=*` |
| ASCII Check | No non-ASCII characters in project source files |
