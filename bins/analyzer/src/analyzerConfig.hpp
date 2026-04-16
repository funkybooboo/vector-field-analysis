#pragma once
#include <array>
#include <string>
#include <string_view>

// Configuration for one analyzer run, parsed from a TOML config file.
// Defaults match single-process benchmark-all behaviour.
// Single source of truth for valid solver names.
// configParser.cpp and solverFactory.cpp both reference this.
inline constexpr std::array<std::string_view, 6> kValidSolvers = {
  "sequential", 
  "openmp",
  "pthreads", 
  "mpi",
  "cuda",
  "all"
};

// Input/output paths are derived from the config filename stem at runtime -- not stored here.
struct AnalyzerConfig {
    // Must be one of kValidSolvers.
    std::string solver = "all";
    // Thread count for the Pthreads solver.  0 = hardware_concurrency.
    unsigned int threadCount = 0;
};
