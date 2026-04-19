#pragma once
#include "fieldTypes.hpp"
#include "streamlineSolver.hpp"

#include <pthread.h>
#include <vector>

class PthreadsStreamlineSolver : public StreamlineSolver {
  public:
    explicit PthreadsStreamlineSolver(unsigned int threadCount);

    void computeTimeStep(Field::Grid& grid) override;

    struct ThreadArgs {
        Field::Grid* grid = nullptr;
        Field::GridCell* neighbors = nullptr;
        int colCount = 0;
        std::size_t startRow = 0;
        std::size_t endRow = 0;
    };

  private:
    static void launchPass(std::vector<pthread_t>& threads, std::vector<ThreadArgs>& args,
                           void* (*worker)(void*), const char* errorMsg);

    unsigned int threadCount_;
};
