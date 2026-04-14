#pragma once
#include "fieldTypes.hpp"

#include <highfive/highfive.hpp>
#include <vector>

// Internal — not part of the public API.
// [frame][row][col] layout for HDF5 read/write operations.
using RawFieldData = std::vector<std::vector<std::vector<float>>>;

// Writes the 6 standard geometry attributes (width, height, xMin, xMax, yMin, yMax)
// onto a HighFive Group. Used by both fieldWriter and streamWriter to avoid duplication.
inline void writeGeometryAttributes(HighFive::Group& group,
                                    const Field::Bounds& bounds,
                                    const Field::GridSize& grid) {
    group.createAttribute("width",  grid.width);
    group.createAttribute("height", grid.height);
    group.createAttribute("xMin",   bounds.xMin);
    group.createAttribute("xMax",   bounds.xMax);
    group.createAttribute("yMin",   bounds.yMin);
    group.createAttribute("yMax",   bounds.yMax);
}
