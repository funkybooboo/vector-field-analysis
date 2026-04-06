#pragma once
#include "fieldGenerator.hpp"
#include "simulatorConfig.hpp"

namespace FieldWriter {

void write(const FieldGenerator::FieldTimeSeries& field, const SimulatorConfig& config);

} // namespace FieldWriter
