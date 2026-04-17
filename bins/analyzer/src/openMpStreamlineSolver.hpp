#pragma once
#include "fieldTypes.hpp"
#include "streamlineSolver.hpp"

#include <vector>

class OpenMpStreamlineSolver : public StreamlineSolver {
  public:
    explicit OpenMpStreamlineSolver(unsigned int threadCount = 0);
    void computeTimeStep(Field::Grid& grid) override;

  private:
#ifdef _OPENMP
    unsigned int threadCount_;
#endif
    std::vector<Field::GridCell> neighbors_;
};
