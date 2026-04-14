#include "pthreadsStreamlineSolver.hpp"

#include <pthread.h>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

std::pair<std::size_t, std::size_t> calculateRowSplit(std::size_t rowCount,
                                                       std::size_t partitionCount) {
    if (partitionCount == 0) {
        throw std::invalid_argument("partitionCount must be greater than 0");
    }
    return {rowCount / partitionCount, rowCount % partitionCount};
}

struct ThreadArgs {
    const Field::Grid* grid;
    Field::GridCell* neighbors;
    int colCount;
    std::size_t startRow;
    std::size_t endRow;
};

// Pass 1 worker: reads neighbor directions for an assigned row range.
// downstreamCell is const and reads no shared mutable state, so
// concurrent calls across disjoint row ranges are race-free.
void* computeNeighbors(void* arg) {
    auto* task = static_cast<ThreadArgs*>(arg);
    for (std::size_t row = task->startRow; row < task->endRow; row++) {
        for (int col = 0; col < task->colCount; col++) {
            task->neighbors[(row * static_cast<std::size_t>(task->colCount)) +
                            static_cast<std::size_t>(col)] =
                task->grid->downstreamCell(static_cast<int>(row), col);
        }
    }
    return nullptr;
}

} // namespace

PthreadsStreamlineSolver::PthreadsStreamlineSolver(unsigned int threadCount)
    : threadCount_(threadCount) {}

void PthreadsStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    if (threadCount_ == 0) {
        return;
    }

    const std::size_t rowCount = grid.rows();
    if (rowCount == 0) {
        return;
    }
    const int colCount = static_cast<int>(grid.cols());

    // Pass 1: parallel -- compute all (src, dest) neighbor pairs.
    std::vector<Field::GridCell> neighbors(rowCount * static_cast<std::size_t>(colCount));

    auto rowSplit = calculateRowSplit(rowCount, threadCount_);
    const std::size_t rowsPerThread = rowSplit.first;
    const std::size_t remainderRows = rowSplit.second;

    std::vector<pthread_t> threads(threadCount_);
    std::vector<ThreadArgs> threadArgs(threadCount_);

    std::size_t currentRow = 0;
    for (unsigned int threadIndex = 0; threadIndex < threadCount_; threadIndex++) {
        threadArgs[threadIndex].grid = &grid;
        threadArgs[threadIndex].neighbors = neighbors.data();
        threadArgs[threadIndex].colCount = colCount;
        threadArgs[threadIndex].startRow = currentRow;

        // Last thread receives remainder rows
        if (threadIndex == threadCount_ - 1) {
            threadArgs[threadIndex].endRow = currentRow + rowsPerThread + remainderRows;
        } else {
            threadArgs[threadIndex].endRow = currentRow + rowsPerThread;
        }
        currentRow += rowsPerThread;

        const int err =
            pthread_create(&threads[threadIndex], nullptr, computeNeighbors, &threadArgs[threadIndex]);
        if (err != 0) {
            // Join all already-running threads before propagating the error so
            // they don't outlive threadArgs and neighbors.
            for (unsigned int j = 0; j < threadIndex; j++) {
                pthread_join(threads[j], nullptr);
            }
            throw std::runtime_error("pthread_create failed with error code " + std::to_string(err));
        }
    }

    for (unsigned int threadIndex = 0; threadIndex < threadCount_; threadIndex++) {
        const int err = pthread_join(threads[threadIndex], nullptr);
        if (err != 0) {
            throw std::runtime_error("pthread_join failed with error code " + std::to_string(err));
        }
    }

    // Pass 2: sequential -- apply streamline merges using the precomputed pairs.
    // traceStreamlineStep writes to streamlines_ and is not thread-safe.
    applyNeighborPairs(grid, neighbors, static_cast<int>(rowCount), colCount);
}
