#include "configParser.hpp"

#include "toml_include.hpp" // NOLINT(misc-include-cleaner) -- wrapper sets TOML_EXCEPTIONS

#include <stdexcept>
#include <string>

namespace ConfigParser {

namespace {

// toString() in simulatorConfig.hpp handles the reverse direction (enum->string).
FieldType parseFieldType(const std::string& typeName) {
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
    if (const auto typeName = table["type"].value<std::string>()) {
        layer.type = parseFieldType(*typeName);
    }
    if (const auto strength = table["strength"].value<double>()) {
        layer.strength = static_cast<float>(*strength);
    }
    if (const auto centerX = table["center_x"].value<double>()) {
        layer.center.x = static_cast<float>(*centerX);
    }
    if (const auto centerY = table["center_y"].value<double>()) {
        layer.center.y = static_cast<float>(*centerY);
    }
    if (const auto angle = table["angle"].value<double>()) {
        layer.angle = static_cast<float>(*angle);
    }
    if (const auto amplitude = table["amplitude"].value<double>()) {
        layer.amplitude = static_cast<float>(*amplitude);
    }
    if (const auto sinkBlend = table["sink_blend"].value<double>()) {
        layer.sinkBlend = static_cast<float>(*sinkBlend);
    }
    if (const auto scale = table["scale"].value<double>()) {
        layer.scale = static_cast<float>(*scale);
    }
    if (const auto seed = table["seed"].value<int64_t>()) {
        layer.seed = static_cast<int>(*seed);
    }
    if (const auto xExpression = table["x_expression"].value<std::string>()) {
        layer.xExpression = *xExpression;
    }
    if (const auto yExpression = table["y_expression"].value<std::string>()) {
        layer.yExpression = *yExpression;
    }
    return layer;
}

SimulatorConfig parseSimulationSection(const toml::table& simulation) {
    // Struct defaults in simulatorConfig.hpp are the single source of truth.
    // The parser only assigns a field when the key is explicitly present in the TOML.
    SimulatorConfig config;
    if (const auto steps = simulation["steps"].value<int64_t>()) {
        config.steps = static_cast<int>(*steps);
    }
    if (const auto dt = simulation["dt"].value<double>()) {
        config.dt = static_cast<float>(*dt);
    }
    if (const auto viscosity = simulation["viscosity"].value<double>()) {
        config.viscosity = static_cast<float>(*viscosity);
    }
    if (const auto width = simulation["width"].value<int64_t>()) {
        config.grid.width = static_cast<int>(*width);
    }
    if (const auto height = simulation["height"].value<int64_t>()) {
        config.grid.height = static_cast<int>(*height);
    }
    if (const auto xMin = simulation["xmin"].value<double>()) {
        config.bounds.xMin = static_cast<float>(*xMin);
    }
    if (const auto xMax = simulation["xmax"].value<double>()) {
        config.bounds.xMax = static_cast<float>(*xMax);
    }
    if (const auto yMin = simulation["ymin"].value<double>()) {
        config.bounds.yMin = static_cast<float>(*yMin);
    }
    if (const auto yMax = simulation["ymax"].value<double>()) {
        config.bounds.yMax = static_cast<float>(*yMax);
    }
    return config;
}

} // namespace

SimulatorConfig parseSimulation(const std::string& path) {
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

    if (config.steps < 1) {
        throw std::runtime_error("steps must be >= 1");
    }
    // indexToCoord requires n >= 2; a 1-cell axis has no valid downstream direction.
    if (config.grid.width < 2) {
        throw std::runtime_error("width must be >= 2");
    }
    if (config.grid.height < 2) {
        throw std::runtime_error("height must be >= 2");
    }
    if (config.bounds.xMin >= config.bounds.xMax) {
        throw std::runtime_error("xmin must be < xmax");
    }
    if (config.bounds.yMin >= config.bounds.yMax) {
        throw std::runtime_error("ymin must be < ymax");
    }
    if (config.dt <= 0.0f) {
        throw std::runtime_error("dt must be > 0");
    }
    if (config.viscosity < 0.0f) {
        throw std::runtime_error("viscosity must be >= 0");
    }

    return config;
}

} // namespace ConfigParser
