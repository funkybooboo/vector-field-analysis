#include <Eigen/Dense>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

// Vortex field: (-y, x) / r -> mirrors the simulator implementation
static Eigen::Vector2f vortex(float x, float y) {
    float r = std::sqrt((x * x) + (y * y));
    if (r < 1e-6f) {
        return Eigen::Vector2f::Zero();
    }
    return {-y / r, x / r};
}

TEST_CASE("Vortex field produces unit vectors", "[simulator]") {
    for (auto [x, y] : std::initializer_list<std::pair<float, float>>{
             {1.0f, 0.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f}, {0.5f, 0.5f}}) {
        float mag = vortex(x, y).norm();
        REQUIRE_THAT(mag, WithinAbs(1.0f, 1e-5f));
    }
}

TEST_CASE("Vortex field is perpendicular to radius", "[simulator]") {
    auto v = vortex(1.0f, 0.0f);
    REQUIRE_THAT(v.x(), WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(v.y(), WithinAbs(1.0f, 1e-5f));

    v = vortex(0.0f, 1.0f);
    REQUIRE_THAT(v.x(), WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(v.y(), WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("Vortex field is zero at origin", "[simulator]") {
    auto v = vortex(0.0f, 0.0f);
    REQUIRE_THAT(v.norm(), WithinAbs(0.0f, 1e-5f));
}
