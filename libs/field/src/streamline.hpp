#pragma once
#include "fieldTypes.hpp"

namespace Field {

class Streamline {
  public:
    // Ordered grid cells on the traced path, starting from the seed cell
    // and growing one step at a time.
    Path path;

    explicit Streamline(GridCell startPoint);
};

} // namespace Field
