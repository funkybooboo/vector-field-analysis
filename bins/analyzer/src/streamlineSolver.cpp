#include "streamlineSolver.hpp"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

std::vector<Field::Path>
StreamlineSolver::reconstructPathsDSU(const Field::Grid& grid,
                                      const std::vector<Field::GridCell>& neighbors) {

    const std::size_t total = neighbors.size();
    if (total == 0) {
        return {};
    }

    const int colCount = static_cast<int>(grid.cols());

    // 1. Compute roots for all cells.
    std::vector<std::size_t> roots(total);
    for (std::size_t i = 0; i < total; ++i) {
        roots[i] = grid.findRoot(i);
    }

    // 2. Sort indices by root to group members of the same component.
    std::vector<std::size_t> indices(total);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return roots[a] < roots[b];
    });

    // 3. Create paths from contiguous segments of the same root.
    std::vector<Field::Path> output;
    std::size_t currentStart = 0;
    while (currentStart < total) {
        std::size_t segmentEnd = currentStart + 1;
        while (segmentEnd < total && roots[indices[segmentEnd]] == roots[indices[currentStart]]) {
            segmentEnd++;
        }

        Field::Path path;
        for (std::size_t j = currentStart; j < segmentEnd; ++j) {
            std::size_t idx = indices[j];
            path.push_back({static_cast<int>(idx / colCount), static_cast<int>(idx % colCount)});
        }
        output.push_back(std::move(path));

        currentStart = segmentEnd;
    }

    return output;
}
