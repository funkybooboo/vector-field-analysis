#pragma once
#include "simulatorConfig.hpp"

#include <string>

namespace ConfigParser {

// Parse the [simulation] + [[layers]] sections from a TOML config file.
SimulatorConfig parseSimulation(const std::string& path);

} // namespace ConfigParser
