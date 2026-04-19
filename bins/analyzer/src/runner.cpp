#include "runner.hpp"

#include "fieldReader.hpp"
#include "formatBytes.hpp"
#include "solverFactory.hpp"
#include "streamWriter.hpp"
#include "streamlineSolver.hpp"
#include "timing.hpp"

#ifdef USE_MPI
#include <mpi.h>
#endif

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

struct RunResult {
    double elapsedMilliseconds{};
    std::vector<StreamWriter::StepStreamlines> streams;
};

static RunResult runSolver(StreamlineSolver& solver, const Field::TimeSeries& timeSeries) {
    RunResult result;
    auto startTime = std::chrono::steady_clock::now();
    for (const auto& frame : timeSeries.frames) {
        Field::Grid grid(timeSeries.bounds, frame);
        solver.computeTimeStep(grid);
        result.streams.push_back(grid.getStreamlines());
    }
    result.elapsedMilliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime)
            .count();
    return result;
}

static std::vector<Field::Path> canonicalize(const StreamWriter::StepStreamlines& step) {
    auto copy = step;
    std::sort(copy.begin(), copy.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.empty() != rhs.empty()) {
            return rhs.empty();
        }
        if (lhs.front() != rhs.front()) {
            return lhs.front() < rhs.front();
        }
        if (lhs.size() != rhs.size()) {
            return lhs.size() < rhs.size();
        }
        return lhs.back() < rhs.back();
    });
    return copy;
}

static void verify(const std::vector<StreamWriter::StepStreamlines>& reference,
                   const std::vector<StreamWriter::StepStreamlines>& other,
                   const std::string& name) {
    if (reference.size() != other.size()) {
        std::cerr << "Error: " << name << " produced " << other.size() << " step(s) but sequential"
                  << " produced " << reference.size() << "\n";
        std::exit(1);
    }
    for (std::size_t stepIndex = 0; stepIndex < reference.size(); ++stepIndex) {
        if (canonicalize(reference[stepIndex]) != canonicalize(other[stepIndex])) {
            std::cerr << "Error: " << name << " streamlines differ from sequential at step "
                      << stepIndex << "\n";
            std::exit(1);
        }
    }
}

static void writeAndReport(const std::string& outPath,
                           const std::vector<StreamWriter::StepStreamlines>& streams,
                           const Field::Bounds& bounds, const Field::GridSize& grid) {
    const std::size_t total =
        std::accumulate(streams.begin(), streams.end(), std::size_t{0},
                        [](std::size_t acc, const auto& step) { return acc + step.size(); });
    const std::size_t numSteps = streams.size();
    const double avgPerStep =
        numSteps > 0 ? static_cast<double>(total) / static_cast<double>(numSteps) : 0.0;
    std::cout << "\nPrecomputed " << total << " streamlines across " << numSteps << " steps"
              << "  (" << std::fixed << std::setprecision(1) << avgPerStep << "/step avg)\n";

    StreamWriter::write(outPath, streams, bounds, grid);

    std::error_code err;
    const auto fileBytes = std::filesystem::file_size(outPath, err);
    std::cout << "Streamlines written to " << outPath;
    if (!err) {
        std::cout << "  (" << Utils::formatBytes(fileBytes) << ")";
    }
    std::cout << "\n";
}

void runBenchmark(const Field::TimeSeries& field, const std::vector<unsigned int>& benchmarkThreads,
                  [[maybe_unused]] const std::vector<unsigned int>& benchmarkCudaBlockSizes,
                  int mpiRank, int mpiSize, const std::string& outPath) {
    // Compute label width upfront for aligned output.
    std::vector<std::size_t> labelSizes = {std::string("sequential (1t)").size()};
    for (unsigned int t : benchmarkThreads) {
        const std::string ts = std::to_string(t);
        labelSizes.push_back(std::string("pthreads (" + ts + "t)").size());
#ifdef _OPENMP
        labelSizes.push_back(std::string("openmp (" + ts + "t)").size());
#endif
    }
#ifdef USE_MPI
    if (mpiSize > 1) {
        labelSizes.push_back(std::string("mpi (" + std::to_string(mpiSize) + " ranks)").size());
    }
#endif
#ifdef ENABLE_CUDA_SOLVER
    for (unsigned int blk : benchmarkCudaBlockSizes) {
        labelSizes.push_back(std::string("cuda (blk=" + std::to_string(blk) + ")").size());
    }
#endif
    const int labelWidth =
        static_cast<int>(*std::max_element(labelSizes.begin(), labelSizes.end())) + 2;

    // MPI must run first -- all ranks must participate before rank 0 diverges.
    RunResult mpiResult{};
#ifdef USE_MPI
    if (mpiSize > 1) {
        auto mpiSolver = makeSolver("mpi", static_cast<unsigned int>(mpiSize));
        mpiResult = runSolver(*mpiSolver, field);
    }
#endif

    if (mpiRank != 0) {
        return;
    }

    // Sequential is the reference for all verifications.
    RunResult seqResult;
    {
        auto seq = makeSolver("sequential", 1);
        seqResult = runSolver(*seq, field);
    }
    std::cout << std::left << std::setw(labelWidth) << "sequential (1t)"
              << seqResult.elapsedMilliseconds << " ms\n";

    // Verify and print MPI now that we have the sequential reference.
#ifdef USE_MPI
    if (mpiSize > 1) {
        verify(seqResult.streams, mpiResult.streams, "mpi (" + std::to_string(mpiSize) + " ranks)");
        printTiming("mpi (" + std::to_string(mpiSize) + " ranks)", mpiResult.elapsedMilliseconds,
                    labelWidth, seqResult.elapsedMilliseconds);
        mpiResult.streams.clear();
        mpiResult.streams.shrink_to_fit();
    }
#endif

    for (unsigned int t : benchmarkThreads) {
        const std::string ts = std::to_string(t);
        {
            auto solver = makeSolver("pthreads", t);
            auto result = runSolver(*solver, field);
            verify(seqResult.streams, result.streams, "pthreads (" + ts + "t)");
            printTiming("pthreads (" + ts + "t)", result.elapsedMilliseconds, labelWidth,
                        seqResult.elapsedMilliseconds);
        }
#ifdef _OPENMP
        {
            auto solver = makeSolver("openmp", t);
            auto result = runSolver(*solver, field);
            verify(seqResult.streams, result.streams, "openmp (" + ts + "t)");
            printTiming("openmp (" + ts + "t)", result.elapsedMilliseconds, labelWidth,
                        seqResult.elapsedMilliseconds);
        }
#endif
    }

#ifdef ENABLE_CUDA_SOLVER
    for (unsigned int blk : benchmarkCudaBlockSizes) {
        auto solver = makeSolver("cuda", 1, blk);
        auto result = runSolver(*solver, field);
        verify(seqResult.streams, result.streams, "cuda (blk=" + std::to_string(blk) + ")");
        printTiming("cuda (blk=" + std::to_string(blk) + ")", result.elapsedMilliseconds,
                    labelWidth, seqResult.elapsedMilliseconds);
    }
#endif

    std::cout << "[all verified vs sequential]\n";
    writeAndReport(outPath, seqResult.streams, field.bounds, field.gridSize());
}

void runOne(const std::string& solverName, const Field::TimeSeries& field, unsigned int threadCount,
            [[maybe_unused]] unsigned int cudaBlockSize, int mpiRank, int mpiSize,
            const std::string& outPath) {
    RunResult result{};
    if (solverName == "mpi") {
        auto solver = makeSolver(solverName, threadCount);
        result = runSolver(*solver, field);
    } else if (mpiRank == 0) {
#ifdef ENABLE_CUDA_SOLVER
        if (solverName == "cuda") {
            auto solver = makeSolver(solverName, threadCount, cudaBlockSize);
            result = runSolver(*solver, field);
        } else {
            auto solver = makeSolver(solverName, threadCount);
            result = runSolver(*solver, field);
        }
#else
        if (solverName == "cuda") {
            std::cerr << "Error: solver \"" << solverName
                      << "\" requires rebuilding with -DENABLE_CUDA=ON\n";
            return;
        }
        if ((solverName == "openmp" || solverName == "pthreads") && threadCount <= 1) {
            std::cout << "(note: " << solverName << " with thread count " << threadCount
                      << " offers no parallelism benefit over sequential)\n";
        }
        auto solver = makeSolver(solverName, threadCount);
        result = runSolver(*solver, field);
#endif
    } else {
        return;
    }

    if (mpiRank == 0) {
        std::string label = solverName;
        if (solverName == "openmp" || solverName == "pthreads") {
            label += " (" + std::to_string(threadCount) + " thr)";
        } else if (solverName == "mpi") {
            label += " (" + std::to_string(mpiSize) + " rank(s))";
#ifdef ENABLE_CUDA_SOLVER
        } else if (solverName == "cuda") {
            label += " (blk=" + std::to_string(cudaBlockSize) + ")";
#endif
        }
        std::cout << label << "  " << result.elapsedMilliseconds << " ms\n";

        writeAndReport(outPath, result.streams, field.bounds, field.gridSize());
    }
}
