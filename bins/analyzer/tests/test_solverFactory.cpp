#include "mpiStreamlineSolver.hpp"
#include "openMpStreamlineSolver.hpp"
#include "pthreadsStreamlineSolver.hpp"
#include "sequentialStreamlineSolver.hpp"
#include "solverFactory.hpp"

#ifdef ENABLE_CUDA_SOLVER
#include "cudaMpiStreamlineSolver.hpp"
#include "cudaStreamlineSolver.hpp"
#endif

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Known solver names return the expected concrete type
// ---------------------------------------------------------------------------

TEST_CASE("makeSolver(\"sequential\") returns SequentialStreamlineSolver", "[factory]") {
    auto solver = makeSolver("sequential", 0, 256);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<SequentialStreamlineSolver*>(solver.get()) != nullptr);
}

TEST_CASE("makeSolver(\"openmp\") returns OpenMpStreamlineSolver", "[factory]") {
    auto solver = makeSolver("openmp", 0, 256);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<OpenMpStreamlineSolver*>(solver.get()) != nullptr);
}

TEST_CASE("makeSolver(\"pthreads\") returns PthreadsStreamlineSolver", "[factory]") {
    auto solver = makeSolver("pthreads", 4, 256);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<PthreadsStreamlineSolver*>(solver.get()) != nullptr);
}

TEST_CASE("makeSolver(\"mpi\") returns MpiStreamlineSolver", "[factory]") {
    auto solver = makeSolver("mpi", 0, 256);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<MpiStreamlineSolver*>(solver.get()) != nullptr);
}

#ifdef ENABLE_CUDA_SOLVER
TEST_CASE("makeSolver(\"cuda\") returns CudaStreamlineSolver", "[factory]") {
    auto solver = makeSolver("cuda", 0, 256);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<CudaStreamlineSolver*>(solver.get()) != nullptr);
}

TEST_CASE("makeSolver(\"cudaMpi\") returns CudaMpiStreamlineSolver", "[factory]") {
    auto solver = makeSolver("cudaMpi", 0, 256);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<CudaMpiStreamlineSolver*>(solver.get()) != nullptr);
}
#endif

// ---------------------------------------------------------------------------
// Unknown names throw with a stable message
// ---------------------------------------------------------------------------

TEST_CASE("makeSolver throws std::runtime_error for unknown name", "[factory]") {
    REQUIRE_THROWS_AS(makeSolver("gpu", 0, 256), std::runtime_error);
}

TEST_CASE("makeSolver error message contains the unknown name", "[factory]") {
    try {
        (void)makeSolver("bogus", 0, 256);
        FAIL("expected exception");
    } catch (const std::runtime_error& ex) {
        REQUIRE(std::string(ex.what()).find("bogus") != std::string::npos);
    }
}
