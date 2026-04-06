# cmake

CMake modules for the project.

## Contents

- **Dependencies.cmake** -> fetches third-party libraries at configure time via `FetchContent`

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| [Eigen](https://eigen.tuxfamily.org) | 3.4.0 | Linear algebra (simulator) |
| [HighFive](https://github.com/BlueBrain/HighFive) | v2.10.0 | HDF5 C++ wrapper (simulator + analyzer) |
| [Catch2](https://github.com/catchorg/Catch2) | v3.5.3 | Unit testing framework |
| [toml++](https://github.com/marzer/tomlplusplus) | v3.4.0 | TOML config parsing (simulator) |
| [exprtk](https://github.com/ArashPartow/exprtk) | 0.0.3 | Math expression evaluation for Custom fields (simulator) |
| [stb_perlin](https://github.com/nothings/stb) | `master`* | Perlin noise for Noise fields (simulator) |

\* `stb` has no release tags; `master` is the only option. This is a known reproducibility gap.

HDF5 must be installed on the system before configuring. Run `mise run deps` to install it.

| Distro | Package |
|---|---|
| Arch Linux | `hdf5` |
| Ubuntu | `libhdf5-dev` |

`clang-format 22.1.1` and `clang-tidy 22.1.0` are managed by mise via the pipx backend (PyPI) and installed automatically with `mise install`. They are pinned to exact versions so all contributors and CI use identical binaries regardless of OS.
