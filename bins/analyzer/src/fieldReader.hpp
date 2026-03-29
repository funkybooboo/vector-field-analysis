#pragma once
#include "vector.hpp"

#include <string>
#include <vector>

namespace FieldReader {

struct FieldTimeSeries {
    std::vector<std::vector<std::vector<Vector::Vec2>>> steps; // [step][row][col]
    float xMin = 0.0f;
    float xMax = 0.0f;
    float yMin = 0.0f;
    float yMax = 0.0f;
};

FieldTimeSeries read(const std::string& path);

} // namespace FieldReader
