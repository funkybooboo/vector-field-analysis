#include "pthreadsStreamlineSolver.hpp"

#include "sequentialStreamlineSolver.hpp"
#include "streamlineSolver.hpp"

#include <pthread.h>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

struct ThreadArgs {
    Field::Grid* grid;
    Field::GridCell* neighbors;
    std::vector<std::size_t>* roots;
    std::vector<std::size_t>* indices;
    int colCount;
    unsigned int threadIndex;
    unsigned int threadCount;
    pthread_barrier_t* barrier;
    std::vector<Field::Path>* localPaths;
};

void* workerFunc(void* arg) {
    auto* task = static_cast<ThreadArgs*>(arg);
    const std::size_t rowCount = task->grid->rows();
    const int colCount = task->colCount;
    const std::size_t totalCells = rowCount * static_cast<std::size_t>(colCount);

    // Pass 1: compute all (src, dest) neighbor pairs.
    {
        const std::size_t rowsPerThread = rowCount / task->threadCount;
        const std::size_t remainderRows = rowCount % task->threadCount;
        const std::size_t startRow =
            task->threadIndex * rowsPerThread +
            std::min(static_cast<std::size_t>(task->threadIndex), remainderRows);
        const std::size_t endRow =
            startRow + rowsPerThread + (task->threadIndex < remainderRows ? 1 : 0);

        for (std::size_t row = startRow; row < endRow; row++) {
            for (int col = 0; col < colCount; col++) {
                task->neighbors[(row * static_cast<std::size_t>(colCount)) +
                                static_cast<std::size_t>(col)] =
                    task->grid->downstreamCell(static_cast<int>(row), col);
            }
        }
    }

    // Barrier 1: wait for all neighbors to be computed.
    pthread_barrier_wait(task->barrier);

    // Pass 2a: Parallel Union-Find.
    {
        const std::size_t cellsPerThread = totalCells / task->threadCount;
        const std::size_t remainderCells = totalCells % task->threadCount;
        const std::size_t startCell =
            task->threadIndex * cellsPerThread +
            std::min(static_cast<std::size_t>(task->threadIndex), remainderCells);
        const std::size_t endCell =
            startCell + cellsPerThread + (task->threadIndex < remainderCells ? 1 : 0);

        for (std::size_t i = startCell; i < endCell; ++i) {
            const std::size_t destIndex =
                task->grid->coordsToIndex(task->neighbors[i].row, task->neighbors[i].col);
            task->grid->unite(i, destIndex);
        }
    }

    // Barrier 2: wait for all unite operations to complete.
    pthread_barrier_wait(task->barrier);

    // Pass 2b part 1: Compute roots for all cells in parallel.
    {
        const std::size_t cellsPerThread = totalCells / task->threadCount;
        const std::size_t remainderCells = totalCells % task->threadCount;
        const std::size_t startCell =
            task->threadIndex * cellsPerThread +
            std::min(static_cast<std::size_t>(task->threadIndex), remainderCells);
        const std::size_t endCell =
            startCell + cellsPerThread + (task->threadIndex < remainderCells ? 1 : 0);

        for (std::size_t i = startCell; i < endCell; ++i)
            (*task->roots)[i] = task->grid->findRoot(i);
    }

    // Barrier 3: wait for all roots to be computed.
    pthread_barrier_wait(task->barrier);

    // Pass 2b part 2: Thread 0 groups indices by root via counting sort (O(n)).
    if (task->threadIndex == 0)
        *task->indices = StreamlineSolver::sortCellsByRoot(*task->roots, totalCells);

    // Barrier 4: wait for sorting to complete.
    pthread_barrier_wait(task->barrier);

    // Pass 2b part 3: Build paths for this thread's range.
    {
        const std::size_t segmentsPerThread = totalCells / task->threadCount;
        const std::size_t remainderSegments = totalCells % task->threadCount;
        const std::size_t startIdx =
            task->threadIndex * segmentsPerThread +
            std::min(static_cast<std::size_t>(task->threadIndex), remainderSegments);
        const std::size_t endIdx =
            startIdx + segmentsPerThread + (task->threadIndex < remainderSegments ? 1 : 0);

        *task->localPaths = StreamlineSolver::buildPathsForRange(
            *task->roots, *task->indices, totalCells, colCount, startIdx, endIdx,
            static_cast<std::size_t>(task->threadIndex));
    }

    return nullptr;
}

} // namespace

PthreadsStreamlineSolver::PthreadsStreamlineSolver(unsigned int threadCount)
    : threadCount_(threadCount) {}

void PthreadsStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    if (threadCount_ == 0) {
        SequentialStreamlineSolver fallback;
        fallback.computeTimeStep(grid);
        return;
    }

    const std::size_t rowCount = grid.rows();
    const int colCount = static_cast<int>(grid.cols());
    const std::size_t totalCells = rowCount * static_cast<std::size_t>(colCount);

    std::vector<Field::GridCell> neighbors(totalCells);
    std::vector<std::size_t> roots(totalCells);
    std::vector<std::size_t> indices;
    std::vector<pthread_t> threads(threadCount_);
    std::vector<ThreadArgs> threadArgs(threadCount_);
    std::vector<std::vector<Field::Path>> localPathsCollection(threadCount_);

    pthread_barrier_t barrier;
    if (pthread_barrier_init(&barrier, nullptr, threadCount_) != 0)
        throw std::runtime_error("pthread_barrier_init failed");

    for (unsigned int i = 0; i < threadCount_; i++) {
        threadArgs[i] = {&grid,        neighbors.data(), &roots,
                         &indices,     colCount,         i,
                         threadCount_, &barrier,         &localPathsCollection[i]};
        if (pthread_create(&threads[i], nullptr, workerFunc, &threadArgs[i]) != 0) {
            for (unsigned int j = 0; j < i; j++) {
                pthread_cancel(threads[j]);
                pthread_join(threads[j], nullptr);
            }
            pthread_barrier_destroy(&barrier);
            throw std::runtime_error("pthread_create failed");
        }
    }

    for (unsigned int i = 0; i < threadCount_; i++)
        pthread_join(threads[i], nullptr);

    pthread_barrier_destroy(&barrier);

    // Final merge of paths from all threads.
    std::vector<Field::Path> finalPaths;
    std::size_t totalPaths = 0;
    for (const auto& local : localPathsCollection)
        totalPaths += local.size();
    finalPaths.reserve(totalPaths);
    for (auto& local : localPathsCollection)
        finalPaths.insert(finalPaths.end(), std::make_move_iterator(local.begin()),
                          std::make_move_iterator(local.end()));

    grid.setPrecomputedStreamlines(std::move(finalPaths));
}
