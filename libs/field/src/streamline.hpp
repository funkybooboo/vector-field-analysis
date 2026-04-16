#pragma once
#include "fieldTypes.hpp"

namespace Field {

class Streamline {
  public:
    explicit Streamline(GridCell startPoint);

    // Ordered grid cells on the traced path, starting from the seed cell.
    // The path is never empty — the seed cell is always present.
    [[nodiscard]] const Path& getPath() const { return path_; }
    void appendPoint(GridCell point) { path_.push_back(point); }

  private:
    Path path_;
};

} // namespace Field
