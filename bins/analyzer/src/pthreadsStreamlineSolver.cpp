#include "pthreadsStreamlineSolver.hpp"

#include "sequentialStreamlineSolver.hpp"
#include "streamlineSolver.hpp"

#include <pthread.h>
#include <stdexcept>
#include <vector>

namespace {

// ---- Round 1: neighbor computation -----------------------------------------

struct Pass1Args {
    Field::Grid* grid;
    Field::GridCell* neighbors;
    int colCount;
    unsigned int threadIndex;
    unsigned int threadCount;
};

void* pass1Worker(void* arg) {
    auto* task = static_cast<Pass1Args*>(arg);
    const std::size_t rowCount = task->grid->rows();
    const int colCount = task->colCount;

    const std::size_t rowsPerThread = rowCount / task->threadCount;
    const std::size_t remainderRows = rowCount % task->threadCount;
    const std::size_t startRow =
        task->threadIndex * rowsPerThread +
        std::min(static_cast<std::size_t>(task->threadIndex), remainderRows);
    const std::size_t endRow =
        startRow + rowsPerThread + (task->threadIndex < remainderRows ? 1 : 0);

    for (std::size_t row = startRow; row < endRow; ++row) {
        for (int col = 0; col < colCount; ++col) {
            task->neighbors[(row * static_cast<std::size_t>(colCount)) +
                            static_cast<std::size_t>(col)] =
                task->grid->downstreamCell(static_cast<int>(row), col);
        }
    }
    return nullptr;
}

// ---- Round 2: findRoot + sort + build --------------------------------------

struct Pass35Args {
    Field::Grid* grid;
    std::vector<std::size_t>* roots;   // shared, written by all threads in pass 3
    std::vector<std::size_t>* indices; // shared, written by thread 0 in pass 4
    int colCount;
    std::size_t totalCells;
    unsigned int threadIndex;
    unsigned int threadCount;
    pthread_barrier_t* barrier;
    std::vector<Field::Path>* localPaths;
};

void* pass35Worker(void* arg) {
    auto* task = static_cast<Pass35Args*>(arg);
    const std::size_t totalCells = task->totalCells;

    // Pass 3: parallel findRoot (fast because sequential unite kept paths short).
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

    // Barrier 1: all roots computed before thread 0 sorts.
    pthread_barrier_wait(task->barrier);

    // Pass 4: thread 0 does counting sort while others wait at barrier 2.
    if (task->threadIndex == 0)
        *task->indices = StreamlineSolver::sortCellsByRoot(*task->roots, totalCells);

    // Barrier 2: sort complete, all threads can now read indices.
    pthread_barrier_wait(task->barrier);

    // Pass 5: parallel path building.
    {
        const std::size_t segmentsPerThread = totalCells / task->threadCount;
        const std::size_t remainderSegments = totalCells % task->threadCount;
        const std::size_t startIdx =
            task->threadIndex * segmentsPerThread +
            std::min(static_cast<std::size_t>(task->threadIndex), remainderSegments);
        const std::size_t endIdx =
            startIdx + segmentsPerThread + (task->threadIndex < remainderSegments ? 1 : 0);

        *task->localPaths = StreamlineSolver::buildPathsForRange(
            *task->roots, *task->indices, totalCells, task->colCount, startIdx, endIdx,
            static_cast<std::size_t>(task->threadIndex));
    }

    return nullptr;
}

void joinAll(std::vector<pthread_t>& threads, unsigned int count) {
    for (unsigned int i = 0; i < count; ++i)
        pthread_join(threads[i], nullptr);
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

    // Round 1: parallel neighbor computation.
    {
        std::vector<Pass1Args> args(threadCount_);
        for (unsigned int i = 0; i < threadCount_; ++i) {
            args[i] = {&grid, neighbors.data(), colCount, i, threadCount_};
            if (pthread_create(&threads[i], nullptr, pass1Worker, &args[i]) != 0) {
                for (unsigned int j = 0; j < i; ++j) {
                    pthread_cancel(threads[j]);
                    pthread_join(threads[j], nullptr);
                }
                throw std::runtime_error("pthread_create failed (pass 1)");
            }
        }
        joinAll(threads, threadCount_);
    }

    // Sequential unite: better DSU quality than parallel CAS -- keeps paths
    // short so the subsequent parallel findRoot is O(n*alpha(n)/P) not O(n*log n/P).
    for (std::size_t i = 0; i < totalCells; ++i)
        grid.unite(i, grid.coordsToIndex(neighbors[i].row, neighbors[i].col));

    // Round 2: parallel findRoot -> serial sort (thread 0) -> parallel build.
    {
        pthread_barrier_t barrier;
        if (pthread_barrier_init(&barrier, nullptr, threadCount_) != 0)
            throw std::runtime_error("pthread_barrier_init failed");

        std::vector<std::vector<Field::Path>> localPathsCollection(threadCount_);
        std::vector<Pass35Args> args(threadCount_);

        for (unsigned int i = 0; i < threadCount_; ++i) {
            args[i] = {&grid,        &roots,     &indices,
                       colCount,     totalCells, i,
                       threadCount_, &barrier,   &localPathsCollection[i]};
            if (pthread_create(&threads[i], nullptr, pass35Worker, &args[i]) != 0) {
                for (unsigned int j = 0; j < i; ++j) {
                    pthread_cancel(threads[j]);
                    pthread_join(threads[j], nullptr);
                }
                pthread_barrier_destroy(&barrier);
                throw std::runtime_error("pthread_create failed (pass 3-5)");
            }
        }
        joinAll(threads, threadCount_);
        pthread_barrier_destroy(&barrier);

        // Merge paths from all threads.
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
}
