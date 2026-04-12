#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>
namespace utils {

// Calculates row splits for a given FieldGrid to support parallel computation.
// Returns a pair containing the rows per computation unit and the leftover rows.
inline std::pair<std::size_t, std::size_t> calculateRowSplit(std::size_t rowCount,
                                                             std::size_t splits) {
    if (splits == 0) {
        throw std::invalid_argument("splits must be greater than 0");
    }
    const std::size_t rowsPerCompute = rowCount / splits;
    const std::size_t leftOverRows = rowCount % splits;
    return std::make_pair(rowsPerCompute, leftOverRows);
}
} // namespace utils
