#include "analyzerConfigParser.hpp"

#include "toml_include.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace ConfigParser {

namespace {

void validateSolver(const std::string& name) {
    if (std::any_of(kValidSolvers.begin(), kValidSolvers.end(),
                    [&name](std::string_view candidate) { return name == candidate; })) {
        return;
    }
    // Build the valid-names list from kValidSolvers so it stays in sync automatically.
    std::string valid;
    for (const auto& solverName : kValidSolvers) {
        if (!valid.empty()) {
            valid += ", ";
        }
        valid += std::string(solverName);
    }
    throw std::runtime_error("Unknown solver: \"" + name + "\". Must be one of: " + valid + ".");
}

} // namespace

AnalyzerConfig parseFile(const std::string& path) {
    const toml::table table = toml::parse_file(path);
    AnalyzerConfig config;

    // Struct defaults in analyzerConfig.hpp are the single source of truth.
    // The parser only assigns a field when the key is explicitly present in the TOML.
    const auto* analyzer = table["analyzer"].as_table();
    if (analyzer == nullptr) {
        return config;
    }

    if (const auto inputPath = (*analyzer)["input"].value<std::string>()) {
        config.inputPath = *inputPath;
    }
    if (const auto outputPath = (*analyzer)["output"].value<std::string>()) {
        config.outputPath = *outputPath;
    }
    if (const auto solverName = (*analyzer)["solver"].value<std::string>()) {
        validateSolver(*solverName);
        config.solver = *solverName;
    }
    if (const auto threadCount = (*analyzer)["threads"].value<int64_t>()) {
        if (*threadCount < 0 ||
            *threadCount > static_cast<int64_t>(std::numeric_limits<unsigned int>::max())) {
            throw std::runtime_error("threads must be between 0 and " +
                                     std::to_string(std::numeric_limits<unsigned int>::max()));
        }
        config.threadCount = static_cast<unsigned int>(*threadCount);
    }

    if (config.inputPath.empty()) {
        throw std::runtime_error("analyzer.input must not be empty");
    }

    return config;
}

} // namespace ConfigParser
