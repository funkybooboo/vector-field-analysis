#pragma once
#include "fieldTypes.hpp"

#include <memory>

namespace Field {

class Streamline : public std::enable_shared_from_this<Streamline> {
  public:
    explicit Streamline(GridCell startPoint);

    // parent points to another streamline if this one was merged.
    // If null, this is the current root.
    std::shared_ptr<Streamline> parent = nullptr;

    // Follow parent pointers to the root streamline of the merged set.
    [[nodiscard]] std::shared_ptr<Streamline> resolve();

    // Transfers all points from other's path to the end of this path in O(1).
    void absorb(Streamline& other);

    // Ordered grid cells on the traced path, starting from the seed cell.
    // The path is never empty -- the seed cell is always present.
    [[nodiscard]] const Path& getPath() const { return path_; }
    void appendPoint(GridCell point) { path_.push_back(point); }

  private:
    Path path_;
};

} // namespace Field
