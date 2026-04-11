#pragma once
#include "simulatorConfig.hpp"
#include "vector.hpp"

namespace FieldGenerator {

Vector::FieldTimeSeries generateTimeSeries(const SimulatorConfig& config);

} // namespace FieldGenerator
