#include "pthreads.hpp"

#include "utils.hpp"

#include <pthread.h>

namespace pthreads {

void* computeRows(void* arg) {
    auto* data = static_cast<ThreadData*>(arg);
    VectorField::FieldGrid& timeStep = *data->field;
    const std::size_t numCol = timeStep.field[0].size();

    for (std::size_t row = data->startRow; row < data->endRow; row++) {
        for (std::size_t col = 0; col < numCol; col++) {
            timeStep.traceStreamlineStep(static_cast<int>(row), static_cast<int>(col));
        }
    }
    return nullptr;
}

void computeTimeStep(VectorField::FieldGrid& field, const unsigned int threadCount) {
    if (threadCount == 0) {
        return;
    }

    auto splits = utils::calculateRowSplit(field.field.size(), threadCount);
    const std::size_t rowsPerThread = splits.first;
    const std::size_t leftOverRows = splits.second;

    std::vector<pthread_t> threads(threadCount);
    std::vector<ThreadData> threadData(threadCount);

    // assign rows
    std::size_t currentRow = 0;
    for (unsigned int id = 0; id < threadCount; id++) {
        threadData[id].field = &field;
        threadData[id].startRow = currentRow;

        // Last thread receives leftover rows
        if (id == threadCount - 1) {
            threadData[id].endRow = currentRow + rowsPerThread + leftOverRows;
        } else {
            threadData[id].endRow = currentRow + rowsPerThread;
        }
        currentRow += rowsPerThread;

        pthread_create(&threads[id], nullptr, computeRows, &threadData[id]);
    }

    for (unsigned int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], nullptr);
    }
}

} // namespace pthreads
