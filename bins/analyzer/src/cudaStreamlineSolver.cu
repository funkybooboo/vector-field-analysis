#include "cudaStreamlineSolver.hpp"

#include <cuda_runtime.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// 2D vector rep
// flatten Field::Slice into this format before copying it to GPU
struct DeviceVec2 {
    float x;
    float y;
};

// representation of one downstream neighbor cell
struct DeviceGridCell {
    int row;
    int col;
};

// helper for CUDA failures
inline void cudaCheck(cudaError_t err, const char* what) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(what) + ": " + cudaGetErrorString(err));
    }
}

// clamp helper for device code
__device__ int clampInt(int value, int lo, int hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

// For each grid cell, compute the downstream neighbor that its vector points toward.
// This kernel only performs pass 1 of the solver:
//   source cell -> downstream destination cell
// It does NOT mutate streamline objects
// Pass 2 remains on the CPU and reuses applyNeighborPairs(...)
__global__ void computeNeighborsKernel(const DeviceVec2* field,
                                       int rows,
                                       int cols,
                                       float xMin,
                                       float xMax,
                                       float yMin,
                                       float yMax,
                                       DeviceGridCell* neighbors) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    const int total = rows * cols;

    if (idx >= total) {
        return;
    }

    const int row = idx / cols;
    const int col = idx % cols;

    // Match Grid::downstreamCell behavior:
    // if either dimension is degenerate, cell maps to itself
    if (rows == 1 || cols == 1) {
        neighbors[idx] = DeviceGridCell{row, col};
        return;
    }

    const DeviceVec2 v = field[idx];

    const float rowSpacing = (yMax - yMin) / static_cast<float>(rows - 1);
    const float colSpacing = (xMax - xMin) / static_cast<float>(cols - 1);

    const float physRow =
        yMin + ((yMax - yMin) * static_cast<float>(row) / static_cast<float>(rows - 1));
    const float physCol =
        xMin + ((xMax - xMin) * static_cast<float>(col) / static_cast<float>(cols - 1));

    const int destRow = clampInt(
        static_cast<int>(roundf((physRow + v.y - yMin) / rowSpacing)),
        0,
        rows - 1);

    const int destCol = clampInt(
        static_cast<int>(roundf((physCol + v.x - xMin) / colSpacing)),
        0,
        cols - 1);

    neighbors[idx] = DeviceGridCell{destRow, destCol};
}

} // namespace

void CudaStreamlineSolver::computeTimeStep(Field::Grid& grid) {
    const int rowCount = static_cast<int>(grid.rows());
    if (rowCount == 0) {
        return;
    }

    const int colCount = static_cast<int>(grid.cols());
    if (colCount == 0) {
        return;
    }

    const auto& bounds = grid.bounds();
    const auto& slice = grid.field();

    const int total = rowCount * colCount;

    // Flatten 2D Field::Slice into a 1D device-friendly array
    std::vector<DeviceVec2> hostField(static_cast<std::size_t>(total));
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < colCount; ++col) {
            const auto& v = slice[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
            hostField[(static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount)) +
                      static_cast<std::size_t>(col)] = DeviceVec2{v.x, v.y};
        }
    }

    DeviceVec2* dField = nullptr;
    DeviceGridCell* dNeighbors = nullptr;

    const auto cleanup = [&]() {
        if (dField != nullptr) {
            cudaFree(dField);
            dField = nullptr;
        }
        if (dNeighbors != nullptr) {
            cudaFree(dNeighbors);
            dNeighbors = nullptr;
        }
    };

    try {
        cudaCheck(cudaMalloc(reinterpret_cast<void**>(&dField),
                             sizeof(DeviceVec2) * static_cast<std::size_t>(total)),
                  "cudaMalloc(dField)");

        cudaCheck(cudaMalloc(reinterpret_cast<void**>(&dNeighbors),
                             sizeof(DeviceGridCell) * static_cast<std::size_t>(total)),
                  "cudaMalloc(dNeighbors)");

        cudaCheck(cudaMemcpy(dField,
                             hostField.data(),
                             sizeof(DeviceVec2) * static_cast<std::size_t>(total),
                             cudaMemcpyHostToDevice),
                  "cudaMemcpy H2D field");

        constexpr int blockSize = 256;
        const int launchSize = (total + blockSize - 1) / blockSize;

        computeNeighborsKernel<<<launchSize, blockSize>>>(
            dField,
            rowCount,
            colCount,
            bounds.xMin,
            bounds.xMax,
            bounds.yMin,
            bounds.yMax,
            dNeighbors);

        cudaCheck(cudaGetLastError(), "computeNeighborsKernel launch");
        cudaCheck(cudaDeviceSynchronize(), "cudaDeviceSynchronize");

        // Copy GPU-computed neighbor pairs back to host
        std::vector<DeviceGridCell> hostNeighbors(static_cast<std::size_t>(total));
        cudaCheck(cudaMemcpy(hostNeighbors.data(),
                             dNeighbors,
                             sizeof(DeviceGridCell) * static_cast<std::size_t>(total),
                             cudaMemcpyDeviceToHost),
                  "cudaMemcpy D2H neighbors");

        cleanup();

        // Convert neighbor buffer into the Field::GridCell
        // vector expected by the existing pass-2 helper
        std::vector<Field::GridCell> neighbors(static_cast<std::size_t>(total));
        for (int row = 0; row < rowCount; ++row) {
            for (int col = 0; col < colCount; ++col) {
                const std::size_t idx =
                    (static_cast<std::size_t>(row) * static_cast<std::size_t>(colCount)) +
                    static_cast<std::size_t>(col);
                neighbors[idx] = Field::GridCell{
                    hostNeighbors[idx].row,
                    hostNeighbors[idx].col
                };
            }
        }

        // Pass 2: keep the existing sequential streamline mutation logic
        applyNeighborPairs(grid, neighbors, rowCount, colCount);
    } catch (...) {
        cleanup();
        throw;
    }
}
