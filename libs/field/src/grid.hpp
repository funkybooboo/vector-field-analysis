#pragma once
#include "fieldTypes.hpp"

#include <atomic>
#include <vector>

namespace Field {

// Owns a single-frame vector field grid and drives the streamline tracing
// algorithm over it. Row index corresponds to the y-axis, col index to x.
//
// Streamline state is stored as a flat union-find over cell indices
// (i = row * colCount + col) plus a next_[] array recording each cell's
// downstream neighbour. traceStreamlineStep is thread-safe: it uses
// atomic CAS operations so multiple threads may call it concurrently.
class Grid {
    const Bounds bounds_;
    Slice field_;
    std::vector<Vector::Vec2> flatField_;
    float rowSpacing_ = 0.0f;
    float colSpacing_ = 0.0f;
    std::size_t colCount_ = 0;

    // Union-find arrays (flat, index = row * colCount_ + col).
    // ufParent_[i] == -1 means unvisited; otherwise holds parent index.
    mutable std::vector<std::atomic<int>> ufParent_;
    mutable std::vector<std::atomic<int>> ufRank_;
    // ufNext_[i] = downstream cell index recorded by traceStreamlineStep.
    // Each cell is written by exactly one thread (its own src index).
    std::vector<int> ufNext_;

    // for externally computed streamline result (used only by cudaFull)
    std::vector<Path> precomputedStreamlines_;
    bool hasPrecomputedStreamlines_ = false;

    int ufFind(int i) const;
    void ufUnion(int a, int b);
    [[nodiscard]] Path buildComponentPath(const std::vector<int>& sources,
                                          std::vector<bool>& visited) const;

  public:
    Grid(Bounds bounds, Slice field)
        : bounds_(bounds),
          field_(std::move(field)) {
        const std::size_t numRows = field_.size();
        const std::size_t numCols = numRows > 0 ? field_[0].size() : 0;
        colCount_ = numCols;
        rowSpacing_ =
            numRows > 1 ? (bounds_.yMax - bounds_.yMin) / static_cast<float>(numRows - 1) : 0.0f;
        colSpacing_ =
            numCols > 1 ? (bounds_.xMax - bounds_.xMin) / static_cast<float>(numCols - 1) : 0.0f;

        const std::size_t total = numRows * numCols;
        flatField_.resize(total);
        for (std::size_t r = 0; r < numRows; ++r) {
            for (std::size_t c = 0; c < numCols; ++c) {
                flatField_[(r * numCols) + c] = field_[r][c];
            }
        }

        ufParent_ = std::vector<std::atomic<int>>(total);
        ufRank_ = std::vector<std::atomic<int>>(total);
        for (std::size_t i = 0; i < total; ++i) {
            ufParent_[i].store(-1, std::memory_order_relaxed);
            ufRank_[i].store(0, std::memory_order_relaxed);
        }
        ufNext_.assign(total, -1);
    }

    [[nodiscard]] std::size_t rows() const { return field_.size(); }
    [[nodiscard]] std::size_t cols() const { return field_.empty() ? 0 : field_[0].size(); }
    [[nodiscard]] std::size_t coordsToIndex(int row, int col) const {
        return (static_cast<std::size_t>(row) * colCount_) + static_cast<std::size_t>(col);
    }

    [[nodiscard]] const Bounds& bounds() const { return bounds_; }
    [[nodiscard]] const Slice& field() const { return field_; }

    void setPrecomputedStreamlines(std::vector<Path> paths) {
        precomputedStreamlines_ = std::move(paths);
        hasPrecomputedStreamlines_ = true;
    }

    // Returns the grid cell (row, col) that the vector at (row, col) points
    // toward. Read-only; safe to call from multiple threads simultaneously.
    [[nodiscard]] GridCell downstreamCell(int row, int col) const;
    [[nodiscard]] GridCell downstreamCell(GridCell coords) const;

    // Records one streamline step: src extends toward dest (or merges if dest
    // is already claimed). Thread-safe -- uses atomic union-find operations so
    // multiple threads may call this concurrently.
    void traceStreamlineStep(GridCell src, GridCell dest);

    // Convenience: computes dest and applies in one call. Thread-safe.
    void traceStreamlineStep(GridCell startCoords) {
        traceStreamlineStep(startCoords, downstreamCell(startCoords));
    }
    void traceStreamlineStep(int row, int col) { traceStreamlineStep(GridCell{row, col}); }

    // Returns the unique streamlines found after tracing. Each streamline is an
    // ordered list of (row, col) grid indices reconstructed from the union-find
    // and next[] graph.
    [[nodiscard]] std::vector<Path> getStreamlines() const;
};

} // namespace Field
