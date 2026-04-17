#include "streamlineSolver.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

std::vector<Field::Path>
StreamlineSolver::reconstructPathsDSU(const Field::Grid& grid,
                                      const std::vector<Field::GridCell>& neighbors) {

    const std::size_t total = neighbors.size();
    if (total == 0) {
        return {};
    }

    const int colCount = static_cast<int>(grid.cols());

    // owner[idx] = streamline id currently owning this cell, or -1 if unclaimed
    std::vector<int> owner(total, -1);
    std::vector<std::vector<int>> paths;
    paths.reserve(total);

    // Replay the same row-major source->destination application order used by
    // the existing sequential implementation. This ensures deterministic output.
    for (std::size_t idx = 0; idx < total; ++idx) {
        int srcOwner = owner[idx];

        if (srcOwner == -1) {
            srcOwner = static_cast<int>(paths.size());
            paths.push_back({static_cast<int>(idx)});
            owner[idx] = srcOwner;
        }

        const auto& neighbor = neighbors[idx];
        const std::size_t dest = grid.coordsToIndex(neighbor.row, neighbor.col);

        if (dest >= total) {
            continue;
        }

        const int destOwner = owner[dest];

        if (destOwner == -1) {
            owner[dest] = srcOwner;
            paths[static_cast<std::size_t>(srcOwner)].push_back(static_cast<int>(dest));
        } else if (destOwner != srcOwner) {
            // Merge source streamline into destination streamline.
            // Match the reference logic of joining source into existing destination.
            for (const int point : paths[static_cast<std::size_t>(srcOwner)]) {
                paths[static_cast<std::size_t>(destOwner)].push_back(point);
                owner[static_cast<std::size_t>(point)] = destOwner;
            }
            paths[static_cast<std::size_t>(srcOwner)].clear();
        }
    }

    // Convert internal integer-path representation to Field::Path (vector of GridCell).
    std::vector<Field::Path> output;
    std::vector<bool> emitted(paths.size(), false);

    for (std::size_t idx = 0; idx < total; ++idx) {
        const int streamId = owner[idx];
        if (streamId < 0 || emitted[static_cast<std::size_t>(streamId)] ||
            paths[static_cast<std::size_t>(streamId)].empty()) {
            continue;
        }

        emitted[static_cast<std::size_t>(streamId)] = true;

        Field::Path path;
        for (const int point : paths[static_cast<std::size_t>(streamId)]) {
            path.push_back({point / colCount, point % colCount});
        }
        output.push_back(std::move(path));
    }

    return output;
}
