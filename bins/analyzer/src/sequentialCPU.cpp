#include "sequentialCPU.hpp"

namespace sequentialCPU {

void computeTimeStep(VectorField::FieldGrid& timeStep) {
    const std::size_t numRow = timeStep.rows();
    if (numRow == 0) {
        return;
    }
    const std::size_t numCol = timeStep.cols();
    if (numCol == 0) {
        return;
    }

    for (std::size_t row = 0; row < numRow; row++) {
        for (std::size_t col = 0; col < numCol; col++) {
            timeStep.traceStreamlineStep(static_cast<int>(row), static_cast<int>(col));
        }
    }
}

} // namespace sequentialCPU
