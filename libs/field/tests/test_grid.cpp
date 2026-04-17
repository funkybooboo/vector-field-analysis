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

TEST_CASE("traceStreamlineStep assigns a streamline", "[grid]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    grid.traceStreamlineStep({0, 0});
    const auto lines = grid.getStreamlines();
    REQUIRE_FALSE(lines.empty());
    REQUIRE_FALSE(lines[0].empty());
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

TEST_CASE("solver handles single-row grid without crash", "[grid]") {
    Field::Grid grid(Field::Bounds{0.0f, 2.0f, 0.0f, 0.0f},
                     Field::Slice(1, std::vector<Vector::Vec2>(3, Vector::Vec2(1.0f, 0.0f))));
    for (int col = 0; col < 3; col++) {
        grid.traceStreamlineStep(0, col);
    }
    // Every cell maps to itself; 3 independent single-cell streamlines
    REQUIRE(grid.getStreamlines().size() == 3);
}

TEST_CASE("solver handles single-column grid without crash", "[grid]") {
    Field::Grid grid(Field::Bounds{0.0f, 0.0f, 0.0f, 2.0f},
                     Field::Slice(3, std::vector<Vector::Vec2>(1, Vector::Vec2(0.0f, 1.0f))));
    for (int row = 0; row < 3; row++) {
        grid.traceStreamlineStep(row, 0);
    }
    REQUIRE(grid.getStreamlines().size() == 3);
}

// ---------------------------------------------------------------------------
// joinStreamlines
// ---------------------------------------------------------------------------

TEST_CASE("joinStreamlines merges end path into start", "[grid]") {
    auto start = std::make_shared<Field::Streamline>(Field::GridCell{0, 0});
    auto end = std::make_shared<Field::Streamline>(Field::GridCell{1, 0});
    end->appendPoint({2, 0});

    auto grid = makeField();
    grid.joinStreamlines(start, end);

    REQUIRE(start->getPath().size() == 3);
    REQUIRE(*std::next(start->getPath().begin(), 1) == (Field::GridCell{1, 0}));
    REQUIRE(*std::next(start->getPath().begin(), 2) == (Field::GridCell{2, 0}));
}

TEST_CASE("joinStreamlines with null start is a no-op", "[grid]") {
    auto end = std::make_shared<Field::Streamline>(Field::GridCell{0, 0});
    auto grid = makeField();
    REQUIRE_NOTHROW(grid.joinStreamlines(nullptr, end));
}

TEST_CASE("joinStreamlines on equal pointers is a no-op", "[grid]") {
    auto sl = std::make_shared<Field::Streamline>(Field::GridCell{0, 0});
    auto grid = makeField();
    const std::size_t sizeBefore = sl->getPath().size();
    grid.joinStreamlines(sl, sl);
    REQUIRE(sl->getPath().size() == sizeBefore);
}

// ---------------------------------------------------------------------------
// traceStreamlineStep multi-step
// ---------------------------------------------------------------------------

TEST_CASE("tracing multiple steps builds a path", "[grid]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    grid.traceStreamlineStep({0, 0});
    grid.traceStreamlineStep({0, 1});
    grid.traceStreamlineStep({0, 2});
    const auto lines = grid.getStreamlines();
    REQUIRE(lines.size() == 1);
    REQUIRE(lines[0].size() == 3);
}

TEST_CASE("tracing into an occupied cell triggers merge", "[grid]") {
    // (0,0) points right; (0,2) points left -- both converge on (0,1)
    Field::Slice f(3, std::vector<Vector::Vec2>(3));
    f[0][0] = Vector::Vec2(1.0f, 0.0f);
    f[0][2] = Vector::Vec2(-1.0f, 0.0f);
    Field::Grid grid(Field::Bounds{0.0f, 2.0f, 0.0f, 2.0f}, std::move(f));
    grid.traceStreamlineStep({0, 0}, {0, 1}); // assigns streamline to (0,0)+(0,1)
    grid.traceStreamlineStep({0, 2}, {0, 1}); // dest (0,1) occupied -> merge
    // All three cells end up on one streamline
    REQUIRE(grid.getStreamlines().size() == 1);
}

// ---------------------------------------------------------------------------
// getStreamlines
// ---------------------------------------------------------------------------

TEST_CASE("getStreamlines on untouched grid returns empty", "[grid][streamlines]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    REQUIRE(grid.getStreamlines().empty());
}

TEST_CASE("getStreamlines deduplicates cells sharing a streamline", "[grid][streamlines]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    grid.traceStreamlineStep({0, 0}, {0, 1});
    const auto lines = grid.getStreamlines();
    REQUIRE(lines.size() == 1);
    REQUIRE(lines[0].size() == 2);
    REQUIRE(*lines[0].begin() == (Field::GridCell{0, 0}));
    REQUIRE(*std::next(lines[0].begin()) == (Field::GridCell{0, 1}));
}

TEST_CASE("getStreamlines returns 3 streamlines after uniform right-pointing field trace",
          "[grid][streamlines]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            grid.traceStreamlineStep(row, col);
        }
    }
    REQUIRE(grid.getStreamlines().size() == 3);
}

TEST_CASE("getStreamlines path contents match expected for uniform right-pointing field",
          "[grid][streamlines]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            grid.traceStreamlineStep(row, col);
        }
    }
    const auto lines = grid.getStreamlines();
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == (Field::Path{{0, 0}, {0, 1}, {0, 2}}));
    REQUIRE(lines[1] == (Field::Path{{1, 0}, {1, 1}, {1, 2}}));
    REQUIRE(lines[2] == (Field::Path{{2, 0}, {2, 1}, {2, 2}}));
}

// ---------------------------------------------------------------------------
// downstreamCell and traceStreamlineStep error paths
// ---------------------------------------------------------------------------

TEST_CASE("Grid constructor throws on empty grid", "[grid]") {
    REQUIRE_THROWS_AS(Field::Grid(Field::Bounds{0.0f, 2.0f, 0.0f, 2.0f}, Field::Slice{}),
                      std::runtime_error);
}

TEST_CASE("traceStreamlineStep throws when src row is out of bounds", "[grid]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    REQUIRE_THROWS_AS(grid.traceStreamlineStep({3, 0}, {0, 0}), std::out_of_range);
}

TEST_CASE("traceStreamlineStep throws when src col is out of bounds", "[grid]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    REQUIRE_THROWS_AS(grid.traceStreamlineStep({0, 3}, {0, 0}), std::out_of_range);
}

TEST_CASE("traceStreamlineStep throws when dest is out of bounds", "[grid]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    REQUIRE_THROWS_AS(grid.traceStreamlineStep({0, 0}, {3, 0}), std::out_of_range);
}

TEST_CASE("getStreamlines returns 1 streamline when paths converge via merge",
          "[grid][streamlines]") {
    Field::Slice f(3, std::vector<Vector::Vec2>(3));
    f[0][0] = Vector::Vec2(1.0f, 0.0f);  // dest (0,1)
    f[0][2] = Vector::Vec2(-1.0f, 0.0f); // dest (0,1)
    Field::Grid grid(Field::Bounds{0.0f, 2.0f, 0.0f, 2.0f}, std::move(f));
    grid.traceStreamlineStep({0, 0}, {0, 1});
    grid.traceStreamlineStep({0, 2}, {0, 1});
    REQUIRE(grid.getStreamlines().size() == 1);
}
