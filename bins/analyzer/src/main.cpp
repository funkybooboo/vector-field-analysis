#include "fieldReader.hpp"
#include "vectorField.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        const std::string inPath = argc > 1 ? argv[1] : "field.h5";
        const FieldReader::FieldTimeSeries data = FieldReader::read(inPath);

        const int numSteps = static_cast<int>(data.steps.size());
        const int height = static_cast<int>(data.steps[0].size());
        const int width = static_cast<int>(data.steps[0][0].size());

        std::cout << "Loaded " << inPath << " (" << width << "x" << height << ", " << numSteps
                  << " steps)\n";

        for (int s = 0; s < numSteps; ++s) {
            VectorField::FieldGrid grid(data.xMin, data.xMax, data.yMin, data.yMax, data.steps[s]);
            grid.traceStreamlineStep({0, 0});
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
