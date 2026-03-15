#include "vector.h"
#include <tuple>
#include <vector>

#pragma once
namespace VectorField {
class VectorField {
    std::vector<std::vector<Vector::Vector>> field;

  public:
    std::tuple<int, int> pointsTo(int x, int y);
    Vector::Vector lookUp(int x, int y);
};
} // namespace VectorField
