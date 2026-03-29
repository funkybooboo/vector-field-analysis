#include "fieldWriter.hpp"

#include <highfive/highfive.hpp>
#include <iostream>
#include <string>

namespace FieldWriter {

namespace {

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

    // Summarize field types for the type attribute
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
