#pragma once
#include <array>
#include <string>
#include <string_view>

// Single source of truth for valid solver names.
// configParser.cpp and solverFactory.cpp both reference this.
inline constexpr std::array<std::string_view, 6> kValidSolvers = {
    "sequential", "openmp", "pthreads", "mpi", "cuda", "cudaMpi"};

struct AnalyzerConfig {
    // Must be one of kValidSolvers.
    std::string solver = "sequential";
    // Thread count for OpenMP/Pthreads.  0 = hardware_concurrency.
    unsigned int threadCount = 0;
    // CUDA block size (threads per block) when solver == "cuda".
    unsigned int cudaBlockSize = 256;
    // Output path for the streams HDF5 file. Empty = derive from config filename stem.
    std::string output;
    // If non-empty, timing results are written here after the solver runs.
    std::string timingOutput;
};
