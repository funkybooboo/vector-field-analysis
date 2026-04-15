#pragma once
#include "fieldTypes.hpp"
#include "simulatorConfig.hpp"

namespace FieldGenerator {

Field::TimeSeries generateTimeSeries(const SimulatorConfig& config);

} // namespace FieldGenerator
