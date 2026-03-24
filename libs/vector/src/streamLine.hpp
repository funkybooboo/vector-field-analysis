#pragma once
#include <tuple>
#include <vector>

namespace StreamLine {
class StreamLine {
  public:
    std::vector<std::pair<int, int>> path;

    StreamLine(std::pair<int, int> startPoint);
};
} // namespace StreamLine
