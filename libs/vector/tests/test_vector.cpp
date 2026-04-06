#include "vector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("Vector magnitude", "[vector]") {
    REQUIRE_THAT(Vector::Vec2(3.0f, 4.0f).magnitude(), WithinAbs(5.0f, 1e-5f));
    REQUIRE_THAT(Vector::Vec2(1.0f, 0.0f).magnitude(), WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(Vector::Vec2(0.0f, 0.0f).magnitude(), WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("Vector unitVector has magnitude 1", "[vector]") {
    Vector::Vec2 v(3.0f, 4.0f);
    REQUIRE_THAT(v.unitVector().magnitude(), WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Vector unitVector preserves direction", "[vector]") {
    Vector::Vec2 v(3.0f, 4.0f);
    auto u = v.unitVector();
    REQUIRE_THAT(u.x, WithinAbs(0.6f, 1e-5f));
    REQUIRE_THAT(u.y, WithinAbs(0.8f, 1e-5f));
}

TEST_CASE("dotProduct", "[vector]") {
    Vector::Vec2 a(1.0f, 0.0f);
    Vector::Vec2 b(0.0f, 1.0f);
    Vector::Vec2 c(1.0f, 0.0f);

    REQUIRE_THAT(Vector::dotProduct(a, b), WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(Vector::dotProduct(a, c), WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(Vector::dotProduct(a, Vector::Vec2(-1.0f, 0.0f)), WithinAbs(-1.0f, 1e-5f));
}

TEST_CASE("almostParallel", "[vector]") {
    Vector::Vec2 sameA(1.0f, 0.0f);
    Vector::Vec2 sameB(1.0f, 0.0f);
    REQUIRE(Vector::almostParallel(sameA, sameB));

    Vector::Vec2 perpA(1.0f, 0.0f);
    Vector::Vec2 perpB(0.0f, 1.0f);
    REQUIRE_FALSE(Vector::almostParallel(perpA, perpB));
}

TEST_CASE("Vec2 unitVector for zero vector returns zero vector", "[vector]") {
    Vector::Vec2 zero(0.0f, 0.0f);
    auto result = zero.unitVector();
    REQUIRE_THAT(result.x, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(result.y, WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("almostParallel threshold boundary", "[vector]") {
    // Sum of differences = 0.05 + 0.05 = 0.10 -- well inside threshold of 0.2, should pass
    Vector::Vec2 a(1.0f, 0.0f);
    Vector::Vec2 inside(0.95f, 0.05f);
    REQUIRE(Vector::almostParallel(a, inside));

    // Sum of differences = 0.15 + 0.15 = 0.30 -- well outside threshold of 0.2, should fail
    Vector::Vec2 outside(0.85f, 0.15f);
    REQUIRE_FALSE(Vector::almostParallel(a, outside));
}

TEST_CASE("dotProduct with zero vector", "[vector]") {
    Vector::Vec2 unit(1.0f, 0.0f);
    Vector::Vec2 zero(0.0f, 0.0f);
    REQUIRE_THAT(Vector::dotProduct(unit, zero), WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(Vector::dotProduct(zero, zero), WithinAbs(0.0f, 1e-5f));
}
