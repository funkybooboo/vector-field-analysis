#include "cuda.hpp"

#include <cuda_runtime.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace cuda {
namespace {

inline Field::GridCell toGridCell(int idx, int cols) {
    return Field::GridCell{idx / cols, idx % cols};
}

inline void cudaCheck(cudaError_t err, const char* what) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(what) + ": " + cudaGetErrorString(err));
    }
}

// Mirrors Grid::downstreamCell using the same simplified index-space formula:
// destRow = round(row + vy/rowSpacing). Division-only; no FMA possible, so
// GPU result is bitwise identical to CPU.
__global__ void computeSuccessorKernel(const float2* field, int rows, int cols,
                                       float rowSpacing, float colSpacing, int* successor) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= rows * cols) {
        return;
    }

    const int row = idx / cols;
    const int col = idx % cols;

    if (rows == 1 || cols == 1) {
        successor[idx] = idx;
        return;
    }

    const float2 v = field[idx];
    const int destRow = max(0, min(rows - 1, static_cast<int>(roundf(static_cast<float>(row) + (v.y / rowSpacing)))));
    const int destCol = max(0, min(cols - 1, static_cast<int>(roundf(static_cast<float>(col) + (v.x / colSpacing)))));
    successor[idx] = destRow * cols + destCol;
}

// Union-find find + path halving
__device__ int findRoot(int* parent, int x) {
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }
    return x;
}

// Union-find union using atomicCAS — MIN-root: smaller index always becomes root,
// matching Grid::unite so all solvers use the same DSU convention.
__device__ void unite(int* parent, int a, int b) {
    while (true) {
        a = findRoot(parent, a);
        b = findRoot(parent, b);
        if (a == b) return;
        if (a > b) {
            const int tmp = a;
            a = b;
            b = tmp;
        }
        const int old = atomicCAS(&parent[b], b, a);
        if (old == b) return;
    }
}

__global__ void initUnionFindKernel(int n, int* parent) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        parent[idx] = idx;
    }
}

__global__ void unionSuccessorKernel(int n, const int* successor, int* parent) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        unite(parent, idx, successor[idx]);
    }
}

__global__ void compressParentsKernel(int n, int* parent) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        parent[idx] = findRoot(parent, idx);
    }
}

__global__ void computeSuccessorSliceKernel(const float2* field, int rows, int cols,
                                            float rowSpacing, float colSpacing,
                                            int startRow, int localRows, int* successor) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= localRows * cols) {
        return;
    }
    const int localRow = idx / cols;
    const int globalRow = startRow + localRow;
    const int col = idx % cols;
    if (rows == 1 || cols == 1) {
        successor[idx] = globalRow * cols + col;
        return;
    }
    const float2 v = field[idx];
    const int destRow = max(0, min(rows - 1, static_cast<int>(roundf(static_cast<float>(globalRow) + (v.y / rowSpacing)))));
    const int destCol = max(0, min(cols - 1, static_cast<int>(roundf(static_cast<float>(col) + (v.x / colSpacing)))));
    successor[idx] = destRow * cols + destCol;
}

} // namespace

Result computeComponents(const std::vector<Vector::Vec2>& field, int rows, int cols,
                         float rowSpacing, float colSpacing, unsigned int cudaBlockSize) {
    if (rows == 0 || cols == 0) {
        return {};
    }

    const int total = rows * cols;

    std::vector<float2> hostField(static_cast<std::size_t>(total));
    for (int i = 0; i < total; ++i) {
        const auto& v = field[static_cast<std::size_t>(i)];
        hostField[static_cast<std::size_t>(i)] = float2{v.x, v.y};
    }

    float2* dField = nullptr;
    int* dSuccessor = nullptr;
    int* dParent = nullptr;

    const auto cleanup = [&]() {
        if (dField != nullptr) {
            cudaFree(dField);
            dField = nullptr;
        }
        if (dSuccessor != nullptr) {
            cudaFree(dSuccessor);
            dSuccessor = nullptr;
        }
        if (dParent != nullptr) {
            cudaFree(dParent);
            dParent = nullptr;
        }
    };

    try {
        cudaCheck(cudaMalloc(reinterpret_cast<void**>(&dField),
                             sizeof(float2) * static_cast<std::size_t>(total)),
                  "cudaMalloc(dField)");

        cudaCheck(cudaMalloc(reinterpret_cast<void**>(&dSuccessor),
                             sizeof(int) * static_cast<std::size_t>(total)),
                  "cudaMalloc(dSuccessor)");

        cudaCheck(cudaMalloc(reinterpret_cast<void**>(&dParent),
                             sizeof(int) * static_cast<std::size_t>(total)),
                  "cudaMalloc(dParent)");

        cudaCheck(cudaMemcpy(dField, hostField.data(),
                             sizeof(float2) * static_cast<std::size_t>(total),
                             cudaMemcpyHostToDevice),
                  "cudaMemcpy H2D field");

        const int blockSize = static_cast<int>(cudaBlockSize);
        const int launchSize = (total + blockSize - 1) / blockSize;

        computeSuccessorKernel<<<launchSize, blockSize>>>(dField, rows, cols, rowSpacing, colSpacing,
                                                         dSuccessor);
        cudaCheck(cudaGetLastError(), "computeSuccessorKernel launch");

        initUnionFindKernel<<<launchSize, blockSize>>>(total, dParent);
        cudaCheck(cudaGetLastError(), "initUnionFindKernel launch");

        unionSuccessorKernel<<<launchSize, blockSize>>>(total, dSuccessor, dParent);
        cudaCheck(cudaGetLastError(), "unionSuccessorKernel launch");

        compressParentsKernel<<<launchSize, blockSize>>>(total, dParent);
        cudaCheck(cudaGetLastError(), "compressParentsKernel launch");

        cudaCheck(cudaDeviceSynchronize(), "cudaDeviceSynchronize");

        std::vector<int> parent(static_cast<std::size_t>(total));

        cudaCheck(cudaMemcpy(parent.data(), dParent, sizeof(int) * static_cast<std::size_t>(total),
                             cudaMemcpyDeviceToHost),
                  "cudaMemcpy D2H parent");

        cleanup();

        // CPU full path compression: compressParentsKernel uses non-atomic writes,
        // so parent[i] may still point to an intermediate ancestor after the GPU pass.
        for (int i = 0; i < total; ++i) {
            int root = parent[static_cast<std::size_t>(i)];
            while (parent[static_cast<std::size_t>(root)] != root)
                root = parent[static_cast<std::size_t>(root)];
            int x = i;
            while (parent[static_cast<std::size_t>(x)] != root) {
                int next = parent[static_cast<std::size_t>(x)];
                parent[static_cast<std::size_t>(x)] = root;
                x = next;
            }
        }

        // Convert raw roots to dense labels 0..N-1
        std::vector<int> rootToLabel(static_cast<std::size_t>(total), -1);
        std::vector<int> componentId(static_cast<std::size_t>(total));

        int nextLabel = 0;
        for (int idx = 0; idx < total; ++idx) {
            const int root = parent[static_cast<std::size_t>(idx)];
            if (rootToLabel[static_cast<std::size_t>(root)] == -1) {
                rootToLabel[static_cast<std::size_t>(root)] = nextLabel++;
            }
            componentId[static_cast<std::size_t>(idx)] =
                rootToLabel[static_cast<std::size_t>(root)];
        }

        Result result;
        result.rows = rows;
        result.cols = cols;
        result.componentId = std::move(componentId);
        return result;
    } catch (...) {
        cleanup();
        throw;
    }
}

std::vector<Field::Path> reconstructPaths(const Result& result) {
    if (result.rows == 0 || result.cols == 0) {
        return {};
    }

    const int total = result.rows * result.cols;
    if (static_cast<int>(result.componentId.size()) != total) {
        throw std::runtime_error("cuda::reconstructPaths received inconsistent result sizes");
    }

    // componentId is assigned by scanning cells 0..N-1, so label k corresponds
    // to the component whose minimum cell index is encountered k-th. Grouping
    // cells by label while iterating in cell-index order produces paths whose
    // cells are in ascending index order -- matching the CPU sortCellsByRoot
    // output and making verify() pass.
    int numComponents = 0;
    for (int id : result.componentId) {
        if (id >= numComponents) {
            numComponents = id + 1;
        }
    }

    std::vector<Field::Path> paths(static_cast<std::size_t>(numComponents));
    for (int idx = 0; idx < total; ++idx) {
        const int compId = result.componentId[static_cast<std::size_t>(idx)];
        paths[static_cast<std::size_t>(compId)].push_back(toGridCell(idx, result.cols));
    }

    std::vector<Field::Path> output;
    output.reserve(static_cast<std::size_t>(numComponents));
    for (auto& path : paths) {
        if (!path.empty()) {
            output.push_back(std::move(path));
        }
    }
    return output;
}

std::vector<int> computeSuccessorSlice(const std::vector<Vector::Vec2>& field, int rows, int cols,
                                       const Field::Bounds& bounds, int startRow, int endRow,
                                       unsigned int cudaBlockSize) {
    if (rows == 0 || cols == 0 || startRow >= endRow) {
        return {};
    }
    if (startRow < 0 || endRow > rows) {
        throw std::runtime_error("cuda::computeSuccessorSlice received an invalid row range");
    }

    const int localRows = endRow - startRow;
    const int localTotal = localRows * cols;

    std::vector<float2> hostField(static_cast<std::size_t>(localTotal));
    for (int localRow = 0; localRow < localRows; ++localRow) {
        const int globalRow = startRow + localRow;
        for (int col = 0; col < cols; ++col) {
            const int globalIdx = globalRow * cols + col;
            const int localIdx = localRow * cols + col;
            const auto& v = field[static_cast<std::size_t>(globalIdx)];
            hostField[static_cast<std::size_t>(localIdx)] = float2{v.x, v.y};
        }
    }

    float2* dField = nullptr;
    int* dSuccessor = nullptr;

    const auto cleanup = [&]() {
        if (dField != nullptr) { cudaFree(dField); dField = nullptr; }
        if (dSuccessor != nullptr) { cudaFree(dSuccessor); dSuccessor = nullptr; }
    };

    try {
        cudaCheck(cudaMalloc(reinterpret_cast<void**>(&dField),
                             sizeof(float2) * static_cast<std::size_t>(localTotal)),
                  "cudaMalloc(dField slice)");
        cudaCheck(cudaMalloc(reinterpret_cast<void**>(&dSuccessor),
                             sizeof(int) * static_cast<std::size_t>(localTotal)),
                  "cudaMalloc(dSuccessor slice)");
        cudaCheck(cudaMemcpy(dField, hostField.data(),
                             sizeof(float2) * static_cast<std::size_t>(localTotal),
                             cudaMemcpyHostToDevice),
                  "cudaMemcpy H2D field slice");

        const float rowSpacing = (rows > 1) ? (bounds.yMax - bounds.yMin) / static_cast<float>(rows - 1) : 1.0f;
        const float colSpacing = (cols > 1) ? (bounds.xMax - bounds.xMin) / static_cast<float>(cols - 1) : 1.0f;

        const int blockSize = static_cast<int>(cudaBlockSize);
        const int launchSize = (localTotal + blockSize - 1) / blockSize;
        computeSuccessorSliceKernel<<<launchSize, blockSize>>>(dField, rows, cols, rowSpacing,
                                                               colSpacing, startRow, localRows,
                                                               dSuccessor);
        cudaCheck(cudaGetLastError(), "computeSuccessorSliceKernel launch");
        cudaCheck(cudaDeviceSynchronize(), "cudaDeviceSynchronize slice");

        std::vector<int> successor(static_cast<std::size_t>(localTotal));
        cudaCheck(cudaMemcpy(successor.data(), dSuccessor,
                             sizeof(int) * static_cast<std::size_t>(localTotal),
                             cudaMemcpyDeviceToHost),
                  "cudaMemcpy D2H successor slice");
        cleanup();
        return successor;
    } catch (...) {
        cleanup();
        throw;
    }
}

} // namespace cuda
