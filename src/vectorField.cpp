#include "vectorField.h"
#include "streamLine.h"
#include <cmath>
#include <utility>

namespace VectorField {

Vector::Vector VectorField::lookUp(int x, int y) { return field[x][y]; }

std::pair<int, int> VectorField::pointsTo(int x, int y) {
    Vector::Vector start = field[x][y];

    // find the nearest coordinate that the vector points to
    int nearestX = static_cast<int>(std::round(x + start.x));
    int nearestY = static_cast<int>(std::round(y + start.y));

    return {nearestX, nearestY};
}

void VectorField::mergeStreamLines(
    std::shared_ptr<StreamLine::StreamLine> start,
    std::shared_ptr<StreamLine::StreamLine> end) {
    if (!start || !end || start == end || start->path.empty())
        return;

    // Get the shared_ptr that points to the 'start' streamline from its first
    // path point
    auto firstPoint = start->path[0];
    std::shared_ptr<StreamLine::StreamLine> startSharedPtr =
        field[firstPoint.first][firstPoint.second].stream;

    // Ensure the shared_ptr actually points to our start object
    if (!startSharedPtr || startSharedPtr.get() != start)
        return;

    // Update start's path to include all coordinates from end's path
    // and update all vectors at those coordinates to point to startSharedPtr
    for (const auto &point : end->path) {
        start->path.push_back(point);
        field[point.first][point.second].stream = startSharedPtr;
    }
}

void VectorField::flowFromVector(Vector::Vector &vector) {
    int x = static_cast<int>(vector.x);
    int y = static_cast<int>(vector.y);

    std::pair<int, int> startCords = std::pair<int, int>(x, y);

    // if not a member of a streamline create a new one
    if (vector.stream == nullptr) {
        auto sl = std::make_shared<StreamLine::StreamLine>(startCords);
        field[x][y].stream = sl;
    }

    std::pair<int, int> destCords = pointsTo(x, y);
    Vector::Vector &destination = field[destCords.first][destCords.second];

    if (destination.stream == nullptr) {
        // Create streamline starting at the source vector's coordinate
        auto sl =
            std::make_shared<StreamLine::StreamLine>(std::pair<int, int>(x, y));
        // Add the destination to the path
        sl->path.push_back(destCords);

        // Update both source and destination vectors to point to this
        // streamline
        field[x][y].stream = sl;
        destination.stream = sl;
    } else {
        mergeStreamLines(destination.stream, vector.stream);
    }
}

} // namespace VectorField
