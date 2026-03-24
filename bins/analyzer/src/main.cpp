#include "vectorField.h"

#include <highfive/highfive.hpp>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    try {
        std::string inPath = argc > 1 ? argv[1] : "field.h5";

        HighFive::File file(inPath, HighFive::File::ReadOnly);
        auto group = file.getGroup("field");

        std::vector<std::vector<float>> vx;
        std::vector<std::vector<float>> vy;
        group.getDataSet("vx").read(vx);
        group.getDataSet("vy").read(vy);

        float xMin = 0.0f;
        float xMax = 0.0f;
        float yMin = 0.0f;
        float yMax = 0.0f;
        group.getAttribute("xMin").read(xMin);
        group.getAttribute("xMax").read(xMax);
        group.getAttribute("yMin").read(yMin);
        group.getAttribute("yMax").read(yMax);

        int height = static_cast<int>(vx.size());
        int width = static_cast<int>(vx[0].size());

        std::vector<std::vector<Vector::Vector>> field(
            height, std::vector<Vector::Vector>(width, Vector::Vector(0.0f, 0.0f)));
        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                field[j][i] = Vector::Vector(vx[j][i], vy[j][i]);
            }
        }

        VectorField::VectorField vf(xMin, xMax, yMin, yMax, field);
        vf.flowFromVector({0, 0});

        std::cout << "Loaded " << inPath << " (" << width << "x" << height << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
