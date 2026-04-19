#pragma once
#include <array>
#include <string>
#include <string_view>
#include <vector>

// Single source of truth for valid solver names.
// configParser.cpp and solverFactory.cpp both reference this.
inline constexpr std::array<std::string_view, 6> kValidSolvers = {
    "sequential", "openmp", "pthreads", "mpi", "cuda", "benchmark"};

struct AnalyzerConfig {
    // Must be one of kValidSolvers.
    std::string solver = "benchmark";
    // Thread count for OpenMP/Pthreads/MPI when solver != "benchmark".  0 = hardware_concurrency.
    unsigned int threadCount = 0;
    // CUDA block size (threads per block) when solver == "cuda".
    unsigned int cudaBlockSize = 256;
    // Output path for the streams HDF5 file. Empty = derive from config filename stem.
    std::string output;
    // benchmark mode: thread counts to test for pthreads/openmp.
    std::vector<unsigned int> benchmarkThreads = {2, 4, 8};
    // benchmark mode: CUDA block sizes to test.
    std::vector<unsigned int> benchmarkCudaBlockSizes = {64, 128, 256, 512};
};
