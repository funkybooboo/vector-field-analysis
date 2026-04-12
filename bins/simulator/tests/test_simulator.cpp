#include "configParser.hpp"
#include "fieldGenerator.hpp"
#include "fieldWriter.hpp"
#include "simulatorConfig.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <highfive/highfive.hpp>
#include <string>

using Catch::Matchers::WithinAbs;
using FieldGenerator::generateTimeSeries;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SimulatorConfig makeConfig(FieldLayerConfig layer, int steps = 1) {
    SimulatorConfig config;
    config.steps = steps;
    config.dt = 0.01f;
    config.width = 32;
    config.height = 32;
    config.xMin = -1.0f;
    config.xMax = 1.0f;
    config.yMin = -1.0f;
    config.yMax = 1.0f;
    config.viscosity = 0.0f;
    config.layers = {std::move(layer)};
    return config;
}

// ---------------------------------------------------------------------------
// Vortex physics tests via generateTimeSeries()
// ---------------------------------------------------------------------------

TEST_CASE("Vortex field produces unit vectors", "[simulator]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    const auto out = generateTimeSeries(makeConfig(layer));
    // Check several off-center cells: each should have unit magnitude
    for (auto [i, j] :
         std::initializer_list<std::pair<int, int>>{{24, 16}, {8, 16}, {16, 24}, {20, 20}}) {
        REQUIRE_THAT(out.steps[0][j][i].magnitude(), WithinAbs(1.0f, 1e-5f));
    }
}

TEST_CASE("Vortex field is perpendicular to radius", "[simulator]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    const auto config = makeConfig(layer);
    const auto out = generateTimeSeries(config);

    // Perpendicularity: dot product of vortex vector and position vector must be ~0
    for (auto [i, j] : std::initializer_list<std::pair<int, int>>{{24, 16}, {8, 24}, {20, 8}}) {
        const float px = gridToWorld(i, config.width, config.xMin, config.xMax);
        const float py = gridToWorld(j, config.height, config.yMin, config.yMax);
        const auto& v = out.steps[0][j][i];
        // dot(v, pos) = vx*px + vy*py must be zero for a pure vortex
        REQUIRE_THAT((v.x * px) + (v.y * py), WithinAbs(0.0f, 1e-4f));
    }
}

TEST_CASE("Vortex field is zero at origin", "[simulator]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    // Use a 3x3 grid so that the center cell (1,1) lands exactly at (0,0)
    SimulatorConfig config = makeConfig(layer);
    config.width = 3;
    config.height = 3;
    const auto out = generateTimeSeries(config);
    REQUIRE_THAT(out.steps[0][1][1].magnitude(), WithinAbs(0.0f, 1e-5f));
}

// ---------------------------------------------------------------------------
// generateTimeSeries() dimension tests
// ---------------------------------------------------------------------------

TEST_CASE("generateTimeSeries() returns correct 3D dimensions", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    SimulatorConfig config = makeConfig(layer, 5);
    auto out = generateTimeSeries(config);

    REQUIRE(static_cast<int>(out.steps.size()) == config.steps);
    REQUIRE(static_cast<int>(out.steps[0].size()) == config.height);
    REQUIRE(static_cast<int>(out.steps[0][0].size()) == config.width);
}

// ---------------------------------------------------------------------------
// Vortex via generateTimeSeries()
// ---------------------------------------------------------------------------

TEST_CASE("Vortex generateTimeSeries() produces unit magnitude away from origin",
          "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    auto out = generateTimeSeries(makeConfig(layer));
    // Check a cell not near the origin: grid center-right
    const int i = 24;
    const int j = 16;
    REQUIRE_THAT(out.steps[0][j][i].magnitude(), WithinAbs(1.0f, 1e-4f));
}

// ---------------------------------------------------------------------------
// Uniform
// ---------------------------------------------------------------------------

TEST_CASE("Uniform field at angle=0 points right", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Uniform;
    layer.angle = 0.0f;
    layer.magnitude = 2.0f;
    auto out = generateTimeSeries(makeConfig(layer));
    REQUIRE_THAT(out.steps[0][16][16].x, WithinAbs(2.0f, 1e-4f));
    REQUIRE_THAT(out.steps[0][16][16].y, WithinAbs(0.0f, 1e-4f));
}

TEST_CASE("Uniform field at angle=90 points up", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Uniform;
    layer.angle = 90.0f;
    layer.magnitude = 1.0f;
    auto out = generateTimeSeries(makeConfig(layer));
    REQUIRE_THAT(out.steps[0][16][16].x, WithinAbs(0.0f, 1e-4f));
    REQUIRE_THAT(out.steps[0][16][16].y, WithinAbs(1.0f, 1e-4f));
}

// ---------------------------------------------------------------------------
// Source / Sink
// ---------------------------------------------------------------------------

TEST_CASE("Source field points away from center", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Source;
    layer.centerX = 0.0f;
    layer.centerY = 0.0f;
    auto config = makeConfig(layer);
    auto out = generateTimeSeries(config);
    // Cell to the right of center: vx should be positive, vy near zero
    const int i = 24;
    const int j = 16;
    REQUIRE(out.steps[0][j][i].x > 0.0f);
    const float px = gridToWorld(i, config.width, config.xMin, config.xMax);
    const float py = gridToWorld(j, config.height, config.yMin, config.yMax);
    // Angle from center should match vector direction
    const float angle = std::atan2(out.steps[0][j][i].y, out.steps[0][j][i].x);
    const float expected = std::atan2(py, px);
    REQUIRE_THAT(angle, WithinAbs(expected, 1e-4f));
}

TEST_CASE("Sink field points toward center", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Sink;
    layer.centerX = 0.0f;
    layer.centerY = 0.0f;
    auto config = makeConfig(layer);
    auto out = generateTimeSeries(config);
    const int i = 24;
    const int j = 16;
    // To the right of center, vx should be negative (pointing toward origin)
    REQUIRE(out.steps[0][j][i].x < 0.0f);
}

// ---------------------------------------------------------------------------
// Saddle
// ---------------------------------------------------------------------------

TEST_CASE("Saddle field has opposite signs in x and y relative to center",
          "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Saddle;
    layer.centerX = 0.0f;
    layer.centerY = 0.0f;
    auto config = makeConfig(layer);
    auto out = generateTimeSeries(config);
    // Point at (+x, +y) quadrant: vx > 0, vy < 0
    const int i = 24;
    const int j = 24;
    REQUIRE(out.steps[0][j][i].x > 0.0f);
    REQUIRE(out.steps[0][j][i].y < 0.0f);
}

// ---------------------------------------------------------------------------
// Spiral
// ---------------------------------------------------------------------------

TEST_CASE("Spiral field is between vortex and sink", "[simulator][generate]") {
    FieldLayerConfig layerVortex;
    layerVortex.type = FieldType::Vortex;
    FieldLayerConfig layerSink;
    layerSink.type = FieldType::Sink;
    FieldLayerConfig layerSpiral;
    layerSpiral.type = FieldType::Spiral;
    layerSpiral.sinkBlend = 0.5f;

    auto outVortex = generateTimeSeries(makeConfig(layerVortex));
    auto outSink = generateTimeSeries(makeConfig(layerSink));
    auto outSpiral = generateTimeSeries(makeConfig(layerSpiral));

    const int i = 24;
    const int j = 8;
    const float expectedVx =
        (0.5f * outVortex.steps[0][j][i].x) + (0.5f * outSink.steps[0][j][i].x);
    const float expectedVy =
        (0.5f * outVortex.steps[0][j][i].y) + (0.5f * outSink.steps[0][j][i].y);
    REQUIRE_THAT(outSpiral.steps[0][j][i].x, WithinAbs(expectedVx, 1e-4f));
    REQUIRE_THAT(outSpiral.steps[0][j][i].y, WithinAbs(expectedVy, 1e-4f));
}

// ---------------------------------------------------------------------------
// Viscosity decay
// ---------------------------------------------------------------------------

TEST_CASE("Viscous decay reduces magnitude over steps", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    SimulatorConfig config = makeConfig(layer, 50);
    config.viscosity = 1.0f;
    auto out = generateTimeSeries(config);

    const int i = 24;
    const int j = 16;
    REQUIRE(out.steps[0][j][i].magnitude() > out.steps[49][j][i].magnitude());
}

// ---------------------------------------------------------------------------
// Custom expression
// ---------------------------------------------------------------------------

TEST_CASE("Custom x_expression = \"x\" evaluates to world x-coordinate", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Custom;
    layer.xExpression = "x";
    layer.yExpression = "0";
    auto config = makeConfig(layer);
    auto out = generateTimeSeries(config);

    const int i = 20;
    const int j = 16;
    const float px = gridToWorld(i, config.width, config.xMin, config.xMax);
    REQUIRE_THAT(out.steps[0][j][i].x, WithinAbs(px, 1e-4f));
    REQUIRE_THAT(out.steps[0][j][i].y, WithinAbs(0.0f, 1e-4f));
}

// ---------------------------------------------------------------------------
// Superposition
// ---------------------------------------------------------------------------

TEST_CASE("Superposition sums field contributions", "[simulator][generate]") {
    FieldLayerConfig layerA;
    layerA.type = FieldType::Uniform;
    layerA.angle = 0.0f;
    layerA.magnitude = 1.0f;
    layerA.strength = 1.0f;

    FieldLayerConfig layerB;
    layerB.type = FieldType::Uniform;
    layerB.angle = 90.0f;
    layerB.magnitude = 1.0f;
    layerB.strength = 1.0f;

    SimulatorConfig config = makeConfig(layerA);
    config.layers = {layerA, layerB};
    auto out = generateTimeSeries(config);

    // vx = 1.0 + 0.0 = 1.0, vy = 0.0 + 1.0 = 1.0
    REQUIRE_THAT(out.steps[0][16][16].x, WithinAbs(1.0f, 1e-4f));
    REQUIRE_THAT(out.steps[0][16][16].y, WithinAbs(1.0f, 1e-4f));
}

// ---------------------------------------------------------------------------
// ConfigParser
// ---------------------------------------------------------------------------

namespace {

struct TempToml {
    std::filesystem::path path;
    TempToml(std::string name, const std::string& content)
        : path(std::filesystem::temp_directory_path() / std::move(name)) {
        std::ofstream ofs(path);
        ofs << content;
    }
    ~TempToml() noexcept {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    TempToml(const TempToml&) = delete;
    TempToml& operator=(const TempToml&) = delete;
    TempToml(TempToml&&) = delete;
    TempToml& operator=(TempToml&&) = delete;
};

} // namespace

// NOLINTBEGIN(readability-function-cognitive-complexity)

TEST_CASE("parseFile() uses defaults when [simulation] section absent", "[simulator][config]") {
    const TempToml tmp("cfg_defaults.toml", "# empty\n");
    const SimulatorConfig config = ConfigParser::parseFile(tmp.path.string());

    REQUIRE(config.steps == 100);
    REQUIRE_THAT(config.dt, WithinAbs(0.01f, 1e-6f));
    REQUIRE_THAT(config.viscosity, WithinAbs(0.0f, 1e-6f));
    REQUIRE(config.width == 64);
    REQUIRE(config.height == 64);
    REQUIRE(config.layers.size() == 1);
    REQUIRE(config.layers[0].type == FieldType::Vortex);
}

TEST_CASE("parseFile() reads [simulation] values correctly", "[simulator][config]") {
    const TempToml tmp("cfg_sim_values.toml", "[simulation]\n"
                                              "steps     = 42\n"
                                              "dt        = 0.05\n"
                                              "viscosity = 0.1\n"
                                              "output    = \"out.h5\"\n"
                                              "width     = 16\n"
                                              "height    = 8\n"
                                              "xmin      = -2.0\n"
                                              "xmax      =  2.0\n"
                                              "ymin      = -0.5\n"
                                              "ymax      =  0.5\n");
    const SimulatorConfig config = ConfigParser::parseFile(tmp.path.string());

    REQUIRE(config.steps == 42);
    REQUIRE_THAT(config.dt, WithinAbs(0.05f, 1e-6f));
    REQUIRE_THAT(config.viscosity, WithinAbs(0.1f, 1e-5f));
    REQUIRE(config.output == std::string("out.h5"));
    REQUIRE(config.width == 16);
    REQUIRE(config.height == 8);
    REQUIRE_THAT(config.xMin, WithinAbs(-2.0f, 1e-6f));
    REQUIRE_THAT(config.xMax, WithinAbs(2.0f, 1e-6f));
    REQUIRE_THAT(config.yMin, WithinAbs(-0.5f, 1e-6f));
    REQUIRE_THAT(config.yMax, WithinAbs(0.5f, 1e-6f));
}

TEST_CASE("parseFile() defaults to one vortex layer when [[layers]] absent",
          "[simulator][config]") {
    const TempToml tmp("cfg_default_layer.toml", "[simulation]\nsteps = 1\n");
    const SimulatorConfig config = ConfigParser::parseFile(tmp.path.string());

    REQUIRE(config.layers.size() == 1);
    REQUIRE(config.layers[0].type == FieldType::Vortex);
}

TEST_CASE("parseFile() parses all 8 field types from [[layers]]", "[simulator][config]") {
    const TempToml tmp("cfg_all_types.toml", "[[layers]]\ntype = \"vortex\"\n"
                                             "[[layers]]\ntype = \"uniform\"\n"
                                             "[[layers]]\ntype = \"source\"\n"
                                             "[[layers]]\ntype = \"sink\"\n"
                                             "[[layers]]\ntype = \"saddle\"\n"
                                             "[[layers]]\ntype = \"spiral\"\n"
                                             "[[layers]]\ntype = \"noise\"\n"
                                             "[[layers]]\ntype = \"custom\"\n"
                                             "x_expression = \"x\"\ny_expression = \"y\"\n");
    const SimulatorConfig config = ConfigParser::parseFile(tmp.path.string());

    REQUIRE(config.layers.size() == 8);
    REQUIRE(config.layers[0].type == FieldType::Vortex);
    REQUIRE(config.layers[1].type == FieldType::Uniform);
    REQUIRE(config.layers[2].type == FieldType::Source);
    REQUIRE(config.layers[3].type == FieldType::Sink);
    REQUIRE(config.layers[4].type == FieldType::Saddle);
    REQUIRE(config.layers[5].type == FieldType::Spiral);
    REQUIRE(config.layers[6].type == FieldType::Noise);
    REQUIRE(config.layers[7].type == FieldType::Custom);
}

TEST_CASE("parseFile() reads layer parameters", "[simulator][config]") {
    const TempToml tmp("cfg_layer_params.toml", "[[layers]]\n"
                                                "type         = \"spiral\"\n"
                                                "strength     = 2.5\n"
                                                "center_x     = 0.3\n"
                                                "center_y     = -0.1\n"
                                                "sink_blend   = 0.7\n"
                                                "scale        = 2.0\n"
                                                "seed         = 42\n"
                                                "x_expression = \"x+1\"\n"
                                                "y_expression = \"y+1\"\n");
    const SimulatorConfig config = ConfigParser::parseFile(tmp.path.string());

    REQUIRE(config.layers.size() == 1);
    const FieldLayerConfig& layer = config.layers[0];
    REQUIRE(layer.type == FieldType::Spiral);
    REQUIRE_THAT(layer.strength, WithinAbs(2.5f, 1e-5f));
    REQUIRE_THAT(layer.centerX, WithinAbs(0.3f, 1e-5f));
    REQUIRE_THAT(layer.centerY, WithinAbs(-0.1f, 1e-5f));
    REQUIRE_THAT(layer.sinkBlend, WithinAbs(0.7f, 1e-5f));
    REQUIRE_THAT(layer.scale, WithinAbs(2.0f, 1e-5f));
    REQUIRE(layer.seed == 42);
    REQUIRE(layer.xExpression == std::string("x+1"));
    REQUIRE(layer.yExpression == std::string("y+1"));
}

// NOLINTEND(readability-function-cognitive-complexity)

TEST_CASE("parseFile() throws for nonexistent file", "[simulator][config]") {
    REQUIRE_THROWS(ConfigParser::parseFile("/nonexistent/path/that/does/not/exist.toml"));
}

TEST_CASE("parseFile() throws for unknown field type", "[simulator][config]") {
    const TempToml tmp("cfg_bad_type.toml", "[[layers]]\ntype = \"unknown_type\"\n");
    REQUIRE_THROWS_AS(ConfigParser::parseFile(tmp.path.string()), std::runtime_error);
}

// ---------------------------------------------------------------------------
// FieldGenerator additional cases
// ---------------------------------------------------------------------------

TEST_CASE("Noise field is non-constant spatially", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Noise;
    layer.scale = 2.5f;
    layer.seed = 7;
    const auto out = generateTimeSeries(makeConfig(layer, 1));
    REQUIRE(std::abs(out.steps[0][0][0].x - out.steps[0][16][24].x) > 1e-4f);
}

TEST_CASE("Noise field is reproducible with the same seed", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Noise;
    layer.scale = 2.5f;
    layer.seed = 7;
    const auto out1 = generateTimeSeries(makeConfig(layer, 1));
    const auto out2 = generateTimeSeries(makeConfig(layer, 1));
    REQUIRE_THAT(out1.steps[0][8][8].x, WithinAbs(out2.steps[0][8][8].x, 1e-6f));
    REQUIRE_THAT(out1.steps[0][8][8].y, WithinAbs(out2.steps[0][8][8].y, 1e-6f));
}

TEST_CASE("Noise field differs between seeds", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Noise;
    layer.scale = 2.5f;
    layer.seed = 0;
    const auto out0 = generateTimeSeries(makeConfig(layer, 1));
    layer.seed = 42;
    const auto out42 = generateTimeSeries(makeConfig(layer, 1));
    REQUIRE(std::abs(out0.steps[0][8][8].x - out42.steps[0][8][8].x) > 1e-4f);
}

TEST_CASE("Spiral at sinkBlend=0 matches pure vortex", "[simulator][generate]") {
    FieldLayerConfig spiralLayer;
    spiralLayer.type = FieldType::Spiral;
    spiralLayer.sinkBlend = 0.0f;

    FieldLayerConfig vortexLayer;
    vortexLayer.type = FieldType::Vortex;

    const auto outSpiral = generateTimeSeries(makeConfig(spiralLayer));
    const auto outVortex = generateTimeSeries(makeConfig(vortexLayer));

    REQUIRE_THAT(outSpiral.steps[0][8][24].x, WithinAbs(outVortex.steps[0][8][24].x, 1e-5f));
    REQUIRE_THAT(outSpiral.steps[0][8][24].y, WithinAbs(outVortex.steps[0][8][24].y, 1e-5f));
}

TEST_CASE("Spiral at sinkBlend=1 matches pure sink", "[simulator][generate]") {
    FieldLayerConfig spiralLayer;
    spiralLayer.type = FieldType::Spiral;
    spiralLayer.sinkBlend = 1.0f;

    FieldLayerConfig sinkLayer;
    sinkLayer.type = FieldType::Sink;

    const auto outSpiral = generateTimeSeries(makeConfig(spiralLayer));
    const auto outSink = generateTimeSeries(makeConfig(sinkLayer));

    REQUIRE_THAT(outSpiral.steps[0][8][24].x, WithinAbs(outSink.steps[0][8][24].x, 1e-5f));
    REQUIRE_THAT(outSpiral.steps[0][8][24].y, WithinAbs(outSink.steps[0][8][24].y, 1e-5f));
}

TEST_CASE("Custom y_expression = \"y\" evaluates to world y-coordinate", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Custom;
    layer.xExpression = "0";
    layer.yExpression = "y";
    const auto config = makeConfig(layer);
    const auto out = generateTimeSeries(config);

    const int row = 20;
    const float py = gridToWorld(row, config.height, config.yMin, config.yMax);
    REQUIRE_THAT(out.steps[0][row][8].y, WithinAbs(py, 1e-4f));
    REQUIRE_THAT(out.steps[0][row][8].x, WithinAbs(0.0f, 1e-4f));
}

TEST_CASE("Custom t expression evaluates to step*dt", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Custom;
    layer.xExpression = "t";
    layer.yExpression = "0";
    SimulatorConfig config = makeConfig(layer, 5);
    const auto out = generateTimeSeries(config);

    REQUIRE_THAT(out.steps[0][16][16].x, WithinAbs(0.0f, 1e-5f));
    const float expectedT3 = 3.0f * config.dt;
    REQUIRE_THAT(out.steps[3][16][16].x, WithinAbs(expectedT3, 1e-5f));
}

TEST_CASE("Invalid custom expression throws", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Custom;
    layer.xExpression = "!!!";
    layer.yExpression = "0";
    REQUIRE_THROWS_AS(generateTimeSeries(makeConfig(layer)), std::runtime_error);
}

TEST_CASE("Empty layers produces all-zero output", "[simulator][generate]") {
    SimulatorConfig config;
    config.steps = 1;
    config.width = 4;
    config.height = 4;
    config.layers = {};
    const auto out = generateTimeSeries(config);

    REQUIRE_THAT(out.steps[0][0][0].x, WithinAbs(0.0f, 1e-6f));
    REQUIRE_THAT(out.steps[0][2][3].y, WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("Layer strength multiplies contribution", "[simulator][generate]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Uniform;
    layer.angle = 0.0f;
    layer.magnitude = 1.0f;
    layer.strength = 1.0f;

    FieldLayerConfig layerDouble = layer;
    layerDouble.strength = 2.0f;

    const auto out1 = generateTimeSeries(makeConfig(layer));
    const auto out2 = generateTimeSeries(makeConfig(layerDouble));

    REQUIRE_THAT(out2.steps[0][16][16].x, WithinAbs(2.0f * out1.steps[0][16][16].x, 1e-5f));
    REQUIRE_THAT(out2.steps[0][16][16].y, WithinAbs(2.0f * out1.steps[0][16][16].y, 1e-5f));
}

// ---------------------------------------------------------------------------
// FieldWriter
// ---------------------------------------------------------------------------

TEST_CASE("FieldWriter::write() creates the HDF5 file", "[simulator][writer]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    SimulatorConfig config = makeConfig(layer, 1);
    const auto tmpPath = std::filesystem::temp_directory_path() / "test_fw_create.h5";
    config.output = tmpPath.string();
    std::error_code ec;
    std::filesystem::remove(tmpPath, ec);

    FieldWriter::write(generateTimeSeries(config), config);

    REQUIRE(std::filesystem::exists(tmpPath));
    std::filesystem::remove(tmpPath, ec);
}

TEST_CASE("FieldWriter::write() stores vx/vy with correct dimensions [steps][height][width]",
          "[simulator][writer]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    SimulatorConfig config = makeConfig(layer, 3);
    const auto tmpPath = std::filesystem::temp_directory_path() / "test_fw_dims.h5";
    config.output = tmpPath.string();

    FieldWriter::write(generateTimeSeries(config), config);

    const HighFive::File file(tmpPath.string(), HighFive::File::ReadOnly);
    const auto group = file.getGroup("field");
    const auto dims = group.getDataSet("vx").getDimensions();
    REQUIRE(static_cast<int>(dims.size()) == 3);
    REQUIRE(dims[0] == static_cast<std::size_t>(config.steps));
    REQUIRE(dims[1] == static_cast<std::size_t>(config.height));
    REQUIRE(dims[2] == static_cast<std::size_t>(config.width));

    std::error_code ec;
    std::filesystem::remove(tmpPath, ec);
}

TEST_CASE("FieldWriter::write() stores correct metadata attributes", "[simulator][writer]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    SimulatorConfig config = makeConfig(layer, 2);
    config.viscosity = 0.5f;
    const auto tmpPath = std::filesystem::temp_directory_path() / "test_fw_attrs.h5";
    config.output = tmpPath.string();

    FieldWriter::write(generateTimeSeries(config), config);

    const HighFive::File file(tmpPath.string(), HighFive::File::ReadOnly);
    const auto group = file.getGroup("field");

    int steps = 0;
    float dt = 0.0f;
    float viscosity = 0.0f;
    std::string typeLabel;
    group.getAttribute("steps").read(steps);
    group.getAttribute("dt").read(dt);
    group.getAttribute("viscosity").read(viscosity);
    group.getAttribute("type").read(typeLabel);

    REQUIRE(steps == config.steps);
    REQUIRE_THAT(dt, WithinAbs(config.dt, 1e-6f));
    REQUIRE_THAT(viscosity, WithinAbs(config.viscosity, 1e-5f));
    REQUIRE(typeLabel == std::string("vortex"));

    std::error_code ec;
    std::filesystem::remove(tmpPath, ec);
}

TEST_CASE("FieldWriter::write() vx/vy values match generateTimeSeries() output",
          "[simulator][writer]") {
    FieldLayerConfig layer;
    layer.type = FieldType::Vortex;
    SimulatorConfig config = makeConfig(layer, 1);
    const auto tmpPath = std::filesystem::temp_directory_path() / "test_fw_values.h5";
    config.output = tmpPath.string();

    const auto field = generateTimeSeries(config);
    FieldWriter::write(field, config);

    const HighFive::File file(tmpPath.string(), HighFive::File::ReadOnly);
    const auto group = file.getGroup("field");
    std::vector<std::vector<std::vector<float>>> vxRead;
    std::vector<std::vector<std::vector<float>>> vyRead;
    group.getDataSet("vx").read(vxRead);
    group.getDataSet("vy").read(vyRead);

    REQUIRE_THAT(vxRead[0][8][24], WithinAbs(field.steps[0][8][24].x, 1e-6f));
    REQUIRE_THAT(vyRead[0][8][24], WithinAbs(field.steps[0][8][24].y, 1e-6f));

    std::error_code ec;
    std::filesystem::remove(tmpPath, ec);
}
