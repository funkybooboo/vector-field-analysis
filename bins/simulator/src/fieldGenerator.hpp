#pragma once
#include "simulatorConfig.hpp"
#include "fieldTypes.hpp"

namespace FieldGenerator {

Field::TimeSeries generateTimeSeries(const SimulatorConfig& config);

} // namespace FieldGenerator
