#include "fieldReader.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <highfive/highfive.hpp>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace {

constexpr int kSteps = 3;
constexpr int kRows = 4;
constexpr int kCols = 5;
constexpr float kVxValue = 0.5f;
constexpr float kVyValue = -0.3f;

void writeFixture(const std::filesystem::path& path) {
    const std::vector<std::vector<std::vector<float>>> vx(
        kSteps, std::vector<std::vector<float>>(kRows, std::vector<float>(kCols, kVxValue)));
    const std::vector<std::vector<std::vector<float>>> vy(
        kSteps, std::vector<std::vector<float>>(kRows, std::vector<float>(kCols, kVyValue)));

    HighFive::File file(path.string(), HighFive::File::Overwrite);
    auto group = file.createGroup("field");
    group.createDataSet("vx", vx);
    group.createDataSet("vy", vy);
    group.createAttribute("xMin", -1.0f);
    group.createAttribute("xMax", 1.0f);
    group.createAttribute("yMin", -0.5f);
    group.createAttribute("yMax", 0.5f);
}

} // namespace

TEST_CASE("FieldReader::read() throws for nonexistent file", "[fieldreader]") {
    REQUIRE_THROWS(FieldReader::read("/nonexistent/path/field.h5"));
}

TEST_CASE("FieldReader::read() returns correct step/row/col dimensions", "[fieldreader]") {
    const auto path = std::filesystem::temp_directory_path() / "test_fr_dims.h5";
    writeFixture(path);

    const auto result = FieldReader::read(path.string());

    REQUIRE(static_cast<int>(result.frames.size()) == kSteps);
    REQUIRE(static_cast<int>(result.frames[0].size()) == kRows);
    REQUIRE(static_cast<int>(result.frames[0][0].size()) == kCols);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("FieldReader::read() populates xMin/xMax/yMin/yMax", "[fieldreader]") {
    const auto path = std::filesystem::temp_directory_path() / "test_fr_attrs.h5";
    writeFixture(path);

    const auto result = FieldReader::read(path.string());

    REQUIRE_THAT(result.bounds.xMin, WithinAbs(-1.0f, 1e-6f));
    REQUIRE_THAT(result.bounds.xMax, WithinAbs(1.0f, 1e-6f));
    REQUIRE_THAT(result.bounds.yMin, WithinAbs(-0.5f, 1e-6f));
    REQUIRE_THAT(result.bounds.yMax, WithinAbs(0.5f, 1e-6f));

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("FieldReader::read() Vec2 values match written floats", "[fieldreader]") {
    const auto path = std::filesystem::temp_directory_path() / "test_fr_values.h5";
    writeFixture(path);

    const auto result = FieldReader::read(path.string());

    REQUIRE_THAT(result.frames[1][2][3].x, WithinAbs(kVxValue, 1e-6f));
    REQUIRE_THAT(result.frames[1][2][3].y, WithinAbs(kVyValue, 1e-6f));

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("FieldReader::read() throws when vx and vy datasets are missing", "[fieldreader]") {
    const auto path = std::filesystem::temp_directory_path() / "test_fr_missing.h5";
    {
        HighFive::File file(path.string(), HighFive::File::Overwrite);
        auto group = file.createGroup("field");
        group.createAttribute("xMin", -1.0f);
        group.createAttribute("xMax", 1.0f);
        group.createAttribute("yMin", -1.0f);
        group.createAttribute("yMax", 1.0f);
        // Intentionally omit vx/vy datasets
    }
    REQUIRE_THROWS(FieldReader::read(path.string()));

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("FieldReader::read() throws when vy dataset is missing", "[fieldreader]") {
    const auto path = std::filesystem::temp_directory_path() / "test_fr_missing_vy.h5";
    {
        HighFive::File file(path.string(), HighFive::File::Overwrite);
        auto group = file.createGroup("field");
        const std::vector<std::vector<std::vector<float>>> vx(
            2, std::vector<std::vector<float>>(3, std::vector<float>(4, 0.0f)));
        group.createDataSet("vx", vx);
        group.createAttribute("xMin", -1.0f);
        group.createAttribute("xMax", 1.0f);
        group.createAttribute("yMin", -1.0f);
        group.createAttribute("yMax", 1.0f);
    }
    REQUIRE_THROWS(FieldReader::read(path.string()));

    std::error_code ec;
    std::filesystem::remove(path, ec);
}
