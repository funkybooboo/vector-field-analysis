#include "fieldTypes.hpp"
#include "vec2.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

// ---------------------------------------------------------------------------
// indexToCoord
// ---------------------------------------------------------------------------

TEST_CASE("indexToCoord returns lo at index 0", "[fieldtypes]") {
    REQUIRE_THAT(Field::indexToCoord(0, 5, -1.0f, 1.0f), WithinAbs(-1.0f, 1e-6f));
}

TEST_CASE("indexToCoord returns hi at index n-1", "[fieldtypes]") {
    REQUIRE_THAT(Field::indexToCoord(4, 5, -1.0f, 1.0f), WithinAbs(1.0f, 1e-6f));
}

TEST_CASE("indexToCoord midpoint is exact linear interpolation", "[fieldtypes]") {
    // 5-point grid over [-1, 1]: index 2 is the midpoint, should be 0.0
    REQUIRE_THAT(Field::indexToCoord(2, 5, -1.0f, 1.0f), WithinAbs(0.0f, 1e-6f));
}

TEST_CASE("indexToCoord works with asymmetric bounds", "[fieldtypes]") {
    // 3-point grid over [0, 3]: index 1 should be 1.5
    REQUIRE_THAT(Field::indexToCoord(1, 3, 0.0f, 3.0f), WithinAbs(1.5f, 1e-6f));
}

// ---------------------------------------------------------------------------
// GridCell comparison operators
// ---------------------------------------------------------------------------

TEST_CASE("GridCell operator== returns true for identical cells", "[fieldtypes]") {
    REQUIRE(Field::GridCell{2, 3} == Field::GridCell{2, 3});
}

TEST_CASE("GridCell operator== returns false when col differs", "[fieldtypes]") {
    REQUIRE_FALSE(Field::GridCell{2, 3} == Field::GridCell{2, 4});
}

TEST_CASE("GridCell operator== returns false when row differs", "[fieldtypes]") {
    REQUIRE_FALSE(Field::GridCell{1, 3} == Field::GridCell{2, 3});
}

TEST_CASE("GridCell operator!= returns true when cells differ", "[fieldtypes]") {
    REQUIRE(Field::GridCell{0, 0} != Field::GridCell{0, 1});
}

TEST_CASE("GridCell operator!= returns false for identical cells", "[fieldtypes]") {
    REQUIRE_FALSE(Field::GridCell{1, 2} != Field::GridCell{1, 2});
}

TEST_CASE("GridCell operator< orders by row first", "[fieldtypes]") {
    REQUIRE(Field::GridCell{0, 5} < Field::GridCell{1, 0});
    REQUIRE_FALSE(Field::GridCell{1, 0} < Field::GridCell{0, 5});
}

TEST_CASE("GridCell operator< orders by col when rows are equal", "[fieldtypes]") {
    REQUIRE(Field::GridCell{2, 1} < Field::GridCell{2, 3});
    REQUIRE_FALSE(Field::GridCell{2, 3} < Field::GridCell{2, 1});
}

TEST_CASE("GridCell operator< is irreflexive", "[fieldtypes]") {
    REQUIRE_FALSE(Field::GridCell{3, 3} < Field::GridCell{3, 3});
}

// ---------------------------------------------------------------------------
// TimeSeries::gridSize()
// ---------------------------------------------------------------------------

TEST_CASE("TimeSeries::gridSize returns zero dimensions for empty series", "[fieldtypes]") {
    Field::TimeSeries ts;
    const auto gs = ts.gridSize();
    REQUIRE(gs.width == 0);
    REQUIRE(gs.height == 0);
}

TEST_CASE("TimeSeries::gridSize returns correct width and height from first frame",
          "[fieldtypes]") {
    Field::TimeSeries ts;
    ts.frames.emplace_back(4, std::vector<Vector::Vec2>(6, Vector::Vec2{}));
    const auto gs = ts.gridSize();
    REQUIRE(gs.width == 6);
    REQUIRE(gs.height == 4);
}
