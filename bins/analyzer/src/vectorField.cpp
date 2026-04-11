#include "vectorField.hpp"

#include "vector.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace VectorField {

std::pair<int, int> FieldGrid::neighborInVectorDirection(int row, int col) {
    const Vector::Vec2 start = field[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];

    const float rowSpacing = (yMax - yMin) / static_cast<float>(field.size() - 1);
    const float colSpacing = (xMax - xMin) / static_cast<float>(field[0].size() - 1);

    // Advance one step in the vector direction, then snap to the nearest grid index.
    // Clamped to valid index bounds so boundary vectors don't reference off-grid cells.
    const int nearestRow =
        std::clamp(static_cast<int>(
                       std::round(((static_cast<float>(row) * rowSpacing) + start.y) / rowSpacing)),
                   0, static_cast<int>(field.size()) - 1);
    const int nearestCol =
        std::clamp(static_cast<int>(
                       std::round(((static_cast<float>(col) * colSpacing) + start.x) / colSpacing)),
                   0, static_cast<int>(field[0].size()) - 1);

    return {nearestRow, nearestCol};
}

std::pair<int, int> FieldGrid::neighborInVectorDirection(std::pair<int, int> coords) {
    return neighborInVectorDirection(coords.first, coords.second);
}

void FieldGrid::joinStreamlines(const std::shared_ptr<Vector::Streamline>& start,
                                const std::shared_ptr<Vector::Streamline>& end) {
    if (!start || !end || start == end || start->path.empty()) {
        return;
    }

    // Absorb end's path into start and redirect all stream entries at those positions
    for (const auto& point : end->path) {
        start->path.push_back(point);
        streams_[static_cast<std::size_t>(point.first)][static_cast<std::size_t>(point.second)]
            = start;
    }
}

// Greedy one-step forward trace: advance from startCoords to the neighbor cell
// its vector points toward. Either extends the current streamline if that cell
// is unclaimed, or merges with the existing streamline if two paths converge there.
void FieldGrid::traceStreamlineStep(std::pair<int, int> startCoords) {
    auto& srcStream = streams_[static_cast<std::size_t>(startCoords.first)]
                               [static_cast<std::size_t>(startCoords.second)];

    if (srcStream == nullptr) {
        srcStream = std::make_shared<Vector::Streamline>(startCoords);
    }

    const std::pair<int, int> destCoords = neighborInVectorDirection(startCoords);
    auto& destStream = streams_[static_cast<std::size_t>(destCoords.first)]
                                [static_cast<std::size_t>(destCoords.second)];

    if (destStream == nullptr) {
        // Destination is unclaimed: extend the source's streamline into it.
        destStream = srcStream;
        srcStream->path.push_back(destCoords);
    } else {
        // Destination already belongs to another streamline: the two lines
        // converge here, so merge them into one.
        joinStreamlines(destStream, srcStream);
    }
}

} // namespace VectorField
