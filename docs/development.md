# Development Guide

## Prerequisites

Install [mise](https://mise.jdx.dev/) then run:

```sh
mise install        # install pinned tools (cmake, ninja, uv, clang-format, clang-tidy, lychee)
mise run deps       # install system dependencies via pacman (HDF5, cppcheck)
```

CMake dependencies (HighFive, Catch2, toml++, exprtk, stb_perlin) are fetched
automatically by `FetchContent` at configure time -- no manual installation needed.

---

## Building

```sh
mise run build                    # configure + build everything (Release)
mise run build:lib:vector         # vector library only
mise run build:bin:simulator      # simulator only
mise run build:bin:analyzer       # analyzer only
```

Build artifacts go in `build/`. Two additional build directories are created for
sanitizers and coverage:

| Directory | Purpose |
|-----------|---------|
| `build/` | Standard Release build |
| `build-sanitize/` | ASan + UBSan enabled |
| `build-coverage/` | Coverage instrumentation |

```sh
mise run clean     # remove all build directories
```

---

## Testing

```sh
mise run test                     # run all tests (Release build)
mise run test:lib:vector          # vector library tests only
mise run test:bin:simulator       # simulator tests only
mise run test:bin:analyzer        # analyzer tests only
```

Tests are written with [Catch2](https://github.com/catchorg/Catch2) v3.

### Sanitizers

```sh
mise run test:sanitize            # AddressSanitizer + UndefinedBehaviourSanitizer
```

Runs the full test suite against the `build-sanitize/` build. Any memory error or
undefined behaviour fails the run immediately.

### Coverage

```sh
mise run test:coverage            # instrument, run, and generate HTML report (gcovr)
```

Coverage output is written to `build-coverage/coverage/`. Open
`build-coverage/coverage/index.html` in a browser to browse line-by-line coverage.

---

## Code Quality

### Formatting

```sh
mise run format           # reformat all source files in place (clang-format)
mise run format:check     # check without modifying (used in CI)
```

Style: LLVM base, 4-space indent, 100-column limit. Configuration is in `.clang-format`
at the project root.

### Linting

```sh
mise run lint             # clang-tidy on all source files (warnings-as-errors)
```

Enabled check groups: `bugprone-*`, `modernize-*`, `performance-*`, `readability-*`.

### Static Analysis

```sh
mise run cppcheck         # cppcheck secondary static analysis
```

### Other Checks

```sh
mise run ascii-check      # fail if any project source file contains non-ASCII characters
mise run spell            # spell check source files and docs (codespell via uvx)
mise run links            # check for broken links in all markdown files (lychee)
```

---

## Running the Simulator

```sh
mise run run:bin:simulator        # build and run with karman_street.toml (writes field.h5)
./build/bins/simulator/simulator bins/simulator/configs/vortex.toml
```

```sh
mise run visualize                # animate field.h5
mise run run:bin:analyzer         # build and run the analyzer (reads field.h5)
```

---

## CI Pipeline

`mise run ci` runs all checks in sequence, mirroring every GitHub Actions job:

| Job | Command | What it checks |
|-----|---------|----------------|
| Build | cmake + ninja | Compiles with `-Werror` |
| Test | ctest | All Catch2 test cases pass |
| Format | clang-format --dry-run | No formatting drift |
| ASCII check | custom script | No non-ASCII in source files |
| Lint | clang-tidy | No warnings in linted checks |
| Spell check | codespell | No misspellings in source and docs |
| Coverage | gcovr | Generates coverage report |
| Cppcheck | cppcheck | No static analysis errors |
| Link check | lychee | No broken links in markdown |

GitHub Actions runs this pipeline on every push and pull request to `main`.

---

## Adding a New Field Type

1. **`simulatorConfig.hpp`** -- add the new value to the `FieldType` enum.

2. **`fieldGenerator.cpp`** -- write an `eval*` function that returns a `Vector::Vec2`
   for a given `(px, py, layer)`. Add a `case` to the `switch` in `generateTimeSeries`.

3. **`configParser.cpp`** -- add an `else if` branch for `"name"` in the if-else chain
   inside `parseFieldType`, returning `FieldType::NewType`.

4. **`simulatorConfig.hpp`** -- add a `case` returning the string name in `toString(FieldType)`.
   (`fieldTypeName()` in `fieldWriter.cpp` was removed; `toString()` in `simulatorConfig.hpp`
   is now the canonical `FieldType`-to-string conversion.)

5. **`simulatorConfig.hpp`** (doc comment) -- document the new type's active parameters in
   the `FieldLayerConfig` comment block.

6. **Tests** -- add cases to `bins/simulator/tests/test_simulator.cpp`.

7. **Config example** -- add a `.toml` file to `bins/simulator/configs/`.

8. **Docs** -- add an entry to `bins/simulator/docs/config-guide.md` and
   `bins/simulator/configs/README.md`.
