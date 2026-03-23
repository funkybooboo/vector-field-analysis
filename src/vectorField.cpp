#include "vectorField.h"
#include "streamLine.h"
#include <cmath>
#include <utility>

namespace VectorField {

Vector::Vector VectorField::lookUp(int x, int y) { return field[x][y]; }

std::pair<int, int> VectorField::pointsTo(int x, int y) {
    Vector::Vector start = field[x][y];

    float xSpacing = (xMax - xMin) / (field.size() - 1);
    float ySpacing = yMax - yMin / (field[0].size() - 1);

    // find the nearest coordinate that the vector points accounting for scale
    int nearestX = static_cast<int>(std::round((x + start.x) / xSpacing));
    int nearestY = static_cast<int>(std::round((y + start.y) / ySpacing));

    return {nearestX, nearestY};
}

void VectorField::mergeStreamLines(
    std::shared_ptr<StreamLine::StreamLine> start,
    std::shared_ptr<StreamLine::StreamLine> end) {
    if (!start || !end || start == end || start->path.empty())
        return;

    // Update start's path to include all coordinates from end's path
    // and update all vectors at those coordinates to point to startSharedPtr
    for (const auto &point : end->path) {
        start->path.push_back(point);
        field[point.first][point.second].stream = start;
    }
}

void VectorField::flowFromVector(Vector::Vector &vector) {
    int x = static_cast<int>(vector.x);
    int y = static_cast<int>(vector.y);

    auto startCords = std::pair<int, int>(x, y);

    // if not a member of a streamline create a new one
    if (vector.stream == nullptr) {
        auto newStream = std::make_shared<StreamLine::StreamLine>(startCords);
        field[x][y].stream = newStream;
    }

    std::pair<int, int> destCords = pointsTo(x, y);
    Vector::Vector &destination = field[destCords.first][destCords.second];

    if (destination.stream == nullptr) {
        destination.stream = vector.stream;
        vector.stream->path.push_back(destCords);
    } else {
        mergeStreamLines(destination.stream, vector.stream);
    }
}

} // namespace VectorField
