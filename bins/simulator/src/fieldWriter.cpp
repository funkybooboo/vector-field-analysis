#include "fieldWriter.hpp"

#include <highfive/highfive.hpp>
#include <iostream>
#include <string>

namespace FieldWriter {

namespace {

// Produces a human-readable label for the "type" HDF5 attribute.
// Not a general enum serializer -- output is informational only.
std::string fieldTypeName(FieldType type) {
    switch (type) {
    case FieldType::Vortex:
        return "vortex";
    case FieldType::Uniform:
        return "uniform";
    case FieldType::Source:
        return "source";
    case FieldType::Sink:
        return "sink";
    case FieldType::Saddle:
        return "saddle";
    case FieldType::Spiral:
        return "spiral";
    case FieldType::Noise:
        return "noise";
    case FieldType::Custom:
        return "custom";
    }
    return "unknown";
}

} // namespace

void write(const FieldGenerator::FieldTimeSeries& field, const SimulatorConfig& config) {
    HighFive::File file(config.output, HighFive::File::Overwrite);
    auto group = file.createGroup("field");

    group.createDataSet("vx", field.vx);
    group.createDataSet("vy", field.vy);

    // Store grid geometry and simulation parameters as HDF5 attributes alongside
    // the data so the file is self-contained -- readers don't need the config file.

    // Summarize all layer types as a '+'-joined label (e.g. "vortex+noise").
    // The format is informational; no reader is expected to parse it back
    // into individual layer types.
    std::string typeLabel;
    for (const auto& fieldConfig : config.layers) {
        if (!typeLabel.empty()) {
            typeLabel += '+';
        }
        typeLabel += fieldTypeName(fieldConfig.type);
    }

    group.createAttribute("type", typeLabel);
    group.createAttribute("steps", config.steps);
    group.createAttribute("dt", config.dt);
    group.createAttribute("viscosity", config.viscosity);
    group.createAttribute("width", config.width);
    group.createAttribute("height", config.height);
    group.createAttribute("xMin", config.xMin);
    group.createAttribute("xMax", config.xMax);
    group.createAttribute("yMin", config.yMin);
    group.createAttribute("yMax", config.yMax);

    std::cout << "Wrote " << config.output << " (" << config.width << "x" << config.height << ", "
              << config.steps << " steps, type=" << typeLabel << ")\n";
}

} // namespace FieldWriter
