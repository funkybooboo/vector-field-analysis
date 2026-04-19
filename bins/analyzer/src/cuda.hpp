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

// Computes streamline connectivity on the GPU.

// does NOT build Field::Grid streamlines directly
// Instead, it returns a GPU-native graph representation that can later be
// reconstructed into Field::Path objects
Result computeComponents(const Field::Slice& field, const Field::Bounds& bounds,
                         unsigned int blockSize = 256);

// Reconstructs deterministic host-side paths from the GPU-native result
std::vector<Field::Path> reconstructPaths(const Result& result);

} // namespace cuda
