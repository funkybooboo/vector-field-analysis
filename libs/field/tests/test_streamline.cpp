#include "streamline.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("StreamLine initialises with start point", "[streamline]") {
    Field::Streamline streamline(Field::GridCell{3, 5});

    REQUIRE(streamline.getPath().size() == 1);
    REQUIRE(streamline.getPath()[0].row == 3);
    REQUIRE(streamline.getPath()[0].col == 5);
}

TEST_CASE("Streamline path grows when points are appended", "[streamline]") {
    Field::Streamline streamline(Field::GridCell{0, 0});
    streamline.appendPoint({1, 0});
    streamline.appendPoint({2, 0});

    REQUIRE(streamline.getPath().size() == 3);
    REQUIRE(streamline.getPath()[1].row == 1);
    REQUIRE(streamline.getPath()[2].row == 2);
}

TEST_CASE("Streamline appended cells preserve col values", "[streamline]") {
    Field::Streamline streamline(Field::GridCell{0, 3});
    streamline.appendPoint({1, 5});
    streamline.appendPoint({2, 7});

    REQUIRE(streamline.getPath()[0].col == 3);
    REQUIRE(streamline.getPath()[1].col == 5);
    REQUIRE(streamline.getPath()[2].col == 7);
}

TEST_CASE("Streamline init cell is not overwritten by appends", "[streamline]") {
    Field::Streamline streamline(Field::GridCell{4, 9});
    streamline.appendPoint({5, 1});

    REQUIRE(streamline.getPath()[0].row == 4);
    REQUIRE(streamline.getPath()[0].col == 9);
}
