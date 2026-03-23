#include "streamLine.h"
#include "vector.h"
#include <memory>
#include <tuple>
#include <vector>

#pragma once
namespace VectorField {

class VectorField {
    std::vector<std::vector<Vector::Vector>> field;

  public:
    std::pair<int, int> pointsTo(int x, int y);
    Vector::Vector lookUp(int x, int y);
    void mergeStreamLines(std::shared_ptr<StreamLine::StreamLine> start,
                          std::shared_ptr<StreamLine::StreamLine> end);
    void flowFromVector(Vector::Vector &vector);
};

} // namespace VectorField
