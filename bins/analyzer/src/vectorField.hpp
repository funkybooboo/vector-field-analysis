#pragma once
#include "streamLine.hpp"
#include "vector.hpp"

#include <memory>
#include <tuple>
#include <vector>

namespace VectorField {

class VectorField {
    const float xMin, xMax, yMin, yMax;
    std::vector<std::vector<Vector::Vector>> field;

  public:
    VectorField(float xMin, float xMax, float yMin, float yMax,
                std::vector<std::vector<Vector::Vector>> field)
        : xMin(xMin),
          xMax(xMax),
          yMin(yMin),
          yMax(yMax),
          field(std::move(field)) {}

    std::pair<int, int> pointsTo(int x, int y);
    std::pair<int, int> pointsTo(std::pair<int, int> coords);

    void mergeStreamLines(const std::shared_ptr<StreamLine::StreamLine>& start,
                          const std::shared_ptr<StreamLine::StreamLine>& end);

    void flowFromVector(std::pair<int, int> startCords);
};

} // namespace VectorField
