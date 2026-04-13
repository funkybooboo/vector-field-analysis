#include "analyzerConfig.hpp"
#include "analyzerConfigParser.hpp"
#include "fieldReader.hpp"
#include "solverFactory.hpp"
#include "streamlineSolver.hpp"
#include "streamWriter.hpp"
#include "vectorField.hpp"

#ifdef USE_MPI
#include <mpi.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
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

static RunResult runSolver(StreamlineSolver& solver, const Vector::FieldTimeSeries& data) {
    RunResult result;
    auto t0 = std::chrono::steady_clock::now();
    for (const auto& step : data.steps) {
        VectorField::FieldGrid grid(data.xMin, data.xMax, data.yMin, data.yMax, step);
        solver.computeTimeStep(grid);
        result.streams.push_back(grid.getStreamlines());
    }
    result.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
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

static void printHelp() {
    std::cout << "Usage: analyzer <config.toml>\n"
              << "\nRuns vector field streamline analysis using the given TOML config file.\n"
              << "See bins/analyzer/configs/ for example configs.\n"
              << "See bins/analyzer/docs/config-guide.md for all config keys.\n"
              << "\n[analyzer] keys:\n"
              << "  input   = \"field.h5\"   HDF5 field file to read\n"
              << "  solver  = \"all\"        sequential | openmp | pthreads | mpi | all\n"
              << "  threads = 0            thread count for pthreads (0 = hardware_concurrency)\n"
              << "\nFor MPI: mpirun -n N analyzer <config.toml>  with solver = \"mpi\"\n";
}

static unsigned int resolveThreadCount(unsigned int requested) {
    if (requested > 0) {
        return requested;
    }
    const unsigned int hw = std::thread::hardware_concurrency();
    return hw > 0 ? hw : 1;
}

static void runAll(const Vector::FieldTimeSeries& data, unsigned int threadCount, int mpiRank,
                   int mpiSize, const std::string& inPath) {
    // Non-MPI solvers: only rank 0 needs their results; skip on other ranks.
    RunResult seqR{};
    RunResult ompR{};
    RunResult ptR{};
    if (mpiRank == 0) {
        auto seq = makeSolver("sequential", threadCount);
        auto omp = makeSolver("openmp", threadCount);
        auto pt  = makeSolver("pthreads", threadCount);
        seqR = runSolver(*seq, data);
        ompR = runSolver(*omp, data);
        ptR  = runSolver(*pt, data);
    }
    // MPI solver: all ranks must participate (collective calls inside).
    auto mpi = makeSolver("mpi", threadCount);
    const auto mpiR = runSolver(*mpi, data);

    if (mpiRank == 0) {
        const std::string seqLabel = "sequential";
        const std::string ompLabel = "openmp";
        const std::string ptLabel  = "pthreads (" + std::to_string(threadCount) + " thr)";
        const std::string mpiLabel = "mpi (" + std::to_string(mpiSize) + " rank(s))";
        const int w = static_cast<int>(
            std::max({seqLabel.size(), ompLabel.size(), ptLabel.size(), mpiLabel.size()})) + 2;
        std::cout << std::left
                  << std::setw(w) << seqLabel << seqR.ms << " ms\n"
                  << std::setw(w) << ompLabel << ompR.ms << " ms"
                  << "  (" << seqR.ms / ompR.ms << "x vs sequential)\n"
                  << std::setw(w) << ptLabel  << ptR.ms  << " ms"
                  << "  (" << seqR.ms / ptR.ms  << "x vs sequential)\n"
                  << std::setw(w) << mpiLabel << mpiR.ms << " ms"
                  << "  (" << seqR.ms / mpiR.ms << "x vs sequential)\n";

        verify(seqR.streams, ompR.streams, "openmp");
        verify(seqR.streams, ptR.streams, "pthreads");
        verify(seqR.streams, mpiR.streams, "mpi");

        const int height = static_cast<int>(data.steps[0].size());
        const int width = static_cast<int>(data.steps[0][0].size());
        const std::string outPath = makeOutPath(inPath);
        StreamWriter::write(outPath, seqR.streams, data.xMin, data.xMax, data.yMin, data.yMax,
                            width, height);
        std::cout << "\nStreamlines written to " << outPath << "\n";
    }
}

static void runOne(const std::string& solverName, const Vector::FieldTimeSeries& data,
                   unsigned int threadCount, int mpiRank, int mpiSize,
                   const std::string& inPath) {
    auto solver = makeSolver(solverName, threadCount);
    const auto result = runSolver(*solver, data);

    if (mpiRank == 0) {
        std::string label = solverName;
        if (solverName == "pthreads") {
            label += " (" + std::to_string(threadCount) + " thr)";
        } else if (solverName == "mpi") {
            label += " (" + std::to_string(mpiSize) + " rank(s))";
        }
        std::cout << label << "  " << result.ms << " ms\n";

        const int height = static_cast<int>(data.steps[0].size());
        const int width = static_cast<int>(data.steps[0][0].size());
        const std::string outPath = makeOutPath(inPath);
        StreamWriter::write(outPath, result.streams, data.xMin, data.xMax, data.yMin, data.yMax,
                            width, height);
        std::cout << "\nStreamlines written to " << outPath << "\n";
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
    // Non-const avoids cppcheck knownConditionTrueFalse on the mpiRank == 0 guards below.
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

    int exitCode = 0;
    try {
        const AnalyzerConfig config = AnalyzerConfigParser::parseFile(argv[1]);
        const unsigned int threadCount = resolveThreadCount(config.threadCount);
        const Vector::FieldTimeSeries data = FieldReader::read(config.inputPath);

        if (mpiRank == 0) {
            const int numSteps = static_cast<int>(data.steps.size());
            const int height = static_cast<int>(data.steps[0].size());
            const int width = static_cast<int>(data.steps[0][0].size());
            std::cout << "Field: " << config.inputPath << "  " << width << "x" << height << "  "
                      << numSteps << " step(s)";
            if (mpiSize > 1) {
                std::cout << "  MPI ranks: " << mpiSize;
            }
            std::cout << "\n\n";
        }

        if (config.solver == "all") {
            runAll(data, threadCount, mpiRank, mpiSize, config.inputPath);
        } else {
            runOne(config.solver, data, threadCount, mpiRank, mpiSize, config.inputPath);
        }
    } catch (const std::exception& e) {
        if (mpiRank == 0) {
            std::cerr << "Error: " << e.what() << "\n";
        }
        exitCode = 1;
    }

#ifdef USE_MPI
    MPI_Finalize();
#endif
    return exitCode;
}
