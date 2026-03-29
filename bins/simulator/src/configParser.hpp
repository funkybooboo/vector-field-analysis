#pragma once
#include "simulatorConfig.hpp"

#include <string>

namespace ConfigParser {

SimulatorConfig parseFile(const std::string& path);

} // namespace ConfigParser
