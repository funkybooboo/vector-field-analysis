#pragma once
#include <utility>
namespace utils {

// used to calculate the splits for a given FieldGrid to allow for easier multi programming
// returns a pair that covers the rows per computation unit and the left over rows
inline std::pair<int, int> calculateRowSplit(unsigned long rowCount, unsigned long splits) {

    const unsigned long rowsPerCompute = rowCount / splits;
    const unsigned long leftOverRows = rowCount % splits;

    return std::make_pair(rowsPerCompute, leftOverRows);
}
} // namespace utils
