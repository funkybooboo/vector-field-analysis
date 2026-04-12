#include "openMP.hpp"

#include "sequentialCPU.hpp"

#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace openMP {
void computeTimeStep(VectorField::FieldGrid& field) {
#ifdef _OPENMP
    const int rowCount = static_cast<int>(field.field.size());
    const int colCount = static_cast<int>(field.field[0].size());

#pragma omp parallel for schedule(dynamic) collapse(2)
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            field.traceStreamlineStep(row, col);
        }
    }
#else
    std::cout << "_OPENMP not included in compiler. Using single threaded implementation.\n";
    sequentialCPU::computeTimeStep(field);
#endif
}
} // namespace openMP
