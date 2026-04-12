#include "pthreads.hpp"

#include "utils.hpp"

#include <pthread.h>

namespace pthreads {

void* computeRows(void* arg) {
    auto* data = static_cast<ThreadData*>(arg);
    VectorField::FieldGrid& timeStep = *data->field;
    unsigned long numCol = timeStep.field[0].size();

    for (unsigned long row = data->startRow; row < data->endRow; row++) {
        for (unsigned long col = 0; col < numCol; col++) {
            timeStep.traceStreamlineStep(static_cast<int>(row), static_cast<int>(col));
        }
    }
    return nullptr;
}

void computeTimeStep(VectorField::FieldGrid& field, const unsigned int threadCount) {

    auto splits = utils::calculateRowSplit(field.field.size(), threadCount);
    const unsigned long rowsPerThread = splits.first;
    const unsigned long leftOverRows = splits.second;

    std::vector<pthread_t> threads(threadCount);
    std::vector<ThreadData> threadData(threadCount);

    // assign rows
    unsigned long currentRow = 0;
    for (unsigned int id = 0; id < threadCount; id++) {
        threadData[id].field = &field;
        threadData[id].startRow = currentRow;

        // Last thread receives left overs
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
