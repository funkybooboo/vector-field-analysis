#include "openMpStreamlineSolver.hpp"

#include "sequentialStreamlineSolver.hpp"
#include "streamlineSolver.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <vector>

OpenMpStreamlineSolver::OpenMpStreamlineSolver([[maybe_unused]] unsigned int threadCount)
#ifdef _OPENMP
    : threadCount_(threadCount)
#endif
{
}

void OpenMpStreamlineSolver::computeTimeStep(Field::Grid& grid) {
#ifdef _OPENMP
    if (threadCount_ > 0)
        omp_set_num_threads(static_cast<int>(threadCount_));

    const int rowCount = static_cast<int>(grid.rows());
    if (rowCount == 0)
        return;
    const int colCount = static_cast<int>(grid.cols());
    const std::size_t totalCells =
        static_cast<std::size_t>(rowCount) * static_cast<std::size_t>(colCount);

    std::vector<Field::GridCell> neighbors(totalCells);
    std::vector<std::size_t> roots(totalCells);
    std::vector<std::size_t> indices;
    std::vector<std::vector<Field::Path>> localPathsCollection;

    // Pass 1: parallel neighbor computation.
#pragma omp parallel for collapse(2) schedule(static)
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            neighbors[(static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount)) +
                      static_cast<std::size_t>(col)] = grid.downstreamCell(row, col);
        }
    }

    // Pass 2: sequential unite -- produces shorter DSU paths than parallel CAS,
    // making the subsequent findRoot pass fast (O(n*alpha(n)) instead of O(n*log n)).
    for (std::size_t i = 0; i < totalCells; ++i)
        grid.unite(i, grid.coordsToIndex(neighbors[i].row, neighbors[i].col));

    // Passes 3-5: parallel findRoot -> serial sort -> parallel build.
#pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        const int numThreads = omp_get_num_threads();

        // Pass 3: parallel findRoot (fast because sequential unite keeps paths short).
#pragma omp for schedule(static)
        for (std::size_t i = 0; i < totalCells; ++i)
            roots[i] = grid.findRoot(i);

        // Pass 4: thread 0 does counting sort while others wait (implicit barrier after omp for).
#pragma omp single
        {
            localPathsCollection.resize(static_cast<std::size_t>(numThreads));
            indices = StreamlineSolver::sortCellsByRoot(roots, totalCells);
        }

        // Pass 5: parallel path building.
        const std::size_t segmentsPerThread = totalCells / static_cast<std::size_t>(numThreads);
        const std::size_t remainderSegments = totalCells % static_cast<std::size_t>(numThreads);
        const std::size_t startIdx = static_cast<std::size_t>(tid) * segmentsPerThread +
                                     std::min(static_cast<std::size_t>(tid), remainderSegments);
        const std::size_t endIdx = startIdx + segmentsPerThread +
                                   (static_cast<std::size_t>(tid) < remainderSegments ? 1 : 0);

        localPathsCollection[static_cast<std::size_t>(tid)] = StreamlineSolver::buildPathsForRange(
            roots, indices, totalCells, colCount, startIdx, endIdx, static_cast<std::size_t>(tid));
    }

    // Merge paths from all threads.
    std::vector<Field::Path> finalPaths;
    std::size_t totalPaths = 0;
    for (const auto& local : localPathsCollection)
        totalPaths += local.size();
    finalPaths.reserve(totalPaths);
    for (auto& local : localPathsCollection)
        finalPaths.insert(finalPaths.end(), std::make_move_iterator(local.begin()),
                          std::make_move_iterator(local.end()));

    grid.setPrecomputedStreamlines(std::move(finalPaths));

#else
    SequentialStreamlineSolver fallback;
    fallback.computeTimeStep(grid);
#endif
}
