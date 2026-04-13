#include "fieldWriter.hpp"

#include <highfive/highfive.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace FieldWriter {

void write(const Vector::FieldTimeSeries& field, const SimulatorConfig& config) {
    HighFive::File file(config.output, HighFive::File::Overwrite);
    auto group = file.createGroup("field");

    // HDF5 requires flat numeric arrays; extract x/y components from Vec2.
    using RawFieldData = std::vector<std::vector<std::vector<float>>>; // [step][row][col]
    const std::size_t numSteps = field.steps.size();
    const auto height = static_cast<std::size_t>(config.grid.height);
    const auto width = static_cast<std::size_t>(config.grid.width);
    RawFieldData vx(numSteps, std::vector<std::vector<float>>(height, std::vector<float>(width)));
    RawFieldData vy(numSteps, std::vector<std::vector<float>>(height, std::vector<float>(width)));
    for (std::size_t s = 0; s < numSteps; ++s) {
        for (std::size_t r = 0; r < height; ++r) {
            for (std::size_t c = 0; c < width; ++c) {
                vx[s][r][c] = field.steps[s][r][c].x;
                vy[s][r][c] = field.steps[s][r][c].y;
            }
        }
    }
    group.createDataSet("vx", vx);
    group.createDataSet("vy", vy);

    // Store grid geometry and simulation parameters as HDF5 attributes alongside
    // the data so the file is self-contained -- readers don't need the config file.

    // Summarize all layer types as a '+'-joined label (e.g. "vortex+noise").
    // The format is informational; no reader is expected to parse it back
    // into individual layer types.
    std::string typeLabel;
    for (const auto& layer : config.layers) {
        if (!typeLabel.empty()) {
            typeLabel += '+';
        }
        typeLabel += toString(layer.type);
    }

    group.createAttribute("type", typeLabel);
    group.createAttribute("steps", config.steps);
    group.createAttribute("dt", config.dt);
    group.createAttribute("viscosity", config.viscosity);
    group.createAttribute("width", config.grid.width);
    group.createAttribute("height", config.grid.height);
    group.createAttribute("xMin", config.bounds.xMin);
    group.createAttribute("xMax", config.bounds.xMax);
    group.createAttribute("yMin", config.bounds.yMin);
    group.createAttribute("yMax", config.bounds.yMax);

    std::cout << "Wrote " << config.output << " (" << config.grid.width << "x" << config.grid.height
              << ", " << config.steps << " steps, type=" << typeLabel << ")\n";
}

} // namespace FieldWriter
