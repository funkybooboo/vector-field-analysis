#pragma once
#include "fieldTypes.hpp"

#include <string>

namespace FieldWriter {

void write(const std::string& path, const Field::TimeSeries& field, const std::string& typeLabel,
           float dt, float viscosity);

} // namespace FieldWriter
