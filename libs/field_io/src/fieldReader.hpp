#pragma once
#include "fieldTypes.hpp"

#include <string>

namespace FieldReader {

Field::TimeSeries read(const std::string& path);

} // namespace FieldReader
