#include "vector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("Vector magnitude", "[vector]") {
    REQUIRE_THAT(Vector::Vector(3.0f, 4.0f).magnitude(), WithinAbs(5.0f, 1e-5f));
    REQUIRE_THAT(Vector::Vector(1.0f, 0.0f).magnitude(), WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(Vector::Vector(0.0f, 0.0f).magnitude(), WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("Vector unitVector has magnitude 1", "[vector]") {
    Vector::Vector v(3.0f, 4.0f);
    REQUIRE_THAT(v.unitVector().magnitude(), WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Vector unitVector preserves direction", "[vector]") {
    Vector::Vector v(3.0f, 4.0f);
    auto u = v.unitVector();
    REQUIRE_THAT(u.x, WithinAbs(0.6f, 1e-5f));
    REQUIRE_THAT(u.y, WithinAbs(0.8f, 1e-5f));
}

TEST_CASE("dotProduct", "[vector]") {
    Vector::Vector a(1.0f, 0.0f);
    Vector::Vector b(0.0f, 1.0f);
    Vector::Vector c(1.0f, 0.0f);

    REQUIRE_THAT(Vector::dotProduct(a, b), WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(Vector::dotProduct(a, c), WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(Vector::dotProduct(a, Vector::Vector(-1.0f, 0.0f)), WithinAbs(-1.0f, 1e-5f));
}

TEST_CASE("almostParallel", "[vector]") {
    Vector::Vector same_a(1.0f, 0.0f);
    Vector::Vector same_b(1.0f, 0.0f);
    REQUIRE(Vector::almostParrallel(same_a, same_b));

    Vector::Vector perp_a(1.0f, 0.0f);
    Vector::Vector perp_b(0.0f, 1.0f);
    REQUIRE_FALSE(Vector::almostParrallel(perp_a, perp_b));
}
