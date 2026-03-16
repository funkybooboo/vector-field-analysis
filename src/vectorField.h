#include "streamLine.h"
#include "vector.h"
#include <tuple>
#include <vector>

#pragma once
namespace VectorField {
class VectorField {
    std::vector<std::vector<Vector::Vector>> field;
    std::vector<std::vector<StreamLine::StreamLine *>> streams;

  public:
    std::tuple<int, int> pointsTo(int x, int y);
    Vector::Vector lookUp(int x, int y);
    void mergeStreamLines(StreamLine::StreamLine *a, StreamLine::StreamLine *b);
};
} // namespace VectorField
