#include "cudaFullStreamlineSolver.hpp"

#include "cudaFull.hpp"

void CudaFullStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    const auto result = cudaFull::computeComponents(grid.field(), grid.bounds());
    auto paths = cudaFull::reconstructPaths(result);
    grid.setPrecomputedStreamlines(std::move(paths));
}
