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

    // Advance one step in the vector direction, then snap to the nearest grid index
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

    // Absorb end's path into start and redirect all field vectors at those positions
    for (const auto& point : end->path) {
        start->path.push_back(point);
        field[static_cast<std::size_t>(point.first)][static_cast<std::size_t>(point.second)]
            .stream = start;
    }
}

void FieldGrid::traceStreamlineStep(std::pair<int, int> startCoords) {
    Vector::Vec2& sourceVec = field[static_cast<std::size_t>(startCoords.first)]
                                   [static_cast<std::size_t>(startCoords.second)];

    if (sourceVec.stream == nullptr) {
        sourceVec.stream = std::make_shared<Vector::Streamline>(startCoords);
    }

    const std::pair<int, int> destCoords = neighborInVectorDirection(startCoords);
    Vector::Vec2& destination = field[static_cast<std::size_t>(destCoords.first)]
                                     [static_cast<std::size_t>(destCoords.second)];

    if (destination.stream == nullptr) {
        destination.stream = sourceVec.stream;
        sourceVec.stream->path.push_back(destCoords);
    } else {
        joinStreamlines(destination.stream, sourceVec.stream);
    }
}

} // namespace VectorField
