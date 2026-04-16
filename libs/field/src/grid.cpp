#include "grid.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace Field {

GridCell Grid::downstreamCell(int row, int col) const {
    const int rowCount = static_cast<int>(field_.size());
    if (rowCount == 0) {
        throw std::runtime_error("downstreamCell called on empty field");
    }
    const int colCount = static_cast<int>(field_[0].size());
    if (colCount == 0) {
        throw std::runtime_error("downstreamCell called on zero-width field");
    }
    if (rowCount == 1 || colCount == 1) {
        // A single-row or single-column grid cannot advance in that dimension; return the cell
        // itself.
        return {row, col};
    }

    const Vector::Vec2 start = field_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];

    const float rowSpacing = (bounds_.yMax - bounds_.yMin) / static_cast<float>(rowCount - 1);
    const float colSpacing = (bounds_.xMax - bounds_.xMin) / static_cast<float>(colCount - 1);

    // Advance one step in the vector direction, then snap to the nearest grid
    // index. Clamped to valid index bounds so boundary vectors don't reference
    // off-grid cells.
    const float physRow = indexToCoord(row, rowCount, bounds_.yMin, bounds_.yMax);
    const float physCol = indexToCoord(col, colCount, bounds_.xMin, bounds_.xMax);
    const int nearestRow =
        std::clamp(static_cast<int>(std::round((physRow + start.y - bounds_.yMin) / rowSpacing)), 0,
                   rowCount - 1);
    const int nearestCol =
        std::clamp(static_cast<int>(std::round((physCol + start.x - bounds_.xMin) / colSpacing)), 0,
                   colCount - 1);

    return {nearestRow, nearestCol};
}

GridCell Grid::downstreamCell(GridCell coords) const {
    return downstreamCell(coords.row, coords.col);
}

void Grid::joinStreamlines(const std::shared_ptr<Streamline>& start,
                           const std::shared_ptr<Streamline>& end) {
    if (!start || !end || start == end || start->getPath().empty()) {
        return;
    }

    // Absorb end's path into start and redirect all stream entries at those positions
    for (const auto& point : end->getPath()) {
        start->appendPoint(point);
        streamlines_[static_cast<std::size_t>(point.row)][static_cast<std::size_t>(point.col)] =
            start;
    }
}

// Greedy one-step forward trace: extend src's streamline to dest, or merge the
// two streamlines if dest is already claimed. Not thread-safe -- callers are
// responsible for calling this sequentially (see downstreamCell for
// the parallel-safe read step).
void Grid::traceStreamlineStep(GridCell src, GridCell dest) {
    const auto gridRowCount = static_cast<std::size_t>(streamlines_.size());
    const auto gridColCount = gridRowCount > 0 ? streamlines_[0].size() : 0;
    if (static_cast<std::size_t>(src.row) >= gridRowCount ||
        static_cast<std::size_t>(src.col) >= gridColCount ||
        static_cast<std::size_t>(dest.row) >= gridRowCount ||
        static_cast<std::size_t>(dest.col) >= gridColCount) {
        throw std::out_of_range("traceStreamlineStep: cell coordinates out of grid bounds");
    }

    auto& srcStream =
        streamlines_[static_cast<std::size_t>(src.row)][static_cast<std::size_t>(src.col)];
    if (srcStream == nullptr) {
        srcStream = std::make_shared<Streamline>(src);
    }

    auto& destStream =
        streamlines_[static_cast<std::size_t>(dest.row)][static_cast<std::size_t>(dest.col)];
    if (destStream == nullptr) {
        // Destination is unclaimed: extend the source's streamline into it.
        destStream = srcStream;
        srcStream->appendPoint(dest);
    } else {
        // Destination already belongs to another streamline: the two lines
        // converge here, so merge them into one.
        joinStreamlines(destStream, srcStream);
    }
}

std::vector<Path> Grid::getStreamlines() const {
    if (hasPrecomputedStreamlines_) {
        return precomputedStreamlines_;
    }

    std::unordered_set<Streamline*> seen;
    std::vector<Path> result;
    for (const auto& row : streamlines_) {
        for (const auto& cell : row) {
            if (cell && seen.insert(cell.get()).second) {
                result.push_back(cell->getPath());
            }
        }
    }
    return result;
}

} // namespace Field
