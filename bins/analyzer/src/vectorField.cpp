#include "vectorField.hpp"

#include "streamLine.hpp"
#include "vector.hpp"

#include <cmath>
#include <utility>

namespace VectorField {

std::pair<int, int> VectorField::pointsTo(int x, int y) {
    Vector::Vector start = field[x][y];

    float xSpacing = (xMax - xMin) / static_cast<float>(field.size() - 1);
    float ySpacing = (yMax - yMin) / static_cast<float>(field[0].size() - 1);

    // find the nearest coordinate that the vector points accounting for scale
    int nearestX =
        static_cast<int>(std::round(((static_cast<float>(x) * xSpacing) + start.x) / xSpacing));
    int nearestY =
        static_cast<int>(std::round(((static_cast<float>(y) * ySpacing) + start.y) / ySpacing));

    return {nearestX, nearestY};
}

std::pair<int, int> VectorField::pointsTo(std::pair<int, int> coords) {
    return VectorField::pointsTo(coords.first, coords.second);
}

void VectorField::mergeStreamLines(const std::shared_ptr<StreamLine::StreamLine>& start,
                                   const std::shared_ptr<StreamLine::StreamLine>& end) {
    if (!start || !end || start == end || start->path.empty()) {
        return;
    }

    // Update start's path to include all coordinates from end's path
    // and update all vectors at those coordinates to point to startSharedPtr
    for (const auto& point : end->path) {
        start->path.push_back(point);
        field[point.first][point.second].stream = start;
    }
}

void VectorField::flowFromVector(std::pair<int, int> startCords) {

    Vector::Vector& vector = field[startCords.first][startCords.second];

    // if not a member of a streamline create a new one
    if (vector.stream == nullptr) {
        auto newStream = std::make_shared<StreamLine::StreamLine>(startCords);
        vector.stream = newStream;
    }

    std::pair<int, int> destCords = pointsTo(startCords);
    Vector::Vector& destination = field[destCords.first][destCords.second];

    if (destination.stream == nullptr) {
        destination.stream = vector.stream;
        vector.stream->path.push_back(destCords);
    } else {
        mergeStreamLines(destination.stream, vector.stream);
    }
}

} // namespace VectorField
