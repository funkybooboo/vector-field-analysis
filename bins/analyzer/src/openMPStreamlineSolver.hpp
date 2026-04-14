#pragma once
#include "streamlineSolver.hpp"

class OpenMPStreamlineSolver : public StreamlineSolver {
  public:
    explicit OpenMPStreamlineSolver(unsigned int threadCount = 0);
    void computeTimeStep(Field::Grid& grid) override;

  private:
#ifdef _OPENMP
    unsigned int threadCount_;
#endif
};
