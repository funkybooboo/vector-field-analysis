#include "configParser.hpp"
#include "fieldGenerator.hpp"
#include "fieldWriter.hpp"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

static std::string formatBytes(std::uintmax_t bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    const auto d = static_cast<double>(bytes);
    if (bytes >= 1024ULL * 1024 * 1024) {
        oss << (d / (1024.0 * 1024 * 1024)) << " GB";
    } else if (bytes >= 1024ULL * 1024) {
        oss << (d / (1024.0 * 1024)) << " MB";
    } else if (bytes >= 1024ULL) {
        oss << (d / 1024.0) << " KB";
    } else {
        oss << std::defaultfloat << bytes << " B";
    }
    return oss.str();
}

int main(int argc, char* argv[]) {
    if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        std::cout << "Usage: simulator <config.toml>\n"
                  << "\nRuns a vector field simulation using the given TOML config file.\n"
                  << "The output path is set via the 'output' key in the config.\n"
                  << "See bins/simulator/configs/ for example configs.\n";
        return 0;
    }
    if (argc < 2) {
        std::cerr << "Usage: simulator <config.toml>\n";
        return 1;
    }
    try {
        const std::string configPath = argv[1];
        const SimulatorConfig config = ConfigParser::parseFile(configPath);

        std::cout << "Config:  " << configPath << "\n"
                  << "Grid:    " << config.grid.width << " x " << config.grid.height
                  << "  |  x [" << config.bounds.xMin << ", " << config.bounds.xMax << "]"
                  << "  y [" << config.bounds.yMin << ", " << config.bounds.yMax << "]\n"
                  << "Steps:   " << config.steps
                  << "  (dt=" << config.dt << ", viscosity=" << config.viscosity << ")\n"
                  << "Layers:  ";
        for (std::size_t i = 0; i < config.layers.size(); ++i) {
            if (i > 0) {
                std::cout << "  ";
            }
            std::cout << toString(config.layers[i].type)
                      << "(s=" << config.layers[i].strength << ")";
        }
        std::cout << "\n\n";

        std::cout << "Generating..." << std::flush;
        const auto t0 = std::chrono::steady_clock::now();
        const Vector::FieldTimeSeries field = FieldGenerator::generateTimeSeries(config);
        const double ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0)
                .count();
        std::cout << " done in " << ms << " ms\n";

        FieldWriter::write(field, config);

        std::error_code ec;
        const auto bytes = std::filesystem::file_size(config.output, ec);
        std::cout << "Wrote " << config.output;
        if (!ec) {
            std::cout << "  (" << formatBytes(bytes) << ")";
        }
        std::cout << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
