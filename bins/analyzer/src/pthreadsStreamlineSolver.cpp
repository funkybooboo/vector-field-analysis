#include "pthreadsStreamlineSolver.hpp"

#include "sequentialStreamlineSolver.hpp"

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

} // namespace

// Worker thread: sleeps until a new generation of work is dispatched, processes
// its assigned row range, then decrements the pending counter.
void* PthreadsStreamlineSolver::workerLoop(void* arg) {
    auto* ctx = static_cast<ThreadCtx*>(arg);
    PthreadsStreamlineSolver* self = ctx->solver;
    const unsigned int idx = ctx->index;
    delete ctx;

    unsigned int lastGen = 0;

    pthread_mutex_lock(&self->mutex_);
    while (true) {
        while (self->workGeneration_ == lastGen && !self->shutdown_) {
            pthread_cond_wait(&self->workReady_, &self->mutex_);
        }
        if (self->shutdown_) {
            pthread_mutex_unlock(&self->mutex_);
            return nullptr;
        }
        lastGen = self->workGeneration_;
        const WorkItem item = self->work_[idx];
        pthread_mutex_unlock(&self->mutex_);

        for (std::size_t row = item.startRow; row < item.endRow; ++row) {
            for (int col = 0; col < item.colCount; ++col) {
                item.neighbors[(row * static_cast<std::size_t>(item.colCount)) +
                               static_cast<std::size_t>(col)] =
                    item.grid->downstreamCell(static_cast<int>(row), col);
            }
        }

        pthread_mutex_lock(&self->mutex_);
        if (--self->pendingCount_ == 0) {
            pthread_cond_signal(&self->workDone_);
        }
        // remain locked for the next loop iteration
    }
}

PthreadsStreamlineSolver::PthreadsStreamlineSolver(unsigned int threadCount)
    : threadCount_(threadCount) {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&workReady_, nullptr);
    pthread_cond_init(&workDone_, nullptr);

    if (threadCount_ == 0) {
        return;
    }

    pool_.resize(threadCount_);
    work_.resize(threadCount_);

    for (unsigned int i = 0; i < threadCount_; ++i) {
        auto* ctx = new ThreadCtx{this, i};
        const int err = pthread_create(&pool_[i], nullptr, workerLoop, ctx);
        if (err != 0) {
            // Signal shutdown and join already-created threads before throwing.
            delete ctx;
            pthread_mutex_lock(&mutex_);
            shutdown_ = true;
            pthread_cond_broadcast(&workReady_);
            pthread_mutex_unlock(&mutex_);
            for (unsigned int j = 0; j < i; ++j) {
                pthread_join(pool_[j], nullptr);
            }
            pthread_mutex_destroy(&mutex_);
            pthread_cond_destroy(&workReady_);
            pthread_cond_destroy(&workDone_);
            throw std::runtime_error("pthread_create failed with error code " +
                                     std::to_string(err));
        }
    }
}

PthreadsStreamlineSolver::~PthreadsStreamlineSolver() {
    pthread_mutex_lock(&mutex_);
    shutdown_ = true;
    pthread_cond_broadcast(&workReady_);
    pthread_mutex_unlock(&mutex_);

    for (auto& t : pool_) {
        pthread_join(t, nullptr);
    }

    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&workReady_);
    pthread_cond_destroy(&workDone_);
}

void PthreadsStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    if (threadCount_ == 0) {
        SequentialStreamlineSolver fallback;
        fallback.computeTimeStep(grid);
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
    neighbors_.resize(rowCount * static_cast<std::size_t>(colCount));

    auto [rowsPerThread, remainderRows] = calculateRowSplit(rowCount, threadCount_);

    std::size_t currentRow = 0;
    for (unsigned int i = 0; i < threadCount_; ++i) {
        work_[i].grid = &grid;
        work_[i].neighbors = neighbors_.data();
        work_[i].colCount = colCount;
        work_[i].startRow = currentRow;
        work_[i].endRow = currentRow + rowsPerThread + (i == threadCount_ - 1 ? remainderRows : 0);
        currentRow += rowsPerThread;
    }

    // Pass 1: dispatch neighbor computation to pool and wait.
    pthread_mutex_lock(&mutex_);
    pendingCount_ = threadCount_;
    ++workGeneration_;
    pthread_cond_broadcast(&workReady_);
    while (pendingCount_ > 0) {
        pthread_cond_wait(&workDone_, &mutex_);
    }
    pthread_mutex_unlock(&mutex_);

    // Pass 2: sequential -- apply streamline merges using the precomputed pairs.
    applyNeighborPairs(grid, neighbors_, static_cast<int>(rowCount), colCount);
}
