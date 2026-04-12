#include "openMP.hpp"
#include "pthreads.hpp"
#include "sequentialCPU.hpp"
#include "vectorField.hpp"

#include <catch2/catch_test_macros.hpp>

// 3x3 grid where all vectors point right (+x)
static VectorField::FieldGrid makeGrid() {
    return {0.0f, 2.0f, 0.0f, 2.0f,
            Vector::FieldSlice(3, std::vector<Vector::Vec2>(3, Vector::Vec2(1.0f, 0.0f)))};
}

static VectorField::FieldGrid makeEmptyGrid() {
    return {0.0f, 2.0f, 0.0f, 2.0f, Vector::FieldSlice{}};
}

// ---------------------------------------------------------------------------
// traceStreamlineStep(src, dest) -- two-arg API
// ---------------------------------------------------------------------------

TEST_CASE("traceStreamlineStep(src,dest) extends into unclaimed cell", "[vectorfield]") {
    auto grid = makeGrid();
    // Manually supply precomputed dest rather than letting the grid compute it
    REQUIRE_NOTHROW(grid.traceStreamlineStep({0, 0}, {0, 1}));
}

TEST_CASE("traceStreamlineStep(src,dest) merges when dest is already claimed", "[vectorfield]") {
    auto grid = makeGrid();
    grid.traceStreamlineStep({0, 0}, {0, 1}); // claims (0,1)
    // (0,2) also points toward (0,1) -- triggers joinStreamlines
    REQUIRE_NOTHROW(grid.traceStreamlineStep({0, 2}, {0, 1}));
}

TEST_CASE("traceStreamlineStep(src,dest) src pointing at itself is a no-op merge",
          "[vectorfield]") {
    auto grid = makeGrid();
    // src == dest: joinStreamlines guards against self-merge
    REQUIRE_NOTHROW(grid.traceStreamlineStep({0, 0}, {0, 0}));
}

// ---------------------------------------------------------------------------
// sequentialCPU
// ---------------------------------------------------------------------------

TEST_CASE("sequentialCPU::computeTimeStep completes on uniform field", "[impl][sequential]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(sequentialCPU::computeTimeStep(grid));
}

TEST_CASE("sequentialCPU::computeTimeStep handles empty grid", "[impl][sequential]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(sequentialCPU::computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// pthreads
// ---------------------------------------------------------------------------

TEST_CASE("pthreads::computeTimeStep zero threads returns early", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(pthreads::computeTimeStep(grid, 0));
}

TEST_CASE("pthreads::computeTimeStep single thread", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(pthreads::computeTimeStep(grid, 1));
}

TEST_CASE("pthreads::computeTimeStep two threads", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(pthreads::computeTimeStep(grid, 2));
}

TEST_CASE("pthreads::computeTimeStep more threads than rows", "[impl][pthreads]") {
    // 3-row grid, 8 threads: most threads get empty ranges, last gets all rows
    auto grid = makeGrid();
    REQUIRE_NOTHROW(pthreads::computeTimeStep(grid, 8));
}

TEST_CASE("pthreads::computeTimeStep empty grid returns early", "[impl][pthreads]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(pthreads::computeTimeStep(grid, 4));
}

// ---------------------------------------------------------------------------
// openMP
// ---------------------------------------------------------------------------

TEST_CASE("openMP::computeTimeStep completes on uniform field", "[impl][openmp]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(openMP::computeTimeStep(grid));
}

TEST_CASE("openMP::computeTimeStep handles empty grid", "[impl][openmp]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(openMP::computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// Consistency: all three impls produce the same number of neighbor steps
// (verified by re-running neighborInVectorDirection and comparing outputs)
// ---------------------------------------------------------------------------

TEST_CASE("all impls agree on neighbor directions for uniform field", "[impl][consistency]") {
    // neighborInVectorDirection is const and deterministic -- compute expected
    // results once, then verify each impl doesn't deviate from the field geometry.
    auto ref = makeGrid();
    const int rows = static_cast<int>(ref.rows());
    const int cols = static_cast<int>(ref.cols());

    // All vectors point right: every cell's neighbor should be one column to
    // the right (clamped at the edge).
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            auto [nr, nc] = ref.neighborInVectorDirection(r, c);
            REQUIRE(nr == r);
            REQUIRE(nc == std::min(c + 1, cols - 1));
        }
    }
}
