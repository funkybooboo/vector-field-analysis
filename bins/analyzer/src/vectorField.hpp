#pragma once
#include "vector.hpp"

#include <memory>
#include <vector>

namespace VectorField {

// Owns a single-step vector field grid and drives the streamline tracing
// algorithm over it. Row index corresponds to the y-axis, col index to x.
class FieldGrid {
    const float xMin, xMax, yMin, yMax;
    std::vector<std::vector<Vector::Vec2>> field;

  public:
    FieldGrid(float xMin, float xMax, float yMin, float yMax,
              std::vector<std::vector<Vector::Vec2>> field)
        : xMin(xMin),
          xMax(xMax),
          yMin(yMin),
          yMax(yMax),
          field(std::move(field)) {}

    // Returns the grid cell (row, col) that the vector at (row, col) points toward
    std::pair<int, int> neighborInVectorDirection(int row, int col);
    std::pair<int, int> neighborInVectorDirection(std::pair<int, int> coords);

    // Merges end's streamline path into start's, redirecting all field vector references.
    // Null or self-merge arguments are silently ignored -- they represent degenerate
    // cases (uninitialized cell, vector pointing back to itself) that produce no path.
    void joinStreamlines(const std::shared_ptr<Vector::Streamline>& start,
                         const std::shared_ptr<Vector::Streamline>& end);

    // Follows the vector at startCoords one step and connects it to the destination streamline
    void traceStreamlineStep(std::pair<int, int> startCoords);
};

} // namespace VectorField
