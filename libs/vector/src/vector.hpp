#pragma once
#include <vector>

namespace Vector {

// Maximum component-wise L1 distance between two unit vectors for them to be
// considered parallel. 0.2 tolerates roughly 8 degrees of divergence while
// still filtering noise in nearly-aligned field vectors.
inline constexpr float kParallelThreshold = 0.2f;

class Vec2 {
  public:
    float x;
    float y;

    Vec2()
        : x(0.0f),
          y(0.0f) {}
    Vec2(float x, float y)
        : x(x),
          y(y) {}

    [[nodiscard]] float magnitude() const;

    [[nodiscard]] Vec2 unitVector() const;

    [[nodiscard]] Vec2 operator-() const;
    [[nodiscard]] Vec2 operator+(const Vec2& other) const;
    Vec2& operator+=(const Vec2& other);
    [[nodiscard]] Vec2 operator*(float scalar) const;
};

[[nodiscard]] Vec2 operator*(float scalar, const Vec2& v);

[[nodiscard]] float dotProduct(const Vec2& a, const Vec2& b);

// Precondition: both vectors must be near-unit. This compares component-wise
// difference, not direction - non-unit inputs give unreliable results.
// L1 distance on unit vectors is used instead of a dot product or angle
// comparison because it is cheaper to compute and works well for the
// axis-aligned grid structure where near-parallel vectors dominate.
[[nodiscard]] bool almostParallel(const Vec2& a, const Vec2& b);

// A grid cell identified by its row and column index.
struct GridCell {
    int row = 0;
    int col = 0;

    bool operator==(const GridCell& other) const { return row == other.row && col == other.col; }
    bool operator!=(const GridCell& other) const { return !(*this == other); }
    bool operator<(const GridCell& other) const {
        return row != other.row ? row < other.row : col < other.col;
    }
};

// Ordered sequence of grid cells tracing a path through the field.
using Path = std::vector<GridCell>;

// Physical-space bounds of the vector field domain.
struct FieldBounds {
    float xMin = 0.0f;
    float xMax = 0.0f;
    float yMin = 0.0f;
    float yMax = 0.0f;
};

// Integer dimensions of a 2D grid (width = columns, height = rows).
struct GridSize {
    int width = 0;
    int height = 0;
};

// A single time-step snapshot of a 2D vector field. Layout: slice[row][col].
using FieldSlice = std::vector<std::vector<Vec2>>;

// A time series of 2D vector field snapshots with physical-space bounds.
// Layout: steps[step][row][col].
struct FieldTimeSeries {
    std::vector<FieldSlice> steps;
    FieldBounds bounds;

    // Returns the grid dimensions derived from the first step.
    // Returns {0, 0} if the series is empty.
    [[nodiscard]] GridSize gridSize() const {
        if (steps.empty() || steps[0].empty()) {
            return {};
        }
        return {static_cast<int>(steps[0][0].size()), static_cast<int>(steps[0].size())};
    }
};

} // namespace Vector
