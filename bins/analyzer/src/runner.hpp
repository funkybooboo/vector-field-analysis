#pragma once

#include "fieldReader.hpp"

#include <string>

void runOne(const std::string& solverName, const Field::TimeSeries& field, unsigned int threadCount,
            unsigned int cudaBlockSize, int mpiRank, int mpiSize, const std::string& outPath,
            const std::string& timingOutput = "");
