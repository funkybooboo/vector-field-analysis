#pragma once
#include "fieldTypes.hpp"
#include "vec2.hpp"

#include <atomic>
#include <cstddef>
#include <vector>

namespace Field {

// Owns a single-frame vector field grid and drives the streamline tracing
// algorithm over it. Row index corresponds to the y-axis, col index to x.
// Streamline associations are tracked in a parallel grid (streamlines_) rather
// than inside Vec2, keeping the generic math type free of domain state.
class Grid {
    const Bounds bounds_;
    std::size_t rows_;
    std::size_t cols_;
    std::vector<Vector::Vec2> flatField_; // row-major: index = row*cols_ + col
    float rowSpacing_;
    float colSpacing_;
    mutable std::vector<std::atomic<std::size_t>> successor_;

    // for externally computed streamline result
    // used only by cuda
    std::vector<Path> precomputedStreamlines_;
    bool hasPrecomputedStreamlines_ = false;

  public:
    Grid(Bounds bounds, Slice field);

    [[nodiscard]] std::size_t coordsToIndex(std::size_t row, std::size_t col) const;
    void initializeSuccessors();

    // DSU functions for lock-free streamline merging (Pass 2)
    [[nodiscard]] std::size_t findRoot(std::size_t index) const;
    void unite(std::size_t a, std::size_t b);

    [[nodiscard]] std::size_t rows() const { return rows_; }
    [[nodiscard]] std::size_t cols() const { return cols_; }

    [[nodiscard]] const Bounds& bounds() const { return bounds_; }
    [[nodiscard]] const std::vector<Vector::Vec2>& field() const { return flatField_; }

    // allows solver to inject complete streamline result directly
    void setPrecomputedStreamlines(std::vector<Path> paths) {
        precomputedStreamlines_ = std::move(paths);
        hasPrecomputedStreamlines_ = true;
    }

    [[nodiscard]] bool hasPrecomputedStreamlines() const { return hasPrecomputedStreamlines_; }

    // Returns the grid cell (row, col) that the vector at (row, col) points
    // toward. Read-only; safe to call from multiple threads simultaneously.
    [[nodiscard]] GridCell downstreamCell(int row, int col) const;
    [[nodiscard]] GridCell downstreamCell(GridCell coords) const;

    // Returns the unique streamlines found after tracing. Each streamline is an
    // ordered list of (row, col) grid indices. Collected in row-major iteration
    // order of streamlines_ -- deterministic for a given trace run.
    [[nodiscard]] const std::vector<Path>& getStreamlines() const;
};

} // namespace Field
