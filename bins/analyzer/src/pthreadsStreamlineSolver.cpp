#include "pthreadsStreamlineSolver.hpp"

#include <stdexcept>
#include <utility>

namespace {

std::pair<std::size_t, std::size_t> calculateRowSplit(std::size_t rowCount,
                                                      std::size_t partitionCount) {
    if (partitionCount == 0) {
        throw std::invalid_argument("partitionCount must be greater than 0");
    }
    return {rowCount / partitionCount, rowCount % partitionCount};
}

// Pass 1 worker: reads neighbor directions for an assigned row range.
void* computeNeighbors(void* arg) {
    auto* task = static_cast<PthreadsStreamlineSolver::ThreadArgs*>(arg);
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
    auto* task = static_cast<PthreadsStreamlineSolver::ThreadArgs*>(arg);
    const std::size_t start = task->startRow;
    const std::size_t end = task->endRow;
    for (std::size_t i = start; i < end; ++i) {
        task->grid->unite(
            i, task->grid->coordsToIndex(task->neighbors[i].row, task->neighbors[i].col));
    }
    return nullptr;
}

} // namespace

PthreadsStreamlineSolver::PthreadsStreamlineSolver(unsigned int threadCount)
    : threadCount_(threadCount) {}

void PthreadsStreamlineSolver::launchPass(std::vector<pthread_t>& threads,
                                          std::vector<ThreadArgs>& args, void* (*worker)(void*),
                                          const char* errorMsg) {
    const auto n = static_cast<unsigned int>(threads.size());
    for (unsigned int i = 0; i < n; ++i) {
        const int err = pthread_create(&threads[i], nullptr, worker, &args[i]);
        if (err != 0) {
            for (unsigned int j = 0; j < i; ++j) {
                pthread_join(threads[j], nullptr);
            }
            throw std::runtime_error(std::string(errorMsg) + std::to_string(err));
        }
    }
    for (unsigned int i = 0; i < n; ++i) {
        pthread_join(threads[i], nullptr);
    }
}

void PthreadsStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    const unsigned int effectiveThreads = threadCount_ == 0 ? 1 : threadCount_;
    const std::size_t rowCount = grid.rows();
    if (rowCount == 0) {
        return;
    }
    const int colCount = static_cast<int>(grid.cols());
    if (colCount == 0) {
        return;
    }

    std::vector<Field::GridCell> neighbors(rowCount * static_cast<std::size_t>(colCount));
    std::vector<pthread_t> threads(effectiveThreads);
    std::vector<ThreadArgs> threadArgs(effectiveThreads);

    const auto [rowsPerThread, remainderRows] = calculateRowSplit(rowCount, effectiveThreads);

    // Pass 1: parallel -- compute all (src, dest) neighbor pairs.
    std::size_t currentRow = 0;
    for (unsigned int i = 0; i < effectiveThreads; ++i) {
        threadArgs[i] = {&grid, neighbors.data(), colCount, currentRow,
                         currentRow + rowsPerThread +
                             (i == effectiveThreads - 1 ? remainderRows : 0)};
        currentRow += rowsPerThread;
    }
    launchPass(threads, threadArgs, computeNeighbors, "pthread_create failed: ");

    // Pass 2a: parallel DSU unite using lock-free atomics.
    const std::size_t totalCells = rowCount * static_cast<std::size_t>(colCount);
    const std::size_t cellsPerThread = totalCells / effectiveThreads;
    const std::size_t remainderCells = totalCells % effectiveThreads;

    std::size_t currentCell = 0;
    for (unsigned int i = 0; i < effectiveThreads; ++i) {
        threadArgs[i] = {&grid, neighbors.data(), colCount, currentCell,
                         currentCell + cellsPerThread +
                             (i == effectiveThreads - 1 ? remainderCells : 0)};
        currentCell = threadArgs[i].endRow;
    }
    launchPass(threads, threadArgs, uniteNeighbors, "pthread_create (Pass 2a) failed: ");
    // Pass 2b: sequential path reconstruction.
    grid.setPrecomputedStreamlines(reconstructPathsDSU(grid, neighbors));
}
