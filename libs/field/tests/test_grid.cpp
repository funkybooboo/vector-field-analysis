#include "grid.hpp"

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

// Uniform-fill 3x3 grid
static Field::Grid makeField(Vector::Vec2 fill = {}) {
    return {Field::Bounds{0.0f, 2.0f, 0.0f, 2.0f},
            Field::Slice(3, std::vector<Vector::Vec2>(3, fill))};
}

// Zero 3x3 grid with one cell overridden
static Field::Grid makeFieldAt(int row, int col, Vector::Vec2 v) {
    Field::Slice f(3, std::vector<Vector::Vec2>(3));
    f[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = v;
    return {Field::Bounds{0.0f, 2.0f, 0.0f, 2.0f}, std::move(f)};
}

TEST_CASE("downstreamCell advances in vector direction", "[grid]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));

    // All vectors point right (+x), so the column index advances, row stays the same
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 1);
}

// ---------------------------------------------------------------------------
// downstreamCell -- remaining directions and edge clamping
// ---------------------------------------------------------------------------

TEST_CASE("downstreamCell pointing left decreases column", "[grid]") {
    auto grid = makeFieldAt(0, 1, Vector::Vec2(-1.0f, 0.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 1);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell pointing up increases row", "[grid]") {
    auto grid = makeFieldAt(0, 0, Vector::Vec2(0.0f, 1.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 0);
    REQUIRE(nearestRow == 1);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell pointing down decreases row", "[grid]") {
    auto grid = makeFieldAt(1, 0, Vector::Vec2(0.0f, -1.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(1, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell clamped at left edge", "[grid]") {
    auto grid = makeFieldAt(0, 0, Vector::Vec2(-1.0f, 0.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell clamped at right edge", "[grid]") {
    auto grid = makeFieldAt(0, 2, Vector::Vec2(1.0f, 0.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 2);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 2);
}

TEST_CASE("downstreamCell clamped at top edge", "[grid]") {
    auto grid = makeFieldAt(2, 0, Vector::Vec2(0.0f, 1.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(2, 0);
    REQUIRE(nearestRow == 2);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell clamped at bottom edge", "[grid]") {
    auto grid = makeFieldAt(0, 0, Vector::Vec2(0.0f, -1.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

// ---------------------------------------------------------------------------
// Single-row and single-column grid edge cases
// ---------------------------------------------------------------------------

TEST_CASE("downstreamCell on single-row grid returns cell itself", "[grid]") {
    // 1x3 grid (1 row, 3 cols): no row advancement possible
    Field::Grid grid(Field::Bounds{0.0f, 2.0f, 0.0f, 0.0f},
                     Field::Slice(1, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 1.0f))));
    auto [r, c] = grid.downstreamCell(0, 1);
    REQUIRE(r == 0);
    REQUIRE(c == 1);
}

TEST_CASE("downstreamCell on single-column grid returns cell itself", "[grid]") {
    // 3x1 grid (3 rows, 1 col): no column advancement possible
    Field::Grid grid(Field::Bounds{0.0f, 0.0f, 0.0f, 2.0f},
                     Field::Slice(3, std::vector<Vector::Vec2>(1, Vector::Vec2(1.0f, 0.0f))));
    auto [r, c] = grid.downstreamCell(1, 0);
    REQUIRE(r == 1);
    REQUIRE(c == 0);
}

// ---------------------------------------------------------------------------
// downstreamCell and constructor error paths
// ---------------------------------------------------------------------------

TEST_CASE("Grid constructor throws on empty grid", "[grid]") {
    REQUIRE_THROWS_AS(Field::Grid(Field::Bounds{0.0f, 2.0f, 0.0f, 2.0f}, Field::Slice{}),
                      std::runtime_error);
}
