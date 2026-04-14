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
    // Pass 2 of the two-pass algorithm: apply precomputed neighbor pairs sequentially.
    // traceStreamlineStep writes to shared streamline state and is not thread-safe;
    // callers must ensure this runs on one thread.
    static void applyNeighborPairs(Field::Grid& grid,
                                   const std::vector<Field::GridCell>& neighbors,
                                   int rowCount, int colCount) {
        for (int row = 0; row < rowCount; row++) {
            for (int col = 0; col < colCount; col++) {
                grid.traceStreamlineStep(
                    {row, col},
                    neighbors[(static_cast<std::size_t>(row) *
                                   static_cast<std::size_t>(colCount)) +
                              static_cast<std::size_t>(col)]);
            }
        }
    }
};
