#include "configParser.hpp"
#include "fieldGenerator.hpp"
#include "fieldWriter.hpp"

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
        const FieldGenerator::FieldTimeSeries field = FieldGenerator::generateTimeSeries(config);
        FieldWriter::write(field, config);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
