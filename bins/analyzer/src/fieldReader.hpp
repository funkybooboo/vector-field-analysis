#pragma once
#include "vector.hpp"

#include <string>
#include <vector>

namespace FieldReader {

// Independent time snapshots of the vector field read from an HDF5 file.
// Each step is a complete grid, not an ODE integration result. Layout [step][row][col]
// matches the axis order of the "vx"/"vy" HDF5 datasets.
// xMin/xMax/yMin/yMax are physical-space bounds, not grid indices.
struct FieldTimeSeries {
    std::vector<std::vector<std::vector<Vector::Vec2>>> steps; // [step][row][col]
    float xMin = 0.0f;
    float xMax = 0.0f;
    float yMin = 0.0f;
    float yMax = 0.0f;
};

FieldTimeSeries read(const std::string& path);

} // namespace FieldReader
