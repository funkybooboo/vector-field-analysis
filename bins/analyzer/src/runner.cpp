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

static void verifyAll(const RunResult& sequential, const RunResult& pthreads,
                      [[maybe_unused]] const RunResult& openmp,
                      [[maybe_unused]] const RunResult& mpi, [[maybe_unused]] const RunResult& cuda,
                      bool runParallel, [[maybe_unused]] bool runMpi) {
    if (runParallel) {
        verify(sequential.streams, pthreads.streams, "pthreads");
#ifdef _OPENMP
        verify(sequential.streams, openmp.streams, "openmp");
#endif
    }
#ifdef USE_MPI
    if (runMpi) {
        verify(sequential.streams, mpi.streams, "mpi");
    }
#endif
#ifdef ENABLE_CUDA_SOLVER
    verify(sequential.streams, cuda.streams, "cuda");
#endif
}

void runAll(const Field::TimeSeries& field, unsigned int threadCount,
            [[maybe_unused]] unsigned int cudaBlockSize, int mpiRank, int mpiSize,
            const std::string& outPath) {
    const bool runParallel = threadCount > 1;
#ifdef USE_MPI
    const bool runMpi = mpiSize > 1;
#else
    [[maybe_unused]] const bool runMpi = false;
#endif

#ifdef USE_MPI
    RunResult mpiResult{};
    if (runMpi) {
        auto mpi = makeSolver("mpi", threadCount);
        mpiResult = runSolver(*mpi, field);
    }
#endif

    RunResult sequentialResult{};
    RunResult pthreadsResult{};
#ifdef _OPENMP
    RunResult openmpResult{};
#endif
#ifdef ENABLE_CUDA_SOLVER
    RunResult cudaResult{};
#endif
    if (mpiRank == 0) {
        auto sequentialSolver = makeSolver("sequential", threadCount);
        sequentialResult = runSolver(*sequentialSolver, field);
        if (runParallel) {
#ifdef _OPENMP
            auto openmpSolver = makeSolver("openmp", threadCount);
            openmpResult = runSolver(*openmpSolver, field);
#endif
            auto pthreadsSolver = makeSolver("pthreads", threadCount);
            pthreadsResult = runSolver(*pthreadsSolver, field);
        }
#ifdef ENABLE_CUDA_SOLVER
        auto cudaSolver = makeSolver("cuda", threadCount, cudaBlockSize);
        cudaResult = runSolver(*cudaSolver, field);
#endif
        const std::string sequentialLabel = "sequential";
        std::vector<std::size_t> labelSizes = {sequentialLabel.size()};
        if (runParallel) {
            labelSizes.push_back(
                std::string("pthreads (" + std::to_string(threadCount) + " thr)").size());
#ifdef _OPENMP
            labelSizes.push_back(
                std::string("openmp (" + std::to_string(threadCount) + " thr)").size());
#endif
        }
#ifdef USE_MPI
        if (runMpi) {
            labelSizes.push_back(
                std::string("mpi (" + std::to_string(mpiSize) + " rank(s))").size());
        }
#endif
#ifdef ENABLE_CUDA_SOLVER
        labelSizes.push_back(
            std::string("cuda (blk=" + std::to_string(cudaBlockSize) + ")").size());
#endif
        const int labelWidth =
            static_cast<int>(*std::max_element(labelSizes.begin(), labelSizes.end())) + 2;

        std::cout << std::left << std::setw(labelWidth) << sequentialLabel
                  << sequentialResult.elapsedMilliseconds << " ms\n";

#ifdef _OPENMP
        if (runParallel) {
            printTiming("openmp (" + std::to_string(threadCount) + " thr)",
                        openmpResult.elapsedMilliseconds, labelWidth,
                        sequentialResult.elapsedMilliseconds);
        } else {
            std::cout << "(openmp skipped -- thread count too low for parallel benefit)\n";
        }
#else
        std::cout << "(openmp skipped -- not compiled with OpenMP support)\n";
#endif

        if (runParallel) {
            printTiming("pthreads (" + std::to_string(threadCount) + " thr)",
                        pthreadsResult.elapsedMilliseconds, labelWidth,
                        sequentialResult.elapsedMilliseconds);
        } else {
            std::cout << "(pthreads skipped -- thread count too low for parallel benefit)\n";
        }

#ifdef USE_MPI
        if (runMpi) {
            printTiming("mpi (" + std::to_string(mpiSize) + " rank(s))",
                        mpiResult.elapsedMilliseconds, labelWidth,
                        sequentialResult.elapsedMilliseconds);
        } else {
            std::cout << "(mpi skipped -- rank count too low for parallel benefit)\n";
        }
#else
        std::cout << "(mpi skipped -- not compiled with MPI support)\n";
#endif

#ifdef ENABLE_CUDA_SOLVER
        printTiming("cuda (blk=" + std::to_string(cudaBlockSize) + ")",
                    cudaResult.elapsedMilliseconds, labelWidth,
                    sequentialResult.elapsedMilliseconds);
#else
        std::cout << "(cuda skipped -- rebuild with -DENABLE_CUDA=ON)\n";
#endif

        verifyAll(sequentialResult, pthreadsResult,
#ifdef _OPENMP
                  openmpResult,
#else
                  RunResult{},
#endif
#ifdef USE_MPI
                  mpiResult,
#else
                  RunResult{},
#endif
#ifdef ENABLE_CUDA_SOLVER
                  cudaResult,
#else
                  RunResult{},
#endif
                  runParallel, runMpi);

        writeAndReport(outPath, sequentialResult.streams, field.bounds, field.gridSize());
    }
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
