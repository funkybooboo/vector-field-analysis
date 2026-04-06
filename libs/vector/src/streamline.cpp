#include "streamline.hpp"

namespace Vector {

Streamline::Streamline(std::pair<int, int> startPoint) {
    // Seed the path immediately so it is never empty -- callers can always
    // read path.front() to recover the streamline's origin cell.
    path.push_back(startPoint);
}

} // namespace Vector
