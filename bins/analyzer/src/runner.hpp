#pragma once

#include "fieldReader.hpp"

#include <string>
#include <vector>

void runBenchmark(const Field::TimeSeries& field, const std::vector<unsigned int>& benchmarkThreads,
                  const std::vector<unsigned int>& benchmarkCudaBlockSizes, int mpiRank,
                  int mpiSize, const std::string& outPath);

void runOne(const std::string& solverName, const Field::TimeSeries& field, unsigned int threadCount,
            unsigned int cudaBlockSize, int mpiRank, int mpiSize, const std::string& outPath);
