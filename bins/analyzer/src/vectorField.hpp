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
    const float xMin, xMax, yMin, yMax;
    Vector::FieldSlice field_;
    std::vector<std::vector<std::shared_ptr<Vector::Streamline>>> streams_;

  public:
    FieldGrid(float xMin, float xMax, float yMin, float yMax, Vector::FieldSlice field)
        : xMin(xMin),
          xMax(xMax),
          yMin(yMin),
          yMax(yMax),
          field_(std::move(field)) {
        const std::size_t r = field_.size();
        const std::size_t c = r > 0 ? field_[0].size() : 0;
        streams_.assign(r, std::vector<std::shared_ptr<Vector::Streamline>>(c, nullptr));
    }

    std::size_t rows() const { return field_.size(); }
    std::size_t cols() const { return field_.empty() ? 0 : field_[0].size(); }

    // Returns the grid cell (row, col) that the vector at (row, col) points
    // toward. Read-only; safe to call from multiple threads simultaneously.
    std::pair<int, int> neighborInVectorDirection(int row, int col) const;
    std::pair<int, int> neighborInVectorDirection(std::pair<int, int> coords) const;

    // Merges end's streamline path into start's, redirecting all field vector
    // references. Null or self-merge arguments are silently ignored -- they
    // represent degenerate cases (uninitialized cell, vector pointing back to
    // itself) that produce no path.
    void joinStreamlines(const std::shared_ptr<Vector::Streamline>& start,
                         const std::shared_ptr<Vector::Streamline>& end);

    // Applies one streamline step: src extends toward dest (or merges if dest
    // is already claimed). NOT thread-safe -- call from one thread at a time.
    // Parallel callers should gather all (src,dest) pairs via
    // neighborInVectorDirection first, then apply sequentially.
    void traceStreamlineStep(std::pair<int, int> src, std::pair<int, int> dest);

    // Convenience: computes dest and applies in one call. Not thread-safe.
    void traceStreamlineStep(std::pair<int, int> startCoords) {
        traceStreamlineStep(startCoords, neighborInVectorDirection(startCoords));
    }
    void traceStreamlineStep(int row, int col) {
        traceStreamlineStep(std::make_pair(row, col));
    }
};

} // namespace VectorField
