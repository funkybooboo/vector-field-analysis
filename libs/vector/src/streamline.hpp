#pragma once
#include <vector>

namespace Vector {

class Streamline {
  public:
    std::vector<std::pair<int, int>> path;

    explicit Streamline(std::pair<int, int> startPoint);
};

} // namespace Vector
