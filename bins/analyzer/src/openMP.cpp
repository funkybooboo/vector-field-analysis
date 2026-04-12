#include "openMP.hpp"

#include "sequentialCPU.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <utility>
#include <vector>

namespace openMP {
void computeTimeStep(VectorField::FieldGrid& field) {
#ifdef _OPENMP
    const int rowCount = static_cast<int>(field.rows());
    if (rowCount == 0) {
        return;
    }
    const int colCount = static_cast<int>(field.cols());

    // Pass 1: parallel — each cell reads its neighbor direction from field_ (read-only).
    // neighborInVectorDirection is const and touches no shared mutable state.
    std::vector<std::pair<int, int>> neighbors(static_cast<std::size_t>(rowCount * colCount));

#pragma omp parallel for schedule(static) collapse(2)
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            neighbors[static_cast<std::size_t>(row * colCount + col)] =
                field.neighborInVectorDirection(row, col);
        }
    }

    // Pass 2: sequential — apply streamline merges using the precomputed pairs.
    // traceStreamlineStep writes to streams_ and is not thread-safe.
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            field.traceStreamlineStep({row, col},
                                      neighbors[static_cast<std::size_t>(row * colCount + col)]);
        }
    }
#else
    sequentialCPU::computeTimeStep(field);
#endif
}
} // namespace openMP
