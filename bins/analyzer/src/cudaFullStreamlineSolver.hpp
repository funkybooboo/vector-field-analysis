#pragma once

#include "streamlineSolver.hpp"

class CudaFullStreamlineSolver : public StreamlineSolver {
  public:
    void computeTimeStep(Field::Grid& grid) override;
};
