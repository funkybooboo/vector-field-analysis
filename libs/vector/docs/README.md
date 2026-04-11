# Vector Library Docs

Documentation for the `vector` library (`libs/vector`).

## Types

### `Vector::Vec2`

A 2D floating-point vector. Default-constructed value is `(0, 0)`.

| Field | Type | Description |
|-------|------|-------------|
| `x` | `float` | X component |
| `y` | `float` | Y component |

### `Vector::Streamline`

A traced path through the vector field, stored as an ordered list of grid coordinates.

| Field | Type | Description |
|-------|------|-------------|
| `path` | `vector<pair<int,int>>` | Sequence of `(row, col)` grid indices visited by the streamline |

### `Vector::FieldSlice`

Type alias: `std::vector<std::vector<Vec2>>`. Represents one time step of the field as a 2-D grid of `Vec2` values indexed `[row][col]`.

### `Vector::FieldTimeSeries`

Holds a complete multi-step field simulation result.

| Field | Type | Description |
|-------|------|-------------|
| `steps` | `vector<FieldSlice>` | One `FieldSlice` per time step |
| `xMin` | `float` | World-space left boundary |
| `xMax` | `float` | World-space right boundary |
| `yMin` | `float` | World-space bottom boundary |
| `yMax` | `float` | World-space top boundary |

## API

### Methods on `Vec2`

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
| `dotProduct(const Vec2& a, const Vec2& b)` | Scalar dot product: `a.x*b.x + a.y*b.y` |
| `almostParallel(const Vec2& a, const Vec2& b)` | Returns `true` if the component-wise difference sum is within `kParallelThreshold` (0.2). **Precondition:** both vectors must be near-unit length. |
| `operator*(float, const Vec2&)` | Scalar multiplication with scalar on the left; returns a new `Vec2` |

## Contents

- **Type definitions** - descriptions of `Vec2` and `Streamline` and their fields
- **Math conventions** - coordinate system, units, and floating-point precision assumptions
- **API reference** - all public methods and free functions
- **Usage examples** - example code demonstrating construction and use of the core types
