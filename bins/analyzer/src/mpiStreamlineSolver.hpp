#pragma once
#include "streamlineSolver.hpp"

#include <vector>

// Distributed memory CPU implementation using MPI.
// When compiled without USE_MPI, falls back to SequentialStreamlineSolver.
// In a multi-rank run all ranks must call computeTimeStep together;
// only rank 0's Field::Grid will contain the merged streamlines after the call.
class MpiStreamlineSolver : public StreamlineSolver {
  public:
    void computeTimeStep(Field::Grid& grid) override;

  private:
    std::vector<int> localNext_; // downstream cell index per local cell (1 int/cell)
    std::vector<int> allNext_;   // gathered on rank 0: downstream index for every cell
};
