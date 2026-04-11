#pragma once
#include "simulatorConfig.hpp"
#include "vector.hpp"

namespace FieldWriter {

void write(const Vector::FieldTimeSeries& field, const SimulatorConfig& config);

} // namespace FieldWriter
