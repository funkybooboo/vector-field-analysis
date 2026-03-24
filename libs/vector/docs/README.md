# Vector Library Docs

Documentation for the `vector` library (`libs/vector`).

## Types

### `Vector::Vector`

A 2D floating-point vector with an optional associated streamline.

| Field | Type | Description |
|---|---|---|
| `x` | `float` | X component |
| `y` | `float` | Y component |
| `stream` | `shared_ptr<StreamLine>` | Streamline this vector belongs to, or `nullptr` |

### `StreamLine::StreamLine`

A traced path through the vector field, stored as an ordered list of grid coordinates.

| Field | Type | Description |
|---|---|---|
| `path` | `vector<pair<int,int>>` | Sequence of grid indices visited by the streamline |

## API

### Methods on `Vector`

| Method | Description |
|---|---|
| `magnitude() const` | Euclidean length: `sqrt(x^2 + y^2)` |
| `unitVector() const` | Returns a new vector with the same direction and magnitude 1 |

### Free functions

| Function | Description |
|---|---|
| `dotProduct(const Vector& a, const Vector& b)` | Scalar dot product: `a.x*b.x + a.y*b.y` |
| `almostParrallel(Vector& a, Vector& b)` | Returns `true` if the component-wise difference is within `PARRALEL_THRESHOLD` (0.2) |

## Contents

- **Type definitions** -> descriptions of `Vector` and `StreamLine` and their fields
- **Math conventions** -> coordinate system, units, and floating-point precision assumptions
- **API reference** -> all public methods and free functions
- **Usage examples** -> example code demonstrating construction and use of the core types
