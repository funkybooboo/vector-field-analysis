#pragma once
#include "streamlineSolver.hpp"

class PthreadsStreamlineSolver : public StreamlineSolver {
    unsigned int threadCount_;

  public:
    explicit PthreadsStreamlineSolver(unsigned int threadCount);

    void computeTimeStep(Field::Grid& grid) override;
};
