#include "openMpStreamlineSolver.hpp"

#include "sequentialStreamlineSolver.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

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
    if (rowCount == 0) {
        return;
    }
    const int colCount = static_cast<int>(grid.cols());

    // Pass 1: parallel -- each cell reads its neighbor direction from field_ (read-only).
    // downstreamCell is const and touches no shared mutable state.
    const std::size_t total =
        static_cast<std::size_t>(rowCount) * static_cast<std::size_t>(colCount);
    std::vector<Field::GridCell> neighbors(total);

#pragma omp parallel for schedule(static) collapse(2)
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            neighbors[(static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount)) +
                      static_cast<std::size_t>(col)] = grid.downstreamCell(row, col);
        }
    }

    // Pass 2a: Parallel Union-Find using lock-free atomics in Field::Grid.
#pragma omp parallel for schedule(static)
    for (std::size_t i = 0; i < total; ++i) {
        grid.unite(i, grid.coordsToIndex(neighbors[i].row, neighbors[i].col));
    }

    // Pass 2b: Sequential reconstruction of deterministic paths.
    grid.setPrecomputedStreamlines(reconstructPathsDSU(grid, neighbors));
#else
    SequentialStreamlineSolver fallback;
    fallback.computeTimeStep(grid);
#endif
}
