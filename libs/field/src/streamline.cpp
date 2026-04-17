#include "streamline.hpp"

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
    path_.splice(path_.end(), other.path_);
}

} // namespace Field
