#include "cuda.hpp"
#include "cudaStreamlineSolver.hpp"

void CudaStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    const auto result = cuda::computeComponents(grid.field(), grid.bounds(), blockSize_);
    auto paths = cuda::reconstructPaths(result);
    grid.setPrecomputedStreamlines(std::move(paths));
}
