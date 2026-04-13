#pragma once
#include "streamlineSolver.hpp"

class OpenMP : public StreamlineSolver {
  public:
    explicit OpenMP(unsigned int threadCount = 0);
    void computeTimeStep(VectorField::FieldGrid& grid) override;

  private:
#ifdef _OPENMP
    unsigned int threadCount_;
#endif
};
