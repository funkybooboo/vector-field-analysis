#include "analyzerConfigParser.hpp"

#include <limits>
#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner)
#define TOML_EXCEPTIONS 1
#include <toml++/toml.hpp>
// NOLINTEND(misc-include-cleaner)

namespace AnalyzerConfigParser {

namespace {

void validateSolver(const std::string& name) {
    if (name == "sequential" || name == "openmp" || name == "pthreads" || name == "mpi" ||
        name == "all") {
        return;
    }
    throw std::runtime_error("Unknown solver: \"" + name +
                             "\". Must be sequential, openmp, pthreads, mpi, or all.");
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

    if (const auto v = (*analyzer)["input"].value<std::string>()) {
        config.inputPath = *v;
    }
    if (const auto v = (*analyzer)["solver"].value<std::string>()) {
        validateSolver(*v);
        config.solver = *v;
    }
    if (const auto v = (*analyzer)["threads"].value<int64_t>()) {
        if (*v < 0 || *v > static_cast<int64_t>(std::numeric_limits<unsigned int>::max())) {
            throw std::runtime_error("threads must be between 0 and " +
                                     std::to_string(std::numeric_limits<unsigned int>::max()));
        }
        config.threadCount = static_cast<unsigned int>(*v);
    }

    return config;
}

} // namespace AnalyzerConfigParser
