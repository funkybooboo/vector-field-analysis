#include "configParser.hpp"

#include "toml_include.hpp" // NOLINT(misc-include-cleaner) — wrapper sets TOML_EXCEPTIONS

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

AnalyzerConfig parseAnalyzer(const std::string& path) {
    const toml::table table = toml::parse_file(path);
    AnalyzerConfig config;

    // Struct defaults in analyzerConfig.hpp are the single source of truth.
    // The parser only assigns a field when the key is explicitly present in the TOML.
    const auto* analyzer = table["analyzer"].as_table();
    if (analyzer == nullptr) {
        return config;
    }

    if (const auto solverName = (*analyzer)["solver"].value<std::string>()) {
        validateSolver(*solverName);
        config.solver = *solverName;
    }
    {
        const auto& threadsNode = (*analyzer)["threads"];
        if (threadsNode) {
            // Reject non-integer types (e.g. threads = 4.5) rather than silently ignoring them.
            if (threadsNode.type() != toml::node_type::integer) {
                throw std::runtime_error("threads must be an integer");
            }
            const auto threadCount = threadsNode.value<int64_t>();
            if (!threadCount.has_value()) {
                throw std::runtime_error("threads must be an integer");
            }
            if (*threadCount < 0 ||
                *threadCount > static_cast<int64_t>(std::numeric_limits<unsigned int>::max())) {
                throw std::runtime_error("threads must be between 0 and " +
                                         std::to_string(std::numeric_limits<unsigned int>::max()));
            }
            config.threadCount = static_cast<unsigned int>(*threadCount);
        }
    }

    return config;
}

} // namespace ConfigParser
