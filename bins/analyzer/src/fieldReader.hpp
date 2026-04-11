#pragma once
#include "vector.hpp"

#include <string>

namespace FieldReader {

Vector::FieldTimeSeries read(const std::string& path);

} // namespace FieldReader
