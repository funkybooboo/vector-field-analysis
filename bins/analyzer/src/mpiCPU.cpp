#include "mpiCPU.hpp"

#include "sequentialCPU.hpp"

#ifdef USE_MPI
#include <algorithm>
#include <limits>
#include <mpi.h>
#include <stdexcept>
#include <utility>
#include <vector>
#endif

void MpiCPU::computeTimeStep(VectorField::FieldGrid& grid) {
#ifdef USE_MPI
    // Each (src, dest) pair is packed as four ints: srcRow, srcCol, destRow, destCol.
    static constexpr int kTupleSize = 4;

    // Fall back to sequential if MPI was not initialised (e.g. in unit tests).
    int mpiReady = 0;
    MPI_Initialized(&mpiReady);
    if (mpiReady == 0) {
        SequentialCPU fallback;
        fallback.computeTimeStep(grid);
        return;
    }

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size == 1) {
        // Single-rank: no communication overhead, identical to sequential.
        SequentialCPU fallback;
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
    // neighborInVectorDirection is const and reads no mutable state -- concurrent
    // calls across disjoint row ranges are race-free.
    const auto kTS = static_cast<std::size_t>(kTupleSize);
    // Guard size_t overflow before computing localCount.
    if (colCount > 0 &&
        static_cast<std::size_t>(localRows) >
            std::numeric_limits<std::size_t>::max() / kTS / static_cast<std::size_t>(colCount)) {
        throw std::overflow_error("grid too large for MPI gather (size_t overflow)");
    }
    const auto localCount =
        static_cast<std::size_t>(localRows) * static_cast<std::size_t>(colCount) * kTS;
    if (localCount > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::overflow_error("localCount exceeds INT_MAX; grid too large for MPI gather");
    }
    const int localCountInt = static_cast<int>(localCount);
    std::vector<int> localData(localCount);
    for (int row = startRow; row < endRow; row++) {
        for (int col = 0; col < colCount; col++) {
            const auto cellIdx =
                (static_cast<std::size_t>(row - startRow) * static_cast<std::size_t>(colCount)) +
                static_cast<std::size_t>(col);
            const std::size_t base = cellIdx * kTS;
            auto [destRow, destCol] = grid.neighborInVectorDirection(row, col);
            localData[base + 0] = row;
            localData[base + 1] = col;
            localData[base + 2] = destRow;
            localData[base + 3] = destCol;
        }
    }

    // Gather per-rank element counts to root.
    // recvCounts/displs are only allocated on rank 0; non-root passes nullptr
    // which is valid per MPI spec (recvbuf/recvcounts/displs ignored on non-root).
    std::vector<int> recvCounts;
    if (rank == 0) {
        recvCounts.resize(static_cast<std::size_t>(size), 0);
    }
    MPI_Gather(&localCountInt, 1, MPI_INT,
               rank == 0 ? recvCounts.data() : nullptr, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Build displacements and receive buffer on rank 0.
    std::vector<int> displs;
    std::vector<int> allData;
    if (rank == 0) {
        displs.resize(static_cast<std::size_t>(size), 0);
        for (int i = 1; i < size; i++) {
            displs[static_cast<std::size_t>(i)] = displs[static_cast<std::size_t>(i - 1)] +
                                                  recvCounts[static_cast<std::size_t>(i - 1)];
        }
        const int totalCount = displs[static_cast<std::size_t>(size - 1)] +
                               recvCounts[static_cast<std::size_t>(size - 1)];
        allData.resize(static_cast<std::size_t>(totalCount));
    }

    // Gather all neighbor pairs to rank 0.
    // MPI spec: recvcounts/displs/recvbuf are significant only at root.
    // allData.data() and recvCounts.data() are nullptr on non-root (empty vectors) -- valid.
    MPI_Gatherv(localData.data(), localCountInt, MPI_INT,
                allData.data(), recvCounts.data(), displs.data(), MPI_INT, 0, MPI_COMM_WORLD);

    // Pass 2 (sequential on rank 0): apply all (src, dest) pairs to build streamlines.
    // traceStreamlineStep writes to streams_ and is not thread-safe, so this must be
    // sequential.  Only rank 0 has the full allData, so only rank 0 does this pass.
    if (rank == 0) {
        const std::size_t totalPairs = allData.size() / kTS;
        for (std::size_t i = 0; i < totalPairs; i++) {
            const std::size_t base = i * kTS;
            grid.traceStreamlineStep({allData[base + 0], allData[base + 1]},
                                     {allData[base + 2], allData[base + 3]});
        }
    }
#else
    SequentialCPU fallback;
    fallback.computeTimeStep(grid);
#endif
}
