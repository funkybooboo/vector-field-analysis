#include "vec2.hpp"

#include <cmath>

namespace Vector {

float Vec2::magnitude() const {
    return std::sqrt((x * x) + (y * y));
}

Vec2 Vec2::unitVector() const {
    const float mag = magnitude();
    // A zero-magnitude vector has no direction; returning zero avoids
    // division by zero and prevents NaN from propagating into the field.
    if (mag == 0.0f) {
        return {0.0f, 0.0f};
    }
    return {x / mag, y / mag};
}

Vec2 Vec2::operator-() const {
    return {-x, -y};
}

Vec2 Vec2::operator+(const Vec2& other) const {
    return {x + other.x, y + other.y};
}

Vec2& Vec2::operator+=(const Vec2& other) {
    x += other.x;
    y += other.y;
    return *this;
}

Vec2 Vec2::operator*(float scalar) const {
    return {x * scalar, y * scalar};
}

Vec2 operator*(float scalar, const Vec2& v) {
    return v * scalar;
}

float dotProduct(const Vec2& a, const Vec2& b) {
    return (a.x * b.x) + (a.y * b.y);
}

bool almostParallel(const Vec2& a, const Vec2& b) {
    const float xDifference = std::abs(a.x - b.x);
    const float yDifference = std::abs(a.y - b.y);
    return xDifference + yDifference <= kParallelThreshold;
}

} // namespace Vector
