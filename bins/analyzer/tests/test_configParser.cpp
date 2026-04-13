#include "analyzerConfigParser.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

// RAII wrapper: creates a temp file with the given content, removes it on destruction.
// Guarantees cleanup even when the test body throws.
struct TmpFile {
    std::string path;
    explicit TmpFile(const std::string& content) {
        path = "/tmp/analyzer_test_XXXXXX";
        // std::string::data() is non-const in C++17, serving as the mutable buffer mkstemp needs.
        int fd = mkstemp(path.data());
        if (fd == -1) {
            throw std::runtime_error("mkstemp failed");
        }
        close(fd);
        std::ofstream out(path);
        out << content;
    }
    ~TmpFile() { std::remove(path.c_str()); }
    TmpFile(const TmpFile&) = delete;
    TmpFile& operator=(const TmpFile&) = delete;
};

// ---------------------------------------------------------------------------
// Valid configs
// ---------------------------------------------------------------------------

TEST_CASE("parseFile returns defaults when [analyzer] table is absent", "[config]") {
    TmpFile tmp("# empty\n");
    const auto cfg = AnalyzerConfigParser::parseFile(tmp.path);

    REQUIRE(cfg.inputPath == "field.h5");
    REQUIRE(cfg.solver == "all");
    REQUIRE(cfg.threadCount == 0);
}

TEST_CASE("parseFile reads input, solver, and threads from [analyzer]", "[config]") {
    TmpFile tmp("[analyzer]\ninput = \"data.h5\"\nsolver = \"sequential\"\nthreads = 4\n");
    const auto cfg = AnalyzerConfigParser::parseFile(tmp.path);

    REQUIRE(cfg.inputPath == "data.h5");
    REQUIRE(cfg.solver == "sequential");
    REQUIRE(cfg.threadCount == 4);
}

TEST_CASE("parseFile accepts all valid solver names", "[config]") {
    for (const auto* solver : {"sequential", "openmp", "pthreads", "mpi", "all"}) {
        TmpFile tmp(std::string("[analyzer]\nsolver = \"") + solver + "\"\n");
        REQUIRE_NOTHROW(AnalyzerConfigParser::parseFile(tmp.path));
    }
}

TEST_CASE("parseFile accepts threads = 0", "[config]") {
    TmpFile tmp("[analyzer]\nthreads = 0\n");
    const auto cfg = AnalyzerConfigParser::parseFile(tmp.path);
    REQUIRE(cfg.threadCount == 0);
}

// ---------------------------------------------------------------------------
// Invalid configs
// ---------------------------------------------------------------------------

TEST_CASE("parseFile throws on unknown solver name", "[config]") {
    TmpFile tmp("[analyzer]\nsolver = \"gpu\"\n");
    REQUIRE_THROWS_AS(AnalyzerConfigParser::parseFile(tmp.path), std::runtime_error);
}

TEST_CASE("parseFile throws on negative threads", "[config]") {
    TmpFile tmp("[analyzer]\nthreads = -1\n");
    REQUIRE_THROWS_AS(AnalyzerConfigParser::parseFile(tmp.path), std::runtime_error);
}

TEST_CASE("parseFile throws on threads exceeding UINT_MAX", "[config]") {
    TmpFile tmp("[analyzer]\nthreads = 4294967296\n"); // UINT_MAX + 1
    REQUIRE_THROWS_AS(AnalyzerConfigParser::parseFile(tmp.path), std::runtime_error);
}

TEST_CASE("parseFile throws on missing file", "[config]") {
    REQUIRE_THROWS(AnalyzerConfigParser::parseFile("/tmp/does_not_exist_analyzer_test.toml"));
}
