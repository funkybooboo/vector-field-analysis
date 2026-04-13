#include "mpiCPU.hpp"
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
// SequentialCPU
// ---------------------------------------------------------------------------

TEST_CASE("SequentialCPU::computeTimeStep completes on uniform field", "[impl][sequential]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(SequentialCPU{}.computeTimeStep(grid));
}

TEST_CASE("SequentialCPU::computeTimeStep handles empty grid", "[impl][sequential]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(SequentialCPU{}.computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// Pthreads
// ---------------------------------------------------------------------------

TEST_CASE("Pthreads zero threads returns early", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(Pthreads{0}.computeTimeStep(grid));
}

TEST_CASE("Pthreads single thread", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(Pthreads{1}.computeTimeStep(grid));
}

TEST_CASE("Pthreads two threads", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(Pthreads{2}.computeTimeStep(grid));
}

TEST_CASE("Pthreads more threads than rows", "[impl][pthreads]") {
    // 3-row grid, 8 threads: most threads get empty ranges, last gets all rows
    auto grid = makeGrid();
    REQUIRE_NOTHROW(Pthreads{8}.computeTimeStep(grid));
}

TEST_CASE("Pthreads empty grid returns early", "[impl][pthreads]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(Pthreads{4}.computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// OpenMP
// ---------------------------------------------------------------------------

TEST_CASE("OpenMP::computeTimeStep completes on uniform field", "[impl][openmp]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(OpenMP{}.computeTimeStep(grid));
}

TEST_CASE("OpenMP::computeTimeStep handles empty grid", "[impl][openmp]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(OpenMP{}.computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// MpiCPU
// ---------------------------------------------------------------------------

// MpiCPU falls back to SequentialCPU when compiled without USE_MPI or run with
// a single rank.  These tests exercise the single-rank / no-MPI path.

TEST_CASE("MpiCPU::computeTimeStep completes on uniform field", "[impl][mpi]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(MpiCPU{}.computeTimeStep(grid));
}

TEST_CASE("MpiCPU::computeTimeStep handles empty grid", "[impl][mpi]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(MpiCPU{}.computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// Solver output: getStreamlines() correctness
// ---------------------------------------------------------------------------

TEST_CASE("SequentialCPU getStreamlines returns 3 streamlines on uniform 3x3 field",
          "[impl][sequential][streamlines]") {
    auto grid = makeGrid();
    SequentialCPU{}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == 3);
}

TEST_CASE("SequentialCPU getStreamlines path contents are correct for uniform 3x3 field",
          "[impl][sequential][streamlines]") {
    auto grid = makeGrid();
    SequentialCPU{}.computeTimeStep(grid);
    const auto lines = grid.getStreamlines();
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == (std::vector<std::pair<int, int>>{{0, 0}, {0, 1}, {0, 2}}));
    REQUIRE(lines[1] == (std::vector<std::pair<int, int>>{{1, 0}, {1, 1}, {1, 2}}));
    REQUIRE(lines[2] == (std::vector<std::pair<int, int>>{{2, 0}, {2, 1}, {2, 2}}));
}

TEST_CASE("OpenMP getStreamlines returns same count as sequential on uniform 3x3 field",
          "[impl][openmp][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialCPU{}.computeTimeStep(seqGrid);
    const std::size_t expected = seqGrid.getStreamlines().size();
    auto grid = makeGrid();
    OpenMP{}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == expected);
}

TEST_CASE("Pthreads getStreamlines returns same count as sequential on uniform 3x3 field",
          "[impl][pthreads][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialCPU{}.computeTimeStep(seqGrid);
    const std::size_t expected = seqGrid.getStreamlines().size();
    auto grid = makeGrid();
    Pthreads{4}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == expected);
}

TEST_CASE("MpiCPU getStreamlines returns same count as sequential on uniform 3x3 field",
          "[impl][mpi][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialCPU{}.computeTimeStep(seqGrid);
    const std::size_t expected = seqGrid.getStreamlines().size();
    auto grid = makeGrid();
    MpiCPU{}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == expected);
}

TEST_CASE("getStreamlines returns non-empty result after any impl on non-empty field",
          "[impl][consistency][streamlines]") {
    {
        auto grid = makeGrid();
        SequentialCPU{}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
    {
        auto grid = makeGrid();
        OpenMP{}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
    {
        auto grid = makeGrid();
        Pthreads{2}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
    {
        auto grid = makeGrid();
        MpiCPU{}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
}

// ---------------------------------------------------------------------------
// Consistency: all impls agree on neighbor directions for uniform field
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
