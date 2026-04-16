# Field

Core vector types and math utilities shared across binaries and libraries.

## Contents

- **Vec2** -> 2D vector type with magnitude, unit vector, dot product, and arithmetic operators (negation, addition, scalar multiplication)
- **Streamline** -> represents a path traced through a vector field as a sequence of `GridCell` coordinates
- **GridCell** -> grid position identified by `(row, col)` indices
- **Path** -> type alias for an ordered sequence of `GridCell` values
- **Bounds** -> physical-space domain boundaries (`xMin`, `xMax`, `yMin`, `yMax`)
- **GridSize** -> integer grid dimensions (`width`, `height`)
- **Slice** -> one time-step snapshot of a 2D field: `vector<vector<Vec2>>` indexed `[row][col]`
- **TimeSeries** -> full multi-step field result (`frames` + `bounds`)
- **Grid** -> owns a single-frame field and drives the streamline tracing algorithm

## Usage

Link against the `field` CMake target to use these types in any binary or library.

## Tasks

```sh
mise run build   # build
mise run test:vector    # test
```
