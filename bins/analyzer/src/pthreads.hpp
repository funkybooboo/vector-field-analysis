#pragma once
#include "vectorField.hpp"

#include <cstddef>

namespace pthreads {

struct ThreadData {
    VectorField::FieldGrid* field;
    std::size_t startRow;
    std::size_t endRow;
};

void computeTimeStep(VectorField::FieldGrid& field, unsigned int threadCount);

} // namespace pthreads
