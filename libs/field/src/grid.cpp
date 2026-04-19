#include "grid.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <unordered_set>

namespace Field {

Grid::Grid(Bounds bounds, Slice field)
    : bounds_(bounds),
      rows_(field.size()),
      cols_(rows_ > 0 ? field[0].size() : 0),
      successor_(rows_ * cols_) {
    flatField_.reserve(rows_ * cols_);
    for (auto& row : field) {
        for (auto& v : row) {
            flatField_.push_back(v);
        }
    }

    rowSpacing_ = (rows_ > 1)
        ? (bounds_.yMax - bounds_.yMin) / static_cast<float>(rows_ - 1)
        : 0.0f;
    colSpacing_ = (cols_ > 1)
        ? (bounds_.xMax - bounds_.xMin) / static_cast<float>(cols_ - 1)
        : 0.0f;

    initializeSuccessors();
}

// Grid construction invariant: rows_ and cols_ are non-zero after construction.
void Grid::initializeSuccessors() {
    if (rows_ == 0) {
        throw std::runtime_error("Can't properly initialize empty field");
    }
    if (cols_ == 0) {
        throw std::runtime_error("Can't properly initialize zero-width field");
    }

    const std::size_t total = rows_ * cols_;
    for (std::size_t i = 0; i < total; ++i) {
        successor_[i].store(i, std::memory_order_relaxed);
    }
}

// converts a given coordinate into a unique index so that it can be looked up in a disjoint set
std::size_t Grid::coordsToIndex(std::size_t row, std::size_t col) const {
    return (row * cols_) + col;
}

// Path halving: make every other node in the path point to its grandparent.
std::size_t Grid::findRoot(std::size_t x) const {
    while (successor_[x] != x) {
        std::size_t parent = successor_[x].load(std::memory_order_relaxed);
        std::size_t grandparent = successor_[parent].load(std::memory_order_relaxed);
        // Relaxed: path compression is a best-effort optimization; stale writes are harmless.
        successor_[x].store(grandparent, std::memory_order_relaxed);
        x = grandparent;
    }
    return x;
}

void Grid::unite(std::size_t a, std::size_t b) {
    while (true) {
        a = findRoot(a);
        b = findRoot(b);
        if (a == b) {
            return;
        }

        // Always merge the smaller index into the larger one to keep the DSU
        // structure deterministic and avoid cycles.
        if (a < b) {
            std::swap(a, b);
        }

        std::size_t expected = b;
        if (successor_[b].compare_exchange_weak(expected, a, std::memory_order_release,
                                                std::memory_order_relaxed)) {
            return;
        }
    }
}

GridCell Grid::downstreamCell(int row, int col) const {
    const int rowCount = static_cast<int>(rows_);
    const int colCount = static_cast<int>(cols_);
    if (rowCount == 1 || colCount == 1) {
        // A single-row or single-column grid cannot advance in that dimension; return the cell
        // itself.
        return {row, col};
    }

    const Vector::Vec2 start =
        flatField_[(static_cast<std::size_t>(row) * cols_) + static_cast<std::size_t>(col)];

    // Advance one step in the vector direction, then snap to the nearest grid
    // index. Clamped to valid index bounds so boundary vectors don't reference
    // off-grid cells.
    const float physRow = indexToCoord(row, rowCount, bounds_.yMin, bounds_.yMax);
    const float physCol = indexToCoord(col, colCount, bounds_.xMin, bounds_.xMax);
    const int nearestRow =
        std::clamp(static_cast<int>(std::round((physRow + start.y - bounds_.yMin) / rowSpacing_)),
                   0, rowCount - 1);
    const int nearestCol =
        std::clamp(static_cast<int>(std::round((physCol + start.x - bounds_.xMin) / colSpacing_)),
                   0, colCount - 1);

    return {nearestRow, nearestCol};
}

GridCell Grid::downstreamCell(GridCell coords) const {
    return downstreamCell(coords.row, coords.col);
}

void Grid::joinStreamlines(const std::shared_ptr<Streamline>& start,
                           const std::shared_ptr<Streamline>& end) {
    if (!start || !end) {
        return;
    }

    auto rootStart = start->resolve();
    auto rootEnd = end->resolve();

    if (rootStart == rootEnd || rootEnd->getPath().empty()) {
        return;
    }

    // O(1) merge: transfers the entire list from end to start.
    rootStart->absorb(*rootEnd);
    rootEnd->parent = rootStart;
}

// Greedy one-step forward trace: extend src's streamline to dest, or merge the
// two streamlines if dest is already claimed. Not thread-safe -- callers are
// responsible for calling this sequentially (see downstreamCell for
// the parallel-safe read step).
void Grid::traceStreamlineStep(GridCell src, GridCell dest) {
    if (streamlines_.empty()) {
        streamlines_.assign(rows_, std::vector<std::shared_ptr<Streamline>>(cols_, nullptr));
    }
    const auto gridRowCount = rows_;
    const auto gridColCount = cols_;
    if (static_cast<std::size_t>(src.row) >= gridRowCount ||
        static_cast<std::size_t>(src.col) >= gridColCount ||
        static_cast<std::size_t>(dest.row) >= gridRowCount ||
        static_cast<std::size_t>(dest.col) >= gridColCount) {
        throw std::out_of_range("traceStreamlineStep: cell coordinates out of grid bounds");
    }

    auto& srcSlot =
        streamlines_[static_cast<std::size_t>(src.row)][static_cast<std::size_t>(src.col)];
    if (srcSlot == nullptr) {
        srcSlot = std::make_shared<Streamline>(src);
    }
    auto srcStream = srcSlot->resolve();

    auto& destSlot =
        streamlines_[static_cast<std::size_t>(dest.row)][static_cast<std::size_t>(dest.col)];
    if (destSlot == nullptr) {
        // Destination is unclaimed: extend the source's streamline into it.
        destSlot = srcStream;
        srcStream->appendPoint(dest);
    } else {
        // Destination already belongs to another streamline: the two lines
        // converge here, so merge them into one.
        joinStreamlines(srcStream, destSlot);
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
            if (cell) {
                auto root = cell->resolve();
                if (!root->getPath().empty() && seen.insert(root.get()).second) {
                    result.push_back(root->getPath());
                }
            }
        }
    }
    return result;
}

} // namespace Field
