#pragma once
#include <array>
#include <string>
#include <string_view>

// Configuration for one analyzer run, parsed from a TOML config file.
// Defaults match single-process benchmark-all behaviour.
// Single source of truth for valid solver names.
// configParser.cpp and solverFactory.cpp both reference this.
inline constexpr std::array<std::string_view, 6> kValidSolvers = {
    "sequential", "openmp", "pthreads", "mpi", "cuda", "all"};

struct AnalyzerConfig {
    // Must be one of kValidSolvers.
    std::string solver = "all";
    // Thread count for OpenMP/Pthreads/MPI.  0 = hardware_concurrency.
    unsigned int threadCount = 0;
    // CUDA block size (threads per block).
    unsigned int cudaBlockSize = 256;
    // Output path for the streams HDF5 file. Empty = derive from config filename stem.
    std::string output;
};
