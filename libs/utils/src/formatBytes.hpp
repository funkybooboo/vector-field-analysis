#pragma once
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace Utils {

inline std::string formatBytes(std::uintmax_t bytes) {
    const auto d = static_cast<double>(bytes);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    if (bytes >= 1024ULL * 1024 * 1024) {
        oss << (d / (1024.0 * 1024 * 1024)) << " GB";
    } else if (bytes >= 1024ULL * 1024) {
        oss << (d / (1024.0 * 1024)) << " MB";
    } else if (bytes >= 1024ULL) {
        oss << (d / 1024.0) << " KB";
    } else {
        oss << std::defaultfloat << bytes << " B";
    }
    return oss.str();
}

} // namespace Utils
