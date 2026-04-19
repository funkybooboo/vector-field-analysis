#include "streamWriter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <filesystem>
#include <highfive/highfive.hpp>
#include <vector>

using Catch::Matchers::WithinAbs;

using StreamWriter::StepStreamlines;

// step 0: two streamlines (3 pts + 2 pts); step 1: one streamline (4 pts)
static const StepStreamlines kStep0 = {{{0, 0}, {0, 1}, {0, 2}}, {{1, 0}, {1, 1}}};
static const StepStreamlines kStep1 = {{{2, 0}, {2, 1}, {2, 2}, {2, 3}}};

static void writeFixture(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    StreamWriter::write(path.string(), {kStep0, kStep1}, {-1.0f, 1.0f, -1.0f, 1.0f}, {4, 3});
}

TEST_CASE("StreamWriter::write() emits correct group attributes", "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_attrs.h5";
    writeFixture(path);
    HighFive::File file(path.string(), HighFive::File::ReadOnly);
    auto grp = file.getGroup("streams");
    REQUIRE(grp.getAttribute("num_steps").read<int>() == 2);
    REQUIRE(grp.getAttribute("width").read<int>() == 4);
    REQUIRE(grp.getAttribute("height").read<int>() == 3);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("StreamWriter::write() step_0 offsets and point count", "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_step0_off.h5";
    writeFixture(path);
    HighFive::File file(path.string(), HighFive::File::ReadOnly);
    auto sg = file.getGroup("streams").getGroup("step_0");
    auto flat = sg.getDataSet("paths_flat").read<std::vector<std::vector<int>>>();
    auto offsets = sg.getDataSet("offsets").read<std::vector<int>>();
    REQUIRE(flat.size() == 5);
    REQUIRE(offsets == std::vector<int>{0, 3, 5});
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("StreamWriter::write() step_0 point values round-trip", "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_step0_pts.h5";
    writeFixture(path);
    HighFive::File file(path.string(), HighFive::File::ReadOnly);
    auto flat = file.getGroup("streams")
                    .getGroup("step_0")
                    .getDataSet("paths_flat")
                    .read<std::vector<std::vector<int>>>();
    REQUIRE(flat[0][0] == 0); // streamline 0, pt 0: row
    REQUIRE(flat[0][1] == 0); // col
    REQUIRE(flat[2][1] == 2); // streamline 0, pt 2: col
    REQUIRE(flat[3][0] == 1); // streamline 1, pt 0: row
    REQUIRE(flat[3][1] == 0); // col
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("StreamWriter::write() step_1 offsets and point count", "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_step1.h5";
    writeFixture(path);
    HighFive::File file(path.string(), HighFive::File::ReadOnly);
    auto sg = file.getGroup("streams").getGroup("step_1");
    auto flat = sg.getDataSet("paths_flat").read<std::vector<std::vector<int>>>();
    auto offsets = sg.getDataSet("offsets").read<std::vector<int>>();
    REQUIRE(flat.size() == 4);
    REQUIRE(offsets == std::vector<int>{0, 4});
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("StreamWriter::write() stores bounds attributes on /streams group", "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_bounds.h5";
    writeFixture(path);
    HighFive::File file(path.string(), HighFive::File::ReadOnly);
    const auto grp = file.getGroup("streams");
    REQUIRE_THAT(grp.getAttribute("xMin").read<float>(), WithinAbs(-1.0f, 1e-6f));
    REQUIRE_THAT(grp.getAttribute("xMax").read<float>(), WithinAbs(1.0f, 1e-6f));
    REQUIRE_THAT(grp.getAttribute("yMin").read<float>(), WithinAbs(-1.0f, 1e-6f));
    REQUIRE_THAT(grp.getAttribute("yMax").read<float>(), WithinAbs(1.0f, 1e-6f));
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("StreamWriter::write() empty step has empty paths_flat and sentinel-only offsets",
          "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_empty_step.h5";
    std::error_code ec;
    std::filesystem::remove(path, ec);
    StreamWriter::write(path.string(), {StepStreamlines{}}, {0.0f, 1.0f, 0.0f, 1.0f}, {2, 2});
    HighFive::File file(path.string(), HighFive::File::ReadOnly);
    const auto sg = file.getGroup("streams").getGroup("step_0");
    const auto flat = sg.getDataSet("paths_flat").read<std::vector<std::vector<int>>>();
    const auto offsets = sg.getDataSet("offsets").read<std::vector<int>>();
    REQUIRE(flat.empty());
    REQUIRE(offsets == std::vector<int>{0});
    std::filesystem::remove(path, ec);
}

TEST_CASE("StreamWriter::write() overwrites existing output file", "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_overwrite.h5";
    writeFixture(path);
    std::vector<StepStreamlines> allSteps = {{}};
    // Truncate mode: re-writing to an existing path must succeed, not throw.
    REQUIRE_NOTHROW(StreamWriter::write(path.string(), allSteps, {0.0f, 1.0f, 0.0f, 1.0f}, {2, 2}));
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST_CASE("StreamWriter::write() handles single-point streamlines", "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_single_pt.h5";
    std::error_code ec;
    std::filesystem::remove(path, ec);
    // Two single-point streamlines
    const StepStreamlines step = {{{0, 0}}, {{1, 2}}};
    StreamWriter::write(path.string(), {step}, {0.0f, 1.0f, 0.0f, 1.0f}, {3, 3});
    HighFive::File file(path.string(), HighFive::File::ReadOnly);
    const auto sg = file.getGroup("streams").getGroup("step_0");
    const auto flat = sg.getDataSet("paths_flat").read<std::vector<std::vector<int>>>();
    const auto offsets = sg.getDataSet("offsets").read<std::vector<int>>();
    REQUIRE(flat.size() == 2);
    REQUIRE(offsets == std::vector<int>{0, 1, 2});
    REQUIRE(flat[0][0] == 0); // streamline 0, row
    REQUIRE(flat[0][1] == 0); // streamline 0, col
    REQUIRE(flat[1][0] == 1); // streamline 1, row
    REQUIRE(flat[1][1] == 2); // streamline 1, col
    std::filesystem::remove(path, ec);
}

TEST_CASE("StreamWriter::write() three steps with varying streamline counts", "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_three_steps.h5";
    std::error_code ec;
    std::filesystem::remove(path, ec);
    const StepStreamlines s0 = {{{0, 0}, {0, 1}}};             // 1 streamline, 2 pts
    const StepStreamlines s1 = {{{1, 0}}, {{1, 1}}, {{1, 2}}}; // 3 streamlines, 1 pt each
    const StepStreamlines s2 = {};                             // empty
    StreamWriter::write(path.string(), {s0, s1, s2}, {0.0f, 1.0f, 0.0f, 1.0f}, {2, 3});
    HighFive::File file(path.string(), HighFive::File::ReadOnly);
    const auto grp = file.getGroup("streams");
    REQUIRE(grp.getAttribute("num_steps").read<int>() == 3);
    REQUIRE(grp.getGroup("step_0")
                .getDataSet("paths_flat")
                .read<std::vector<std::vector<int>>>()
                .size() == 2);
    REQUIRE(grp.getGroup("step_1")
                .getDataSet("paths_flat")
                .read<std::vector<std::vector<int>>>()
                .size() == 3);
    REQUIRE(grp.getGroup("step_2")
                .getDataSet("paths_flat")
                .read<std::vector<std::vector<int>>>()
                .empty());
    std::filesystem::remove(path, ec);
}
