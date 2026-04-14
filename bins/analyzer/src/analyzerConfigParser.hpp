#pragma once
#include "analyzerConfig.hpp"

#include <string>

namespace ConfigParser {

AnalyzerConfig parseFile(const std::string& path);

} // namespace ConfigParser
