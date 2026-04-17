#include "configParser.hpp"
#include "fieldReader.hpp"
#include "formatBytes.hpp"
#include "solverFactory.hpp"
#include "streamWriter.hpp"
#include "streamlineSolver.hpp"

#ifdef USE_MPI
#include <mpi.h>
#endif

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>

struct RunResult {
    double elapsedMilliseconds;
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

// Canonicalize one step's streamlines for comparison by sorting only the
// collection of streamlines. Each streamline remains an ordered list of
// points, so path traversal order is preserved while comparison stays
// independent of the order in which unique streamlines were encountered
// during grid iteration.
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

// Crash with a descriptive message if impl's output differs from the reference
// (sequential). Comparison is canonical so iteration order doesn't matter.
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

static void printHelp() {
    std::cout << "Usage: analyzer <config.toml>\n"
              << "\nRuns vector field streamline analysis using the given TOML config file.\n"
              << "Reads data/<config-stem>/field.h5 and writes data/<config-stem>/streams.h5.\n"
              << "See configs/ for example configs.\n"
              << "\n[analyzer] keys (all optional):\n"
              << "  solver               = \"all\"  sequential | openmp | pthreads | mpi | cuda | "
                 "cuda_full | all\n"
              << "                                (cuda/cuda_full require -DENABLE_CUDA=ON at "
                 "build time)\n"
              << "  threads              = 0      thread/rank count (0 = hardware_concurrency)\n"
              << "  cuda_block_size      = 256    CUDA threads per block for cuda solver\n"
              << "  cuda_full_block_size = 256    CUDA threads per block for cuda_full solver\n"
              << "\nFor MPI: mpirun -n N analyzer <config.toml>  with solver = \"mpi\"\n";
}

static unsigned int resolveThreadCount(unsigned int requested) {
    if (requested > 0) {
        return requested;
    }
    if (const char* env = std::getenv("ANALYZER_THREADS")) {
        char* end = nullptr;
        errno = 0;
        const long val = std::strtol(env, &end, 10);
        if (end != env && errno != ERANGE && val > 0 &&
            val <= static_cast<long>(std::numeric_limits<unsigned int>::max())) {
            return static_cast<unsigned int>(val);
        }
    }
    const unsigned int hw = std::thread::hardware_concurrency();
    return hw > 0 ? hw : 1;
}

static void runAll(const Field::TimeSeries& field, unsigned int threadCount,
                   [[maybe_unused]] unsigned int cudaBlockSize,
                   [[maybe_unused]] unsigned int cudaFullBlockSize, int mpiRank, int mpiSize,
                   const std::string& outPath) {
    // Parallel solvers need at least 2 workers to add signal over sequential.
    const bool runPthreads = threadCount > 1;
#ifdef _OPENMP
    const bool runOpenmp = threadCount > 1;
#endif
#ifdef USE_MPI
    const bool runMpi = mpiSize > 1;
#endif

    // Non-MPI solvers (sequential, openmp, pthreads, cuda) run only on rank 0.
    RunResult sequentialResult{};
    RunResult pthreadsResult{};
#ifdef _OPENMP
    RunResult openmpResult{};
#endif
#ifdef ENABLE_CUDA_SOLVER
    RunResult cudaResult{};
    RunResult cudaFullResult{};
#endif
    if (mpiRank == 0) {
        auto sequentialSolver = makeSolver("sequential", threadCount);
        sequentialResult = runSolver(*sequentialSolver, field);
#ifdef _OPENMP
        if (runOpenmp) {
            auto openmpSolver = makeSolver("openmp", threadCount);
            openmpResult = runSolver(*openmpSolver, field);
        }
#endif
        if (runPthreads) {
            auto pthreadsSolver = makeSolver("pthreads", threadCount);
            pthreadsResult = runSolver(*pthreadsSolver, field);
        }
#ifdef ENABLE_CUDA_SOLVER
        auto cudaSolver = makeSolver("cuda", threadCount, cudaBlockSize);
        cudaResult = runSolver(*cudaSolver, field);
        auto cudaFullSolver = makeSolver("cuda_full", threadCount, cudaFullBlockSize);
        cudaFullResult = runSolver(*cudaFullSolver, field);
#endif
    }

    // MPI solver: all ranks must participate (collective calls inside).
    RunResult mpiResult{};
#ifdef USE_MPI
    if (runMpi) {
        auto mpi = makeSolver("mpi", threadCount);
        mpiResult = runSolver(*mpi, field);
    }
#endif

    if (mpiRank == 0) {
        // Build labels for solvers that will print a timing row.
        // Skip-message-only solvers don't need a label, so they don't affect labelWidth.
        const std::string sequentialLabel = "sequential";
        std::vector<std::size_t> labelSizes = {sequentialLabel.size()};
        if (runPthreads) {
            labelSizes.push_back(
                std::string("pthreads (" + std::to_string(threadCount) + " thr)").size());
        }
#ifdef _OPENMP
        if (runOpenmp) {
            labelSizes.push_back(
                std::string("openmp (" + std::to_string(threadCount) + " thr)").size());
        }
#endif
#ifdef USE_MPI
        if (runMpi) {
            labelSizes.push_back(
                std::string("mpi (" + std::to_string(mpiSize) + " rank(s))").size());
        }
#endif
#ifdef ENABLE_CUDA_SOLVER
        labelSizes.push_back(
            std::string("cuda (blk=" + std::to_string(cudaBlockSize) + ")").size());
        labelSizes.push_back(
            std::string("cuda_full (blk=" + std::to_string(cudaFullBlockSize) + ")").size());
#endif
        const int labelWidth =
            static_cast<int>(*std::max_element(labelSizes.begin(), labelSizes.end())) + 2;

        const auto printTiming = [&](const std::string& label, double ms) {
            std::cout << std::left << std::setw(labelWidth) << label << ms << " ms"
                      << "  (" << (ms > 0 ? sequentialResult.elapsedMilliseconds / ms : 0.0)
                      << "x vs sequential)\n";
        };

        std::cout << std::left << std::setw(labelWidth) << sequentialLabel
                  << sequentialResult.elapsedMilliseconds << " ms\n";

#ifdef _OPENMP
        if (runOpenmp) {
            printTiming("openmp (" + std::to_string(threadCount) + " thr)",
                        openmpResult.elapsedMilliseconds);
        } else {
            std::cout << "(openmp skipped -- thread count too low for parallel benefit)\n";
        }
#else
        std::cout << "(openmp skipped -- not compiled with OpenMP support)\n";
#endif

        if (runPthreads) {
            printTiming("pthreads (" + std::to_string(threadCount) + " thr)",
                        pthreadsResult.elapsedMilliseconds);
        } else {
            std::cout << "(pthreads skipped -- thread count too low for parallel benefit)\n";
        }

#ifdef USE_MPI
        if (runMpi) {
            printTiming("mpi (" + std::to_string(mpiSize) + " rank(s))",
                        mpiResult.elapsedMilliseconds);
        } else {
            std::cout << "(mpi skipped -- rank count too low for parallel benefit)\n";
        }
#else
        std::cout << "(mpi skipped -- not compiled with MPI support)\n";
#endif

#ifdef ENABLE_CUDA_SOLVER
        printTiming("cuda (blk=" + std::to_string(cudaBlockSize) + ")",
                    cudaResult.elapsedMilliseconds);
        printTiming("cuda_full (blk=" + std::to_string(cudaFullBlockSize) + ")",
                    cudaFullResult.elapsedMilliseconds);
#else
        std::cout << "(cuda skipped -- rebuild with -DENABLE_CUDA=ON)\n";
        std::cout << "(cuda_full skipped -- rebuild with -DENABLE_CUDA=ON)\n";
#endif

        if (runPthreads) {
            verify(sequentialResult.streams, pthreadsResult.streams, "pthreads");
        }
#ifdef _OPENMP
        if (runOpenmp) {
            verify(sequentialResult.streams, openmpResult.streams, "openmp");
        }
#endif
#ifdef USE_MPI
        if (runMpi) {
            verify(sequentialResult.streams, mpiResult.streams, "mpi");
        }
#endif
#ifdef ENABLE_CUDA_SOLVER
        verify(sequentialResult.streams, cudaResult.streams, "cuda");
        verify(sequentialResult.streams, cudaFullResult.streams, "cuda_full");
#endif

        writeAndReport(outPath, sequentialResult.streams, field.bounds, field.gridSize());
    }
}

static void runOne(const std::string& solverName, const Field::TimeSeries& field,
                   unsigned int threadCount, [[maybe_unused]] unsigned int cudaBlockSize,
                   [[maybe_unused]] unsigned int cudaFullBlockSize, int mpiRank, int mpiSize,
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
        } else if (solverName == "cuda_full") {
            auto solver = makeSolver(solverName, threadCount, cudaFullBlockSize);
            result = runSolver(*solver, field);
        } else {
            auto solver = makeSolver(solverName, threadCount);
            result = runSolver(*solver, field);
        }
#else
        if (solverName == "cuda" || solverName == "cuda_full") {
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
        // Non-zero ranks: completed collective role in runSolver(); only rank 0 writes output.
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
        } else if (solverName == "cuda_full") {
            label += " (blk=" + std::to_string(cudaFullBlockSize) + ")";
#endif
        }
        std::cout << label << "  " << result.elapsedMilliseconds << " ms\n";

        writeAndReport(outPath, result.streams, field.bounds, field.gridSize());
    }
}

int main(int argc, char* argv[]) {
#ifdef USE_MPI
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cerr << "Error: MPI_Init failed\n";
        std::exit(1);
    }
    int mpiRank = 0;
    int mpiSize = 1;
    if (MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank) != MPI_SUCCESS ||
        MPI_Comm_size(MPI_COMM_WORLD, &mpiSize) != MPI_SUCCESS) {
        std::cerr << "Error: MPI_Comm_rank/size failed\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
#else
    int mpiRank = 0;
    int mpiSize = 1;
#endif

    const bool wantHelp =
        argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h");
    if (wantHelp) {
        if (mpiRank == 0) {
            printHelp();
        }
#ifdef USE_MPI
        MPI_Finalize();
#endif
        return 0;
    }

    if (argc < 2) {
        if (mpiRank == 0) {
            std::cerr << "Usage: analyzer <config.toml>\n";
        }
#ifdef USE_MPI
        MPI_Finalize();
#endif
        return 1;
    }

    try {
        const AnalyzerConfig config = ConfigParser::parseAnalyzer(argv[1]);
        const std::string stem = std::filesystem::path(argv[1]).stem().string();
        const std::string fieldPath = "data/" + stem + "/field.h5";
        const std::string outPath =
            config.output.empty() ? "data/" + stem + "/streams.h5" : config.output;
        std::filesystem::create_directories(
            std::filesystem::path(outPath).parent_path().empty()
                ? "."
                : std::filesystem::path(outPath).parent_path().string());
        const Field::TimeSeries field = FieldReader::read(fieldPath);

        if (field.frames.empty()) {
            throw std::runtime_error("field file contains no time steps: " + fieldPath);
        }

        // Resolve thread count from config, then adapt for fair comparison in "all" mode:
        // if MPI is active, align thread count to rank count so every parallel solver
        // (openmp, pthreads, mpi) works with the same number of workers.
        unsigned int threadCount = resolveThreadCount(config.threadCount);
        if (config.solver == "all" && mpiSize > 1) {
            threadCount = static_cast<unsigned int>(mpiSize);
        }

        const unsigned int cudaBlockSize = config.cudaBlockSize;
        const unsigned int cudaFullBlockSize = config.cudaFullBlockSize;

        if (mpiRank == 0) {
            const int numSteps = static_cast<int>(field.frames.size());
            const auto [width, height] = field.gridSize();
            std::cout << "Field:   " << fieldPath << "\n"
                      << "Grid:    " << width << " x " << height << "  |  x [" << field.bounds.xMin
                      << ", " << field.bounds.xMax << "]"
                      << "  y [" << field.bounds.yMin << ", " << field.bounds.yMax << "]\n"
                      << "Steps:   " << numSteps << "\n\n";
        }

        if (config.solver == "all") {
            runAll(field, threadCount, cudaBlockSize, cudaFullBlockSize, mpiRank, mpiSize, outPath);
        } else {
            runOne(config.solver, field, threadCount, cudaBlockSize, cudaFullBlockSize, mpiRank,
                   mpiSize, outPath);
        }
    } catch (const std::exception& e) {
        if (mpiRank == 0) {
            std::cerr << "Error: " << e.what() << "\n";
        }
        // MPI_Abort terminates all ranks immediately. If we only set an exit code and fall
        // through to MPI_Finalize, any rank that threw before reaching a collective will
        // cause the other ranks to hang waiting for that collective to complete.
#ifdef USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
        return 1;
    }

#ifdef USE_MPI
    MPI_Finalize();
#endif
    return 0;
}
