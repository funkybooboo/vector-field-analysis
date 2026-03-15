#include "vectorField.h"
#include "vector.h"
#include <cmath>
#include <tuple>

namespace VectorField {

Vector::Vector VectorField::lookUp(int x, int y) { return field[x][y]; }

std::tuple<int, int> VectorField::pointsTo(int x, int y) {
    Vector::Vector start = field[x][y];

    int nearestX = static_cast<int>(std::round(x + start.x));
    int nearestY = static_cast<int>(std::round(y + start.y));

    return {nearestX, nearestY};
}

} // namespace VectorField
