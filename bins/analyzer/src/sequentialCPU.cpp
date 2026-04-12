#include "sequentialCPU.hpp"

namespace sequentialCPU {

void computeTimeStep(VectorField::FieldGrid& timeStep) {
    const unsigned long numRow = timeStep.field.size();
    if (numRow == 0) {
        return;
    }
    const unsigned long numCol = timeStep.field[0].size();
    if (numCol == 0) {
        return;
    }

    for (unsigned long row = 0; row < numRow; row++) {
        for (unsigned long col = 0; col < numCol; col++) {
            timeStep.traceStreamlineStep(static_cast<int>(row), static_cast<int>(col));
        }
    }
}

} // namespace sequentialCPU
