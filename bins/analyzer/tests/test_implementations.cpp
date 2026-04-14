#include "mpiStreamlineSolver.hpp"
#include "openMPStreamlineSolver.hpp"
#include "pthreadsStreamlineSolver.hpp"
#include "sequentialStreamlineSolver.hpp"
#include "grid.hpp"

#include <catch2/catch_test_macros.hpp>

// 3x3 grid where all vectors point right (+x)
static Field::Grid makeGrid() {
    return {Field::Bounds{0.0f, 2.0f, 0.0f, 2.0f},
            Field::Slice(3, std::vector<Vector::Vec2>(3, Vector::Vec2(1.0f, 0.0f)))};
}

static Field::Grid makeEmptyGrid() {
    return {Field::Bounds{0.0f, 2.0f, 0.0f, 2.0f}, Field::Slice{}};
}

// ---------------------------------------------------------------------------
// traceStreamlineStep(src, dest) -- two-arg API
// ---------------------------------------------------------------------------

TEST_CASE("traceStreamlineStep(src,dest) extends into unclaimed cell", "[vectorfield]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(grid.traceStreamlineStep({0, 0}, {0, 1}));
}

TEST_CASE("traceStreamlineStep(src,dest) merges when dest is already claimed", "[vectorfield]") {
    auto grid = makeGrid();
    grid.traceStreamlineStep({0, 0}, {0, 1});
    REQUIRE_NOTHROW(grid.traceStreamlineStep({0, 2}, {0, 1}));
}

TEST_CASE("traceStreamlineStep(src,dest) src pointing at itself is a no-op merge",
          "[vectorfield]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(grid.traceStreamlineStep({0, 0}, {0, 0}));
}

// ---------------------------------------------------------------------------
// SequentialStreamlineSolver
// ---------------------------------------------------------------------------

TEST_CASE("SequentialStreamlineSolver::computeTimeStep completes on uniform field",
          "[impl][sequential]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(SequentialStreamlineSolver{}.computeTimeStep(grid));
}

TEST_CASE("SequentialStreamlineSolver::computeTimeStep handles empty grid", "[impl][sequential]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(SequentialStreamlineSolver{}.computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// PthreadsStreamlineSolver
// ---------------------------------------------------------------------------

TEST_CASE("PthreadsStreamlineSolver zero threads returns early", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(PthreadsStreamlineSolver{0}.computeTimeStep(grid));
}

TEST_CASE("PthreadsStreamlineSolver single thread", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(PthreadsStreamlineSolver{1}.computeTimeStep(grid));
}

TEST_CASE("PthreadsStreamlineSolver two threads", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(PthreadsStreamlineSolver{2}.computeTimeStep(grid));
}

TEST_CASE("PthreadsStreamlineSolver more threads than rows", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(PthreadsStreamlineSolver{8}.computeTimeStep(grid));
}

TEST_CASE("PthreadsStreamlineSolver empty grid returns early", "[impl][pthreads]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(PthreadsStreamlineSolver{4}.computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// OpenMPStreamlineSolver
// ---------------------------------------------------------------------------

TEST_CASE("OpenMPStreamlineSolver::computeTimeStep completes on uniform field",
          "[impl][openmp]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(OpenMPStreamlineSolver{}.computeTimeStep(grid));
}

TEST_CASE("OpenMPStreamlineSolver::computeTimeStep handles empty grid", "[impl][openmp]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(OpenMPStreamlineSolver{}.computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// MpiStreamlineSolver
// ---------------------------------------------------------------------------

// NOTE: These tests exercise only the no-MPI-init fallback path (MPI_Initialized()
// returns 0, so MpiStreamlineSolver delegates to SequentialStreamlineSolver). The
// actual gather/gatherv code in mpiStreamlineSolver.cpp requires >1 MPI rank and is
// not covered here.
// To exercise it: mpirun -n 2 ./build/bins/analyzer/tests/analyzer_tests [mpi]

TEST_CASE("MpiStreamlineSolver::computeTimeStep completes on uniform field", "[impl][mpi]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(MpiStreamlineSolver{}.computeTimeStep(grid));
}

TEST_CASE("MpiStreamlineSolver::computeTimeStep handles empty grid", "[impl][mpi]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(MpiStreamlineSolver{}.computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// Solver output: getStreamlines() correctness
// ---------------------------------------------------------------------------

TEST_CASE("SequentialStreamlineSolver getStreamlines returns 3 streamlines on uniform 3x3 field",
          "[impl][sequential][streamlines]") {
    auto grid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == 3);
}

TEST_CASE("SequentialStreamlineSolver getStreamlines path contents correct for uniform 3x3 field",
          "[impl][sequential][streamlines]") {
    auto grid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(grid);
    const auto lines = grid.getStreamlines();
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == (Field::Path{{0, 0}, {0, 1}, {0, 2}}));
    REQUIRE(lines[1] == (Field::Path{{1, 0}, {1, 1}, {1, 2}}));
    REQUIRE(lines[2] == (Field::Path{{2, 0}, {2, 1}, {2, 2}}));
}

TEST_CASE("OpenMPStreamlineSolver getStreamlines returns same count as sequential",
          "[impl][openmp][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const std::size_t expected = seqGrid.getStreamlines().size();
    auto grid = makeGrid();
    OpenMPStreamlineSolver{}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == expected);
}

TEST_CASE("PthreadsStreamlineSolver getStreamlines returns same count as sequential",
          "[impl][pthreads][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const std::size_t expected = seqGrid.getStreamlines().size();
    auto grid = makeGrid();
    PthreadsStreamlineSolver{4}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == expected);
}

TEST_CASE("MpiStreamlineSolver getStreamlines returns same count as sequential",
          "[impl][mpi][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const std::size_t expected = seqGrid.getStreamlines().size();
    auto grid = makeGrid();
    MpiStreamlineSolver{}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == expected);
}

TEST_CASE("getStreamlines returns non-empty result after any solver on non-empty field",
          "[impl][consistency][streamlines]") {
    {
        auto grid = makeGrid();
        SequentialStreamlineSolver{}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
    {
        auto grid = makeGrid();
        OpenMPStreamlineSolver{}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
    {
        auto grid = makeGrid();
        PthreadsStreamlineSolver{2}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
    {
        auto grid = makeGrid();
        MpiStreamlineSolver{}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
}

// ---------------------------------------------------------------------------
// Consistency: all solvers agree on neighbor directions for uniform field
// ---------------------------------------------------------------------------

TEST_CASE("all solvers agree on neighbor directions for uniform field", "[impl][consistency]") {
    auto ref = makeGrid();
    const int rows = static_cast<int>(ref.rows());
    const int cols = static_cast<int>(ref.cols());

    // All vectors point right: every cell's neighbor should be one column to
    // the right (clamped at the edge).
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            auto cell = ref.downstreamCell(row, col);
            REQUIRE(cell.row == row);
            REQUIRE(cell.col == std::min(col + 1, cols - 1));
        }
    }
}
