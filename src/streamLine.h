#include <tuple>
#include <vector>

#pragma once
namespace StreamLine {
class StreamLine {
  public:
    std::vector<std::pair<int, int>> path;

    StreamLine(std::pair<int, int> startPoint);
};
} // namespace StreamLine
