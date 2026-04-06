#pragma once
#include "streamline.hpp"

#include <memory>

namespace Vector {

// Maximum component-wise L1 distance between two unit vectors for them to be
// considered parallel. 0.2 tolerates roughly 8 degrees of divergence while
// still filtering noise in nearly-aligned field vectors.
inline constexpr float kParallelThreshold = 0.2f;

class Vec2 {
  public:
    float x;
    float y;
    // The streamline this vector belongs to, if one has been traced through it.
    // shared_ptr lets multiple grid cells reference the same Streamline after
    // a merge, without duplicating the path data.
    std::shared_ptr<Streamline> stream;

    Vec2(float x, float y)
        : x(x),
          y(y),
          stream(nullptr) {}

    [[nodiscard]] float magnitude() const;

    [[nodiscard]] Vec2 unitVector() const;
};

[[nodiscard]] float dotProduct(const Vec2& a, const Vec2& b);

// Precondition: both vectors must be near-unit. This compares component-wise
// difference, not direction - non-unit inputs give unreliable results.
// L1 distance on unit vectors is used instead of a dot product or angle
// comparison because it is cheaper to compute and works well for the
// axis-aligned grid structure where near-parallel vectors dominate.
[[nodiscard]] bool almostParallel(const Vec2& a, const Vec2& b);

} // namespace Vector
