#include "streamLine.hpp"

namespace StreamLine {

StreamLine::StreamLine(std::pair<int, int> startPoint) {
    path.push_back(startPoint);
};
} // namespace StreamLine
