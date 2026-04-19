#include "configParser.hpp"
#include "tempFile.hpp"

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// Valid configs
// ---------------------------------------------------------------------------

TEST_CASE("parseAnalyzer returns defaults when [analyzer] table is absent", "[config]") {
    TempFile tmp("# empty\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());

    REQUIRE(cfg.solver == "benchmark");
    REQUIRE(cfg.threadCount == 0);
}

TEST_CASE("parseAnalyzer reads solver and threads from [analyzer]", "[config]") {
    TempFile tmp("[analyzer]\nsolver = \"sequential\"\nthreads = 4\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());

    REQUIRE(cfg.solver == "sequential");
    REQUIRE(cfg.threadCount == 4);
}

TEST_CASE("parseAnalyzer accepts all valid solver names", "[config]") {
    for (const auto* solver : {"sequential", "openmp", "pthreads", "mpi", "hybrid", "all"}) {
        TempFile tmp(std::string("[analyzer]\nsolver = \"") + solver + "\"\n");
        REQUIRE_NOTHROW(ConfigParser::parseAnalyzer(tmp.path.string()));
    }
}

TEST_CASE("parseAnalyzer accepts threads = 0", "[config]") {
    TempFile tmp("[analyzer]\nthreads = 0\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());
    REQUIRE(cfg.threadCount == 0);
}

TEST_CASE("parseAnalyzer returns default solver when [analyzer] present but solver absent",
          "[config]") {
    TempFile tmp("[analyzer]\nthreads = 2\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());
    REQUIRE(cfg.solver == "all");
}

TEST_CASE("parseAnalyzer returns default threadCount when [analyzer] present but threads absent",
          "[config]") {
    TempFile tmp("[analyzer]\nsolver = \"sequential\"\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());
    REQUIRE(cfg.threadCount == 0);
}

TEST_CASE("parseAnalyzer error message for unknown solver contains the bad name", "[config]") {
    TempFile tmp("[analyzer]\nsolver = \"badname\"\n");
    try {
        ConfigParser::parseAnalyzer(tmp.path.string());
        FAIL("expected exception");
    } catch (const std::runtime_error& ex) {
        REQUIRE(std::string(ex.what()).find("badname") != std::string::npos);
    }
}

// ---------------------------------------------------------------------------
// Invalid configs
// ---------------------------------------------------------------------------

TEST_CASE("parseAnalyzer throws on unknown solver name", "[config]") {
    TempFile tmp("[analyzer]\nsolver = \"gpu\"\n");
    REQUIRE_THROWS_AS(ConfigParser::parseAnalyzer(tmp.path.string()), std::runtime_error);
}

TEST_CASE("parseAnalyzer throws on negative threads", "[config]") {
    TempFile tmp("[analyzer]\nthreads = -1\n");
    REQUIRE_THROWS_AS(ConfigParser::parseAnalyzer(tmp.path.string()), std::runtime_error);
}

TEST_CASE("parseAnalyzer throws on threads exceeding UINT_MAX", "[config]") {
    TempFile tmp("[analyzer]\nthreads = 4294967296\n"); // UINT_MAX + 1
    REQUIRE_THROWS_AS(ConfigParser::parseAnalyzer(tmp.path.string()), std::runtime_error);
}

TEST_CASE("parseAnalyzer throws on float threads value", "[config]") {
    TempFile tmp("[analyzer]\nthreads = 4.5\n");
    REQUIRE_THROWS_AS(ConfigParser::parseAnalyzer(tmp.path.string()), std::runtime_error);
}

TEST_CASE("parseAnalyzer throws on missing file", "[config]") {
    REQUIRE_THROWS(ConfigParser::parseAnalyzer("/tmp/does_not_exist_analyzer_test.toml"));
}

// ---------------------------------------------------------------------------
// Block size defaults and parsing
// ---------------------------------------------------------------------------

TEST_CASE("parseAnalyzer returns default cudaBlockSize 256 when key absent", "[config]") {
    TempFile tmp("# empty\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());
    REQUIRE(cfg.cudaBlockSize == 256);
}

TEST_CASE("parseAnalyzer reads cuda_block_size", "[config]") {
    TempFile tmp("[analyzer]\ncuda_block_size = 512\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());
    REQUIRE(cfg.cudaBlockSize == 512);
}

TEST_CASE("parseAnalyzer accepts cuda_block_size = 1 (lower boundary)", "[config]") {
    TempFile tmp("[analyzer]\ncuda_block_size = 1\n");
    REQUIRE_NOTHROW(ConfigParser::parseAnalyzer(tmp.path.string()));
}

TEST_CASE("parseAnalyzer accepts cuda_block_size = 1024 (upper boundary)", "[config]") {
    TempFile tmp("[analyzer]\ncuda_block_size = 1024\n");
    REQUIRE_NOTHROW(ConfigParser::parseAnalyzer(tmp.path.string()));
}

TEST_CASE("parseAnalyzer throws on cuda_block_size = 0", "[config]") {
    TempFile tmp("[analyzer]\ncuda_block_size = 0\n");
    REQUIRE_THROWS_AS(ConfigParser::parseAnalyzer(tmp.path.string()), std::runtime_error);
}

TEST_CASE("parseAnalyzer throws on cuda_block_size = 1025", "[config]") {
    TempFile tmp("[analyzer]\ncuda_block_size = 1025\n");
    REQUIRE_THROWS_AS(ConfigParser::parseAnalyzer(tmp.path.string()), std::runtime_error);
}

TEST_CASE("parseAnalyzer throws on float cuda_block_size", "[config]") {
    TempFile tmp("[analyzer]\ncuda_block_size = 4.5\n");
    REQUIRE_THROWS_AS(ConfigParser::parseAnalyzer(tmp.path.string()), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Output field
// ---------------------------------------------------------------------------

TEST_CASE("parseAnalyzer reads output path", "[config]") {
    TempFile tmp("[analyzer]\noutput = \"results/out.h5\"\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());
    REQUIRE(cfg.output == "results/out.h5");
}

TEST_CASE("parseAnalyzer output defaults to empty string when absent", "[config]") {
    TempFile tmp("# empty\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());
    REQUIRE(cfg.output.empty());
}
