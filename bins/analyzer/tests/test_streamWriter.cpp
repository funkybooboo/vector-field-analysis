#include "streamWriter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <highfive/highfive.hpp>
#include <vector>

using StreamWriter::StepStreamlines;

// step 0: two streamlines (3 pts + 2 pts); step 1: one streamline (4 pts)
static const StepStreamlines kStep0 = {{{0, 0}, {0, 1}, {0, 2}}, {{1, 0}, {1, 1}}};
static const StepStreamlines kStep1 = {{{2, 0}, {2, 1}, {2, 2}, {2, 3}}};

static void writeFixture(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    StreamWriter::write(path.string(), {kStep0, kStep1}, -1.0f, 1.0f, -1.0f, 1.0f, 4, 3);
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

TEST_CASE("StreamWriter::write() throws if output file already exists", "[streamwriter]") {
    const auto path = std::filesystem::temp_directory_path() / "test_sw_excl.h5";
    writeFixture(path);
    std::vector<StepStreamlines> allSteps = {{}};
    REQUIRE_THROWS(StreamWriter::write(path.string(), allSteps, 0.0f, 1.0f, 0.0f, 1.0f, 2, 2));
    std::error_code ec;
    std::filesystem::remove(path, ec);
}
