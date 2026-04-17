#pragma once

#include "streamlineSolver.hpp"

class CudaStreamlineSolver : public StreamlineSolver {
  public:
    explicit CudaStreamlineSolver(unsigned int blockSize = 256)
        : blockSize_(blockSize) {}
    void computeTimeStep(Field::Grid& grid) override;

  private:
    unsigned int blockSize_;
};
