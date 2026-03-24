#pragma once
#include "streamLine.h"

#include <cmath>
#include <cstddef>
#include <memory>

#define PARRALEL_THRESHOLD 0.2

namespace Vector {

class Vector {
  public:
    float x;
    float y;
    std::shared_ptr<StreamLine::StreamLine> stream;

    Vector(float x, float y)
        : x(x),
          y(y),
          stream(nullptr) {};

    [[nodiscard]] float magnitude() const;

    [[nodiscard]] Vector unitVector() const;
};
float dotProduct(const Vector& a, const Vector& b);

bool almostParrallel(Vector& a, Vector& b);
} // namespace Vector
