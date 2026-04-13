#pragma once
#include "vector.hpp"
#include <string>
#include <vector>

namespace StreamWriter {

// StepStreamlines: all unique streamlines for one time step.
using StepStreamlines = std::vector<Vector::Path>;

// Writes all-steps streamline data to an HDF5 file at outPath.
//
// File layout:
//   streams/                     (group)
//     attrs: xMin, xMax, yMin, yMax, width, height, num_steps
//     step_0/                    (group per time step)
//       paths_flat  int32[N, 2]  all (row, col) pairs concatenated
//       offsets     int32[S+1]   CSR: streamline i spans [offsets[i], offsets[i+1])
//     step_1/ ...
//
// Python usage:
//   flat = f["streams/step_0/paths_flat"][:]   # (N, 2)
//   off  = f["streams/step_0/offsets"][:]      # (S+1,)
//   path = flat[off[i]:off[i+1]]              # (len, 2) row, col
void write(const std::string& outPath, const std::vector<StepStreamlines>& streamlinesByStep,
           const Vector::FieldBounds& bounds, const Vector::GridSize& grid);

} // namespace StreamWriter
