#include "mpiStreamlineSolver.hpp"

#include "sequentialStreamlineSolver.hpp"
#include "streamlineSolver.hpp"

#ifdef USE_MPI
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <mpi.h>
#include <vector>
#endif

void MpiStreamlineSolver::computeTimeStep(Field::Grid& grid) {
#ifdef USE_MPI
    int mpiReady = 0;
    MPI_Initialized(&mpiReady);
    if (mpiReady == 0) {
        SequentialStreamlineSolver fallback;
        fallback.computeTimeStep(grid);
        return;
    }

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size == 1) {
        SequentialStreamlineSolver fallback;
        fallback.computeTimeStep(grid);
        return;
    }

    const int rowCount = static_cast<int>(grid.rows());
    const int colCount = static_cast<int>(grid.cols());
    const std::size_t totalCells =
        static_cast<std::size_t>(rowCount) * static_cast<std::size_t>(colCount);

    if (totalCells > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        std::cerr << "rank " << rank << ": fatal: grid too large for MPI AllReduce count\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    const int totalCellsInt = static_cast<int>(totalCells);

    // Row range for this rank.
    const int rowsPerRank = rowCount / size;
    const int remainder = rowCount % size;
    const int startRow = (rank * rowsPerRank) + std::min(rank, remainder);
    const int endRow = startRow + rowsPerRank + (rank < remainder ? 1 : 0);

    // Pass 1 + 2a (parallel): each rank computes downstream cells for its rows
    // and immediately unites them in its local DSU.
    for (int row = startRow; row < endRow; ++row) {
        for (int col = 0; col < colCount; ++col) {
            const std::size_t srcIdx =
                static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount) +
                static_cast<std::size_t>(col);
            auto [destRow, destCol] = grid.downstreamCell(row, col);
            const std::size_t destIdx = grid.coordsToIndex(static_cast<std::size_t>(destRow),
                                                           static_cast<std::size_t>(destCol));
            grid.unite(srcIdx, destIdx);
        }
    }

    // Pass 2b: iterative AllReduce to converge cross-boundary components.
    //
    // Each rank has a local DSU that correctly merges cells within its own row
    // range, but cross-boundary connections (where src is on one rank and dest
    // on another) are only partially resolved locally. Each AllReduce round
    // propagates the global minimum root one hop further across rank boundaries.
    // p rounds are sufficient for any functional graph partitioned across p ranks.
    std::vector<std::uint64_t> localRoots(totalCells);
    std::vector<std::uint64_t> globalRoots(totalCells);

    for (int round = 0; round < size; ++round) {
        for (std::size_t i = 0; i < totalCells; ++i)
            localRoots[i] = static_cast<std::uint64_t>(grid.findRoot(i));

        MPI_Allreduce(localRoots.data(), globalRoots.data(), totalCellsInt, MPI_UINT64_T, MPI_MIN,
                      MPI_COMM_WORLD);

        int localChanged = 0;
        for (std::size_t i = 0; i < totalCells; ++i) {
            if (globalRoots[i] < localRoots[i]) {
                grid.unite(i, static_cast<std::size_t>(globalRoots[i]));
                localChanged = 1;
            }
        }

        // Early exit if all ranks converged.
        int globalChanged = 0;
        MPI_Allreduce(&localChanged, &globalChanged, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        if (globalChanged == 0)
            break;
    }

    // Pass 3: all ranks build their portion of paths in parallel, then gather to rank 0.
    {
        std::vector<std::size_t> roots(totalCells);
        for (std::size_t i = 0; i < totalCells; ++i) {
            roots[i] = grid.findRoot(i);
        }

        const std::vector<std::size_t> indices =
            StreamlineSolver::sortCellsByRoot(roots, totalCells);

        const std::size_t cellsPerRank = totalCells / static_cast<std::size_t>(size);
        const std::size_t cellRemainder = totalCells % static_cast<std::size_t>(size);
        const std::size_t startIdx = static_cast<std::size_t>(rank) * cellsPerRank +
                                     std::min(static_cast<std::size_t>(rank), cellRemainder);
        const std::size_t endIdx =
            startIdx + cellsPerRank + (static_cast<std::size_t>(rank) < cellRemainder ? 1 : 0);

        const std::vector<Field::Path> localPaths = StreamlineSolver::buildPathsForRange(
            roots, indices, totalCells, colCount, startIdx, endIdx, static_cast<std::size_t>(rank));

        // Serialize local paths: [nPaths, len0, r0, c0, ..., len1, r0, c0, ...]
        std::vector<int> buf;
        buf.push_back(static_cast<int>(localPaths.size()));
        for (const auto& path : localPaths) {
            buf.push_back(static_cast<int>(path.size()));
            for (const auto& cell : path) {
                buf.push_back(cell.row);
                buf.push_back(cell.col);
            }
        }

        // Gather buffer sizes, then buffer contents to rank 0.
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
            int pos = 0;
            for (int r = 0; r < size; ++r) {
                const int base = displs[static_cast<std::size_t>(r)];
                const int nPaths = recvBuf[static_cast<std::size_t>(base)];
                pos = base + 1;
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
    }
#else
    SequentialStreamlineSolver fallback;
    fallback.computeTimeStep(grid);
#endif
}
