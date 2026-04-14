#include "streamWriter.hpp"

#include "fieldIOCommon.hpp"

#include <cstdint>
#include <highfive/highfive.hpp>
#include <limits>
#include <stdexcept>
#include <vector>

namespace StreamWriter {

void write(const std::string& outPath, const std::vector<StepStreamlines>& streamlinesByStep,
           const Field::Bounds& bounds, const Field::GridSize& grid) {
    // Truncate: overwrite any existing file so reruns don't require manual cleanup.
    HighFive::File file(outPath, HighFive::File::Truncate);
    auto streamsGroup = file.createGroup("streams");

    writeGeometryAttributes(streamsGroup, bounds, grid);
    streamsGroup.createAttribute("num_steps", static_cast<int>(streamlinesByStep.size()));

    for (std::size_t stepIndex = 0; stepIndex < streamlinesByStep.size(); ++stepIndex) {
        const StepStreamlines& stepStreamlines = streamlinesByStep[stepIndex];
        auto stepGroup = streamsGroup.createGroup("step_" + std::to_string(stepIndex));

        // Build CSR-style flat array and offsets.
        // offsets[streamlineIndex] = start index in paths_flat for that streamline.
        // offsets[S] = total number of points (sentinel).
        const auto numStreams = stepStreamlines.size();
        std::vector<int> offsets(numStreams + 1);
        offsets[0] = 0;
        for (std::size_t streamlineIndex = 0; streamlineIndex < numStreams; ++streamlineIndex) {
            const int64_t next = static_cast<int64_t>(offsets[streamlineIndex]) +
                                 static_cast<int64_t>(stepStreamlines[streamlineIndex].size());
            if (next > std::numeric_limits<int>::max()) {
                throw std::runtime_error("streamline data exceeds INT_MAX points in step " +
                                         std::to_string(stepIndex));
            }
            offsets[streamlineIndex + 1] = static_cast<int>(next);
        }

        const auto totalPoints = static_cast<std::size_t>(offsets[numStreams]);

        // paths_flat: shape (totalPoints, 2), each row is [row, col]
        std::vector<std::vector<int>> pathsFlat(totalPoints, std::vector<int>(2));
        std::size_t i = 0;
        for (const auto& path : stepStreamlines) {
            for (const auto& point : path) {
                pathsFlat[i][0] = point.row;
                pathsFlat[i][1] = point.col;
                ++i;
            }
        }

        stepGroup.createDataSet("paths_flat", pathsFlat);
        stepGroup.createDataSet("offsets", offsets);
    }
}

} // namespace StreamWriter
