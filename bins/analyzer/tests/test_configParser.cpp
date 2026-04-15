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

    REQUIRE(cfg.solver == "all");
    REQUIRE(cfg.threadCount == 0);
}

TEST_CASE("parseAnalyzer reads solver and threads from [analyzer]", "[config]") {
    TempFile tmp("[analyzer]\nsolver = \"sequential\"\nthreads = 4\n");
    const auto cfg = ConfigParser::parseAnalyzer(tmp.path.string());

    REQUIRE(cfg.solver == "sequential");
    REQUIRE(cfg.threadCount == 4);
}

TEST_CASE("parseAnalyzer accepts all valid solver names", "[config]") {
    for (const auto* solver : {"sequential", "openmp", "pthreads", "mpi", "all"}) {
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
