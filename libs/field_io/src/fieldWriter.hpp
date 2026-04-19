#pragma once
#include "fieldTypes.hpp"

#include <cstddef>
#include <highfive/highfive.hpp>
#include <optional>
#include <string>

namespace FieldWriter {

// Writes an entire TimeSeries to HDF5 in one shot.
// Allocates full vx/vy arrays in memory; only suitable for small grids.
void write(const std::string& path, const Field::TimeSeries& field, const std::string& typeLabel,
           float dt, float viscosity);

// Streams frames to HDF5 one at a time -- holds only one frame in memory.
// Call writeFrame() exactly numFrames times in order (0 .. numFrames-1).
class StreamingWriter {
  public:
    StreamingWriter(const std::string& path, const Field::Bounds& bounds,
                    const Field::GridSize& gridSize, std::size_t numFrames,
                    const std::string& typeLabel, float dt, float viscosity);
    void writeFrame(std::size_t frameIndex, const Field::Slice& frame);

  private:
    HighFive::File file_;
    std::optional<HighFive::DataSet> vxDs_;
    std::optional<HighFive::DataSet> vyDs_;
    std::size_t numRows_;
    std::size_t numCols_;
};

} // namespace FieldWriter
