#pragma once
#include "vectorField.hpp"

namespace pthreads {

void computeTimeStep(VectorField::FieldGrid& field, unsigned int threadCount);

} // namespace pthreads
