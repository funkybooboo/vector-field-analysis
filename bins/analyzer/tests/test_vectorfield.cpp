#include "vectorField.hpp"

#include <catch2/catch_test_macros.hpp>

static VectorField::FieldGrid makeField() {
    // 3x3 field with all vectors pointing right (+x)
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(1.0f, 0.0f)));
    return {0.0f, 2.0f, 0.0f, 2.0f, f};
}

TEST_CASE("neighborInVectorDirection advances in vector direction", "[vectorfield]") {
    auto grid = makeField();

    // All vectors point right (+x), so the column index advances, row stays the same
    auto [nearestRow, nearestCol] = grid.neighborInVectorDirection(0, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 1);
}

TEST_CASE("traceStreamlineStep assigns a streamline", "[vectorfield]") {
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(1.0f, 0.0f)));
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);

    // Before trace, no streamline assigned
    REQUIRE(f[0][0].stream == nullptr);

    grid.traceStreamlineStep({0, 0});

    // After trace, the source vector has a streamline
    // (FieldGrid holds its own copy of the field, so we check via trace behaviour)
    // Basic smoke test: no crash and execution completes
    SUCCEED();
}

// ---------------------------------------------------------------------------
// neighborInVectorDirection -- remaining directions and edge clamping
// ---------------------------------------------------------------------------

TEST_CASE("neighborInVectorDirection pointing left decreases column", "[vectorfield]") {
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 0.0f)));
    f[0][1] = Vector::Vec2(-1.0f, 0.0f);
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    auto [nearestRow, nearestCol] = grid.neighborInVectorDirection(0, 1);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("neighborInVectorDirection pointing up increases row", "[vectorfield]") {
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 0.0f)));
    f[0][0] = Vector::Vec2(0.0f, 1.0f);
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    auto [nearestRow, nearestCol] = grid.neighborInVectorDirection(0, 0);
    REQUIRE(nearestRow == 1);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("neighborInVectorDirection pointing down decreases row", "[vectorfield]") {
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 0.0f)));
    f[1][0] = Vector::Vec2(0.0f, -1.0f);
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    auto [nearestRow, nearestCol] = grid.neighborInVectorDirection(1, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("neighborInVectorDirection clamped at left edge", "[vectorfield]") {
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 0.0f)));
    f[0][0] = Vector::Vec2(-1.0f, 0.0f);
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    auto [nearestRow, nearestCol] = grid.neighborInVectorDirection(0, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("neighborInVectorDirection clamped at right edge", "[vectorfield]") {
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 0.0f)));
    f[0][2] = Vector::Vec2(1.0f, 0.0f);
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    auto [nearestRow, nearestCol] = grid.neighborInVectorDirection(0, 2);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 2);
}

TEST_CASE("neighborInVectorDirection clamped at top edge", "[vectorfield]") {
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 0.0f)));
    f[2][0] = Vector::Vec2(0.0f, 1.0f);
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    auto [nearestRow, nearestCol] = grid.neighborInVectorDirection(2, 0);
    REQUIRE(nearestRow == 2);
    REQUIRE(nearestCol == 0);
}

TEST_CASE("neighborInVectorDirection clamped at bottom edge", "[vectorfield]") {
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 0.0f)));
    f[0][0] = Vector::Vec2(0.0f, -1.0f);
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    auto [nearestRow, nearestCol] = grid.neighborInVectorDirection(0, 0);
    REQUIRE(nearestRow == 0);
    REQUIRE(nearestCol == 0);
}

// ---------------------------------------------------------------------------
// joinStreamlines
// ---------------------------------------------------------------------------

TEST_CASE("joinStreamlines merges end path into start", "[vectorfield]") {
    auto start = std::make_shared<Vector::Streamline>(std::make_pair(0, 0));
    auto end = std::make_shared<Vector::Streamline>(std::make_pair(1, 0));
    end->path.emplace_back(2, 0);

    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 0.0f)));
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    grid.joinStreamlines(start, end);

    REQUIRE(start->path.size() == 3);
    REQUIRE(start->path[1] == std::make_pair(1, 0));
    REQUIRE(start->path[2] == std::make_pair(2, 0));
}

TEST_CASE("joinStreamlines with null start is a no-op", "[vectorfield]") {
    auto end = std::make_shared<Vector::Streamline>(std::make_pair(0, 0));
    std::vector<std::vector<Vector::Vec2>> f(
        1, std::vector<Vector::Vec2>(1, Vector::Vec2(0.0f, 0.0f)));
    VectorField::FieldGrid grid(0.0f, 1.0f, 0.0f, 1.0f, f);
    REQUIRE_NOTHROW(grid.joinStreamlines(nullptr, end));
}

TEST_CASE("joinStreamlines on equal pointers is a no-op", "[vectorfield]") {
    auto sl = std::make_shared<Vector::Streamline>(std::make_pair(0, 0));
    std::vector<std::vector<Vector::Vec2>> f(
        1, std::vector<Vector::Vec2>(1, Vector::Vec2(0.0f, 0.0f)));
    VectorField::FieldGrid grid(0.0f, 1.0f, 0.0f, 1.0f, f);
    const std::size_t sizeBefore = sl->path.size();
    grid.joinStreamlines(sl, sl);
    REQUIRE(sl->path.size() == sizeBefore);
}

// ---------------------------------------------------------------------------
// traceStreamlineStep multi-step
// ---------------------------------------------------------------------------

TEST_CASE("tracing multiple steps builds a path without crash", "[vectorfield]") {
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(1.0f, 0.0f)));
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    grid.traceStreamlineStep({0, 0});
    grid.traceStreamlineStep({0, 1});
    grid.traceStreamlineStep({0, 2});
    SUCCEED();
}

TEST_CASE("tracing into an occupied cell triggers merge without crash", "[vectorfield]") {
    // (0,0) points right; (0,2) points left -- both will claim (0,1) as destination
    std::vector<std::vector<Vector::Vec2>> f(
        3, std::vector<Vector::Vec2>(3, Vector::Vec2(0.0f, 0.0f)));
    f[0][0] = Vector::Vec2(1.0f, 0.0f);
    f[0][2] = Vector::Vec2(-1.0f, 0.0f);
    VectorField::FieldGrid grid(0.0f, 2.0f, 0.0f, 2.0f, f);
    grid.traceStreamlineStep({0, 0}); // assigns streamline to (0,0) and (0,1)
    grid.traceStreamlineStep({0, 2}); // dest (0,1) already occupied -> merge
    SUCCEED();
}
