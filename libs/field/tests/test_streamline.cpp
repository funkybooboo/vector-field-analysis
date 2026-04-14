#include "streamline.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("StreamLine initialises with start point", "[streamline]") {
    Field::Streamline streamline(Field::GridCell{3, 5});

    REQUIRE(streamline.path.size() == 1);
    REQUIRE(streamline.path[0].row == 3);
    REQUIRE(streamline.path[0].col == 5);
}

TEST_CASE("Streamline path grows when points are appended", "[streamline]") {
    Field::Streamline streamline(Field::GridCell{0, 0});
    streamline.path.push_back({1, 0});
    streamline.path.push_back({2, 0});

    REQUIRE(streamline.path.size() == 3);
    REQUIRE(streamline.path[1].row == 1);
    REQUIRE(streamline.path[2].row == 2);
}
