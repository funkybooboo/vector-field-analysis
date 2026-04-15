#include "fieldReader.hpp"
#include "fieldWriter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <highfive/highfive.hpp>
#include <stdexcept>

using Catch::Matchers::WithinAbs;

namespace {

Field::TimeSeries makeTimeSeries(int steps, int height, int width) {
    Field::TimeSeries ts;
    ts.bounds = {-1.0f, 1.0f, -0.5f, 0.5f};
    ts.frames.resize(static_cast<std::size_t>(steps),
                     Field::Slice(static_cast<std::size_t>(height),
                                  std::vector<Vector::Vec2>(static_cast<std::size_t>(width),
                                                            Vector::Vec2{0.5f, -0.3f})));
    return ts;
}

} // namespace

TEST_CASE("FieldWriter::write() throws on empty frames", "[fieldwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_fw_empty.h5";
    Field::TimeSeries empty;
    REQUIRE_THROWS_AS(FieldWriter::write(path.string(), empty, "test", 0.01f, 0.0f),
                      std::runtime_error);
}

TEST_CASE("FieldWriter::write() stores geometry attributes correctly", "[fieldwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_fw_geom.h5";
    const auto ts = makeTimeSeries(2, 4, 6);
    FieldWriter::write(path.string(), ts, "test", 0.01f, 0.0f);

    HighFive::File file(path.string(), HighFive::File::ReadOnly);
    const auto group = file.getGroup("field");
    REQUIRE_THAT(group.getAttribute("xMin").read<float>(), WithinAbs(-1.0f, 1e-6f));
    REQUIRE_THAT(group.getAttribute("xMax").read<float>(), WithinAbs(1.0f, 1e-6f));
    REQUIRE_THAT(group.getAttribute("yMin").read<float>(), WithinAbs(-0.5f, 1e-6f));
    REQUIRE_THAT(group.getAttribute("yMax").read<float>(), WithinAbs(0.5f, 1e-6f));
    REQUIRE(group.getAttribute("width").read<int>() == 6);
    REQUIRE(group.getAttribute("height").read<int>() == 4);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("FieldWriter::write() round-trips dimensions and bounds via FieldReader::read()",
          "[fieldwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_fw_roundtrip.h5";
    const auto ts = makeTimeSeries(3, 4, 5);
    FieldWriter::write(path.string(), ts, "test", 0.01f, 0.0f);

    const auto result = FieldReader::read(path.string());
    REQUIRE(result.frames.size() == 3);
    REQUIRE(result.frames[0].size() == 4);
    REQUIRE(result.frames[0][0].size() == 5);
    REQUIRE_THAT(result.bounds.xMin, WithinAbs(-1.0f, 1e-6f));
    REQUIRE_THAT(result.bounds.xMax, WithinAbs(1.0f, 1e-6f));
    REQUIRE_THAT(result.bounds.yMin, WithinAbs(-0.5f, 1e-6f));
    REQUIRE_THAT(result.bounds.yMax, WithinAbs(0.5f, 1e-6f));

    std::error_code ec;
    std::filesystem::remove(path, ec);
}
