#pragma once
#include <cstdint>
#include <string>
#include <string_view>
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

inline std::string_view toString(FieldType type) {
    switch (type) {
    case FieldType::Vortex:  return "vortex";
    case FieldType::Uniform: return "uniform";
    case FieldType::Source:  return "source";
    case FieldType::Sink:    return "sink";
    case FieldType::Saddle:  return "saddle";
    case FieldType::Spiral:  return "spiral";
    case FieldType::Noise:   return "noise";
    case FieldType::Custom:  return "custom";
    }
    return "unknown";
}

// Maps grid index i (0..n-1) linearly onto [lo, hi]. Precondition: n >= 2.
inline float gridToWorld(int i, int n, float lo, float hi) {
    return lo + ((hi - lo) * static_cast<float>(i) / static_cast<float>(n - 1));
}

// Configuration for one layer in a superposed vector field.
// All types support: strength, centerX, centerY, magnitude (default 1.0 = unit length).
//   magnitude scales the output vector length before strength is applied.
// Type-specific parameters:
//   Uniform -> angle (degrees from positive x-axis)
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
    // Time increment per step. Advances the `t` variable in time-varying
    // field expressions and the viscosity decay -- not an ODE integration step size.
    float dt = 0.01f;
    // Controls exponential energy decay: field *= exp(-viscosity * t) per step.
    // viscosity=0 means no damping; larger values damp the field more aggressively.
    float viscosity = 0.0f;
    std::string output = "field.h5";
    int width = 64;
    int height = 64;
    float xMin = -1.0f;
    float xMax = 1.0f;
    float yMin = -1.0f;
    float yMax = 1.0f;
    // Each layer's contribution is added together (linear superposition),
    // so layer order does not affect the result.
    std::vector<FieldLayerConfig> layers;
};
