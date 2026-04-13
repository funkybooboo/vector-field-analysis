#include "streamWriter.hpp"

#include <highfive/highfive.hpp>
#include <vector>

namespace StreamWriter {

void write(const std::string& outPath, const std::vector<StepStreamlines>& streamlinesByStep,
           const Vector::FieldBounds& bounds, const Vector::GridSize& grid) {
    // Truncate: overwrite any existing file so reruns don't require manual cleanup.
    HighFive::File file(outPath, HighFive::File::Truncate);
    auto streamsGroup = file.createGroup("streams");

    streamsGroup.createAttribute("xMin", bounds.xMin);
    streamsGroup.createAttribute("xMax", bounds.xMax);
    streamsGroup.createAttribute("yMin", bounds.yMin);
    streamsGroup.createAttribute("yMax", bounds.yMax);
    streamsGroup.createAttribute("width", grid.width);
    streamsGroup.createAttribute("height", grid.height);
    streamsGroup.createAttribute("num_steps", static_cast<int>(streamlinesByStep.size()));

    for (std::size_t s = 0; s < streamlinesByStep.size(); ++s) {
        const StepStreamlines& stepStreamlines = streamlinesByStep[s];
        auto stepGroup = streamsGroup.createGroup("step_" + std::to_string(s));

        // Build CSR-style flat array and offsets.
        // offsets[i] = start index in paths_flat for streamline i.
        // offsets[S] = total number of points (sentinel).
        const auto numStreams = stepStreamlines.size();
        std::vector<int> offsets(numStreams + 1);
        offsets[0] = 0;
        for (std::size_t i = 0; i < numStreams; ++i) {
            offsets[i + 1] = offsets[i] + static_cast<int>(stepStreamlines[i].size());
        }

        const auto totalPoints = static_cast<std::size_t>(offsets[numStreams]);

        // paths_flat: shape (totalPoints, 2), each row is [row, col]
        std::vector<std::vector<int>> pathsFlat(totalPoints, std::vector<int>(2));
        std::size_t idx = 0;
        for (const auto& path : stepStreamlines) {
            for (const auto& pt : path) {
                pathsFlat[idx][0] = pt.row;
                pathsFlat[idx][1] = pt.col;
                ++idx;
            }
        }

        stepGroup.createDataSet("paths_flat", pathsFlat);
        stepGroup.createDataSet("offsets", offsets);
    }
}

} // namespace StreamWriter
