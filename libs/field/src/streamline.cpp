#include "streamline.hpp"

#include <iterator>

namespace Field {

Streamline::Streamline(GridCell startPoint) {
    // Seed the path immediately so it is never empty -- callers can always
    // read getPath().front() to recover the streamline's origin cell.
    path_.push_back(startPoint);
}

std::shared_ptr<Streamline> Streamline::resolve() {
    auto current = shared_from_this();
    while (current->parent) {
        current = current->parent;
    }
    return current;
}

void Streamline::absorb(Streamline& other) {
    path_.insert(path_.end(),
                 std::make_move_iterator(other.path_.begin()),
                 std::make_move_iterator(other.path_.end()));
    other.path_.clear();
}

} // namespace Field
