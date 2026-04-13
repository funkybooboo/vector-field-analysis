#pragma once
#include "streamline.hpp"
#include "vector.hpp"

#include <memory>
#include <vector>

namespace VectorField {

// Owns a single-step vector field grid and drives the streamline tracing
// algorithm over it. Row index corresponds to the y-axis, col index to x.
// Stream associations are tracked in a parallel grid (streams_) rather than
// inside Vec2, keeping the generic math type free of domain state.
class FieldGrid {
    const Vector::FieldBounds bounds_;
    Vector::FieldSlice field_;
    std::vector<std::vector<std::shared_ptr<Vector::Streamline>>> streams_;

  public:
    FieldGrid(Vector::FieldBounds bounds, Vector::FieldSlice field)
        : bounds_(bounds),
          field_(std::move(field)) {
        const std::size_t numRows = field_.size();
        const std::size_t numCols = numRows > 0 ? field_[0].size() : 0;
        streams_.assign(numRows, std::vector<std::shared_ptr<Vector::Streamline>>(numCols, nullptr));
    }

    [[nodiscard]] std::size_t rows() const { return field_.size(); }
    [[nodiscard]] std::size_t cols() const { return field_.empty() ? 0 : field_[0].size(); }

    // Returns the grid cell (row, col) that the vector at (row, col) points
    // toward. Read-only; safe to call from multiple threads simultaneously.
    [[nodiscard]] Vector::GridCell downstreamCell(int row, int col) const;
    [[nodiscard]] Vector::GridCell downstreamCell(Vector::GridCell coords) const;

    // Merges end's streamline path into start's, redirecting all field vector
    // references. Null or self-merge arguments are silently ignored -- they
    // represent degenerate cases (uninitialized cell, vector pointing back to
    // itself) that produce no path.
    void joinStreamlines(const std::shared_ptr<Vector::Streamline>& start,
                         const std::shared_ptr<Vector::Streamline>& end);

    // Applies one streamline step: src extends toward dest (or merges if dest
    // is already claimed). NOT thread-safe -- call from one thread at a time.
    // Parallel callers should gather all (src,dest) pairs via
    // downstreamCell first, then apply sequentially.
    void traceStreamlineStep(Vector::GridCell src, Vector::GridCell dest);

    // Convenience: computes dest and applies in one call. Not thread-safe.
    void traceStreamlineStep(Vector::GridCell startCoords) {
        traceStreamlineStep(startCoords, downstreamCell(startCoords));
    }
    void traceStreamlineStep(int row, int col) {
        traceStreamlineStep(Vector::GridCell{row, col});
    }

    // Returns the unique streamlines found after tracing. Each streamline is an
    // ordered list of (row, col) grid indices. Collected in row-major iteration
    // order of streams_ — deterministic for a given trace run.
    [[nodiscard]] std::vector<Vector::Path> getStreamlines() const;
};

} // namespace VectorField
