#include "grid.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <vector>

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
        return {row, col};
    }

    const Vector::Vec2 start =
        flatField_[(static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount)) +
                   static_cast<std::size_t>(col)];

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

// find with halving path compression (lock-free, atomic CAS).
int Grid::ufFind(int i) const {
    while (true) {
        int parent = ufParent_[i].load(std::memory_order_acquire);
        if (parent == i) {
            return i;
        }
        int grandparent = ufParent_[parent].load(std::memory_order_acquire);
        // Path halving: try to point i directly at grandparent.
        ufParent_[i].compare_exchange_weak(parent, grandparent, std::memory_order_acq_rel,
                                           std::memory_order_relaxed);
        i = grandparent;
    }
}

// Union by rank (atomic CAS). Idempotent if a and b are already in the same set.
void Grid::ufUnion(int a, int b) {
    while (true) {
        a = ufFind(a);
        b = ufFind(b);
        if (a == b) {
            return;
        }

        int ra = ufRank_[a].load(std::memory_order_acquire);
        int rb = ufRank_[b].load(std::memory_order_acquire);

        // Always union lower-rank root under higher-rank root.
        // When ranks are equal, make a the root by convention (lower index wins
        // if a < b to keep merges deterministic).
        if (ra < rb || (ra == rb && a > b)) {
            std::swap(a, b);
            std::swap(ra, rb);
        }

        // Try to set b's parent to a.
        int expected = b;
        if (!ufParent_[b].compare_exchange_weak(expected, a, std::memory_order_acq_rel,
                                                std::memory_order_relaxed)) {
            // Another thread changed b's parent; retry.
            continue;
        }

        // If ranks were equal, increment a's rank.
        if (ra == rb) {
            ufRank_[a].fetch_add(1, std::memory_order_relaxed);
        }
        return;
    }
}

void Grid::traceStreamlineStep(GridCell src, GridCell dest) {
    const std::size_t gridRowCount = colCount_ > 0 ? ufParent_.size() / colCount_ : 0;
    if (static_cast<std::size_t>(src.row) >= gridRowCount ||
        static_cast<std::size_t>(src.col) >= colCount_ ||
        static_cast<std::size_t>(dest.row) >= gridRowCount ||
        static_cast<std::size_t>(dest.col) >= colCount_) {
        throw std::out_of_range("traceStreamlineStep: cell coordinates out of grid bounds");
    }

    const int si = (src.row * static_cast<int>(colCount_)) + src.col;
    const int di = (dest.row * static_cast<int>(colCount_)) + dest.col;

    // Lazy-initialize each cell's parent to itself on first visit.
    int expected = -1;
    ufParent_[static_cast<std::size_t>(si)].compare_exchange_strong(
        expected, si, std::memory_order_acq_rel, std::memory_order_relaxed);
    expected = -1;
    ufParent_[static_cast<std::size_t>(di)].compare_exchange_strong(
        expected, di, std::memory_order_acq_rel, std::memory_order_relaxed);

    // Record the downstream pointer for src (each cell is written by one thread).
    ufNext_[static_cast<std::size_t>(si)] = di;

    ufUnion(si, di);
}

// Reconstruct the ordered path for one streamline component by following
// ufNext_ links from each source cell (in-degree 0). Sources are processed in
// ascending row-major order so output is deterministic across parallel runs.
Path Grid::buildComponentPath(const std::vector<int>& sources, std::vector<bool>& visited) const {
    Path path;
    path.reserve(sources.size() * 2);
    for (int src : sources) {
        int cur = src;
        while (cur >= 0 && !visited[static_cast<std::size_t>(cur)]) {
            visited[static_cast<std::size_t>(cur)] = true;
            path.push_back({cur / static_cast<int>(colCount_), cur % static_cast<int>(colCount_)});
            const int nxt = ufNext_[static_cast<std::size_t>(cur)];
            if (nxt < 0 || nxt == cur) {
                break;
            }
            cur = nxt;
        }
    }
    return path;
}

std::vector<Path> Grid::getStreamlines() const {
    if (hasPrecomputedStreamlines_) {
        return precomputedStreamlines_;
    }

    const std::size_t total = ufParent_.size();
    if (total == 0) {
        return {};
    }

    // Compute in-degree of each cell in the ufNext_ graph (visited cells only).
    std::vector<int> inDegree(total, 0);
    for (std::size_t i = 0; i < total; ++i) {
        if (ufParent_[i].load(std::memory_order_relaxed) == -1) {
            continue;
        }
        const int nxt = ufNext_[i];
        if (nxt >= 0 && nxt != static_cast<int>(i)) {
            ++inDegree[static_cast<std::size_t>(nxt)];
        }
    }

    // Group visited cells by union-find root. std::map keeps roots in ascending
    // index order so getStreamlines output is deterministic across parallel runs.
    std::map<int, std::vector<int>> components;
    for (std::size_t i = 0; i < total; ++i) {
        if (ufParent_[i].load(std::memory_order_relaxed) == -1) {
            continue;
        }
        components[ufFind(static_cast<int>(i))].push_back(static_cast<int>(i));
    }

    std::vector<Path> result;
    result.reserve(components.size());

    std::vector<bool> visited(total, false);

    for (auto& [root, cells] : components) {
        std::vector<int> sources;
        std::copy_if(cells.begin(), cells.end(), std::back_inserter(sources),
                     [&](int c) { return inDegree[static_cast<std::size_t>(c)] == 0; });
        std::sort(sources.begin(), sources.end());

        Path path = buildComponentPath(sources, visited);
        if (!path.empty()) {
            result.push_back(std::move(path));
        }
    }

    // Sort by first cell so output order is canonical (row-major ascending).
    std::sort(result.begin(), result.end(), [](const Path& a, const Path& b) {
        if (a.empty()) {
            return !b.empty();
        }
        if (b.empty()) {
            return false;
        }
        return a.front() < b.front();
    });
    return result;
}

} // namespace Field
