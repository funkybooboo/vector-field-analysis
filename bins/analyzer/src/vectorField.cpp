#include "vectorField.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace VectorField {

std::pair<int, int> FieldGrid::neighborInVectorDirection(int row, int col) const {
    const Vector::Vec2 start = field_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];

    const float rowSpacing = (yMax - yMin) / static_cast<float>(field_.size() - 1);
    const float colSpacing = (xMax - xMin) / static_cast<float>(field_[0].size() - 1);

    // Advance one step in the vector direction, then snap to the nearest grid
    // index. Clamped to valid index bounds so boundary vectors don't reference
    // off-grid cells.
    const int nearestRow =
        std::clamp(static_cast<int>(
                       std::round(((static_cast<float>(row) * rowSpacing) + start.y) / rowSpacing)),
                   0, static_cast<int>(field_.size()) - 1);
    const int nearestCol =
        std::clamp(static_cast<int>(
                       std::round(((static_cast<float>(col) * colSpacing) + start.x) / colSpacing)),
                   0, static_cast<int>(field_[0].size()) - 1);

    return {nearestRow, nearestCol};
}

std::pair<int, int> FieldGrid::neighborInVectorDirection(std::pair<int, int> coords) const {
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
        streams_[static_cast<std::size_t>(point.first)][static_cast<std::size_t>(point.second)] =
            start;
    }
}

// Greedy one-step forward trace: extend src's streamline to dest, or merge the
// two streamlines if dest is already claimed. Not thread-safe -- callers are
// responsible for calling this sequentially (see neighborInVectorDirection for
// the parallel-safe read step).
void FieldGrid::traceStreamlineStep(std::pair<int, int> src, std::pair<int, int> dest) {
    auto& srcStream =
        streams_[static_cast<std::size_t>(src.first)][static_cast<std::size_t>(src.second)];
    if (srcStream == nullptr) {
        srcStream = std::make_shared<Vector::Streamline>(src);
    }

    auto& destStream =
        streams_[static_cast<std::size_t>(dest.first)][static_cast<std::size_t>(dest.second)];
    if (destStream == nullptr) {
        // Destination is unclaimed: extend the source's streamline into it.
        destStream = srcStream;
        srcStream->path.push_back(dest);
    } else {
        // Destination already belongs to another streamline: the two lines
        // converge here, so merge them into one.
        joinStreamlines(destStream, srcStream);
    }
}

} // namespace VectorField
