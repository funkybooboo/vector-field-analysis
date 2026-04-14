#pragma once
#include "vec2.hpp"

#include <vector>

namespace Field {

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
struct Bounds {
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
using Slice = std::vector<std::vector<Vector::Vec2>>;

// A time series of 2D vector field snapshots with physical-space bounds.
// Layout: frames[frame][row][col].
struct TimeSeries {
    std::vector<Slice> frames;
    Bounds bounds;

    // Returns the grid dimensions derived from the first frame.
    // Returns {0, 0} if the series is empty.
    [[nodiscard]] GridSize gridSize() const {
        if (frames.empty() || frames[0].empty()) {
            return {};
        }
        return {static_cast<int>(frames[0][0].size()), static_cast<int>(frames[0].size())};
    }
};

// Maps grid index i (0..n-1) linearly onto [lo, hi]. Precondition: n >= 2.
[[nodiscard]] inline float indexToCoord(int i, int n, float lo, float hi) {
    return lo + ((hi - lo) * static_cast<float>(i) / static_cast<float>(n - 1));
}

} // namespace Field
