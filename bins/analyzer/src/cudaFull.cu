#include "cudaFull.hpp"

#include <algorithm>
#include <cmath>
#include <cuda_runtime.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace cudaFull {
namespace {

// flat 2D index helper
__host__ __device__ inline int toIndex(int row, int col, int cols) {
    return row * cols + col;
}

inline Field::GridCell toGridCell(int idx, int cols) {
    return Field::GridCell{idx / cols, idx % cols};
}

inline void cudaCheck(cudaError_t err, const char* what) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(what) + ": " + cudaGetErrorString(err));
    }
}

__device__ int clampInt(int value, int lo, int hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

// Union-find find + path halving
__device__ int findRoot(int* parent, int x) {
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }
    return x;
}

// Union-find union using atomicCAS
__device__ void unite(int* parent, int* rank, int a, int b) {
    while (true) {
        a = findRoot(parent, a);
        b = findRoot(parent, b);

        if (a == b) {
            return;
        }

        if (rank[a] < rank[b]) {
            const int tmp = a;
            a = b;
            b = tmp;
        }

        const int old = atomicCAS(&parent[b], b, a);
        if (old == b) {
            if (rank[a] == rank[b]) {
                atomicAdd(&rank[a], 1);
            }
            return;
        }
    }
}

__global__ void computeSuccessorKernel(const float2* field, int rows, int cols, float xMin,
                                       float xMax, float yMin, float yMax, int* successor) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    const int total = rows * cols;

    if (idx >= total) {
        return;
    }

    const int row = idx / cols;
    const int col = idx % cols;

    if (rows == 1 || cols == 1) {
        successor[idx] = idx;
        return;
    }

    const float2 v = field[idx];

    const float rowSpacing = (yMax - yMin) / static_cast<float>(rows - 1);
    const float colSpacing = (xMax - xMin) / static_cast<float>(cols - 1);

    // Match the same physical-coordinate logic used by Grid::downstreamCell.
    const float physY =
        yMin + ((yMax - yMin) * static_cast<float>(row) / static_cast<float>(rows - 1));
    const float physX =
        xMin + ((xMax - xMin) * static_cast<float>(col) / static_cast<float>(cols - 1));

    const int destRow =
        clampInt(static_cast<int>(roundf((physY + v.y - yMin) / rowSpacing)), 0, rows - 1);

    const int destCol =
        clampInt(static_cast<int>(roundf((physX + v.x - xMin) / colSpacing)), 0, cols - 1);

    successor[idx] = toIndex(destRow, destCol, cols);
}

__global__ void initUnionFindKernel(int n, int* parent, int* rank) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        parent[idx] = idx;
        rank[idx] = 0;
    }
}

__global__ void unionSuccessorKernel(int n, const int* successor, int* parent, int* rank) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        unite(parent, rank, idx, successor[idx]);
    }
}

__global__ void compressParentsKernel(int n, int* parent) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        parent[idx] = findRoot(parent, idx);
    }
}

} // namespace

Result computeComponents(const Field::Slice& field, const Field::Bounds& bounds,
                         unsigned int cudaBlockSize) {
    const int rows = static_cast<int>(field.size());
    if (rows == 0) {
        return {};
    }

    const int cols = static_cast<int>(field[0].size());
    if (cols == 0) {
        return {};
    }

    for (int row = 0; row < rows; ++row) {
        if (static_cast<int>(field[static_cast<std::size_t>(row)].size()) != cols) {
            throw std::runtime_error("cudaFull::computeComponents requires a rectangular field");
        }
    }

    const int total = rows * cols;

    std::vector<float2> hostField(static_cast<std::size_t>(total));
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const auto& v = field[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
            hostField[static_cast<std::size_t>(toIndex(row, col, cols))] = float2{v.x, v.y};
        }
    }

    float2* dField = nullptr;
    int* dSuccessor = nullptr;
    int* dParent = nullptr;
    int* dRank = nullptr;

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
        if (dRank != nullptr) {
            cudaFree(dRank);
            dRank = nullptr;
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

        cudaCheck(cudaMalloc(reinterpret_cast<void**>(&dRank),
                             sizeof(int) * static_cast<std::size_t>(total)),
                  "cudaMalloc(dRank)");

        cudaCheck(cudaMemcpy(dField, hostField.data(),
                             sizeof(float2) * static_cast<std::size_t>(total),
                             cudaMemcpyHostToDevice),
                  "cudaMemcpy H2D field");

        const int blockSize = static_cast<int>(cudaBlockSize);
        const int launchSize = (total + blockSize - 1) / blockSize;

        computeSuccessorKernel<<<launchSize, blockSize>>>(
            dField, rows, cols, bounds.xMin, bounds.xMax, bounds.yMin, bounds.yMax, dSuccessor);
        cudaCheck(cudaGetLastError(), "computeSuccessorKernel launch");

        initUnionFindKernel<<<launchSize, blockSize>>>(total, dParent, dRank);
        cudaCheck(cudaGetLastError(), "initUnionFindKernel launch");

        unionSuccessorKernel<<<launchSize, blockSize>>>(total, dSuccessor, dParent, dRank);
        cudaCheck(cudaGetLastError(), "unionSuccessorKernel launch");

        compressParentsKernel<<<launchSize, blockSize>>>(total, dParent);
        cudaCheck(cudaGetLastError(), "compressParentsKernel launch");

        cudaCheck(cudaDeviceSynchronize(), "cudaDeviceSynchronize");

        std::vector<int> successor(static_cast<std::size_t>(total));
        std::vector<int> parent(static_cast<std::size_t>(total));

        cudaCheck(cudaMemcpy(successor.data(), dSuccessor,
                             sizeof(int) * static_cast<std::size_t>(total), cudaMemcpyDeviceToHost),
                  "cudaMemcpy D2H successor");

        cudaCheck(cudaMemcpy(parent.data(), dParent, sizeof(int) * static_cast<std::size_t>(total),
                             cudaMemcpyDeviceToHost),
                  "cudaMemcpy D2H parent");

        cleanup();

        // Convert raw roots to dense labels 0..N-1
        std::unordered_map<int, int> rootToDense;
        std::vector<int> componentId(static_cast<std::size_t>(total));

        int nextLabel = 0;
        for (int idx = 0; idx < total; ++idx) {
            const int root = parent[static_cast<std::size_t>(idx)];
            const auto it = rootToDense.find(root);
            if (it == rootToDense.end()) {
                rootToDense.emplace(root, nextLabel);
                componentId[static_cast<std::size_t>(idx)] = nextLabel;
                ++nextLabel;
            } else {
                componentId[static_cast<std::size_t>(idx)] = it->second;
            }
        }

        Result result;
        result.rows = rows;
        result.cols = cols;
        result.successor = std::move(successor);
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
    if (static_cast<int>(result.successor.size()) != total) {
        throw std::runtime_error("cudaFull::reconstructPaths received inconsistent result sizes");
    }

    // owner[idx] = streamline id currently owning this cell, or -1 if unclaimed
    std::vector<int> owner(static_cast<std::size_t>(total), -1);

    // Each streamline is stored as a flattened list of cell indices.
    std::vector<std::vector<int>> paths;
    paths.reserve(static_cast<std::size_t>(total));

    // Replay the same row-major source->destination application order used by
    // the existing CPU implementations.
    for (int idx = 0; idx < total; ++idx) {
        int srcOwner = owner[static_cast<std::size_t>(idx)];

        // If this source cell does not yet belong to a streamline, create one
        // rooted at this source cell.
        if (srcOwner == -1) {
            srcOwner = static_cast<int>(paths.size());
            paths.push_back(std::vector<int>{idx});
            owner[static_cast<std::size_t>(idx)] = srcOwner;
        }

        const int dest = result.successor[static_cast<std::size_t>(idx)];
        if (dest < 0 || dest >= total) {
            continue;
        }

        const int destOwner = owner[static_cast<std::size_t>(dest)];

        if (destOwner == -1) {
            // Destination is unclaimed: extend the source streamline into it.
            owner[static_cast<std::size_t>(dest)] = srcOwner;
            paths[static_cast<std::size_t>(srcOwner)].push_back(dest);
        } else if (destOwner != srcOwner) {
            // Destination already belongs to another streamline:
            // merge source into destination, matching joinStreamlines(dest, src).
            for (const int point : paths[static_cast<std::size_t>(srcOwner)]) {
                paths[static_cast<std::size_t>(destOwner)].push_back(point);
                owner[static_cast<std::size_t>(point)] = destOwner;
            }
            paths[static_cast<std::size_t>(srcOwner)].clear();
        }
        // If destOwner == srcOwner, do nothing.
        // This matches the CPU logic where self-merges are ignored.
    }

    // Emit unique non-empty paths in deterministic row-major order.
    std::vector<Field::Path> output;
    std::vector<bool> emitted(paths.size(), false);

    for (int idx = 0; idx < total; ++idx) {
        const int streamId = owner[static_cast<std::size_t>(idx)];
        if (streamId < 0 ||
            emitted[static_cast<std::size_t>(streamId)] ||
            paths[static_cast<std::size_t>(streamId)].empty()) {
            continue;
        }

        emitted[static_cast<std::size_t>(streamId)] = true;

        Field::Path path;
        path.reserve(paths[static_cast<std::size_t>(streamId)].size());
        for (const int point : paths[static_cast<std::size_t>(streamId)]) {
            path.push_back(toGridCell(point, result.cols));
        }
        output.push_back(std::move(path));
    }

    return output;
}
} // namespace cudaFull
