#pragma once
#include <vector>

namespace Vector {

class Streamline {
  public:
    // Ordered grid indices (row, col) of each cell on the traced path,
    // starting from the seed cell and growing one step at a time.
    std::vector<std::pair<int, int>> path;

    explicit Streamline(std::pair<int, int> startPoint);
};

} // namespace Vector
