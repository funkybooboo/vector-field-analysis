# Vector Library Docs

Documentation for the `vector` library (`libs/vector`).

## Types

### `Vector::Vec2`

A 2D floating-point vector with an optional associated streamline.

| Field | Type | Description |
|-------|------|-------------|
| `x` | `float` | X component |
| `y` | `float` | Y component |
| `stream` | `shared_ptr<Streamline>` | Streamline this vector belongs to, or `nullptr` |

### `Vector::Streamline`

A traced path through the vector field, stored as an ordered list of grid coordinates.

| Field | Type | Description |
|-------|------|-------------|
| `path` | `vector<pair<int,int>>` | Sequence of `(row, col)` grid indices visited by the streamline |

## API

### Methods on `Vec2`

| Method | Description |
|--------|-------------|
| `magnitude() const` | Euclidean length: `sqrt(x^2 + y^2)` |
| `unitVector() const` | Returns a new `Vec2` with the same direction and magnitude 1 |

### Free functions

| Function | Description |
|----------|-------------|
| `dotProduct(const Vec2& a, const Vec2& b)` | Scalar dot product: `a.x*b.x + a.y*b.y` |
| `almostParallel(const Vec2& a, const Vec2& b)` | Returns `true` if the component-wise difference sum is within `kParallelThreshold` (0.2). **Precondition:** both vectors must be near-unit length. |

## Contents

- **Type definitions** - descriptions of `Vec2` and `Streamline` and their fields
- **Math conventions** - coordinate system, units, and floating-point precision assumptions
- **API reference** - all public methods and free functions
- **Usage examples** - example code demonstrating construction and use of the core types
