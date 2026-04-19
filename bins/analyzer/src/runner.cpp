#include "runner.hpp"

#include "solverFactory.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef USE_MPI
#include <mpi.h>
#endif

namespace {
using Clock = std::chrono::steady_clock;

RunResult runSolver(StreamlineSolver& solver, const Field::TimeSeries& field) {
    RunResult result;

    const auto start = Clock::now();

    result.streams.reserve(field.steps());
    for (std::size_t step = 0; step < field.steps(); ++step) {
        Field::Grid grid(field.fieldAt(step), field.rows(), field.cols(), field.bounds());
        solver.computeTimeStep(grid);
        result.streams.push_back(grid.getStreamlines());
    }

    const auto end = Clock::now();
    result.elapsedMilliseconds =
        std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}

bool equalPath(const Field::Path& a, const Field::Path& b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].row != b[i].row || a[i].col != b[i].col) {
            return false;
        }
    }

    return true;
}

bool equalPaths(const std::vector<Field::Path>& a, const std::vector<Field::Path>& b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!equalPath(a[i], b[i])) {
            return false;
        }
    }

    return true;
}

void verify(const std::vector<std::vector<Field::Path>>& reference,
            const std::vector<std::vector<Field::Path>>& candidate, const std::string& label) {
    if (reference.size() != candidate.size()) {
        throw std::runtime_error("Verification failed for " + label +
                                 ": time-step count mismatch");
    }

    for (std::size_t step = 0; step < reference.size(); ++step) {
        if (!equalPaths(reference[step], candidate[step])) {
            throw std::runtime_error("Verification failed for " + label + " at time step " +
                                     std::to_string(step));
        }
    }
}

void printTiming(const std::string& label, double elapsedMs, int labelWidth,
                 double baselineMs = 0.0) {
    std::cout << std::left << std::setw(labelWidth) << label << elapsedMs << " ms";
    if (baselineMs > 0.0) {
        std::cout << "  speedup " << std::fixed << std::setprecision(2)
                  << (baselineMs / elapsedMs) << "x";
        std::cout.unsetf(std::ios::floatfield);
    }
    std::cout << '\n';
}
} // namespace

void runBenchmark(const Field::TimeSeries& field, const std::vector<unsigned int>& benchmarkThreads,
                  const std::vector<unsigned int>& benchmarkCudaBlockSizes) {
    int mpiRank = 0;
    int mpiSize = 1;
#ifdef USE_MPI
    int mpiInitialized = 0;
    MPI_Initialized(&mpiInitialized);
    if (mpiInitialized) {
        MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    }
#endif

    std::vector<std::size_t> labelSizes{
        std::string("sequential (1t)").size(),
        std::string("openmp (1t)").size(),
        std::string("pthreads (1t)").size(),
    };
#ifdef USE_MPI
    if (mpiSize > 1) {
        labelSizes.push_back(std::string("mpi (" + std::to_string(mpiSize) + " ranks)").size());
    }
#endif
#ifdef ENABLE_CUDA_SOLVER
#ifdef USE_MPI
    if (mpiSize > 1) {
        for (unsigned int blk : benchmarkCudaBlockSizes) {
            labelSizes.push_back(std::string("hybrid (" + std::to_string(mpiSize) +
                                             " ranks, blk=" + std::to_string(blk) + ")")
                                     .size());
        }
    }
#endif
    for (unsigned int blk : benchmarkCudaBlockSizes) {
        labelSizes.push_back(std::string("cuda (blk=" + std::to_string(blk) + ")").size());
    }
#endif
    const int labelWidth =
        static_cast<int>(*std::max_element(labelSizes.begin(), labelSizes.end())) + 2;

    // MPI-collective solvers must run first -- all ranks must participate before rank 0 diverges.
    RunResult mpiResult{};
#ifdef USE_MPI
    if (mpiSize > 1) {
        auto mpiSolver = makeSolver("mpi", static_cast<unsigned int>(mpiSize));
        mpiResult = runSolver(*mpiSolver, field);
    }
#endif

#ifdef ENABLE_CUDA_SOLVER
    std::vector<std::pair<unsigned int, RunResult>> hybridResults;
#ifdef USE_MPI
    if (mpiSize > 1) {
        hybridResults.reserve(benchmarkCudaBlockSizes.size());
        for (unsigned int blk : benchmarkCudaBlockSizes) {
            auto hybridSolver = makeSolver("hybrid", static_cast<unsigned int>(mpiSize), blk);
            hybridResults.emplace_back(blk, runSolver(*hybridSolver, field));
        }
    }
#endif
#endif

    if (mpiRank != 0) {
        return;
    }

    std::cout << "\n=== benchmark: streamline solvers ===\n";

    auto seqSolver = makeSolver("sequential", 1);
    RunResult seqResult = runSolver(*seqSolver, field);
    std::cout << "Verified implementations against sequential reference.\n\n";
    printTiming("solver", 0.0, labelWidth); // header alignment helper, ignored visually
    std::cout << std::left << std::setw(labelWidth) << "solver"
              << "time"
              << "  speedup\n";
    std::cout << std::string(labelWidth + 22, '-') << '\n';
    std::cout << std::left << std::setw(labelWidth) << "sequential (1t)"
              << seqResult.elapsedMilliseconds << " ms\n";

    // Verify and print MPI-collective results now that we have the sequential reference.
#ifdef USE_MPI
    if (mpiSize > 1) {
        verify(seqResult.streams, mpiResult.streams, "mpi (" + std::to_string(mpiSize) + " ranks)");
        printTiming("mpi (" + std::to_string(mpiSize) + " ranks)", mpiResult.elapsedMilliseconds,
                    labelWidth, seqResult.elapsedMilliseconds);
        mpiResult.streams.clear();
        mpiResult.streams.shrink_to_fit();
    }
#endif

#ifdef ENABLE_CUDA_SOLVER
#ifdef USE_MPI
    if (mpiSize > 1) {
        for (auto& hybridEntry : hybridResults) {
            const unsigned int blk = hybridEntry.first;
            auto& result = hybridEntry.second;
            verify(seqResult.streams, result.streams,
                   "hybrid (" + std::to_string(mpiSize) + " ranks, blk=" + std::to_string(blk) +
                       ")");
            printTiming("hybrid (" + std::to_string(mpiSize) + " ranks, blk=" +
                            std::to_string(blk) + ")",
                        result.elapsedMilliseconds, labelWidth, seqResult.elapsedMilliseconds);
            result.streams.clear();
            result.streams.shrink_to_fit();
        }
    }
#endif
#endif

    for (unsigned int t : benchmarkThreads) {
        const std::string ts = std::to_string(t);
        {
            auto ompSolver = makeSolver("openmp", t);
            RunResult result = runSolver(*ompSolver, field);
            verify(seqResult.streams, result.streams, "openmp (" + ts + "t)");
            printTiming("openmp (" + ts + "t)", result.elapsedMilliseconds, labelWidth,
                        seqResult.elapsedMilliseconds);
        }
        {
            auto pthreadSolver = makeSolver("pthreads", t);
            RunResult result = runSolver(*pthreadSolver, field);
            verify(seqResult.streams, result.streams, "pthreads (" + ts + "t)");
            printTiming("pthreads (" + ts + "t)", result.elapsedMilliseconds, labelWidth,
                        seqResult.elapsedMilliseconds);
        }
    }

#ifdef ENABLE_CUDA_SOLVER
    for (unsigned int blk : benchmarkCudaBlockSizes) {
        auto cudaSolver = makeSolver("cuda", 1, blk);
        RunResult result = runSolver(*cudaSolver, field);
        verify(seqResult.streams, result.streams, "cuda (blk=" + std::to_string(blk) + ")");
        printTiming("cuda (blk=" + std::to_string(blk) + ")", result.elapsedMilliseconds,
                    labelWidth, seqResult.elapsedMilliseconds);
    }
#endif

    std::cout << std::endl;
}

void runOne(const std::string& solverName, const Field::TimeSeries& field, unsigned int threadCount,
            unsigned int cudaBlockSize) {
    int mpiRank = 0;
    int mpiSize = 1;
#ifdef USE_MPI
    int mpiInitialized = 0;
    MPI_Initialized(&mpiInitialized);
    if (mpiInitialized) {
        MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    }
#endif

    RunResult result{};
    if (solverName == "mpi") {
        auto solver = makeSolver(solverName, threadCount);
        result = runSolver(*solver, field);
    } else if (solverName == "hybrid") {
#ifdef ENABLE_CUDA_SOLVER
        auto solver = makeSolver(solverName, threadCount, cudaBlockSize);
        result = runSolver(*solver, field);
#else
        if (mpiRank == 0) {
            std::cerr << "Error: solver \"" << solverName
                      << "\" requires rebuilding with -DENABLE_CUDA=ON\n";
        }
        return;
#endif
    } else if (mpiRank == 0) {
#ifdef ENABLE_CUDA_SOLVER
        if (solverName == "cuda") {
            auto solver = makeSolver(solverName, threadCount, cudaBlockSize);
            result = runSolver(*solver, field);
        } else
#endif
        {
            auto solver = makeSolver(solverName, threadCount);
            result = runSolver(*solver, field);
        }
    }

    if (mpiRank == 0) {
        std::string label = solverName;
        if (solverName == "openmp" || solverName == "pthreads") {
            label += " (" + std::to_string(threadCount) + "t)";
        } else if (solverName == "mpi") {
            label += " (" + std::to_string(mpiSize) + " rank(s))";
#ifdef ENABLE_CUDA_SOLVER
        } else if (solverName == "hybrid") {
            label += " (" + std::to_string(mpiSize) + " rank(s), blk=" +
                     std::to_string(cudaBlockSize) + ")";
        } else if (solverName == "cuda") {
            label += " (blk=" + std::to_string(cudaBlockSize) + ")";
#endif
        }

        std::cout << '\n';
        printTiming(label, result.elapsedMilliseconds, static_cast<int>(label.size()) + 2);
        std::cout << std::endl;
    }
}
