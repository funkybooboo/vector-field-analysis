#pragma once

#include "streamlineSolver.hpp"

class CudaFullStreamlineSolver : public StreamlineSolver {
  public:
    explicit CudaFullStreamlineSolver(unsigned int blockSize = 256)
        : blockSize_(blockSize) {}
    void computeTimeStep(Field::Grid& grid) override;

  private:
    unsigned int blockSize_;
};
