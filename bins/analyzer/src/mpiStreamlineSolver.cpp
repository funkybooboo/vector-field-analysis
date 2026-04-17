#include "mpiStreamlineSolver.hpp"

#include "sequentialStreamlineSolver.hpp"

#ifdef USE_MPI
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <mpi.h>
#include <vector>

#endif

void MpiStreamlineSolver::computeTimeStep(Field::Grid& grid) {
#ifdef USE_MPI
    // Fall back to sequential if MPI was not initialised (e.g. in unit tests).
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
    if (rowCount == 0) {
        return;
    }
    const int colCount = static_cast<int>(grid.cols());
    if (colCount == 0) {
        return;
    }

    // Assign a contiguous row range to this rank.
    const int rowsPerRank = rowCount / size;
    const int remainder = rowCount % size;
    const int startRow = (rank * rowsPerRank) + std::min(rank, remainder);
    const int endRow = startRow + rowsPerRank + (rank < remainder ? 1 : 0);
    const int localRows = endRow - startRow;

    // Guard size_t overflow before computing localCellCount.
    if (static_cast<std::size_t>(localRows) >
        std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(colCount)) {
        std::cerr << "rank " << rank << ": fatal: grid too large (size_t overflow)\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    const auto localCellCount =
        static_cast<std::size_t>(localRows) * static_cast<std::size_t>(colCount);
    if (localCellCount > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        std::cerr << "rank " << rank << ": fatal: localCellCount exceeds INT_MAX\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    const int localCellCountInt = static_cast<int>(localCellCount);

    // Pass 1 (parallel): each rank computes downstream indices for its row range.
    // Store as a single int per cell (destRow * colCount + destCol) -- 4x smaller
    // than the previous 4-tuple (srcRow, srcCol, destRow, destCol) format.
    localNext_.resize(localCellCount);
    for (int row = startRow; row < endRow; ++row) {
        for (int col = 0; col < colCount; ++col) {
            const std::size_t localIdx =
                (static_cast<std::size_t>(row - startRow) * static_cast<std::size_t>(colCount)) +
                static_cast<std::size_t>(col);
            auto [destRow, destCol] = grid.downstreamCell(row, col);
            localNext_[localIdx] = (destRow * colCount) + destCol;
        }
    }

    // Pass 2 (communication + apply): all ranks share neighbor data via Allgatherv,
    // then every rank independently applies all N union-find steps. This keeps the
    // critical path at N/size (Pass 1) + N (Pass 2) instead of the old approach where
    // rank 0 had to apply 7/8 of the grid serially after a one-sided Gatherv.
    std::vector<int> recvCounts(static_cast<std::size_t>(size), 0);
    MPI_Allgather(&localCellCountInt, 1, MPI_INT, recvCounts.data(), 1, MPI_INT, MPI_COMM_WORLD);

    std::vector<int> displacements(static_cast<std::size_t>(size), 0);
    {
        int64_t running = 0;
        for (int r = 1; r < size; ++r) {
            running += recvCounts[static_cast<std::size_t>(r - 1)];
            if (running > std::numeric_limits<int>::max()) {
                std::cerr << "rank " << rank << ": fatal: total gather size exceeds INT_MAX\n";
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            displacements[static_cast<std::size_t>(r)] = static_cast<int>(running);
        }
        const int64_t total = running + recvCounts[static_cast<std::size_t>(size - 1)];
        if (total > std::numeric_limits<int>::max()) {
            std::cerr << "rank " << rank << ": fatal: total gather size exceeds INT_MAX\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        allNext_.resize(static_cast<std::size_t>(total));
    }

    MPI_Allgatherv(localNext_.data(), localCellCountInt, MPI_INT, allNext_.data(),
                   recvCounts.data(), displacements.data(), MPI_INT, MPI_COMM_WORLD);

    // All ranks apply the complete neighbor graph sequentially.
    std::vector<Field::GridCell> neighbors(static_cast<std::size_t>(rowCount) *
                                           static_cast<std::size_t>(colCount));
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < colCount; ++col) {
            const std::size_t idx =
                (static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount)) +
                static_cast<std::size_t>(col);
            const int dstIdx = allNext_[idx];
            neighbors[idx] = {dstIdx / colCount, dstIdx % colCount};
        }
    }
    applyNeighborPairs(grid, neighbors, rowCount, colCount);
#else
    SequentialStreamlineSolver fallback;
    fallback.computeTimeStep(grid);
#endif
}
