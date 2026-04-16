#include "streamline.hpp"

namespace Field {

Streamline::Streamline(GridCell startPoint) {
    // Seed the path immediately so it is never empty -- callers can always
    // read getPath().front() to recover the streamline's origin cell.
    path_.push_back(startPoint);
}

} // namespace Field
