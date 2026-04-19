#include "sequentialStreamlineSolver.hpp"

#include <stdexcept>
#include <vector>

void SequentialStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    const std::size_t rowCount = grid.rows();
    if (rowCount == 0) {
        throw std::runtime_error("Can't properly initialize empty field");
    }
    const std::size_t colCount = grid.cols();
    if (colCount == 0) {
        throw std::runtime_error("Can't properly initialize zero-width field");
    }

    const std::size_t total = rowCount * colCount;
    std::vector<Field::GridCell> neighbors(total);

    for (std::size_t row = 0; row < rowCount; ++row) {
        for (std::size_t col = 0; col < colCount; ++col) {
            neighbors[(row * colCount) + col] =
                grid.downstreamCell(static_cast<int>(row), static_cast<int>(col));
        }
    }

    for (std::size_t i = 0; i < total; ++i) {
        grid.unite(i, grid.coordsToIndex(static_cast<std::size_t>(neighbors[i].row),
                                         static_cast<std::size_t>(neighbors[i].col)));
    }

    grid.setPrecomputedStreamlines(reconstructPathsDSU(grid, neighbors));
}
