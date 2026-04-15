#pragma once
#include "streamlineSolver.hpp"

class OpenMpStreamlineSolver : public StreamlineSolver {
  public:
    explicit OpenMpStreamlineSolver(unsigned int threadCount = 0);
    void computeTimeStep(Field::Grid& grid) override;

  private:
#ifdef _OPENMP
    unsigned int threadCount_;
#endif
};
