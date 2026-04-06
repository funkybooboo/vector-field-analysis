#include "configParser.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>

// NOLINTBEGIN(misc-include-cleaner)
#define TOML_EXCEPTIONS 1
#include <toml++/toml.hpp>
// NOLINTEND(misc-include-cleaner)

namespace ConfigParser {

namespace {

FieldType parseFieldType(const std::string& typeName) {
    static const std::unordered_map<std::string, FieldType> typeMap = {
        {"vortex", FieldType::Vortex}, {"uniform", FieldType::Uniform},
        {"source", FieldType::Source}, {"sink", FieldType::Sink},
        {"saddle", FieldType::Saddle}, {"spiral", FieldType::Spiral},
        {"noise", FieldType::Noise},   {"custom", FieldType::Custom},
    };
    const auto it = typeMap.find(typeName);
    if (it == typeMap.end()) {
        throw std::runtime_error("Unknown field type: \"" + typeName + "\"");
    }
    return it->second;
}

FieldLayerConfig parseLayerConfig(const toml::table& table) {
    FieldLayerConfig layer;
    if (const auto value = table["type"].value<std::string>()) {
        layer.type = parseFieldType(*value);
    }
    layer.strength = static_cast<float>(table["strength"].value_or(1.0));
    layer.centerX = static_cast<float>(table["center_x"].value_or(0.0));
    layer.centerY = static_cast<float>(table["center_y"].value_or(0.0));
    layer.angle = static_cast<float>(table["angle"].value_or(0.0));
    layer.magnitude = static_cast<float>(table["magnitude"].value_or(1.0));
    layer.sinkBlend = static_cast<float>(table["sink_blend"].value_or(0.5));
    layer.scale = static_cast<float>(table["scale"].value_or(1.0));
    layer.seed = static_cast<int>(table["seed"].value_or(int64_t{0}));
    layer.xExpression = table["x_expression"].value_or(std::string{});
    layer.yExpression = table["y_expression"].value_or(std::string{});
    return layer;
}

} // namespace

SimulatorConfig parseFile(const std::string& path) {
    const toml::table table = toml::parse_file(path);
    SimulatorConfig config;

    if (const auto* simulation = table["simulation"].as_table()) {
        config.steps = static_cast<int>((*simulation)["steps"].value_or(int64_t{100}));
        config.dt = static_cast<float>((*simulation)["dt"].value_or(0.01));
        config.viscosity = static_cast<float>((*simulation)["viscosity"].value_or(0.0));
        config.output = (*simulation)["output"].value_or(std::string{"field.h5"});
        config.width = static_cast<int>((*simulation)["width"].value_or(int64_t{64}));
        config.height = static_cast<int>((*simulation)["height"].value_or(int64_t{64}));
        config.xMin = static_cast<float>((*simulation)["xmin"].value_or(-1.0));
        config.xMax = static_cast<float>((*simulation)["xmax"].value_or(1.0));
        config.yMin = static_cast<float>((*simulation)["ymin"].value_or(-1.0));
        config.yMax = static_cast<float>((*simulation)["ymax"].value_or(1.0));
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
