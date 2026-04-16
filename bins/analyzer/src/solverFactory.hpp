#pragma once
#include "streamlineSolver.hpp"

#include <memory>
#include <string_view>

// Returns the StreamlineSolver implementation for the given name.
// name must be one of: "sequential", "openmp", "pthreads", "mpi".
// threadCount is forwarded to Pthreads (ignored by other implementations).
// Throws std::runtime_error for unknown names.
[[nodiscard]] std::unique_ptr<StreamlineSolver> makeSolver(std::string_view name,
                                                           unsigned int threadCount);
