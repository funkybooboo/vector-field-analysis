#include "sequentialStreamlineSolver.hpp"

void SequentialStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    const std::size_t rowCount = grid.rows();
    if (rowCount == 0) {
        return;
    }
    const std::size_t colCount = grid.cols();
    if (colCount == 0) {
        return;
    }

    for (std::size_t row = 0; row < rowCount; row++) {
        for (std::size_t col = 0; col < colCount; col++) {
            grid.traceStreamlineStep(static_cast<int>(row), static_cast<int>(col));
        }
    }
}
