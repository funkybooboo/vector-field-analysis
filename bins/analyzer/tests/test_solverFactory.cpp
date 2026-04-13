#include "mpiCPU.hpp"
#include "openMP.hpp"
#include "pthreads.hpp"
#include "sequentialCPU.hpp"
#include "solverFactory.hpp"

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Known solver names return the expected concrete type
// ---------------------------------------------------------------------------

TEST_CASE("makeSolver(\"sequential\") returns SequentialCPU", "[factory]") {
    auto solver = makeSolver("sequential", 0);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<SequentialCPU*>(solver.get()) != nullptr);
}

TEST_CASE("makeSolver(\"openmp\") returns OpenMP", "[factory]") {
    auto solver = makeSolver("openmp", 0);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<OpenMP*>(solver.get()) != nullptr);
}

TEST_CASE("makeSolver(\"pthreads\") returns Pthreads", "[factory]") {
    auto solver = makeSolver("pthreads", 4);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<Pthreads*>(solver.get()) != nullptr);
}

TEST_CASE("makeSolver(\"mpi\") returns MpiCPU", "[factory]") {
    auto solver = makeSolver("mpi", 0);
    REQUIRE(solver != nullptr);
    REQUIRE(dynamic_cast<MpiCPU*>(solver.get()) != nullptr);
}

// ---------------------------------------------------------------------------
// Unknown names throw with a stable message
// ---------------------------------------------------------------------------

TEST_CASE("makeSolver throws std::runtime_error for unknown name", "[factory]") {
    REQUIRE_THROWS_AS(makeSolver("gpu", 0), std::runtime_error);
}

TEST_CASE("makeSolver error message contains the unknown name", "[factory]") {
    try {
        (void)makeSolver("bogus", 0);
        FAIL("expected exception");
    } catch (const std::runtime_error& ex) {
        REQUIRE(std::string(ex.what()).find("bogus") != std::string::npos);
    }
}
