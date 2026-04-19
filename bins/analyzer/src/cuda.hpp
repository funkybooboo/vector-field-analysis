#pragma once

#include "fieldTypes.hpp"

#include <vector>

namespace cuda {

// GPU-native result for one analyzed field slice
//   flattened destination index for cell idx
// componentId[idx]:
//   dense connected-component / streamline label for cell idx
struct Result {
    int rows = 0;
    int cols = 0;
    std::vector<int> successor;
    std::vector<int> componentId;
};

// Runs the union-find on the GPU over a pre-computed CPU successor array.
// Caller must supply successor[i] = downstream cell index for cell i,
// computed via Grid::downstreamCell so it matches the sequential reference.
Result computeComponents(const std::vector<int>& successor, int rows, int cols,
                         unsigned int blockSize = 256);

// Reconstructs deterministic host-side paths from the GPU-native result
std::vector<Field::Path> reconstructPaths(const Result& result);

} // namespace cuda
