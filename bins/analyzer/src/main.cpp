#include "analyzerConfig.hpp"
#include "analyzerConfigParser.hpp"
#include "fieldReader.hpp"
#include "formatBytes.hpp"
#include "solverFactory.hpp"
#include "streamWriter.hpp"
#include "streamlineSolver.hpp"

#ifdef USE_MPI
#include <mpi.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <string_view>
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
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
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
            std::cerr << "Error: " << name << " streamlines differ from sequential at step " << stepIndex
                      << "\n";
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

// Derive output path: strip trailing .h5 (if present) and append .streams.h5
static std::string makeOutPath(const std::string& inPath) {
    const std::string suffix = ".h5";
    if (inPath.size() > suffix.size() &&
        inPath.compare(inPath.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return inPath.substr(0, inPath.size() - suffix.size()) + ".streams.h5";
    }
    return inPath + ".streams.h5";
}

static void printHelp() {
    std::cout << "Usage: analyzer <config.toml>\n"
              << "\nRuns vector field streamline analysis using the given TOML config file.\n"
              << "See bins/analyzer/configs/ for example configs.\n"
              << "See bins/analyzer/docs/config-guide.md for all config keys.\n"
              << "\n[analyzer] keys:\n"
              << "  input   = \"field.h5\"   HDF5 field file to read\n"
              << "  output  = \"\"           output path (default: <input>.streams.h5)\n"
              << "  solver  = \"all\"        sequential | openmp | pthreads | mpi | all\n"
              << "  threads = 0            thread count for pthreads (0 = hardware_concurrency)\n"
              << "\nFor MPI: mpirun -n N analyzer <config.toml>  with solver = \"mpi\"\n";
}

static unsigned int resolveThreadCount(unsigned int requested) {
    if (requested > 0) {
        return requested;
    }
    if (const char* env = std::getenv("ANALYZER_THREADS")) {
        char* end = nullptr;
        const long val = std::strtol(env, &end, 10);
        if (end != env && val > 0) {
            return static_cast<unsigned int>(val);
        }
    }
    const unsigned int hw = std::thread::hardware_concurrency();
    return hw > 0 ? hw : 1;
}

static void runAll(const Field::TimeSeries& field, unsigned int threadCount, int mpiRank,
                   int mpiSize, const std::string& outPath) {
    // Non-MPI solvers run only on rank 0.
    RunResult sequentialResult{};
    RunResult openmpResult{};
    RunResult pthreadsResult{};
    if (mpiRank == 0) {
        auto sequentialSolver = makeSolver("sequential", threadCount);
        auto openmpSolver = makeSolver("openmp", threadCount);
        auto pthreadsSolver = makeSolver("pthreads", threadCount);
        sequentialResult = runSolver(*sequentialSolver, field);
        openmpResult = runSolver(*openmpSolver, field);
        pthreadsResult = runSolver(*pthreadsSolver, field);
    }

    // MPI solver: all ranks must participate (collective calls inside).
    // Skip when running single-rank -- it falls back to sequential and adds no signal.
    RunResult mpiResult{};
    const bool runMpi = mpiSize > 1;
    if (runMpi) {
        auto mpi = makeSolver("mpi", threadCount);
        mpiResult = runSolver(*mpi, field);
    }

    if (mpiRank == 0) {
        const std::string sequentialLabel = "sequential";
        const std::string openmpLabel = "openmp (" + std::to_string(threadCount) + " thr)";
        const std::string pthreadsLabel = "pthreads (" + std::to_string(threadCount) + " thr)";
        const std::string mpiLabel = "mpi (" + std::to_string(mpiSize) + " rank(s))";

        int labelWidth =
            static_cast<int>(std::max({sequentialLabel.size(), openmpLabel.size(),
                                       pthreadsLabel.size(), mpiLabel.size()})) +
            2;
        std::cout << std::left << std::setw(labelWidth) << sequentialLabel << sequentialResult.elapsedMilliseconds
                  << " ms\n"
                  << std::setw(labelWidth) << openmpLabel << openmpResult.elapsedMilliseconds << " ms"
                  << "  ("
                  << (openmpResult.elapsedMilliseconds > 0 ? sequentialResult.elapsedMilliseconds / openmpResult.elapsedMilliseconds : 0.0)
                  << "x vs sequential)\n"
                  << std::setw(labelWidth) << pthreadsLabel << pthreadsResult.elapsedMilliseconds << " ms"
                  << "  ("
                  << (pthreadsResult.elapsedMilliseconds > 0 ? sequentialResult.elapsedMilliseconds / pthreadsResult.elapsedMilliseconds : 0.0)
                  << "x vs sequential)\n";
        if (runMpi) {
            std::cout << std::setw(labelWidth) << mpiLabel << mpiResult.elapsedMilliseconds << " ms"
                      << "  ("
                      << (mpiResult.elapsedMilliseconds > 0 ? sequentialResult.elapsedMilliseconds / mpiResult.elapsedMilliseconds : 0.0)
                      << "x vs sequential)\n";
        } else {
            std::cout << "(mpi skipped -- rerun with mpirun -n N for a multi-rank comparison)\n";
        }

        verify(sequentialResult.streams, openmpResult.streams, "openmp");
        verify(sequentialResult.streams, pthreadsResult.streams, "pthreads");
        if (runMpi) {
            verify(sequentialResult.streams, mpiResult.streams, "mpi");
        }

        writeAndReport(outPath, sequentialResult.streams, field.bounds, field.gridSize());
    }
}

static void runOne(const std::string& solverName, const Field::TimeSeries& field,
                   unsigned int threadCount, int mpiRank, int mpiSize, const std::string& outPath) {
    RunResult result{};
    if (solverName == "mpi") {
        auto solver = makeSolver(solverName, threadCount);
        result = runSolver(*solver, field);
    } else if (mpiRank == 0) {
        auto solver = makeSolver(solverName, threadCount);
        result = runSolver(*solver, field);
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
        }
        std::cout << label << "  " << result.elapsedMilliseconds << " ms\n";

        writeAndReport(outPath, result.streams, field.bounds, field.gridSize());
    }
}

int main(int argc, char* argv[]) {
#ifdef USE_MPI
    MPI_Init(&argc, &argv);
    int mpiRank = 0;
    int mpiSize = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
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
        const AnalyzerConfig config = ConfigParser::parseFile(argv[1]);
        const Field::TimeSeries field = FieldReader::read(config.inputPath);
        // Use the TOML output path if set; otherwise derive from the input field path.
        const std::string outPath =
            config.outputPath.empty() ? makeOutPath(config.inputPath) : config.outputPath;

        if (field.frames.empty()) {
            throw std::runtime_error("field file contains no time steps: " + config.inputPath);
        }

        // Resolve thread count from config, then adapt for fair comparison in "all" mode:
        // if MPI is active, align thread count to rank count so every parallel solver
        // (openmp, pthreads, mpi) works with the same number of workers.
        unsigned int threadCount = resolveThreadCount(config.threadCount);
        if (config.solver == "all" && mpiSize > 1) {
            threadCount = static_cast<unsigned int>(mpiSize);
        }

        if (mpiRank == 0) {
            const int numSteps = static_cast<int>(field.frames.size());
            const auto [width, height] = field.gridSize();
            std::cout << "Field:   " << config.inputPath << "\n"
                      << "Grid:    " << width << " x " << height << "  |  x [" << field.bounds.xMin
                      << ", " << field.bounds.xMax << "]"
                      << "  y [" << field.bounds.yMin << ", " << field.bounds.yMax << "]\n"
                      << "Steps:   " << numSteps << "\n\n";
        }

        if (config.solver == "all") {
            runAll(field, threadCount, mpiRank, mpiSize, outPath);
        } else {
            runOne(config.solver, field, threadCount, mpiRank, mpiSize, outPath);
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
