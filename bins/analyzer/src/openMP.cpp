#include "openMP.hpp"

#include "sequentialCPU.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <vector>

OpenMP::OpenMP(unsigned int threadCount)
#ifdef _OPENMP
    : threadCount_(threadCount)
#endif
{
#ifndef _OPENMP
    (void)threadCount;
#endif
}

void OpenMP::computeTimeStep(VectorField::FieldGrid& grid) {
#ifdef _OPENMP
    if (threadCount_ > 0) {
        omp_set_num_threads(static_cast<int>(threadCount_));
    }

    const int rowCount = static_cast<int>(grid.rows());
    if (rowCount == 0) {
        return;
    }
    const int colCount = static_cast<int>(grid.cols());

    // Pass 1: parallel — each cell reads its neighbor direction from field_ (read-only).
    // downstreamCell is const and touches no shared mutable state.
    std::vector<Vector::GridCell> neighbors(static_cast<std::size_t>(rowCount) *
                                            static_cast<std::size_t>(colCount));

#pragma omp parallel for schedule(static) collapse(2)
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            neighbors[static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount) +
                      static_cast<std::size_t>(col)] = grid.downstreamCell(row, col);
        }
    }

    // Pass 2: sequential — apply streamline merges using the precomputed pairs.
    // traceStreamlineStep writes to streams_ and is not thread-safe.
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            grid.traceStreamlineStep(
                {row, col},
                neighbors[static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount) +
                          static_cast<std::size_t>(col)]);
        }
    }
#else
    SequentialCPU fallback;
    fallback.computeTimeStep(grid);
#endif
}
