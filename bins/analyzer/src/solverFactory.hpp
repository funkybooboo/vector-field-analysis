#pragma once
#include "streamlineSolver.hpp"

#include <memory>
#include <string_view>

// Returns the StreamlineSolver implementation for the given name.
// name must be one of: "sequential", "openmp", "pthreads", "mpi", "cuda", "cudaMpi".
// threadCount is forwarded to OpenMP/Pthreads/MPI (ignored by others).
// cudaBlockSize is forwarded to CUDA solvers (ignored by others).
// Throws std::runtime_error for unknown names.
[[nodiscard]] std::unique_ptr<StreamlineSolver>
makeSolver(std::string_view name, unsigned int threadCount, unsigned int cudaBlockSize = 256);
