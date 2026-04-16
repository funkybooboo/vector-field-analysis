#include "solverFactory.hpp"

#include "analyzerConfig.hpp"
#include "mpiStreamlineSolver.hpp"
#include "openMpStreamlineSolver.hpp"
#include "pthreadsStreamlineSolver.hpp"
#include "sequentialStreamlineSolver.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

// "all" is handled by main.cpp before makeSolver is called.
static_assert(kValidSolvers.size() == 5, "kValidSolvers changed -- update makeSolver() to match");

std::unique_ptr<StreamlineSolver> makeSolver(std::string_view name, unsigned int threadCount) {
    if (name == "sequential") {
        return std::make_unique<SequentialStreamlineSolver>();
    }
    if (name == "openmp") {
        return std::make_unique<OpenMpStreamlineSolver>(threadCount);
    }
    if (name == "pthreads") {
        return std::make_unique<PthreadsStreamlineSolver>(threadCount);
    }
    if (name == "mpi") {
        return std::make_unique<MpiStreamlineSolver>();
    }
    throw std::runtime_error("Unknown solver: \"" + std::string(name) + "\"");
}
