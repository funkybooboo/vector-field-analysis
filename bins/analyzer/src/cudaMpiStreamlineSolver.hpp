#pragma once

#include "streamlineSolver.hpp"

class CudaMpiStreamlineSolver : public StreamlineSolver {
  public:
    explicit CudaMpiStreamlineSolver(unsigned int blockSize = 256)
        : blockSize_(blockSize) {}

    void computeTimeStep(Field::Grid& grid) override;

  private:
    unsigned int blockSize_;
};
