#include "fieldReader.hpp"

#include <highfive/highfive.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace FieldReader {

Vector::FieldTimeSeries read(const std::string& path) {
    HighFive::File file(path, HighFive::File::ReadOnly);
    const auto group = file.getGroup("field");

    std::vector<std::vector<std::vector<float>>> vx3d;
    std::vector<std::vector<std::vector<float>>> vy3d;
    group.getDataSet("vx").read(vx3d);
    group.getDataSet("vy").read(vy3d);

    // Guard against malformed files before indexing into the 3-D array below.
    if (vx3d.empty() || vx3d[0].empty() || vx3d[0][0].empty()) {
        throw std::runtime_error("Field dataset is empty in: " + path);
    }

    Vector::FieldTimeSeries result;
    group.getAttribute("xMin").read(result.xMin);
    group.getAttribute("xMax").read(result.xMax);
    group.getAttribute("yMin").read(result.yMin);
    group.getAttribute("yMax").read(result.yMax);

    const std::size_t numSteps = vx3d.size();
    const std::size_t height = vx3d[0].size();
    const std::size_t width = vx3d[0][0].size();

    result.steps.resize(numSteps);
    for (std::size_t step = 0; step < numSteps; ++step) {
        result.steps[step].resize(height);
        for (std::size_t row = 0; row < height; ++row) {
            result.steps[step][row].resize(width, Vector::Vec2{});
            for (std::size_t col = 0; col < width; ++col) {
                result.steps[step][row][col] =
                    Vector::Vec2(vx3d[step][row][col], vy3d[step][row][col]);
            }
        }
    }

    return result;
}

} // namespace FieldReader
