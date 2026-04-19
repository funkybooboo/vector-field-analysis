#pragma once
#include "fieldTypes.hpp"
#include "simulatorConfig.hpp"

#include <cstddef>
#include <functional>

namespace FieldGenerator {

// Streaming variant: calls onFrame(stepIndex, slice) for each frame in order.
// Holds only one frame in memory at a time -- use this for large grids.
void generateTimeSeries(const SimulatorConfig& config,
                        std::function<void(std::size_t, const Field::Slice&)> onFrame);

// Convenience wrapper that collects all frames into a TimeSeries.
// Allocates the full series in memory; only suitable for small grids.
Field::TimeSeries generateTimeSeries(const SimulatorConfig& config);

} // namespace FieldGenerator
