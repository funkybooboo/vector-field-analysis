#include "openMpStreamlineSolver.hpp"

#include "sequentialStreamlineSolver.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

OpenMpStreamlineSolver::OpenMpStreamlineSolver([[maybe_unused]] unsigned int threadCount)
#ifdef _OPENMP
    : threadCount_(threadCount)
#endif
{
}

void OpenMpStreamlineSolver::computeTimeStep(Field::Grid& grid) {
#ifdef _OPENMP
    if (threadCount_ > 0) {
        omp_set_num_threads(static_cast<int>(threadCount_));
    }

    const int rowCount = static_cast<int>(grid.rows());
    if (rowCount == 0)
        return;
    const int colCount = static_cast<int>(grid.cols());
    const std::size_t totalCells =
        static_cast<std::size_t>(rowCount) * static_cast<std::size_t>(colCount);

    std::vector<Field::GridCell> neighbors(totalCells);
    std::vector<std::size_t> roots(totalCells);
    std::vector<std::size_t> indices(totalCells);
    std::vector<std::vector<Field::Path>> localPathsCollection(
        threadCount_ > 0 ? threadCount_ : std::thread::hardware_concurrency());

#pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        const int numThreads = omp_get_num_threads();

        // Pass 1: compute all (src, dest) neighbor pairs.
#pragma omp for collapse(2) schedule(static)
        for (int row = 0; row < rowCount; row++) {
            for (int col = 0; col < colCount; col++) {
                neighbors[(static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount)) +
                          static_cast<std::size_t>(col)] = grid.downstreamCell(row, col);
            }
        }

        // Pass 2a: Parallel Union-Find.
#pragma omp for schedule(static)
        for (std::size_t i = 0; i < totalCells; ++i) {
            const std::size_t srcIndex = i;
            const std::size_t destIndex = grid.coordsToIndex(neighbors[i].row, neighbors[i].col);
            grid.unite(srcIndex, destIndex);
        }

        // Pass 2b part 1: Compute roots for all cells in parallel.
#pragma omp for schedule(static)
        for (std::size_t i = 0; i < totalCells; ++i) {
            roots[i] = grid.findRoot(i);
        }

        // Pass 2b part 2: Thread 0 sorts indices by root.
#pragma omp single
        {
            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(),
                      [&](std::size_t a, std::size_t b) { return roots[a] < roots[b]; });
        }

        // Pass 2b part 3: Create paths from segments in parallel.
        // We divide the sorted indices among threads and find segment boundaries.
        const std::size_t segmentsPerThread = totalCells / static_cast<std::size_t>(numThreads);
        const std::size_t remainderSegments = totalCells % static_cast<std::size_t>(numThreads);
        const std::size_t startIdx = static_cast<std::size_t>(tid) * segmentsPerThread +
                                     std::min(static_cast<std::size_t>(tid), remainderSegments);
        const std::size_t endIdx = startIdx + segmentsPerThread +
                                   (static_cast<std::size_t>(tid) < remainderSegments ? 1 : 0);

        if (startIdx < endIdx) {
            std::size_t currentStart = startIdx;

            // Adjust currentStart so we don't start in the middle of a segment owned by previous
            // thread.
            if (tid > 0) {
                while (currentStart < endIdx &&
                       roots[indices[currentStart]] == roots[indices[currentStart - 1]]) {
                    currentStart++;
                }
            }

            while (currentStart < endIdx) {
                std::size_t segmentEnd = currentStart + 1;
                while (segmentEnd < totalCells &&
                       roots[indices[segmentEnd]] == roots[indices[currentStart]]) {
                    segmentEnd++;
                }

                Field::Path path;
                for (std::size_t j = currentStart; j < segmentEnd; ++j) {
                    std::size_t idx = indices[j];
                    path.push_back(
                        {static_cast<int>(idx / colCount), static_cast<int>(idx % colCount)});
                }
                localPathsCollection[static_cast<std::size_t>(tid)].push_back(std::move(path));

                currentStart = segmentEnd;
            }
        }
    }

    // Final merge of paths from all threads.
    std::vector<Field::Path> finalPaths;
    for (auto& local : localPathsCollection) {
        finalPaths.insert(finalPaths.end(), std::make_move_iterator(local.begin()),
                          std::make_move_iterator(local.end()));
    }
    grid.setPrecomputedStreamlines(std::move(finalPaths));

#else
    SequentialStreamlineSolver fallback;
    fallback.computeTimeStep(grid);
#endif
}
