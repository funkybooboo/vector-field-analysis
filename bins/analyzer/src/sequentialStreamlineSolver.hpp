#pragma once
#include "streamlineSolver.hpp"

class SequentialStreamlineSolver : public StreamlineSolver {
  public:
    void computeTimeStep(Field::Grid& grid) override;
};
