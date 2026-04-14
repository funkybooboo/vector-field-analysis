#include "fieldWriter.hpp"
#include "fieldIOCommon.hpp"

#include <highfive/highfive.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace FieldWriter {

void write(const std::string& path, const Field::TimeSeries& field,
           const std::string& typeLabel, float dt, float viscosity) {
    const Field::GridSize gridSize = field.gridSize();
    if (field.frames.empty() || gridSize.width == 0 || gridSize.height == 0) {
        throw std::runtime_error("Cannot write empty field to: " + path);
    }

    HighFive::File file(path, HighFive::File::Overwrite);
    auto group = file.createGroup("field");

    // HDF5 requires flat numeric arrays; extract x/y components from Vec2.
    const std::size_t numFrames = field.frames.size();
    const auto numRows = static_cast<std::size_t>(gridSize.height);
    const auto numCols = static_cast<std::size_t>(gridSize.width);
    RawFieldData vx(numFrames, std::vector<std::vector<float>>(numRows, std::vector<float>(numCols)));
    RawFieldData vy(numFrames, std::vector<std::vector<float>>(numRows, std::vector<float>(numCols)));
    for (std::size_t frameIndex = 0; frameIndex < numFrames; ++frameIndex) {
        for (std::size_t rowIndex = 0; rowIndex < numRows; ++rowIndex) {
            for (std::size_t colIndex = 0; colIndex < numCols; ++colIndex) {
                vx[frameIndex][rowIndex][colIndex] = field.frames[frameIndex][rowIndex][colIndex].x;
                vy[frameIndex][rowIndex][colIndex] = field.frames[frameIndex][rowIndex][colIndex].y;
            }
        }
    }
    group.createDataSet("vx", vx);
    group.createDataSet("vy", vy);

    // Store grid geometry and simulation parameters as HDF5 attributes alongside
    // the data so the file is self-contained -- readers don't need the config file.
    group.createAttribute("type", typeLabel);
    group.createAttribute("steps", static_cast<int>(numFrames));
    group.createAttribute("dt", dt);
    group.createAttribute("viscosity", viscosity);
    writeGeometryAttributes(group, field.bounds, gridSize);
}

} // namespace FieldWriter
