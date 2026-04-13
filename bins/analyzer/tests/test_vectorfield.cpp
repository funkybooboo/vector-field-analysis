#include "vectorField.hpp"

#include <catch2/catch_test_macros.hpp>

// Uniform-fill 3x3 grid
static VectorField::FieldGrid makeField(Vector::Vec2 fill = {}) {
    return {Vector::FieldBounds{0.0f, 2.0f, 0.0f, 2.0f},
            Vector::FieldSlice(3, std::vector<Vector::Vec2>(3, fill))};
}

// Zero 3x3 grid with one cell overridden
static VectorField::FieldGrid makeFieldAt(int row, int col, Vector::Vec2 v) {
    Vector::FieldSlice f(3, std::vector<Vector::Vec2>(3));
    f[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = v;
    return {Vector::FieldBounds{0.0f, 2.0f, 0.0f, 2.0f}, std::move(f)};
}

TEST_CASE("downstreamCell advances in vector direction", "[vectorfield]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));

    // All vectors point right (+x), so the column index advances, row stays the same
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 1);
}

TEST_CASE("traceStreamlineStep assigns a streamline", "[vectorfield]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    grid.traceStreamlineStep({0, 0});
    // Basic smoke test: no crash and execution completes
    SUCCEED();
}

// ---------------------------------------------------------------------------
// downstreamCell -- remaining directions and edge clamping
// ---------------------------------------------------------------------------

TEST_CASE("downstreamCell pointing left decreases column", "[vectorfield]") {
    auto grid = makeFieldAt(0, 1, Vector::Vec2(-1.0f, 0.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 1);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell pointing up increases row", "[vectorfield]") {
    auto grid = makeFieldAt(0, 0, Vector::Vec2(0.0f, 1.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 0);
    REQUIRE(nearestRow == 1);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell pointing down decreases row", "[vectorfield]") {
    auto grid = makeFieldAt(1, 0, Vector::Vec2(0.0f, -1.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(1, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell clamped at left edge", "[vectorfield]") {
    auto grid = makeFieldAt(0, 0, Vector::Vec2(-1.0f, 0.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell clamped at right edge", "[vectorfield]") {
    auto grid = makeFieldAt(0, 2, Vector::Vec2(1.0f, 0.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 2);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 2);
}

TEST_CASE("downstreamCell clamped at top edge", "[vectorfield]") {
    auto grid = makeFieldAt(2, 0, Vector::Vec2(0.0f, 1.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(2, 0);
    REQUIRE(nearestRow == 2);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("downstreamCell clamped at bottom edge", "[vectorfield]") {
    auto grid = makeFieldAt(0, 0, Vector::Vec2(0.0f, -1.0f));
    auto [nearestRow, nearestCol] = grid.downstreamCell(0, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

// ---------------------------------------------------------------------------
// joinStreamlines
// ---------------------------------------------------------------------------

TEST_CASE("joinStreamlines merges end path into start", "[vectorfield]") {
    auto start = std::make_shared<Vector::Streamline>(Vector::GridCell{0, 0});
    auto end = std::make_shared<Vector::Streamline>(Vector::GridCell{1, 0});
    end->path.push_back({2, 0});

    auto grid = makeField();
    grid.joinStreamlines(start, end);

    REQUIRE(start->path.size() == 3);
    REQUIRE(start->path[1] == (Vector::GridCell{1, 0}));
    REQUIRE(start->path[2] == (Vector::GridCell{2, 0}));
}

TEST_CASE("joinStreamlines with null start is a no-op", "[vectorfield]") {
    auto end = std::make_shared<Vector::Streamline>(Vector::GridCell{0, 0});
    auto grid = makeField();
    REQUIRE_NOTHROW(grid.joinStreamlines(nullptr, end));
}

TEST_CASE("joinStreamlines on equal pointers is a no-op", "[vectorfield]") {
    auto sl = std::make_shared<Vector::Streamline>(Vector::GridCell{0, 0});
    auto grid = makeField();
    const std::size_t sizeBefore = sl->path.size();
    grid.joinStreamlines(sl, sl);
    REQUIRE(sl->path.size() == sizeBefore);
}

// ---------------------------------------------------------------------------
// traceStreamlineStep multi-step
// ---------------------------------------------------------------------------

TEST_CASE("tracing multiple steps builds a path without crash", "[vectorfield]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    grid.traceStreamlineStep({0, 0});
    grid.traceStreamlineStep({0, 1});
    grid.traceStreamlineStep({0, 2});
    SUCCEED();
}

TEST_CASE("tracing into an occupied cell triggers merge without crash", "[vectorfield]") {
    // (0,0) points right; (0,2) points left -- both will claim (0,1) as destination
    Vector::FieldSlice f(3, std::vector<Vector::Vec2>(3));
    f[0][0] = Vector::Vec2(1.0f, 0.0f);
    f[0][2] = Vector::Vec2(-1.0f, 0.0f);
    VectorField::FieldGrid grid(Vector::FieldBounds{0.0f, 2.0f, 0.0f, 2.0f}, std::move(f));
    grid.traceStreamlineStep({0, 0}); // assigns streamline to (0,0) and (0,1)
    grid.traceStreamlineStep({0, 2}); // dest (0,1) already occupied -> merge
    SUCCEED();
}

// ---------------------------------------------------------------------------
// getStreamlines
// ---------------------------------------------------------------------------

TEST_CASE("getStreamlines on untouched grid returns empty", "[vectorfield][streamlines]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    REQUIRE(grid.getStreamlines().empty());
}

TEST_CASE("getStreamlines deduplicates cells sharing a streamline", "[vectorfield][streamlines]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    grid.traceStreamlineStep({0, 0}, {0, 1});
    const auto lines = grid.getStreamlines();
    REQUIRE(lines.size() == 1);
    REQUIRE(lines[0].size() == 2);
    REQUIRE(lines[0][0] == (Vector::GridCell{0, 0}));
    REQUIRE(lines[0][1] == (Vector::GridCell{0, 1}));
}

TEST_CASE("getStreamlines returns 3 streamlines after uniform right-pointing field trace",
          "[vectorfield][streamlines]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            grid.traceStreamlineStep(row, col);
        }
    }
    REQUIRE(grid.getStreamlines().size() == 3);
}

TEST_CASE("getStreamlines path contents match expected for uniform right-pointing field",
          "[vectorfield][streamlines]") {
    auto grid = makeField(Vector::Vec2(1.0f, 0.0f));
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            grid.traceStreamlineStep(row, col);
        }
    }
    const auto lines = grid.getStreamlines();
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == (Vector::Path{{0, 0}, {0, 1}, {0, 2}}));
    REQUIRE(lines[1] == (Vector::Path{{1, 0}, {1, 1}, {1, 2}}));
    REQUIRE(lines[2] == (Vector::Path{{2, 0}, {2, 1}, {2, 2}}));
}

TEST_CASE("getStreamlines returns 1 streamline when paths converge via merge",
          "[vectorfield][streamlines]") {
    Vector::FieldSlice f(3, std::vector<Vector::Vec2>(3));
    f[0][0] = Vector::Vec2(1.0f, 0.0f);  // dest (0,1)
    f[0][2] = Vector::Vec2(-1.0f, 0.0f); // dest (0,1)
    VectorField::FieldGrid grid(Vector::FieldBounds{0.0f, 2.0f, 0.0f, 2.0f}, std::move(f));
    grid.traceStreamlineStep({0, 0}, {0, 1});
    grid.traceStreamlineStep({0, 2}, {0, 1});
    REQUIRE(grid.getStreamlines().size() == 1);
}
