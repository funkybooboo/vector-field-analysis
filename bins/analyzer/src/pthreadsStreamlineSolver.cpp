#include "pthreadsStreamlineSolver.hpp"

#include <pthread.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

struct ThreadArgs {
    Field::Grid* grid;
    Field::GridCell* neighbors;
    std::size_t* roots;
    std::size_t* indices;
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
        const std::size_t startRow = task->threadIndex * rowsPerThread + std::min(static_cast<std::size_t>(task->threadIndex), remainderRows);
        const std::size_t endRow = startRow + rowsPerThread + (task->threadIndex < remainderRows ? 1 : 0);

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
        const std::size_t startCell = task->threadIndex * cellsPerThread + std::min(static_cast<std::size_t>(task->threadIndex), remainderCells);
        const std::size_t endCell = startCell + cellsPerThread + (task->threadIndex < remainderCells ? 1 : 0);

        for (std::size_t i = startCell; i < endCell; ++i) {
            const std::size_t srcIndex = i;
            const std::size_t destIndex =
                task->grid->coordsToIndex(task->neighbors[i].row, task->neighbors[i].col);
            task->grid->unite(srcIndex, destIndex);
        }
    }

    // Barrier 2: wait for all unite operations to complete.
    pthread_barrier_wait(task->barrier);

    // Pass 2b part 1: Compute roots for all cells in parallel.
    {
        const std::size_t cellsPerThread = totalCells / task->threadCount;
        const std::size_t remainderCells = totalCells % task->threadCount;
        const std::size_t startCell = task->threadIndex * cellsPerThread + std::min(static_cast<std::size_t>(task->threadIndex), remainderCells);
        const std::size_t endCell = startCell + cellsPerThread + (task->threadIndex < remainderCells ? 1 : 0);

        for (std::size_t i = startCell; i < endCell; ++i) {
            task->roots[i] = task->grid->findRoot(i);
        }
    }

    // Barrier 3: wait for all roots to be computed.
    pthread_barrier_wait(task->barrier);

    // Pass 2b part 2: Thread 0 sorts indices by root.
    // (A parallel sort would be better, but std::sort is O(N log N) and already fast).
    if (task->threadIndex == 0) {
        std::iota(task->indices, task->indices + totalCells, 0);
        std::sort(task->indices, task->indices + totalCells, [&](std::size_t a, std::size_t b) {
            return task->roots[a] < task->roots[b];
        });
    }

    // Barrier 4: wait for sorting to complete.
    pthread_barrier_wait(task->barrier);

    // Pass 2b part 3: Create paths from segments in parallel.
    {
        // Each thread identifies segments within its range of the sorted indices.
        const std::size_t segmentsPerThread = totalCells / task->threadCount;
        const std::size_t remainderSegments = totalCells % task->threadCount;
        const std::size_t startIdx = task->threadIndex * segmentsPerThread + std::min(static_cast<std::size_t>(task->threadIndex), remainderSegments);
        const std::size_t endIdx = startIdx + segmentsPerThread + (task->threadIndex < remainderSegments ? 1 : 0);

        if (startIdx < endIdx) {
            std::size_t currentStart = startIdx;
            
            // Adjust currentStart so we don't start in the middle of a segment 
            // owned by the previous thread, unless we are the first thread.
            if (task->threadIndex > 0) {
                while (currentStart < endIdx && task->roots[task->indices[currentStart]] == task->roots[task->indices[currentStart - 1]]) {
                    currentStart++;
                }
            }

            while (currentStart < endIdx) {
                std::size_t segmentEnd = currentStart + 1;
                while (segmentEnd < totalCells && task->roots[task->indices[segmentEnd]] == task->roots[task->indices[currentStart]]) {
                    segmentEnd++;
                }

                // This thread owns the segment starting at currentStart.
                Field::Path path;
                for (std::size_t j = currentStart; j < segmentEnd; ++j) {
                    std::size_t idx = task->indices[j];
                    path.push_back({static_cast<int>(idx / colCount), static_cast<int>(idx % colCount)});
                }
                task->localPaths->push_back(std::move(path));

                currentStart = segmentEnd;
            }
        }
    }

    return nullptr;
}

} // namespace

PthreadsStreamlineSolver::PthreadsStreamlineSolver(unsigned int threadCount)
    : threadCount_(threadCount) {}

void PthreadsStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    if (threadCount_ == 0) return;

    const std::size_t rowCount = grid.rows();
    if (rowCount == 0) return;
    const int colCount = static_cast<int>(grid.cols());
    const std::size_t totalCells = rowCount * static_cast<std::size_t>(colCount);

    std::vector<Field::GridCell> neighbors(totalCells);
    std::vector<std::size_t> roots(totalCells);
    std::vector<std::size_t> indices(totalCells);
    std::vector<pthread_t> threads(threadCount_);
    std::vector<ThreadArgs> threadArgs(threadCount_);
    std::vector<std::vector<Field::Path>> localPathsCollection(threadCount_);

    pthread_barrier_t barrier;
    if (pthread_barrier_init(&barrier, nullptr, threadCount_) != 0) {
        throw std::runtime_error("pthread_barrier_init failed");
    }

    for (unsigned int i = 0; i < threadCount_; i++) {
        threadArgs[i] = {&grid, neighbors.data(), roots.data(), indices.data(), colCount, i, threadCount_, &barrier, &localPathsCollection[i]};
        if (pthread_create(&threads[i], nullptr, workerFunc, &threadArgs[i]) != 0) {
            // Clean up already created threads
            for (unsigned int j = 0; j < i; j++) {
                pthread_cancel(threads[j]);
                pthread_join(threads[j], nullptr);
            }
            pthread_barrier_destroy(&barrier);
            throw std::runtime_error("pthread_create failed");
        }
    }

    for (unsigned int i = 0; i < threadCount_; i++) {
        pthread_join(threads[i], nullptr);
    }

    pthread_barrier_destroy(&barrier);

    // Final merge of paths from all threads.
    std::vector<Field::Path> finalPaths;
    for (auto& local : localPathsCollection) {
        finalPaths.insert(finalPaths.end(), std::make_move_iterator(local.begin()), std::make_move_iterator(local.end()));
    }
    grid.setPrecomputedStreamlines(std::move(finalPaths));
}

