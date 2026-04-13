#include "solverFactory.hpp"

#include "analyzerConfig.hpp"
#include "mpiCPU.hpp"
#include "openMP.hpp"
#include "pthreads.hpp"
#include "sequentialCPU.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

// "all" is handled by main.cpp before makeSolver is called.
static_assert(kValidSolvers.size() == 5, "kValidSolvers changed -- update makeSolver() to match");

std::unique_ptr<StreamlineSolver> makeSolver(std::string_view name, unsigned int threadCount) {
    if (name == "sequential") {
        return std::make_unique<SequentialCPU>();
    }
    if (name == "openmp") {
        return std::make_unique<OpenMP>(threadCount);
    }
    if (name == "pthreads") {
        return std::make_unique<Pthreads>(threadCount);
    }
    if (name == "mpi") {
        return std::make_unique<MpiCPU>();
    }
    throw std::runtime_error("Unknown solver: \"" + std::string(name) + "\"");
}
