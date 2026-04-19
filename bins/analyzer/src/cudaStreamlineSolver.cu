#include "cuda.hpp"
#include "cudaStreamlineSolver.hpp"

#include <vector>

void CudaStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    const int rows = static_cast<int>(grid.rows());
    const int cols = static_cast<int>(grid.cols());
    const int total = rows * cols;

    // Compute successor on CPU using the same Grid::downstreamCell as the
    // sequential solver so floating-point results are bitwise identical.
    std::vector<int> successor(static_cast<std::size_t>(total));
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const auto [drow, dcol] = grid.downstreamCell(row, col);
            successor[static_cast<std::size_t>(row * cols + col)] = drow * cols + dcol;
        }
    }

    const auto result = cuda::computeComponents(successor, rows, cols, blockSize_);
    auto paths = cuda::reconstructPaths(result);
    grid.setPrecomputedStreamlines(std::move(paths));
}
