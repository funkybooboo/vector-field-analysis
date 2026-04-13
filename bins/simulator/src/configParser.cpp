#include "configParser.hpp"

#include <stdexcept>
#include <string>

// NOLINTBEGIN(misc-include-cleaner)
#define TOML_EXCEPTIONS 1
#include <toml++/toml.hpp>
// NOLINTEND(misc-include-cleaner)

namespace ConfigParser {

namespace {

// Struct defaults in simulatorConfig.hpp are the single source of truth.
// The parser only assigns a field when the key is explicitly present in the TOML.

FieldType parseFieldType(const std::string& typeName) {
    // toString() in simulatorConfig.hpp handles the reverse direction (enum->string).
    if (typeName == "vortex") {
        return FieldType::Vortex;
    }
    if (typeName == "uniform") {
        return FieldType::Uniform;
    }
    if (typeName == "source") {
        return FieldType::Source;
    }
    if (typeName == "sink") {
        return FieldType::Sink;
    }
    if (typeName == "saddle") {
        return FieldType::Saddle;
    }
    if (typeName == "spiral") {
        return FieldType::Spiral;
    }
    if (typeName == "noise") {
        return FieldType::Noise;
    }
    if (typeName == "custom") {
        return FieldType::Custom;
    }
    throw std::runtime_error("Unknown field type: \"" + typeName + "\"");
}

FieldLayerConfig parseLayerConfig(const toml::table& table) {
    FieldLayerConfig layer;
    if (const auto v = table["type"].value<std::string>()) {
        layer.type = parseFieldType(*v);
    }
    if (const auto v = table["strength"].value<double>()) {
        layer.strength = static_cast<float>(*v);
    }
    if (const auto v = table["center_x"].value<double>()) {
        layer.center.x = static_cast<float>(*v);
    }
    if (const auto v = table["center_y"].value<double>()) {
        layer.center.y = static_cast<float>(*v);
    }
    if (const auto v = table["angle"].value<double>()) {
        layer.angle = static_cast<float>(*v);
    }
    if (const auto v = table["magnitude"].value<double>()) {
        layer.magnitude = static_cast<float>(*v);
    }
    if (const auto v = table["sink_blend"].value<double>()) {
        layer.sinkBlend = static_cast<float>(*v);
    }
    if (const auto v = table["scale"].value<double>()) {
        layer.scale = static_cast<float>(*v);
    }
    if (const auto v = table["seed"].value<int64_t>()) {
        layer.seed = static_cast<int>(*v);
    }
    if (const auto v = table["x_expression"].value<std::string>()) {
        layer.xExpression = *v;
    }
    if (const auto v = table["y_expression"].value<std::string>()) {
        layer.yExpression = *v;
    }
    return layer;
}

SimulatorConfig parseSimulationSection(const toml::table& simulation) {
    SimulatorConfig config;
    if (const auto v = simulation["steps"].value<int64_t>()) {
        config.steps = static_cast<int>(*v);
    }
    if (const auto v = simulation["dt"].value<double>()) {
        config.dt = static_cast<float>(*v);
    }
    if (const auto v = simulation["viscosity"].value<double>()) {
        config.viscosity = static_cast<float>(*v);
    }
    if (const auto v = simulation["output"].value<std::string>()) {
        config.output = *v;
    }
    if (const auto v = simulation["width"].value<int64_t>()) {
        config.grid.width = static_cast<int>(*v);
    }
    if (const auto v = simulation["height"].value<int64_t>()) {
        config.grid.height = static_cast<int>(*v);
    }
    if (const auto v = simulation["xmin"].value<double>()) {
        config.bounds.xMin = static_cast<float>(*v);
    }
    if (const auto v = simulation["xmax"].value<double>()) {
        config.bounds.xMax = static_cast<float>(*v);
    }
    if (const auto v = simulation["ymin"].value<double>()) {
        config.bounds.yMin = static_cast<float>(*v);
    }
    if (const auto v = simulation["ymax"].value<double>()) {
        config.bounds.yMax = static_cast<float>(*v);
    }
    return config;
}

} // namespace

SimulatorConfig parseFile(const std::string& path) {
    const toml::table table = toml::parse_file(path);
    SimulatorConfig config;

    if (const auto* simulation = table["simulation"].as_table()) {
        config = parseSimulationSection(*simulation);
    }

    if (const auto* layersArray = table["layers"].as_array()) {
        for (const auto& element : *layersArray) {
            if (const auto* layerTable = element.as_table()) {
                config.layers.push_back(parseLayerConfig(*layerTable));
            }
        }
    }

    // Default to a single vortex layer if none are specified in the config
    if (config.layers.empty()) {
        config.layers.push_back(FieldLayerConfig{});
    }

    return config;
}

} // namespace ConfigParser
