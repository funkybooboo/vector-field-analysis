# Vector

Core vector types and math utilities shared across binaries and libraries.

## Contents

- **Vec2** -> 2D vector type with magnitude, unit vector, dot product, and arithmetic operators (negation, addition, scalar multiplication)
- **Streamline** -> represents a path traced through a vector field as a sequence of coordinates

## Usage

Link against the `vector` CMake target to use these types in any binary or library.

## Tasks

```sh
mise run build   # build
mise run test:vector    # test
```
