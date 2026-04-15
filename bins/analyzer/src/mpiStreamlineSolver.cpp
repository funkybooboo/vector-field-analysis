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
    // Each (src, dest) pair is packed as four ints: srcRow, srcCol, destRow, destCol.
    static constexpr std::size_t kCellPairPackSize = 4;

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
        // Single-rank: no communication overhead, identical to sequential.
        SequentialStreamlineSolver fallback;
        fallback.computeTimeStep(grid);
        return;
    }

    const int rowCount = static_cast<int>(grid.rows());
    if (rowCount == 0) {
        return;
    }
    const int colCount = static_cast<int>(grid.cols());

    // Assign a contiguous row range to this rank.
    // Distribute remainder rows one-per-rank starting from rank 0.
    const int rowsPerRank = rowCount / size;
    const int remainder = rowCount % size;
    const int startRow = (rank * rowsPerRank) + std::min(rank, remainder);
    const int endRow = startRow + rowsPerRank + (rank < remainder ? 1 : 0);
    const int localRows = endRow - startRow;

    // Pass 1 (parallel): each rank reads neighbor directions for its row range.
    // downstreamCell is const and reads no mutable state -- concurrent
    // calls across disjoint row ranges are race-free.
    // Guard size_t overflow before computing localCount.
    // colCount == 0 -> localCount == 0, no overflow possible; guard avoids division by zero
    // in the overflow formula below.
    if (colCount > 0 && static_cast<std::size_t>(localRows) >
                            std::numeric_limits<std::size_t>::max() / kCellPairPackSize /
                                static_cast<std::size_t>(colCount)) {
        std::cerr << "rank " << rank
                  << ": fatal: grid too large for MPI gather (size_t overflow)\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    const auto localCount = static_cast<std::size_t>(localRows) *
                            static_cast<std::size_t>(colCount) * kCellPairPackSize;
    if (localCount > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        std::cerr << "rank " << rank
                  << ": fatal: localCount exceeds INT_MAX; grid too large for MPI gather\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    const int localCountInt = static_cast<int>(localCount);
    std::vector<int> localPairs(localCount);
    for (int row = startRow; row < endRow; row++) {
        for (int col = 0; col < colCount; col++) {
            const auto cellIdx =
                (static_cast<std::size_t>(row - startRow) * static_cast<std::size_t>(colCount)) +
                static_cast<std::size_t>(col);
            const std::size_t base = cellIdx * kCellPairPackSize;
            auto [destRow, destCol] = grid.downstreamCell(row, col);
            localPairs[base + 0] = row;
            localPairs[base + 1] = col;
            localPairs[base + 2] = destRow;
            localPairs[base + 3] = destCol;
        }
    }

    // Gather per-rank element counts to root.
    // Always allocate recvCounts on all ranks so .data() is never null; some MPI
    // implementations warn or assert on null recvbuf even when the argument is
    // formally ignored on non-root.
    std::vector<int> recvCounts(static_cast<std::size_t>(size), 0);
    MPI_Gather(&localCountInt, 1, MPI_INT, recvCounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Build displacements and receive buffer.
    // Root allocates real buffers; non-root gets 1-element dummies so .data()
    // is non-null for MPI_Gatherv (same portability reason as above).
    // displacements[0] = 0 intentionally (rank 0's data starts at offset 0); the loop below
    // starts at rankIndex=1 and fills the remaining entries from recvCounts.
    std::vector<int> displacements(rank == 0 ? static_cast<std::size_t>(size) : 1, 0);
    std::vector<int> allPairs(1);
    if (rank == 0) {
        int64_t runningDisplacement = 0;
        for (int rankIndex = 1; rankIndex < size; rankIndex++) {
            runningDisplacement += recvCounts[static_cast<std::size_t>(rankIndex - 1)];
            if (runningDisplacement > std::numeric_limits<int>::max()) {
                std::cerr << "rank 0: fatal: total gather size exceeds INT_MAX\n";
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            displacements[static_cast<std::size_t>(rankIndex)] =
                static_cast<int>(runningDisplacement);
        }
        const int64_t total = runningDisplacement + recvCounts[static_cast<std::size_t>(size - 1)];
        if (total > std::numeric_limits<int>::max()) {
            std::cerr << "rank 0: fatal: total gather size exceeds INT_MAX\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        allPairs.resize(static_cast<std::size_t>(total));
    }

    // Gather all neighbor pairs to rank 0.
    // recvcounts/displacements/recvbuf are significant only at root per MPI spec.
    MPI_Gatherv(localPairs.data(), localCountInt, MPI_INT, allPairs.data(), recvCounts.data(),
                displacements.data(), MPI_INT, 0, MPI_COMM_WORLD);

    // Pass 2 (sequential on rank 0): apply all (src, dest) pairs to build streamlines.
    // traceStreamlineStep writes to streamlines_ and is not thread-safe, so this must be
    // sequential.  Only rank 0 has the full allPairs, so only rank 0 does this pass.
    if (rank == 0) {
        if (allPairs.size() % kCellPairPackSize != 0) {
            std::cerr << "rank 0: fatal: gathered pairs buffer size " << allPairs.size()
                      << " is not a multiple of " << kCellPairPackSize << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        const std::size_t totalPairs = allPairs.size() / kCellPairPackSize;
        for (std::size_t pairIndex = 0; pairIndex < totalPairs; pairIndex++) {
            const std::size_t base = pairIndex * kCellPairPackSize;
            grid.traceStreamlineStep({allPairs[base + 0], allPairs[base + 1]},
                                     {allPairs[base + 2], allPairs[base + 3]});
        }
    }
#else
    SequentialStreamlineSolver fallback;
    fallback.computeTimeStep(grid);
#endif
}
