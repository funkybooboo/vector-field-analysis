#pragma once
#include "vectorField.hpp"

namespace pthreads {

struct ThreadData {
    VectorField::FieldGrid* field;
    unsigned long startRow;
    unsigned long endRow;
};

void computeTimeStep(VectorField::FieldGrid& field, unsigned int threadCount);

} // namespace pthreads
