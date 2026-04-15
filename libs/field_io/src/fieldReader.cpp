#include "fieldReader.hpp"

#include "fieldIoCommon.hpp"

#include <highfive/highfive.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace FieldReader {

Field::TimeSeries read(const std::string& path) {
    HighFive::File file(path, HighFive::File::ReadOnly);
    const auto group = file.getGroup("field");

    RawFieldData vxRaw;
    RawFieldData vyRaw;
    group.getDataSet("vx").read(vxRaw);
    group.getDataSet("vy").read(vyRaw);

    // Guard against malformed files before indexing into the 3-D array below.
    if (vxRaw.empty() || vxRaw[0].empty() || vxRaw[0][0].empty()) {
        throw std::runtime_error("Field dataset is empty in: " + path);
    }
    if (vyRaw.size() != vxRaw.size() || vyRaw[0].size() != vxRaw[0].size() ||
        vyRaw[0][0].size() != vxRaw[0][0].size()) {
        throw std::runtime_error("vx and vy dataset shapes do not match in: " + path);
    }

    Field::TimeSeries result;
    group.getAttribute("xMin").read(result.bounds.xMin);
    group.getAttribute("xMax").read(result.bounds.xMax);
    group.getAttribute("yMin").read(result.bounds.yMin);
    group.getAttribute("yMax").read(result.bounds.yMax);

    const std::size_t numFrames = vxRaw.size();
    const std::size_t height = vxRaw[0].size();
    const std::size_t width = vxRaw[0][0].size();

    result.frames.resize(numFrames);
    for (std::size_t frame = 0; frame < numFrames; ++frame) {
        result.frames[frame].resize(height);
        for (std::size_t row = 0; row < height; ++row) {
            result.frames[frame][row].resize(width, Vector::Vec2{});
            for (std::size_t col = 0; col < width; ++col) {
                result.frames[frame][row][col] =
                    Vector::Vec2(vxRaw[frame][row][col], vyRaw[frame][row][col]);
            }
        }
    }

    return result;
}

} // namespace FieldReader
