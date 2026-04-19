#include "cuda.hpp"
#include "cudaStreamlineSolver.hpp"

void CudaStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    const auto result =
        cuda::computeComponents(grid.field(), static_cast<int>(grid.rows()),
                                static_cast<int>(grid.cols()), grid.bounds(), blockSize_);
    auto paths = cuda::reconstructPaths(result);
    grid.setPrecomputedStreamlines(std::move(paths));
}
