#pragma once

#include "fieldReader.hpp"

#include <string>

void runAll(const Field::TimeSeries& field, unsigned int threadCount, unsigned int cudaBlockSize,
            int mpiRank, int mpiSize, const std::string& outPath);

void runOne(const std::string& solverName, const Field::TimeSeries& field, unsigned int threadCount,
            unsigned int cudaBlockSize, int mpiRank, int mpiSize, const std::string& outPath);
