#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class FieldType : std::uint8_t {
    Vortex,
    Uniform,
    Source,
    Sink,
    Saddle,
    Spiral,
    Noise,
    Custom
};

// Configuration for one layer in a superposed vector field.
// All types support: strength, centerX, centerY.
// Type-specific parameters:
//   Uniform -> angle (degrees from positive x-axis), magnitude
//   Spiral  -> sinkBlend (0 = pure vortex rotation, 1 = pure sink attraction)
//   Noise   -> scale (spatial frequency multiplier), seed (integer offset)
//   Custom  -> xExpression, yExpression (exprtk math strings, variables: x, y, t)
struct FieldLayerConfig {
    FieldType type = FieldType::Vortex;
    float strength = 1.0f;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float angle = 0.0f;
    float magnitude = 1.0f;
    float sinkBlend = 0.5f;
    float scale = 1.0f;
    int seed = 0;
    std::string xExpression;
    std::string yExpression;
};

struct SimulatorConfig {
    int steps = 100;
    float dt = 0.01f;
    float viscosity = 0.0f;
    std::string output = "field.h5";
    int width = 64;
    int height = 64;
    float xMin = -1.0f;
    float xMax = 1.0f;
    float yMin = -1.0f;
    float yMax = 1.0f;
    std::vector<FieldLayerConfig> layers;
};
