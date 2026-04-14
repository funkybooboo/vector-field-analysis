#include "configParser.hpp"
#include "fieldGenerator.hpp"
#include "fieldWriter.hpp"
#include "formatBytes.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

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
                  << "Grid:    " << config.grid.width << " x " << config.grid.height << "  |  x ["
                  << config.bounds.xMin << ", " << config.bounds.xMax << "]"
                  << "  y [" << config.bounds.yMin << ", " << config.bounds.yMax << "]\n"
                  << "Steps:   " << config.steps << "  (dt=" << config.dt
                  << ", viscosity=" << config.viscosity << ")\n"
                  << "Layers:  ";
        for (std::size_t layerIndex = 0; layerIndex < config.layers.size(); ++layerIndex) {
            if (layerIndex > 0) {
                std::cout << "  ";
            }
            std::cout << toString(config.layers[layerIndex].type) << "(s=" << config.layers[layerIndex].strength
                      << ")";
        }
        std::cout << "\n\n";

        std::cout << "Generating..." << std::flush;
        const auto startTime = std::chrono::steady_clock::now();
        const Field::TimeSeries field = FieldGenerator::generateTimeSeries(config);
        const double ms =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime)
                .count();
        std::cout << " done in " << ms << " ms\n";

        std::string typeLabel;
        for (const auto& layer : config.layers) {
            if (!typeLabel.empty()) {
                typeLabel += '+';
            }
            typeLabel += toString(layer.type);
        }
        FieldWriter::write(config.output, field, typeLabel, config.dt, config.viscosity);

        std::error_code err;
        const auto bytes = std::filesystem::file_size(config.output, err);
        std::cout << "Wrote " << config.output;
        if (!err) {
            std::cout << "  (" << Utils::formatBytes(bytes) << ")";
        }
        std::cout << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
