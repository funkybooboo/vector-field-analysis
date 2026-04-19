#include "streamlineSolver.hpp"

#include <cstddef>
#include <vector>

std::vector<std::size_t>
StreamlineSolver::sortCellsByRoot(const std::vector<std::size_t>& roots, std::size_t total) {
    std::vector<std::size_t> counts(total, 0);
    for (std::size_t i = 0; i < total; ++i)
        counts[roots[i]]++;

    std::vector<std::size_t> writePos(total, 0);
    for (std::size_t i = 1; i < total; ++i)
        writePos[i] = writePos[i - 1] + counts[i - 1];

    std::vector<std::size_t> indices(total);
    for (std::size_t i = 0; i < total; ++i)
        indices[writePos[roots[i]]++] = i;

    return indices;
}

std::vector<Field::Path>
StreamlineSolver::buildPathsForRange(const std::vector<std::size_t>& roots,
                                     const std::vector<std::size_t>& indices,
                                     std::size_t totalCells,
                                     int colCount,
                                     std::size_t startIdx,
                                     std::size_t endIdx,
                                     std::size_t threadIdx) {
    std::vector<Field::Path> localPaths;
    if (startIdx >= endIdx)
        return localPaths;

    std::size_t currentStart = startIdx;

    // Skip leading elements that belong to the same root as the tail of the
    // previous thread's range, so each root is owned by exactly one thread.
    // Guard: threadIdx==0 implies startIdx==0; currentStart-1 would underflow.
    if (threadIdx > 0) {
        while (currentStart < endIdx &&
               roots[indices[currentStart]] == roots[indices[currentStart - 1]])
            ++currentStart;
    }

    while (currentStart < endIdx) {
        std::size_t segmentEnd = currentStart + 1;
        // Scan to the true end of this root's segment — may extend past endIdx,
        // which is correct: this thread owns all cells of any root it starts.
        while (segmentEnd < totalCells &&
               roots[indices[segmentEnd]] == roots[indices[currentStart]])
            ++segmentEnd;

        Field::Path path;
        for (std::size_t j = currentStart; j < segmentEnd; ++j) {
            const std::size_t idx = indices[j];
            path.push_back({static_cast<int>(idx / static_cast<std::size_t>(colCount)),
                            static_cast<int>(idx % static_cast<std::size_t>(colCount))});
        }
        localPaths.push_back(std::move(path));
        currentStart = segmentEnd;
    }

    return localPaths;
}

std::vector<Field::Path>
StreamlineSolver::reconstructPathsDSU(const Field::Grid& grid,
                                      const std::vector<Field::GridCell>& neighbors) {
    const std::size_t total = neighbors.size();
    const int colCount = static_cast<int>(grid.cols());

    std::vector<std::size_t> roots(total);
    for (std::size_t i = 0; i < total; ++i)
        roots[i] = grid.findRoot(i);

    const std::vector<std::size_t> indices = sortCellsByRoot(roots, total);
    return buildPathsForRange(roots, indices, total, colCount, 0, total, 0);
}
