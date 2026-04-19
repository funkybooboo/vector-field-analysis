#include "configParser.hpp"
#include "fieldReader.hpp"
#include "runner.hpp"

#ifdef USE_MPI
#include <mpi.h>
#endif

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

static void printHelp() {
    std::cout << "Usage: analyzer <config.toml>\n"
              << "\nRuns vector field streamline analysis using the given TOML config file.\n"
              << "Reads data/<config-stem>/field.h5 and writes data/<config-stem>/streams.h5.\n"
              << "See configs/ for example configs.\n"
              << "\n[analyzer] keys (all optional):\n"
              << "  solver          = \"all\"  sequential | openmp | pthreads | mpi | cuda | all\n"
              << "                            (cuda requires -DENABLE_CUDA=ON at build time)\n"
              << "  threads         = 0      thread/rank count (0 = hardware_concurrency)\n"
              << "  cuda_block_size = 256    CUDA threads per block\n"
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
            runAll(field, threadCount, cudaBlockSize, mpiRank, mpiSize, outPath);
        } else {
            runOne(config.solver, field, threadCount, cudaBlockSize, mpiRank, mpiSize, outPath);
        }
    } catch (const std::exception& e) {
        if (mpiRank == 0) {
            std::cerr << "Error: " << e.what() << "\n";
        }
        // MPI_Abort terminates all ranks immediately; fall-through to MPI_Finalize would hang
        // ranks waiting on collectives that the throwing rank never reaches.
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
