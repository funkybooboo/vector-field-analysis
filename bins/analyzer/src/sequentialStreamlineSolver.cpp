#include "sequentialStreamlineSolver.hpp"

#include <stdexcept>
#include <vector>

void SequentialStreamlineSolver::computeTimeStep(Field::Grid& grid) {

    const int rowCount = static_cast<int>(grid.field().size());
    if (rowCount == 0) {
        throw std::runtime_error("Can't properly initialize empty field");
    }
    const int colCount = static_cast<int>(grid.field()[0].size());
    if (colCount == 0) {
        throw std::runtime_error("Can't properly initialize zero-width field");
    }

    // compute downstream neighbors.
    std::vector<Field::GridCell> neighbors(static_cast<std::size_t>(rowCount) *
                                           static_cast<std::size_t>(colCount));

    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            neighbors[(static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount)) +
                      static_cast<std::size_t>(col)] = grid.downstreamCell(row, col);
        }
    }

    // apply streamline merges using the pre-computed pairs.
    applyNeighborPairs(grid, neighbors, rowCount, colCount);
}
