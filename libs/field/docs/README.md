# Field Library Docs

Documentation for the `field` library (`libs/field`).

## Types

### `Vector::Vec2`

A 2D floating-point vector. Default-constructed value is `(0, 0)`.

| Field | Type | Description |
|-------|------|-------------|
| `x` | `float` | X component |
| `y` | `float` | Y component |

### `Field::GridCell`

A grid position identified by its row and column index.

| Field | Type | Description |
|-------|------|-------------|
| `row` | `int` | Row index (y-axis) |
| `col` | `int` | Column index (x-axis) |

Supports `==`, `!=`, and `<` (row-major order) for use in sorted containers.

### `Field::Path`

Type alias: `std::vector<GridCell>`. An ordered sequence of grid positions tracing a path
through the field.

### `Field::Bounds`

Physical-space boundaries of the vector field domain.

| Field | Type | Description |
|-------|------|-------------|
| `xMin` | `float` | World-space left boundary |
| `xMax` | `float` | World-space right boundary |
| `yMin` | `float` | World-space bottom boundary |
| `yMax` | `float` | World-space top boundary |

### `Field::GridSize`

Integer dimensions of a 2D grid.

| Field | Type | Description |
|-------|------|-------------|
| `width` | `int` | Number of columns |
| `height` | `int` | Number of rows |

### `Field::Slice`

Type alias: `std::vector<std::vector<Vector::Vec2>>`. Represents one time step of the field
as a 2D grid of `Vec2` values indexed `[row][col]`.

### `Field::TimeSeries`

Holds a complete multi-step field simulation result.

| Field | Type | Description |
|-------|------|-------------|
| `frames` | `vector<Slice>` | One `Slice` per time step |
| `bounds` | `Bounds` | Physical-space domain boundaries |

| Method | Description |
|--------|-------------|
| `gridSize() const` | Returns `GridSize` derived from the first frame; returns `{0, 0}` if empty |

### `Field::Streamline`

A traced path through the vector field, stored as an ordered list of grid coordinates.

| Method | Description |
|--------|-------------|
| `getPath() const` | Returns a `const Path&` -- the ordered sequence of `GridCell` values visited |
| `appendPoint(GridCell)` | Appends a cell to the path |

The path is never empty -- the seed cell is always present (set at construction).

### `Field::Grid`

Owns a single-frame vector field and drives the streamline tracing algorithm. Row index
corresponds to the y-axis; col index to the x-axis. Streamline associations are tracked in
a parallel internal grid rather than inside `Vec2`, keeping the math type free of domain
state.

| Method | Description |
|--------|-------------|
| `rows() const` | Number of rows in the grid |
| `cols() const` | Number of columns in the grid |
| `downstreamCell(int row, int col) const` | Returns the grid cell the vector at `(row, col)` points toward. Read-only; thread-safe. |
| `downstreamCell(GridCell) const` | Same as above, accepts a `GridCell` |
| `traceStreamlineStep(GridCell src, GridCell dest)` | Applies one streamline step: extends or merges. **Not thread-safe.** |
| `traceStreamlineStep(GridCell src)` | Convenience: computes dest then applies. Not thread-safe. |
| `traceStreamlineStep(int row, int col)` | Convenience overload. Not thread-safe. |
| `joinStreamlines(shared_ptr<Streamline>, shared_ptr<Streamline>)` | Absorbs end's path into start; null/self-merge silently ignored |
| `getStreamlines() const` | Returns the unique `Path`s found after tracing, in row-major order |

## API

### Methods on `Vector::Vec2`

| Method | Description |
|--------|-------------|
| `magnitude() const` | Euclidean length: `sqrt(x^2 + y^2)` |
| `unitVector() const` | Returns a new `Vec2` with the same direction and magnitude 1 |
| `operator-() const` | Unary negation; returns `Vec2{-x, -y}` |
| `operator+(const Vec2&) const` | Component-wise addition; returns a new `Vec2` |
| `operator+=(const Vec2&)` | Component-wise addition in place; returns `*this` |
| `operator*(float) const` | Scalar multiplication; returns a new `Vec2` |

### Free functions

| Function | Description |
|----------|-------------|
| `Vector::dotProduct(const Vec2& a, const Vec2& b)` | Scalar dot product: `a.x*b.x + a.y*b.y` |
| `Vector::almostParallel(const Vec2& a, const Vec2& b)` | Returns `true` if the component-wise difference sum is within `kParallelThreshold` (0.2). **Precondition:** both vectors must be near-unit length. |
| `Vector::operator*(float, const Vec2&)` | Scalar multiplication with scalar on the left; returns a new `Vec2` |
| `Field::indexToCoord(int i, int n, float lo, float hi)` | Maps grid index `i` (0..n-1) linearly onto `[lo, hi]`. Precondition: `n >= 2`. |

## Contents

- **Type definitions** - descriptions of all types and their fields
- **Math conventions** - coordinate system, units, and floating-point precision assumptions
- **API reference** - all public methods and free functions
- **Usage examples** - example code demonstrating construction and use of the core types
