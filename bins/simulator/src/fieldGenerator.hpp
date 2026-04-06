#pragma once
#include "simulatorConfig.hpp"

#include <vector>

namespace FieldGenerator {

// Three-dimensional vector field: vx[steps][height][width], vy[steps][height][width]
using Field3D = std::vector<std::vector<std::vector<float>>>;

struct FieldTimeSeries {
    Field3D vx;
    Field3D vy;
};

FieldTimeSeries generateTimeSeries(const SimulatorConfig& config);

} // namespace FieldGenerator
