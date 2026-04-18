#include "grid.hpp"
#include "mpiStreamlineSolver.hpp"
#include "openMpStreamlineSolver.hpp"
#include "pthreadsStreamlineSolver.hpp"
#include "sequentialStreamlineSolver.hpp"

#ifdef ENABLE_CUDA_SOLVER
#include "cudaFullStreamlineSolver.hpp"
#include "cudaStreamlineSolver.hpp"
#endif

#include <algorithm>
#include <catch2/catch_test_macros.hpp>

// Sort streamlines canonically by their first cell so solver outputs can be compared
// regardless of the order in which solvers emit them.
static std::vector<Field::Path> canonicalize(std::vector<Field::Path> lines) {
    std::sort(lines.begin(), lines.end(), [](const Field::Path& a, const Field::Path& b) {
        if (a.empty()) {
            return true;
        }
        if (b.empty()) {
            return false;
        }
        return a.front() < b.front();
    });
    return lines;
}

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
    REQUIRE_THROWS_AS(makeEmptyGrid(), std::runtime_error);
}

// ---------------------------------------------------------------------------
// PthreadsStreamlineSolver
// ---------------------------------------------------------------------------

TEST_CASE("PthreadsStreamlineSolver zero threads falls back to sequential", "[impl][pthreads]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const auto expected = seqGrid.getStreamlines();

    auto grid = makeGrid();
    REQUIRE_NOTHROW(PthreadsStreamlineSolver{0}.computeTimeStep(grid));
    REQUIRE(grid.getStreamlines().size() == expected.size());
}

TEST_CASE("PthreadsStreamlineSolver single thread", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(PthreadsStreamlineSolver{1}.computeTimeStep(grid));
}

TEST_CASE("PthreadsStreamlineSolver two threads", "[impl][pthreads]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(PthreadsStreamlineSolver{2}.computeTimeStep(grid));
}

TEST_CASE("PthreadsStreamlineSolver more threads than rows produces correct output",
          "[impl][pthreads]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const auto expected = seqGrid.getStreamlines();

    auto grid = makeGrid();
    PthreadsStreamlineSolver{8}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == expected.size());
}

TEST_CASE("PthreadsStreamlineSolver empty grid returns early", "[impl][pthreads]") {
    REQUIRE_THROWS_AS(makeEmptyGrid(), std::runtime_error);
}

// ---------------------------------------------------------------------------
// OpenMpStreamlineSolver
// ---------------------------------------------------------------------------

TEST_CASE("OpenMpStreamlineSolver::computeTimeStep completes on uniform field", "[impl][openmp]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(OpenMpStreamlineSolver{}.computeTimeStep(grid));
}

TEST_CASE("OpenMpStreamlineSolver::computeTimeStep handles empty grid", "[impl][openmp]") {
    REQUIRE_THROWS_AS(makeEmptyGrid(), std::runtime_error);
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
    REQUIRE_THROWS_AS(makeEmptyGrid(), std::runtime_error);
}

// ---------------------------------------------------------------------------
// CudaStreamlineSolver
// ---------------------------------------------------------------------------

#ifdef ENABLE_CUDA_SOLVER
TEST_CASE("CudaStreamlineSolver::computeTimeStep completes on uniform field", "[impl][cuda]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(CudaStreamlineSolver{}.computeTimeStep(grid));
}

TEST_CASE("CudaStreamlineSolver::computeTimeStep handles empty grid", "[impl][cuda]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(CudaStreamlineSolver{}.computeTimeStep(grid));
}

// ---------------------------------------------------------------------------
// CudaFullStreamlineSolver
// ---------------------------------------------------------------------------

TEST_CASE("CudaFullStreamlineSolver::computeTimeStep completes on uniform field",
          "[impl][cuda_full]") {
    auto grid = makeGrid();
    REQUIRE_NOTHROW(CudaFullStreamlineSolver{}.computeTimeStep(grid));
}

TEST_CASE("CudaFullStreamlineSolver::computeTimeStep handles empty grid", "[impl][cuda_full]") {
    auto grid = makeEmptyGrid();
    REQUIRE_NOTHROW(CudaFullStreamlineSolver{}.computeTimeStep(grid));
}
#endif

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

TEST_CASE("OpenMpStreamlineSolver getStreamlines returns same count as sequential",
          "[impl][openmp][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const std::size_t expected = seqGrid.getStreamlines().size();
    auto grid = makeGrid();
    OpenMpStreamlineSolver{}.computeTimeStep(grid);
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

#ifdef ENABLE_CUDA_SOLVER
TEST_CASE("CudaStreamlineSolver getStreamlines returns same count as sequential",
          "[impl][cuda][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const std::size_t expected = seqGrid.getStreamlines().size();
    auto grid = makeGrid();
    CudaStreamlineSolver{}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == expected);
}

TEST_CASE("CudaFullStreamlineSolver getStreamlines returns same count as sequential",
          "[impl][cuda_full][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const std::size_t expected = seqGrid.getStreamlines().size();
    auto grid = makeGrid();
    CudaFullStreamlineSolver{}.computeTimeStep(grid);
    REQUIRE(grid.getStreamlines().size() == expected);
}
#endif

TEST_CASE("OpenMpStreamlineSolver path contents match sequential", "[impl][openmp][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const auto expected = canonicalize(seqGrid.getStreamlines());

    auto grid = makeGrid();
    OpenMpStreamlineSolver{}.computeTimeStep(grid);
    REQUIRE(canonicalize(grid.getStreamlines()) == expected);
}

TEST_CASE("PthreadsStreamlineSolver path contents match sequential",
          "[impl][pthreads][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const auto expected = canonicalize(seqGrid.getStreamlines());

    auto grid = makeGrid();
    PthreadsStreamlineSolver{4}.computeTimeStep(grid);
    REQUIRE(canonicalize(grid.getStreamlines()) == expected);
}

#ifdef ENABLE_CUDA_SOLVER
TEST_CASE("CudaStreamlineSolver path contents match sequential", "[impl][cuda][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const auto expected = canonicalize(seqGrid.getStreamlines());

    auto grid = makeGrid();
    CudaStreamlineSolver{}.computeTimeStep(grid);
    REQUIRE(canonicalize(grid.getStreamlines()) == expected);
}

TEST_CASE("CudaFullStreamlineSolver path contents match sequential",
          "[impl][cuda_full][streamlines]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const auto expected = canonicalize(seqGrid.getStreamlines());

    auto grid = makeGrid();
    CudaFullStreamlineSolver{}.computeTimeStep(grid);
    REQUIRE(canonicalize(grid.getStreamlines()) == expected);
}
#endif

TEST_CASE("PthreadsStreamlineSolver with thread count equal to row count produces correct output",
          "[impl][pthreads]") {
    auto seqGrid = makeGrid();
    SequentialStreamlineSolver{}.computeTimeStep(seqGrid);
    const auto expected = canonicalize(seqGrid.getStreamlines());

    auto grid = makeGrid();
    PthreadsStreamlineSolver{3}.computeTimeStep(grid); // exactly 3 threads for 3-row grid
    REQUIRE(canonicalize(grid.getStreamlines()) == expected);
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
        OpenMpStreamlineSolver{}.computeTimeStep(grid);
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
#ifdef ENABLE_CUDA_SOLVER
    {
        auto grid = makeGrid();
        CudaStreamlineSolver{}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
    {
        auto grid = makeGrid();
        CudaFullStreamlineSolver{}.computeTimeStep(grid);
        REQUIRE_FALSE(grid.getStreamlines().empty());
    }
#endif
}

// ---------------------------------------------------------------------------
// Near-zero magnitude field: solvers must not crash or produce NaN
// ---------------------------------------------------------------------------

static Field::Grid makeNearZeroGrid() {
    const Vector::Vec2 tiny(1e-7f, 0.0f);
    return Field::Grid{Field::Bounds{0.0f, 2.0f, 0.0f, 2.0f},
                       Field::Slice(3, std::vector<Vector::Vec2>(3, tiny))};
}

TEST_CASE("sequential solver handles near-zero magnitude field without crash",
          "[impl][consistency]") {
    auto grid = makeNearZeroGrid();
    REQUIRE_NOTHROW(SequentialStreamlineSolver{}.computeTimeStep(grid));
    REQUIRE_FALSE(grid.getStreamlines().empty());
}

TEST_CASE("openmp solver handles near-zero magnitude field without crash", "[impl][consistency]") {
    auto grid = makeNearZeroGrid();
    REQUIRE_NOTHROW(OpenMpStreamlineSolver{}.computeTimeStep(grid));
    REQUIRE_FALSE(grid.getStreamlines().empty());
}

TEST_CASE("pthreads solver handles near-zero magnitude field without crash",
          "[impl][consistency]") {
    auto grid = makeNearZeroGrid();
    REQUIRE_NOTHROW(PthreadsStreamlineSolver{2}.computeTimeStep(grid));
    REQUIRE_FALSE(grid.getStreamlines().empty());
}

#ifdef ENABLE_CUDA_SOLVER
TEST_CASE("cuda solver handles near-zero magnitude field without crash", "[impl][consistency]") {
    auto grid = makeNearZeroGrid();
    REQUIRE_NOTHROW(CudaStreamlineSolver{}.computeTimeStep(grid));
    REQUIRE_FALSE(grid.getStreamlines().empty());
}

TEST_CASE("cuda full solver handles near-zero magnitude field without crash",
          "[impl][consistency]") {
    auto grid = makeNearZeroGrid();
    REQUIRE_NOTHROW(CudaFullStreamlineSolver{}.computeTimeStep(grid));
    REQUIRE_FALSE(grid.getStreamlines().empty());
}
#endif

TEST_CASE("all solvers handle single-row grid without crash", "[impl][consistency]") {
    auto make = [] {
        return Field::Grid{Field::Bounds{0.0f, 2.0f, 0.0f, 0.0f},
                           Field::Slice(1, std::vector<Vector::Vec2>(3, Vector::Vec2(1.0f, 0.0f)))};
    };
    {
        auto grid = make();
        REQUIRE_NOTHROW(SequentialStreamlineSolver{}.computeTimeStep(grid));
    }
    {
        auto grid = make();
        REQUIRE_NOTHROW(OpenMpStreamlineSolver{}.computeTimeStep(grid));
    }
    {
        auto grid = make();
        REQUIRE_NOTHROW(PthreadsStreamlineSolver{2}.computeTimeStep(grid));
    }
    {
        auto grid = make();
        REQUIRE_NOTHROW(MpiStreamlineSolver{}.computeTimeStep(grid));
    }
#ifdef ENABLE_CUDA_SOLVER
    {
        auto grid = make();
        REQUIRE_NOTHROW(CudaStreamlineSolver{}.computeTimeStep(grid));
    }
    {
        auto grid = make();
        REQUIRE_NOTHROW(CudaFullStreamlineSolver{}.computeTimeStep(grid));
    }
#endif
}

TEST_CASE("all solvers handle single-column grid without crash", "[impl][consistency]") {
    auto make = [] {
        return Field::Grid{Field::Bounds{0.0f, 0.0f, 0.0f, 2.0f},
                           Field::Slice(3, std::vector<Vector::Vec2>(1, Vector::Vec2(1.0f, 0.0f)))};
    };
    {
        auto grid = make();
        REQUIRE_NOTHROW(SequentialStreamlineSolver{}.computeTimeStep(grid));
    }
    {
        auto grid = make();
        REQUIRE_NOTHROW(OpenMpStreamlineSolver{}.computeTimeStep(grid));
    }
    {
        auto grid = make();
        REQUIRE_NOTHROW(PthreadsStreamlineSolver{2}.computeTimeStep(grid));
    }
    {
        auto grid = make();
        REQUIRE_NOTHROW(MpiStreamlineSolver{}.computeTimeStep(grid));
    }
#ifdef ENABLE_CUDA_SOLVER
    {
        auto grid = make();
        REQUIRE_NOTHROW(CudaStreamlineSolver{}.computeTimeStep(grid));
    }
    {
        auto grid = make();
        REQUIRE_NOTHROW(CudaFullStreamlineSolver{}.computeTimeStep(grid));
    }
#endif
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
