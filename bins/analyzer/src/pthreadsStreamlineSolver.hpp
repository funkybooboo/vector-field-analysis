#pragma once
#include "fieldTypes.hpp"
#include "streamlineSolver.hpp"

#include <cstdint>
#include <pthread.h>
#include <vector>

class PthreadsStreamlineSolver : public StreamlineSolver {
  public:
    explicit PthreadsStreamlineSolver(unsigned int threadCount);
    ~PthreadsStreamlineSolver() override;

    void computeTimeStep(Field::Grid& grid) override;

  private:
    struct WorkItem {
        Field::Grid* grid = nullptr;
        Field::GridCell* neighbors = nullptr;
        int colCount = 0;
        std::size_t startRow = 0;
        std::size_t endRow = 0;
    };

    struct ThreadCtx {
        PthreadsStreamlineSolver* solver;
        unsigned int index;
    };

    static void* workerLoop(void* arg);

    unsigned int threadCount_;
    std::vector<Field::GridCell> neighbors_;

    std::vector<pthread_t> pool_;
    std::vector<WorkItem> work_;

    pthread_mutex_t mutex_;
    pthread_cond_t workReady_;
    pthread_cond_t workDone_;

    unsigned int workGeneration_ = 0;
    unsigned int pendingCount_ = 0;
    bool shutdown_ = false;
};
