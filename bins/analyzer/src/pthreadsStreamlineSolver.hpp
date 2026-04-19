#pragma once
#include "fieldTypes.hpp"
#include "streamlineSolver.hpp"

#include <pthread.h>

class PthreadsStreamlineSolver : public StreamlineSolver {
  public:
    explicit PthreadsStreamlineSolver(unsigned int threadCount);

    void computeTimeStep(Field::Grid& grid) override;

  private:
    unsigned int threadCount_;
};
