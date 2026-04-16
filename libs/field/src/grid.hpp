#pragma once
#include "fieldTypes.hpp"
#include "streamline.hpp"

#include <memory>
#include <vector>

namespace Field {

// Owns a single-frame vector field grid and drives the streamline tracing
// algorithm over it. Row index corresponds to the y-axis, col index to x.
// Streamline associations are tracked in a parallel grid (streamlines_) rather
// than inside Vec2, keeping the generic math type free of domain state.
class Grid {
    const Bounds bounds_;
    Slice field_;
    std::vector<std::vector<std::shared_ptr<Streamline>>> streamlines_;

    // for externally computed streamline result
    // used only by cudaFull
    std::vector<Path> precomputedStreamlines_;
    bool hasPrecomputedStreamlines_ = false;

  public:
    Grid(Bounds bounds, Slice field)
        : bounds_(bounds),
          field_(std::move(field)) {
        const std::size_t numRows = field_.size();
        const std::size_t numCols = numRows > 0 ? field_[0].size() : 0;
        streamlines_.assign(numRows, std::vector<std::shared_ptr<Streamline>>(numCols, nullptr));
    }

    [[nodiscard]] std::size_t rows() const { return field_.size(); }
    [[nodiscard]] std::size_t cols() const { return field_.empty() ? 0 : field_[0].size(); }

    [[nodiscard]] const Bounds& bounds() const { return bounds_; }
    [[nodiscard]] const Slice& field() const { return field_; }

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

    // Merges end's streamline path into start's, redirecting all field vector
    // references. Null or self-merge arguments are silently ignored -- they
    // represent degenerate cases (uninitialized cell, vector pointing back to
    // itself) that produce no path.
    void joinStreamlines(const std::shared_ptr<Streamline>& start,
                         const std::shared_ptr<Streamline>& end);

    // Applies one streamline step: src extends toward dest (or merges if dest
    // is already claimed). NOT thread-safe -- call from one thread at a time.
    // Parallel callers should gather all (src,dest) pairs via
    // downstreamCell first, then apply sequentially.
    void traceStreamlineStep(GridCell src, GridCell dest);

    // Convenience: computes dest and applies in one call. Not thread-safe.
    void traceStreamlineStep(GridCell startCoords) {
        traceStreamlineStep(startCoords, downstreamCell(startCoords));
    }
    void traceStreamlineStep(int row, int col) { traceStreamlineStep(GridCell{row, col}); }

    // Returns the unique streamlines found after tracing. Each streamline is an
    // ordered list of (row, col) grid indices. Collected in row-major iteration
    // order of streamlines_ -- deterministic for a given trace run.
    [[nodiscard]] std::vector<Path> getStreamlines() const;
};

} // namespace Field
