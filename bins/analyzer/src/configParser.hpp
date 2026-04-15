#pragma once
#include "analyzerConfig.hpp"

#include <string>

namespace ConfigParser {

// Parse the [analyzer] section from a TOML config file.
// The [analyzer] section is optional; struct defaults apply when absent.
AnalyzerConfig parseAnalyzer(const std::string& path);

} // namespace ConfigParser
