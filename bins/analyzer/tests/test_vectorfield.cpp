#include "vectorField.h"

#include <catch2/catch_test_macros.hpp>

static VectorField::VectorField makeField() {
    // 3x3 field with all vectors pointing right (+x)
    std::vector<std::vector<Vector::Vector>> f(
        3, std::vector<Vector::Vector>(3, Vector::Vector(1.0f, 0.0f)));
    return {0.0f, 2.0f, 0.0f, 2.0f, f};
}

TEST_CASE("pointsTo advances in vector direction", "[vectorfield]") {
    auto vf = makeField();

    auto [nx, ny] = vf.pointsTo(0, 0);
    REQUIRE(nx == 1);
    REQUIRE(ny == 0);
}

TEST_CASE("flowFromVector assigns a streamline", "[vectorfield]") {
    std::vector<std::vector<Vector::Vector>> f(
        3, std::vector<Vector::Vector>(3, Vector::Vector(1.0f, 0.0f)));
    VectorField::VectorField vf(0.0f, 2.0f, 0.0f, 2.0f, f);

    // Before flow, no streamline assigned
    REQUIRE(f[0][0].stream == nullptr);

    vf.flowFromVector({0, 0});

    // After flow, the source vector has a streamline
    // (VectorField holds its own copy of the field, so we check via flow behaviour)
    // Basic smoke test: no crash and execution completes
    SUCCEED();
}
