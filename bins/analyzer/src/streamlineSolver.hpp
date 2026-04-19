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

  public:
    // Counting sort: returns indices[0..total-1] grouped so cells sharing a root
    // are contiguous. roots[i] = grid.findRoot(i) must be precomputed by the caller.
    static std::vector<std::size_t> sortCellsByRoot(const std::vector<std::size_t>& roots,
                                                    std::size_t total);

    // Builds paths for all DSU roots whose first sorted position falls in
    // [startIdx, endIdx). If threadIdx > 0, skips any leading partial root
    // that belongs to the previous thread's range.
    static std::vector<Field::Path> buildPathsForRange(const std::vector<std::size_t>& roots,
                                                       const std::vector<std::size_t>& indices,
                                                       std::size_t totalCells, int colCount,
                                                       std::size_t startIdx, std::size_t endIdx,
                                                       std::size_t threadIdx);

  protected:
    // Sequential O(n) path reconstruction using owner-array. Does not call
    // findRoot or require the DSU to be fully converged -- safe to call after
    // unite() passes or directly on the neighbors array alone.
    static std::vector<Field::Path>
    reconstructPathsDSU(const Field::Grid& grid, const std::vector<Field::GridCell>& neighbors);
};
