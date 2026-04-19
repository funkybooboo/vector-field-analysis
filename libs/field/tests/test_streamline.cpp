#include "streamline.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("StreamLine initialises with start point", "[streamline]") {
    Field::Streamline streamline(Field::GridCell{3, 5});

    REQUIRE(streamline.getPath().size() == 1);
    REQUIRE(streamline.getPath().front().row == 3);
    REQUIRE(streamline.getPath().front().col == 5);
}

TEST_CASE("Streamline path grows when points are appended", "[streamline]") {
    Field::Streamline streamline(Field::GridCell{0, 0});
    streamline.appendPoint({1, 0});
    streamline.appendPoint({2, 0});

    REQUIRE(streamline.getPath().size() == 3);
    auto it = streamline.getPath().begin();
    REQUIRE(it->row == 0);
    REQUIRE((++it)->row == 1);
    REQUIRE((++it)->row == 2);
}
