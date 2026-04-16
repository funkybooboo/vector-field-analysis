#include "cudaFull.hpp"

#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
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

__global__ void computeSuccessorKernel(const float2* field,
                                       int rows,
                                       int cols,
                                       float xMin,
                                       float xMax,
                                       float yMin,
                                       float yMax,
                                       int* successor) {
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

    const int destRow = clampInt(
        static_cast<int>(roundf((physY + v.y - yMin) / rowSpacing)),
        0,
        rows - 1);

    const int destCol = clampInt(
        static_cast<int>(roundf((physX + v.x - xMin) / colSpacing)),
        0,
        cols - 1);

    successor[idx] = toIndex(destRow, destCol, cols);
}

__global__ void initUnionFindKernel(int n, int* parent, int* rank) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        parent[idx] = idx;
        rank[idx] = 0;
    }
}

__global__ void unionSuccessorKernel(int n,
                                     const int* successor,
                                     int* parent,
                                     int* rank) {
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

Result computeComponents(const Field::Slice& field, const Field::Bounds& bounds) {
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

        cudaCheck(cudaMemcpy(dField,
                             hostField.data(),
                             sizeof(float2) * static_cast<std::size_t>(total),
                             cudaMemcpyHostToDevice),
                  "cudaMemcpy H2D field");

        constexpr int blockSize = 256;
        const int launchSize = (total + blockSize - 1) / blockSize;

        computeSuccessorKernel<<<launchSize, blockSize>>>(
            dField,
            rows,
            cols,
            bounds.xMin,
            bounds.xMax,
            bounds.yMin,
            bounds.yMax,
            dSuccessor);
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

        cudaCheck(cudaMemcpy(successor.data(),
                             dSuccessor,
                             sizeof(int) * static_cast<std::size_t>(total),
                             cudaMemcpyDeviceToHost),
                  "cudaMemcpy D2H successor");

        cudaCheck(cudaMemcpy(parent.data(),
                             dParent,
                             sizeof(int) * static_cast<std::size_t>(total),
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
    if (static_cast<int>(result.successor.size()) != total ||
        static_cast<int>(result.componentId.size()) != total) {
        throw std::runtime_error("cudaFull::reconstructPaths received inconsistent result sizes");
    }

    const int componentCount =
        *std::max_element(result.componentId.begin(), result.componentId.end()) + 1;

    std::vector<int> indegree(static_cast<std::size_t>(total), 0);
    for (int idx = 0; idx < total; ++idx) {
        const int succ = result.successor[static_cast<std::size_t>(idx)];
        if (succ >= 0 && succ < total) {
            indegree[static_cast<std::size_t>(succ)]++;
        }
    }

    std::vector<std::vector<int>> nodesByComponent(static_cast<std::size_t>(componentCount));
    for (int idx = 0; idx < total; ++idx) {
        nodesByComponent[static_cast<std::size_t>(result.componentId[static_cast<std::size_t>(idx)])]
            .push_back(idx);
    }

    std::vector<Field::Path> paths;

    for (int component = 0; component < componentCount; ++component) {
        const auto& nodes = nodesByComponent[static_cast<std::size_t>(component)];
        std::vector<bool> visited(static_cast<std::size_t>(total), false);

        // First prefer source-like starts (indegree == 0) for deterministic paths.
        for (const int start : nodes) {
            if (indegree[static_cast<std::size_t>(start)] != 0 ||
                visited[static_cast<std::size_t>(start)]) {
                continue;
            }

            Field::Path path;
            int current = start;

            while (!visited[static_cast<std::size_t>(current)]) {
                visited[static_cast<std::size_t>(current)] = true;
                path.push_back(toGridCell(current, result.cols));

                const int next = result.successor[static_cast<std::size_t>(current)];
                if (next < 0 || next >= total ||
                    result.componentId[static_cast<std::size_t>(next)] != component) {
                    break;
                }
                current = next;
            }

            if (!path.empty()) {
                paths.push_back(std::move(path));
            }
        }

        // handle cycles / remaining nodes not reached from indegree-0 starts
        for (const int start : nodes) {
            if (visited[static_cast<std::size_t>(start)]) {
                continue;
            }

            Field::Path path;
            int current = start;

            while (!visited[static_cast<std::size_t>(current)]) {
                visited[static_cast<std::size_t>(current)] = true;
                path.push_back(toGridCell(current, result.cols));

                const int next = result.successor[static_cast<std::size_t>(current)];
                if (next < 0 || next >= total ||
                    result.componentId[static_cast<std::size_t>(next)] != component) {
                    break;
                }
                current = next;
            }

            if (!path.empty()) {
                paths.push_back(std::move(path));
            }
        }
    }

    return paths;
}

} // namespace cudaFull
