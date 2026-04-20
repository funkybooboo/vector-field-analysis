#include "cudaMpiStreamlineSolver.hpp"

#include "mpiStreamlineSolver.hpp"
#include "sequentialStreamlineSolver.hpp"

#ifdef ENABLE_CUDA_SOLVER
#include "cuda.hpp"
#include "cudaStreamlineSolver.hpp"

#include <cuda_runtime.h>
#endif

#ifdef USE_MPI
#include <mpi.h>
#endif

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

void CudaMpiStreamlineSolver::computeTimeStep(Field::Grid& grid) {
#if defined(ENABLE_CUDA_SOLVER) && defined(USE_MPI)
    int mpiReady = 0;
    MPI_Initialized(&mpiReady);
    if (mpiReady == 0) {
        CudaStreamlineSolver fallback(blockSize_);
        fallback.computeTimeStep(grid);
        return;
    }

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size == 1) {
        CudaStreamlineSolver fallback(blockSize_);
        fallback.computeTimeStep(grid);
        return;
    }

    int deviceCount = 0;
    const cudaError_t deviceCountStatus = cudaGetDeviceCount(&deviceCount);
    int localGpuReady = (deviceCountStatus == cudaSuccess && deviceCount > 0) ? 1 : 0;
    if (localGpuReady == 1 && cudaSetDevice(rank % deviceCount) != cudaSuccess) {
        localGpuReady = 0;
    }

    int globalGpuReady = 0;
    MPI_Allreduce(&localGpuReady, &globalGpuReady, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    if (globalGpuReady == 0) {
        MpiStreamlineSolver fallback;
        fallback.computeTimeStep(grid);
        return;
    }

    const int rowCount = static_cast<int>(grid.rows());
    const int colCount = static_cast<int>(grid.cols());
    const std::size_t totalCells =
        static_cast<std::size_t>(rowCount) * static_cast<std::size_t>(colCount);

    if (totalCells > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        std::cerr << "rank " << rank << ": fatal: grid too large for hybrid MPI/CUDA solver\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    const int totalCellsInt = static_cast<int>(totalCells);

    const int rowsPerRank = rowCount / size;
    const int remainder = rowCount % size;
    const int startRow = (rank * rowsPerRank) + std::min(rank, remainder);
    const int endRow = startRow + rowsPerRank + (rank < remainder ? 1 : 0);
    const int localRowCount = endRow - startRow;

    const std::vector<int> localSuccessors = cuda::computeSuccessorSlice(
        grid.field(), rowCount, colCount, grid.bounds(), startRow, endRow, blockSize_);

    for (int localRow = 0; localRow < localRowCount; ++localRow) {
        const int globalRow = startRow + localRow;
        for (int col = 0; col < colCount; ++col) {
            const std::size_t localIdx =
                static_cast<std::size_t>(localRow) * static_cast<std::size_t>(colCount) +
                static_cast<std::size_t>(col);
            const std::size_t srcIdx =
                static_cast<std::size_t>(globalRow) * static_cast<std::size_t>(colCount) +
                static_cast<std::size_t>(col);
            const std::size_t destIdx = static_cast<std::size_t>(localSuccessors[localIdx]);
            grid.unite(srcIdx, destIdx);
        }
    }

    std::vector<std::uint64_t> localRoots(totalCells);
    std::vector<std::uint64_t> globalRoots(totalCells);

    for (int round = 0; round < size; ++round) {
        for (std::size_t i = 0; i < totalCells; ++i) {
            localRoots[i] = static_cast<std::uint64_t>(grid.findRoot(i));
        }

        MPI_Allreduce(localRoots.data(), globalRoots.data(), totalCellsInt, MPI_UINT64_T, MPI_MIN,
                      MPI_COMM_WORLD);

        int localChanged = 0;
        for (std::size_t i = 0; i < totalCells; ++i) {
            if (globalRoots[i] < localRoots[i]) {
                grid.unite(i, static_cast<std::size_t>(globalRoots[i]));
                localChanged = 1;
            }
        }

        int globalChanged = 0;
        MPI_Allreduce(&localChanged, &globalChanged, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        if (globalChanged == 0) {
            break;
        }
    }

    std::vector<std::size_t> roots(totalCells);
    for (std::size_t i = 0; i < totalCells; ++i) {
        roots[i] = grid.findRoot(i);
    }

    const std::vector<std::size_t> indices = StreamlineSolver::sortCellsByRoot(roots, totalCells);

    const std::size_t cellsPerRank = totalCells / static_cast<std::size_t>(size);
    const std::size_t cellRemainder = totalCells % static_cast<std::size_t>(size);
    const std::size_t startIdx = static_cast<std::size_t>(rank) * cellsPerRank +
                                 std::min(static_cast<std::size_t>(rank), cellRemainder);
    const std::size_t endIdx =
        startIdx + cellsPerRank + (static_cast<std::size_t>(rank) < cellRemainder ? 1 : 0);

    const std::vector<Field::Path> localPaths = StreamlineSolver::buildPathsForRange(
        roots, indices, totalCells, colCount, startIdx, endIdx, static_cast<std::size_t>(rank));

    std::vector<int> buf;
    buf.push_back(static_cast<int>(localPaths.size()));
    for (const auto& path : localPaths) {
        buf.push_back(static_cast<int>(path.size()));
        for (const auto& cell : path) {
            buf.push_back(cell.row);
            buf.push_back(cell.col);
        }
    }

    const int localSize = static_cast<int>(buf.size());
    std::vector<int> allSizes(static_cast<std::size_t>(size));
    MPI_Gather(&localSize, 1, MPI_INT, allSizes.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    std::vector<int> displs;
    int totalBufSize = 0;
    std::vector<int> recvBuf;
    if (rank == 0) {
        displs.resize(static_cast<std::size_t>(size));
        for (int r = 0; r < size; ++r) {
            displs[static_cast<std::size_t>(r)] = totalBufSize;
            totalBufSize += allSizes[static_cast<std::size_t>(r)];
        }
        recvBuf.resize(static_cast<std::size_t>(totalBufSize));
    }

    MPI_Gatherv(buf.data(), localSize, MPI_INT, recvBuf.data(), allSizes.data(), displs.data(),
                MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::vector<Field::Path> finalPaths;
        for (int r = 0; r < size; ++r) {
            int pos = displs[static_cast<std::size_t>(r)] + 1;
            const int nPaths =
                recvBuf[static_cast<std::size_t>(displs[static_cast<std::size_t>(r)])];
            for (int p = 0; p < nPaths; ++p) {
                const int pathLen = recvBuf[static_cast<std::size_t>(pos++)];
                Field::Path path;
                path.reserve(static_cast<std::size_t>(pathLen));
                for (int k = 0; k < pathLen; ++k) {
                    const int row = recvBuf[static_cast<std::size_t>(pos++)];
                    const int col = recvBuf[static_cast<std::size_t>(pos++)];
                    path.push_back({row, col});
                }
                finalPaths.push_back(std::move(path));
            }
        }
        grid.setPrecomputedStreamlines(std::move(finalPaths));
    }
#else
#ifdef ENABLE_CUDA_SOLVER
    CudaStreamlineSolver fallback(blockSize_);
    fallback.computeTimeStep(grid);
#elif defined(USE_MPI)
    MpiStreamlineSolver fallback;
    fallback.computeTimeStep(grid);
#else
    SequentialStreamlineSolver fallback;
    fallback.computeTimeStep(grid);
#endif
#endif
}
