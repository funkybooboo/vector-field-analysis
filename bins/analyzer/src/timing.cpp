#include "timing.hpp"

#include <iomanip>
#include <iostream>

void printTiming(const std::string& label, double ms, int labelWidth, double sequentialMs) {
    std::cout << std::left << std::setw(labelWidth) << label << ms << " ms"
              << "  (" << (ms > 0 ? sequentialMs / ms : 0.0) << "x vs sequential)\n";
}
