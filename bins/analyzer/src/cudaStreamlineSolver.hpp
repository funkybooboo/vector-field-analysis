#pragma once

#include "streamlineSolver.hpp"

class CudaStreamlineSolver : public StreamlineSolver {
  public:
    void computeTimeStep(Field::Grid& grid) override;
};
