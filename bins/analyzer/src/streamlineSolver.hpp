#pragma once
#include "grid.hpp"

#include <cstddef>
#include <vector>

class StreamlineSolver {
  public:
    virtual ~StreamlineSolver() = default;

    StreamlineSolver() = default;
    StreamlineSolver(const StreamlineSolver&) = delete;
    StreamlineSolver& operator=(const StreamlineSolver&) = delete;
    StreamlineSolver(StreamlineSolver&&) = delete;
    StreamlineSolver& operator=(StreamlineSolver&&) = delete;

    virtual void computeTimeStep(Field::Grid& grid) = 0;

  protected:
    static std::vector<Field::Path>
    reconstructPathsDSU(const Field::Grid& grid, const std::vector<Field::GridCell>& neighbors);
};
