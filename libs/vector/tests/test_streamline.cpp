#include "streamLine.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("StreamLine initialises with start point", "[streamline]") {
    StreamLine::StreamLine sl({3, 5});

    REQUIRE(sl.path.size() == 1);
    REQUIRE(sl.path[0].first == 3);
    REQUIRE(sl.path[0].second == 5);
}
