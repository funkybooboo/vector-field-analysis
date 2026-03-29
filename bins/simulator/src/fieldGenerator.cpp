#include "fieldGenerator.hpp"

#include <Eigen/Dense>
#include <cmath>
#include <memory>
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

constexpr float kSingularityRadius = 1e-6f;

// ---------------------------------------------------------------------------
// Per-layer base vector at physical coords (px, py, t=time)
// ---------------------------------------------------------------------------

Eigen::Vector2f evalVortex(float px, float py, const FieldLayerConfig& layer) {
    const float dx = px - layer.centerX;
    const float dy = py - layer.centerY;
    const float radius = std::sqrt((dx * dx) + (dy * dy));
    if (radius < kSingularityRadius) {
        return Eigen::Vector2f::Zero();
    }
    return {-dy / radius, dx / radius};
}

Eigen::Vector2f evalUniform(const FieldLayerConfig& layer) {
    const float radians = layer.angle * (static_cast<float>(M_PI) / 180.0f);
    return {layer.magnitude * std::cos(radians), layer.magnitude * std::sin(radians)};
}

Eigen::Vector2f evalSource(float px, float py, const FieldLayerConfig& layer) {
    const float dx = px - layer.centerX;
    const float dy = py - layer.centerY;
    const float radius = std::sqrt((dx * dx) + (dy * dy));
    if (radius < kSingularityRadius) {
        return Eigen::Vector2f::Zero();
    }
    return {dx / radius, dy / radius};
}

Eigen::Vector2f evalSink(float px, float py, const FieldLayerConfig& layer) {
    return -evalSource(px, py, layer);
}

Eigen::Vector2f evalSaddle(float px, float py, const FieldLayerConfig& layer) {
    const float dx = px - layer.centerX;
    const float dy = py - layer.centerY;
    const float radius = std::sqrt((dx * dx) + (dy * dy));
    if (radius < kSingularityRadius) {
        return Eigen::Vector2f::Zero();
    }
    return {dx / radius, -dy / radius};
}

Eigen::Vector2f evalSpiral(float px, float py, const FieldLayerConfig& layer) {
    const float sinkWeight = std::clamp(layer.sinkBlend, 0.0f, 1.0f);
    return (1.0f - sinkWeight) * evalVortex(px, py, layer) + sinkWeight * evalSink(px, py, layer);
}

Eigen::Vector2f evalNoise(float px, float py, float time, const FieldLayerConfig& layer) {
    const float seedOffset = static_cast<float>(layer.seed) * 100.0f;
    const float scaledX = px * layer.scale;
    const float scaledY = py * layer.scale;
    const float velocityX =
        stb_perlin_noise3(scaledX + seedOffset, scaledY + seedOffset, time, 0, 0, 0);
    const float velocityY = stb_perlin_noise3(scaledX + seedOffset + 31.41f,
                                              scaledY + seedOffset + 27.18f, time, 0, 0, 0);
    return {velocityX, velocityY};
}

// ---------------------------------------------------------------------------
// Custom exprtk field - compiled once per generateTimeSeries() call
// ---------------------------------------------------------------------------

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

    Eigen::Vector2f eval(float px, float py, float time) {
        x = px;
        y = py;
        t = time;
        return {xExpr.value(), yExpr.value()};
    }
};

} // namespace

FieldTimeSeries generateTimeSeries(const SimulatorConfig& config) {
    const int numSteps = config.steps;
    const int height = config.height;
    const int width = config.width;

    // Pre-compute physical coordinates for each grid index
    std::vector<float> xCoords(static_cast<std::size_t>(width));
    std::vector<float> yCoords(static_cast<std::size_t>(height));
    for (int col = 0; col < width; ++col) {
        xCoords[static_cast<std::size_t>(col)] =
            config.xMin +
            ((config.xMax - config.xMin) * static_cast<float>(col) / static_cast<float>(width - 1));
    }
    for (int row = 0; row < height; ++row) {
        yCoords[static_cast<std::size_t>(row)] =
            config.yMin + ((config.yMax - config.yMin) * static_cast<float>(row) /
                           static_cast<float>(height - 1));
    }

    // Pre-compile custom field expressions - one evaluator per layer, nullptr for non-custom
    std::vector<std::unique_ptr<CustomExpressionEvaluator>> evaluators(config.layers.size());
    for (std::size_t layerIdx = 0; layerIdx < config.layers.size(); ++layerIdx) {
        if (config.layers[layerIdx].type == FieldType::Custom) {
            evaluators[layerIdx] = std::make_unique<CustomExpressionEvaluator>(
                config.layers[layerIdx].xExpression, config.layers[layerIdx].yExpression);
        }
    }

    // Allocate output: vx[steps][height][width], vy[steps][height][width]
    FieldTimeSeries output;
    output.vx.assign(
        static_cast<std::size_t>(numSteps),
        std::vector<std::vector<float>>(static_cast<std::size_t>(height),
                                        std::vector<float>(static_cast<std::size_t>(width), 0.0f)));
    output.vy.assign(
        static_cast<std::size_t>(numSteps),
        std::vector<std::vector<float>>(static_cast<std::size_t>(height),
                                        std::vector<float>(static_cast<std::size_t>(width), 0.0f)));

    for (int step = 0; step < numSteps; ++step) {
        const float time = static_cast<float>(step) * config.dt;
        // Decay scalar computed once per step, not per cell
        const float decay = std::exp(-config.viscosity * time);

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                const float px = xCoords[static_cast<std::size_t>(col)];
                const float py = yCoords[static_cast<std::size_t>(row)];

                Eigen::Vector2f sum = Eigen::Vector2f::Zero();

                for (std::size_t layerIdx = 0; layerIdx < config.layers.size(); ++layerIdx) {
                    const FieldLayerConfig& layer = config.layers[layerIdx];
                    Eigen::Vector2f contribution = Eigen::Vector2f::Zero();

                    switch (layer.type) {
                    case FieldType::Vortex:
                        contribution = evalVortex(px, py, layer);
                        break;
                    case FieldType::Uniform:
                        contribution = evalUniform(layer);
                        break;
                    case FieldType::Source:
                        contribution = evalSource(px, py, layer);
                        break;
                    case FieldType::Sink:
                        contribution = evalSink(px, py, layer);
                        break;
                    case FieldType::Saddle:
                        contribution = evalSaddle(px, py, layer);
                        break;
                    case FieldType::Spiral:
                        contribution = evalSpiral(px, py, layer);
                        break;
                    case FieldType::Noise:
                        contribution = evalNoise(px, py, time, layer);
                        break;
                    case FieldType::Custom:
                        contribution = evaluators[layerIdx]->eval(px, py, time);
                        break;
                    }

                    sum += layer.strength * contribution;
                }

                output.vx[static_cast<std::size_t>(step)][static_cast<std::size_t>(row)]
                         [static_cast<std::size_t>(col)] = sum.x() * decay;
                output.vy[static_cast<std::size_t>(step)][static_cast<std::size_t>(row)]
                         [static_cast<std::size_t>(col)] = sum.y() * decay;
            }
        }
    }

    return output;
}

} // namespace FieldGenerator
