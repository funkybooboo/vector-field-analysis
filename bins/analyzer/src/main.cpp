#include "fieldReader.hpp"
#include "openMP.hpp"
#include "pthreads.hpp"
#include "sequentialCPU.hpp"
#include "streamWriter.hpp"
#include "vectorField.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

// One step's worth of streamlines: [streamline][point] = (row, col)
using StepStreamlines = StreamWriter::StepStreamlines;
// All steps: [step][streamline][point]
using AllStepStreamlines = std::vector<StepStreamlines>;

struct RunResult {
    double ms;
    AllStepStreamlines streams;
};

template <typename Fn>
static RunResult runAndCollect(const Vector::FieldTimeSeries& data, Fn computeFn) {
    RunResult result;
    auto t0 = std::chrono::steady_clock::now();
    for (const auto& step : data.steps) {
        VectorField::FieldGrid grid(data.xMin, data.xMax, data.yMin, data.yMax, step);
        computeFn(grid);
        result.streams.push_back(grid.getStreamlines());
    }
    auto t1 = std::chrono::steady_clock::now();
    result.ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return result;
}

// Canonicalize one step's streamlines for comparison by sorting only the
// collection of streamlines. Each streamline remains an ordered list of
// points, so path traversal order is preserved while comparison stays
// independent of the order in which unique streamlines were encountered
// during grid iteration.
static std::vector<std::vector<std::pair<int, int>>> canonicalize(const StepStreamlines& step) {
    auto copy = step;
    std::sort(copy.begin(), copy.end(), [](const auto& a, const auto& b) {
        if (a.empty() != b.empty()) {
            return b.empty();
        }
        if (a.front() != b.front()) {
            return a.front() < b.front();
        }
        if (a.size() != b.size()) {
            return a.size() < b.size();
        }
        return a.back() < b.back();
    });
    return copy;
}

// Crash with a descriptive message if impl's output differs from the reference
// (sequential). Comparison is canonical so iteration order doesn't matter.
static void verify(const AllStepStreamlines& reference, const AllStepStreamlines& other,
                   const std::string& name) {
    if (reference.size() != other.size()) {
        std::cerr << "Error: " << name << " produced " << other.size() << " step(s) but sequential"
                  << " produced " << reference.size() << "\n";
        std::exit(1);
    }
    for (std::size_t s = 0; s < reference.size(); ++s) {
        if (canonicalize(reference[s]) != canonicalize(other[s])) {
            std::cerr << "Error: " << name << " streamlines differ from sequential at step " << s
                      << "\n";
            std::exit(1);
        }
    }
}

// Derive output path: strip trailing .h5 (if present) and append .streams.h5
static std::string makeOutPath(const std::string& inPath) {
    const std::string suffix = ".h5";
    if (inPath.size() > suffix.size() &&
        inPath.compare(inPath.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return inPath.substr(0, inPath.size() - suffix.size()) + ".streams.h5";
    }
    return inPath + ".streams.h5";
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

        const auto seq = runAndCollect(
            data, [](VectorField::FieldGrid& g) { sequentialCPU::computeTimeStep(g); });
        std::cout << "sequential          " << seq.ms << " ms\n";

        const auto omp =
            runAndCollect(data, [](VectorField::FieldGrid& g) { openMP::computeTimeStep(g); });
        std::cout << "openmp              " << omp.ms << " ms"
                  << "  (" << seq.ms / omp.ms << "x vs sequential)\n";

        const auto pt = runAndCollect(
            data, [&](VectorField::FieldGrid& g) { pthreads::computeTimeStep(g, hwThreads); });
        std::cout << "pthreads (" << hwThreads << " thr)  " << pt.ms << " ms"
                  << "  (" << seq.ms / pt.ms << "x vs sequential)\n";

        // All implementations must agree — crash if they don't.
        verify(seq.streams, omp.streams, "openMP");
        verify(seq.streams, pt.streams, "pthreads");

        const std::string outPath = makeOutPath(inPath);
        StreamWriter::write(outPath, seq.streams, data.xMin, data.xMax, data.yMin, data.yMax, width,
                            height);
        std::cout << "\nStreamlines written to " << outPath << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
