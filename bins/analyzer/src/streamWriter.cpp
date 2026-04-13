#include "streamWriter.hpp"

#include <highfive/highfive.hpp>
#include <vector>

namespace StreamWriter {

void write(const std::string& outPath, const std::vector<StepStreamlines>& allSteps, float xMin,
           float xMax, float yMin, float yMax, int width, int height) {
    HighFive::File file(outPath, HighFive::File::Truncate);
    auto grp = file.createGroup("streams");

    grp.createAttribute("xMin", xMin);
    grp.createAttribute("xMax", xMax);
    grp.createAttribute("yMin", yMin);
    grp.createAttribute("yMax", yMax);
    grp.createAttribute("width", width);
    grp.createAttribute("height", height);
    grp.createAttribute("num_steps", static_cast<int>(allSteps.size()));

    for (std::size_t s = 0; s < allSteps.size(); ++s) {
        const StepStreamlines& stepStreams = allSteps[s];
        auto stepGrp = grp.createGroup("step_" + std::to_string(s));

        // Build CSR-style flat array and offsets.
        // offsets[i] = start index in paths_flat for streamline i.
        // offsets[S] = total number of points (sentinel).
        const auto numStreams = stepStreams.size();
        std::vector<int> offsets(numStreams + 1);
        offsets[0] = 0;
        for (std::size_t i = 0; i < numStreams; ++i) {
            offsets[i + 1] = offsets[i] + static_cast<int>(stepStreams[i].size());
        }

        const auto totalPoints = static_cast<std::size_t>(offsets[numStreams]);

        // paths_flat: shape (totalPoints, 2), each row is [row, col]
        std::vector<std::vector<int>> pathsFlat(totalPoints, std::vector<int>(2));
        std::size_t idx = 0;
        for (const auto& path : stepStreams) {
            for (const auto& pt : path) {
                pathsFlat[idx][0] = pt.first;
                pathsFlat[idx][1] = pt.second;
                ++idx;
            }
        }

        stepGrp.createDataSet("paths_flat", pathsFlat);
        stepGrp.createDataSet("offsets", offsets);
    }
}

} // namespace StreamWriter
