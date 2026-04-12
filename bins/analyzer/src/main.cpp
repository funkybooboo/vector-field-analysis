#include "fieldReader.hpp"
#include "openMP.hpp"
#include "pthreads.hpp"
#include "sequentialCPU.hpp"
#include "vectorField.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

static double runSequential(const Vector::FieldTimeSeries& data) {
    auto t0 = std::chrono::steady_clock::now();
    for (const auto& step : data.steps) {
        VectorField::FieldGrid grid(data.xMin, data.xMax, data.yMin, data.yMax, step);
        sequentialCPU::computeTimeStep(grid);
    }
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static double runOpenMP(const Vector::FieldTimeSeries& data) {
    auto t0 = std::chrono::steady_clock::now();
    for (const auto& step : data.steps) {
        VectorField::FieldGrid grid(data.xMin, data.xMax, data.yMin, data.yMax, step);
        openMP::computeTimeStep(grid);
    }
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static double runPthreads(const Vector::FieldTimeSeries& data, unsigned int threadCount) {
    auto t0 = std::chrono::steady_clock::now();
    for (const auto& step : data.steps) {
        VectorField::FieldGrid grid(data.xMin, data.xMax, data.yMin, data.yMax, step);
        pthreads::computeTimeStep(grid, threadCount);
    }
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main(int argc, char* argv[]) {
    const std::string inPath = argc > 1 ? argv[1] : "field.h5";

    unsigned int hwThreads = std::thread::hardware_concurrency();
    if (hwThreads == 0) {
        hwThreads = 1;
    }

    try {
        const Vector::FieldTimeSeries data = FieldReader::read(inPath);

        const int numSteps = static_cast<int>(data.steps.size());
        const int height = static_cast<int>(data.steps[0].size());
        const int width = static_cast<int>(data.steps[0][0].size());

        std::cout << "Field: " << inPath << "  " << width << "x" << height << "  " << numSteps
                  << " step(s)\n\n";

        const double seqMs = runSequential(data);
        std::cout << "sequential          " << seqMs << " ms\n";

        const double ompMs = runOpenMP(data);
        std::cout << "openmp              " << ompMs << " ms"
                  << "  (" << seqMs / ompMs << "x vs sequential)\n";

        const double ptMs = runPthreads(data, hwThreads);
        std::cout << "pthreads (" << hwThreads << " thr)  " << ptMs << " ms"
                  << "  (" << seqMs / ptMs << "x vs sequential)\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
