#include "fieldGenerator.hpp"

#include "vector.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

// Third-party single-header libs - excluded from clang-tidy via HeaderFilterRegex
// NOLINTBEGIN(misc-include-cleaner)
#define EXPRTK_DISABLE_CASEINSENSITIVITY
#include <exprtk.hpp>
#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>
// NOLINTEND(misc-include-cleaner)

namespace FieldGenerator {

namespace {

// All radial field types are mathematically undefined at their center point
// (radius = 0 causes division by zero). Vectors inside this radius are zeroed
// out to represent the stagnation point at the singularity.
constexpr float kSingularityRadius = 1e-6f;

struct RadialComponents {
    float dx;
    float dy;
    float radius;
};

// Returns nullopt when the point is at the singularity (radius < kSingularityRadius).
std::optional<RadialComponents> computeRadial(Vector::Vec2 pos, const FieldLayerConfig& layer) {
    const float dx = pos.x - layer.center.x;
    const float dy = pos.y - layer.center.y;
    const float radius = std::sqrt((dx * dx) + (dy * dy));
    if (radius < kSingularityRadius) {
        return std::nullopt;
    }
    return RadialComponents{dx, dy, radius};
}

// ---------------------------------------------------------------------------
// Per-layer base vector at physical position pos, t=time
// ---------------------------------------------------------------------------

// Returns a unit tangent vector perpendicular to the radius, producing pure
// counter-clockwise rotation around the center point.
Vector::Vec2 evalVortex(Vector::Vec2 pos, const FieldLayerConfig& layer) {
    const auto radial = computeRadial(pos, layer);
    if (!radial) {
        return Vector::Vec2{};
    }
    return {-radial->dy / radial->radius, radial->dx / radial->radius};
}

Vector::Vec2 evalUniform(const FieldLayerConfig& layer) {
    const float radians = layer.angle * (static_cast<float>(M_PI) / 180.0f);
    return {std::cos(radians), std::sin(radians)};
}

Vector::Vec2 evalSource(Vector::Vec2 pos, const FieldLayerConfig& layer) {
    const auto radial = computeRadial(pos, layer);
    if (!radial) {
        return Vector::Vec2{};
    }
    return {radial->dx / radial->radius, radial->dy / radial->radius};
}

// A sink is a source with the direction reversed -- flow points inward rather than outward.
Vector::Vec2 evalSink(Vector::Vec2 pos, const FieldLayerConfig& layer) {
    return -evalSource(pos, layer);
}

// Hyperbolic flow: stretches along the x-axis and compresses along y,
// with two attracting and two repelling sectors separated by the axes.
Vector::Vec2 evalSaddle(Vector::Vec2 pos, const FieldLayerConfig& layer) {
    const auto radial = computeRadial(pos, layer);
    if (!radial) {
        return Vector::Vec2{};
    }
    return {radial->dx / radial->radius, -radial->dy / radial->radius};
}

// A spiral is a convex blend of rotation (vortex) and attraction (sink).
// sinkBlend=0 is a pure circular vortex; sinkBlend=1 is a pure inward sink.
Vector::Vec2 evalSpiral(Vector::Vec2 pos, const FieldLayerConfig& layer) {
    const float sinkWeight = std::clamp(layer.sinkBlend, 0.0f, 1.0f);
    return (1.0f - sinkWeight) * evalVortex(pos, layer) + sinkWeight * evalSink(pos, layer);
}

Vector::Vec2 evalNoise(Vector::Vec2 pos, float time, const FieldLayerConfig& layer) {
    const float seedOffset = static_cast<float>(layer.seed) * 100.0f;
    const float scaledX = pos.x * layer.scale;
    const float scaledY = pos.y * layer.scale;
    const float velocityX =
        stb_perlin_noise3(scaledX + seedOffset, scaledY + seedOffset, time, 0, 0, 0);
    // Sample y velocity at a spatially offset location so vx and vy are
    // uncorrelated -- without the offset both components would be identical.
    const float velocityY = stb_perlin_noise3(scaledX + seedOffset + 31.41f,
                                              scaledY + seedOffset + 27.18f, time, 0, 0, 0);
    return {velocityX, velocityY};
}

// ---------------------------------------------------------------------------
// Custom exprtk field - compiled once per generateTimeSeries() call
// ---------------------------------------------------------------------------

// Compiles the user-supplied x/y math expressions once per generateTimeSeries()
// call. Compilation is expensive, so it cannot happen per cell. The variables
// x, y, t are bound by reference in the symbol table, so updating them before
// each eval() call makes the compiled expression evaluate at the new position.
struct CustomExpressionEvaluator {
    exprtk::symbol_table<float> symbolTable;
    exprtk::expression<float> xExpr;
    exprtk::expression<float> yExpr;
    float x = 0.0f;
    float y = 0.0f;
    float t = 0.0f;

    explicit CustomExpressionEvaluator(const std::string& xExpression,
                                       const std::string& yExpression) {
        symbolTable.add_variable("x", x);
        symbolTable.add_variable("y", y);
        symbolTable.add_variable("t", t);
        symbolTable.add_constants();

        xExpr.register_symbol_table(symbolTable);
        yExpr.register_symbol_table(symbolTable);

        exprtk::parser<float> parser;
        if (!parser.compile(xExpression, xExpr)) {
            throw std::runtime_error("Failed to compile x_expression: \"" + xExpression + "\"");
        }
        if (!parser.compile(yExpression, yExpr)) {
            throw std::runtime_error("Failed to compile y_expression: \"" + yExpression + "\"");
        }
    }

    Vector::Vec2 eval(Vector::Vec2 pos, float time) {
        x = pos.x;
        y = pos.y;
        t = time;
        return {xExpr.value(), yExpr.value()};
    }
};

} // namespace

Vector::FieldTimeSeries generateTimeSeries(const SimulatorConfig& config) {
    const auto numSteps = static_cast<std::size_t>(config.steps);
    const auto height = static_cast<std::size_t>(config.grid.height);
    const auto width = static_cast<std::size_t>(config.grid.width);

    // Pre-compute physical coordinates for each grid index
    std::vector<float> xCoords(width);
    std::vector<float> yCoords(height);
    for (std::size_t col = 0; col < width; ++col) {
        xCoords[col] = gridToWorld(static_cast<int>(col), config.grid.width, config.bounds.xMin,
                                   config.bounds.xMax);
    }
    for (std::size_t row = 0; row < height; ++row) {
        yCoords[row] = gridToWorld(static_cast<int>(row), config.grid.height, config.bounds.yMin,
                                   config.bounds.yMax);
    }

    // Pre-compile custom field expressions - one evaluator per layer, nullptr for non-custom
    std::vector<std::unique_ptr<CustomExpressionEvaluator>> evaluators(config.layers.size());
    for (std::size_t layerIdx = 0; layerIdx < config.layers.size(); ++layerIdx) {
        if (config.layers[layerIdx].type == FieldType::Custom) {
            evaluators[layerIdx] = std::make_unique<CustomExpressionEvaluator>(
                config.layers[layerIdx].xExpression, config.layers[layerIdx].yExpression);
        }
    }

    // Allocate result: steps[steps][height][width]
    Vector::FieldTimeSeries result;
    result.bounds = config.bounds;
    result.steps.assign(numSteps, Vector::FieldSlice(height, std::vector<Vector::Vec2>(width)));

    // For each time step, sample every layer at every grid cell and sum their
    // contributions (linear superposition). Strength is the per-layer weight.
    for (std::size_t step = 0; step < numSteps; ++step) {
        const float time = static_cast<float>(step) * config.dt;
        // Viscosity damps the field exponentially over time, modelling energy
        // dissipation. Computed once per step because it is spatially uniform.
        const float decay = std::exp(-config.viscosity * time);

        for (std::size_t row = 0; row < height; ++row) {
            for (std::size_t col = 0; col < width; ++col) {
                const Vector::Vec2 pos{xCoords[col], yCoords[row]};

                Vector::Vec2 sum{};

                for (std::size_t layerIdx = 0; layerIdx < config.layers.size(); ++layerIdx) {
                    const FieldLayerConfig& layer = config.layers[layerIdx];
                    Vector::Vec2 contribution{};

                    switch (layer.type) {
                    case FieldType::Vortex:
                        contribution = evalVortex(pos, layer);
                        break;
                    case FieldType::Uniform:
                        contribution = evalUniform(layer);
                        break;
                    case FieldType::Source:
                        contribution = evalSource(pos, layer);
                        break;
                    case FieldType::Sink:
                        contribution = evalSink(pos, layer);
                        break;
                    case FieldType::Saddle:
                        contribution = evalSaddle(pos, layer);
                        break;
                    case FieldType::Spiral:
                        contribution = evalSpiral(pos, layer);
                        break;
                    case FieldType::Noise:
                        contribution = evalNoise(pos, time, layer);
                        break;
                    case FieldType::Custom:
                        contribution = evaluators[layerIdx]->eval(pos, time);
                        break;
                    }

                    sum += layer.strength * (layer.magnitude * contribution);
                }

                result.steps[step][row][col] = decay * sum;
            }
        }
    }

    return result;
}

} // namespace FieldGenerator
