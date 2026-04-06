#include "streamline.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("StreamLine initialises with start point", "[streamline]") {
    Vector::Streamline streamline({3, 5});

    REQUIRE(streamline.path.size() == 1);
    REQUIRE(streamline.path[0].first == 3);
    REQUIRE(streamline.path[0].second == 5);
}

TEST_CASE("Streamline path grows when points are appended", "[streamline]") {
    Vector::Streamline streamline({0, 0});
    streamline.path.emplace_back(1, 0);
    streamline.path.emplace_back(2, 0);

    REQUIRE(streamline.path.size() == 3);
    REQUIRE(streamline.path[1].first == 1);
    REQUIRE(streamline.path[2].first == 2);
}
