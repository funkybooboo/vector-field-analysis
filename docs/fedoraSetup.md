# Vector Field Analysis — Fedora Setup

## 1. Install System Dependencies

Install the required packages with `dnf`:

    sudo dnf install -y gcc-c++ cmake ninja-build hdf5-devel cppcheck python3 python3-pip git

NOTE: on Fedora, use `hdf5-devel`, not just `hdf5`, because the build needs the HDF5 headers and development libraries

## 2. Clean Machine-Specific Files (if needed)

If this repo was copied from another machine and contains local build artifacts or bundled library files, clean them first:

    git restore CMakeLists.txt
    rm -rf build build-sanitize build-coverage
    rm -rf bin include lib share
    rm -f cmake/hdf5-config*.cmake
    rm -f cmake/hdf5-targets*.cmake
    rm -f cmake/hdf5_static-targets*.cmake
    rm -rf cmake/Modules

(prevents CMake from accidentally using someone else's local HDF5 install or cached build outputs)

## 3. Verify Fedora HDF5 Tools

Make sure the system HDF5 wrappers are available:

    which h5cc
    which h5c++

Expected output should point to `/usr/bin/h5cc` and `/usr/bin/h5c++`

## 4. Configure and Build Manually

If your `cmake` command resolves to the `mise` CMake install, use the policy compatibility flag below:

    rm -rf build
    cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5

    cmake --build build -j

If you want to check which CMake is being used:

    cmake --version
    which cmake
    type -a cmake

You can explicitly use Fedora’s system CMake instead:

    /usr/bin/cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

## 5. Run Tests

    ctest --test-dir build --output-on-failure

Expected successful result:

    100% tests passed, 0 tests failed out of 64

## 6. Run the Project Manually

Generate a field:

    ./build/bins/simulator/simulator bins/simulator/configs/karman_street.toml

Example successful output:

    Wrote field.h5 (128x64, 120 steps, type=uniform+custom+custom)

Confirm the output file exists:

    ls -lh field.h5

Analyze the generated field:

    ./build/bins/analyzer/analyzer field.h5

Example successful output:

    Loaded field.h5 (128x64, 120 steps)
    Loaded field.h5: 128x64, 120 steps, type=uniform+custom+custom

Visualize the generated field:

    uv run tools/visualize.py field.h5

To render a single step instead of the full animation:

    uv run tools/visualize.py field.h5 --step 0

## 7. Other Example Simulator Configs

You can swap the config file to generate different vector fields:

    ./build/bins/simulator/simulator bins/simulator/configs/vortex.toml
    ./build/bins/simulator/simulator bins/simulator/configs/spiral_galaxy.toml
    ./build/bins/simulator/simulator bins/simulator/configs/wind_tunnel.toml

These overwrite `field.h5`, so if you want to keep multiple outputs, rename the file after each run

## 8. One-line Smoke Test

Make sure the configure/build task uses the CMake policy flag if `mise` is providing CMake 4.x:

    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5

Then run:

    mise run deps
    mise run build
    mise run test

You can also use the runtime tasks:

    mise run run:bin:simulator
    mise run run:bin:analyzer
    mise run visualize

## 10. Troubleshooting

If configure fails in HighFive / CMake compatibility, use:

    -DCMAKE_POLICY_VERSION_MINIMUM=3.5

during the `cmake` configure step.

If HDF5 is not found:

    sudo dnf install -y hdf5-devel
    which h5cc
    which h5c++

If wrong local libraries or build artifacts interfere, run the cleanup commands from section 2, then rebuild from scratch.

If you are not sure which `.h5` file was written:

    find . -maxdepth 2 -name "*.h5" | sort

## 11. Working Fedora workflow

    sudo dnf install -y gcc-c++ cmake ninja-build hdf5-devel cppcheck python3 python3-pip git

    git restore CMakeLists.txt
    rm -rf build build-sanitize build-coverage
    rm -rf bin include lib share
    rm -f cmake/hdf5-config*.cmake
    rm -f cmake/hdf5-targets*.cmake
    rm -f cmake/hdf5_static-targets*.cmake
    rm -rf cmake/Modules

    which h5cc
    which h5c++

    rm -rf build
    cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5

    cmake --build build -j
    ctest --test-dir build --output-on-failure

    ./build/bins/simulator/simulator bins/simulator/configs/karman_street.toml
    ./build/bins/analyzer/analyzer field.h5
    uv run tools/visualize.py field.h5 --step 0
