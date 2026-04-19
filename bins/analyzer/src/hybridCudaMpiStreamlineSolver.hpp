#pragma once

#include "streamlineSolver.hpp"

class HybridCudaMpiStreamlineSolver : public StreamlineSolver {
  public:
    explicit HybridCudaMpiStreamlineSolver(unsigned int blockSize = 256)
        : blockSize_(blockSize) {}

    void computeTimeStep(Field::Grid& grid) override;

  private:
    unsigned int blockSize_;
};
