#include "pthreadsStreamlineSolver.hpp"

#include "timing.hpp"

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
    Field::Grid* grid;
    Field::GridCell* neighbors;
    int colCount;
    std::size_t startRow;
    std::size_t endRow;
    std::size_t totalCells;
};

// Pass 1 worker: reads neighbor directions for an assigned row range.
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

// Pass 2a worker: performs lock-free DSU union for an assigned cell range.
void* uniteNeighbors(void* arg) {
    auto* task = static_cast<ThreadArgs*>(arg);
    const std::size_t start = task->startRow; // Reusing fields for flat cell range
    const std::size_t end = task->endRow;
    for (std::size_t i = start; i < end; ++i) {
        const std::size_t srcIndex = i;
        const std::size_t destIndex =
            task->grid->coordsToIndex(task->neighbors[i].row, task->neighbors[i].col);
        task->grid->unite(srcIndex, destIndex);
    }
    return nullptr;
}

} // namespace

PthreadsStreamlineSolver::PthreadsStreamlineSolver(unsigned int threadCount)
    : threadCount_(threadCount) {}

void PthreadsStreamlineSolver::computeTimeStep(Field::Grid& grid) {

    printTiming("PthreadsStreamlineSolver");

    if (threadCount_ == 0) {
        return;
    }

    const std::size_t rowCount = grid.rows();
    if (rowCount == 0) {
        return;
    }
    const int colCount = static_cast<int>(grid.cols());
    if (colCount == 0) {
        return;
    }

    // Pass 1: parallel -- compute all (src, dest) neighbor pairs.
    std::vector<Field::GridCell> neighbors(rowCount * static_cast<std::size_t>(colCount));

    auto lastTime = getCurrentTimeMs();
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

        const int err = pthread_create(&threads[threadIndex], nullptr, computeNeighbors,
                                       &threadArgs[threadIndex]);
        if (err != 0) {
            // Join all already-running threads before propagating the error so
            // they don't outlive threadArgs and neighbors.
            for (unsigned int j = 0; j < threadIndex; j++) {
                pthread_join(threads[j], nullptr);
            }
            throw std::runtime_error("pthread_create failed with error code " +
                                     std::to_string(err));
        }
    }

    for (unsigned int threadIndex = 0; threadIndex < threadCount_; threadIndex++) {
        const int err = pthread_join(threads[threadIndex], nullptr);
        if (err != 0) {
            // Join remaining threads before propagating so they don't outlive threadArgs/neighbors.
            for (unsigned int j = threadIndex + 1; j < threadCount_; j++) {
                pthread_join(threads[j], nullptr);
            }
            throw std::runtime_error("pthread_join failed with error code " + std::to_string(err));
        }
    }
    printTiming("PthreadsStreamlineSolver: Finished Pass 1", lastTime);

    lastTime = getCurrentTimeMs();
    // Pass 2a: Parallel Union-Find using lock-free atomics in Field::Grid.
    const std::size_t totalCells = rowCount * static_cast<std::size_t>(colCount);
    const std::size_t cellsPerThread = totalCells / threadCount_;
    const std::size_t remainderCells = totalCells % threadCount_;

    std::size_t currentCell = 0;
    for (unsigned int threadIndex = 0; threadIndex < threadCount_; threadIndex++) {
        threadArgs[threadIndex].grid = &grid;
        threadArgs[threadIndex].neighbors = neighbors.data();
        threadArgs[threadIndex].colCount = colCount;
        threadArgs[threadIndex].startRow = currentCell; // Reusing startRow for startCell

        if (threadIndex == threadCount_ - 1) {
            threadArgs[threadIndex].endRow = currentCell + cellsPerThread + remainderCells;
        } else {
            threadArgs[threadIndex].endRow = currentCell + cellsPerThread;
        }
        currentCell = threadArgs[threadIndex].endRow;

        const int err = pthread_create(&threads[threadIndex], nullptr, uniteNeighbors,
                                       &threadArgs[threadIndex]);
        if (err != 0) {
            for (unsigned int j = 0; j < threadIndex; j++) {
                pthread_join(threads[j], nullptr);
            }
            throw std::runtime_error("pthread_create (Pass 2a) failed: " + std::to_string(err));
        }
    }

    for (unsigned int threadIndex = 0; threadIndex < threadCount_; threadIndex++) {
        pthread_join(threads[threadIndex], nullptr);
    }
    printTiming("PthreadsStreamlineSolver: Finished Pass 2a", lastTime);

    lastTime = getCurrentTimeMs();
    // Pass 2b: Sequential reconstruction of deterministic paths.
    grid.setPrecomputedStreamlines(reconstructPathsDSU(grid, neighbors));
    printTiming("PthreadsStreamlineSolver: Finished Pass 2b", lastTime);
}
