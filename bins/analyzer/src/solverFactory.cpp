#include "solverFactory.hpp"

#include "analyzerConfig.hpp"
#include "mpiStreamlineSolver.hpp"
#include "openMpStreamlineSolver.hpp"
#include "pthreadsStreamlineSolver.hpp"
#include "sequentialStreamlineSolver.hpp"

#ifdef ENABLE_CUDA_SOLVER
#include "cudaFullStreamlineSolver.hpp"
#include "cudaStreamlineSolver.hpp"
#endif

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

// "all" is handled by main.cpp before makeSolver is called.
static_assert(kValidSolvers.size() == 7, "kValidSolvers changed -- update makeSolver() to match");

std::unique_ptr<StreamlineSolver> makeSolver(std::string_view name, unsigned int threadCount,
                                             [[maybe_unused]] unsigned int cudaBlockSize) {
    if (name == "sequential") {
        return std::make_unique<SequentialStreamlineSolver>();
    }
    if (name == "openmp") {
        return std::make_unique<OpenMpStreamlineSolver>(threadCount);
    }
    if (name == "pthreads") {
        return std::make_unique<PthreadsStreamlineSolver>(threadCount);
    }
#ifdef ENABLE_CUDA_SOLVER
    if (name == "cuda") {
        return std::make_unique<CudaStreamlineSolver>(cudaBlockSize);
    }
    if (name == "cuda_full") {
        return std::make_unique<CudaFullStreamlineSolver>(cudaBlockSize);
    }
#endif
    if (name == "mpi") {
        return std::make_unique<MpiStreamlineSolver>();
    }
    throw std::runtime_error("Unknown solver: \"" + std::string(name) + "\"");
}
