# analyzer/src

Source and header files for the `analyzer` binary.

| File | Description |
|------|-------------|
| `main.cpp` | Entry point -- orchestrates config, I/O, solver selection, timing, and output |
| `configParser.hpp` / `configParser.cpp` | Parses `[analyzer]` TOML table into `AnalyzerConfig` |
| `fieldReader.hpp` / `fieldReader.cpp` | Reads an HDF5 file into a `Field::TimeSeries` |
| `streamlineSolver.hpp` | Abstract `StreamlineSolver` base class with `computeTimeStep(Field::Grid&)` |
| `solverFactory.hpp` / `solverFactory.cpp` | Factory: solver name → `std::unique_ptr<StreamlineSolver>` |
| `sequentialStreamlineSolver.hpp` / `sequentialStreamlineSolver.cpp` | Single-threaded reference implementation |
| `openMpStreamlineSolver.hpp` / `openMpStreamlineSolver.cpp` | Shared-memory implementation via OpenMP |
| `pthreadsStreamlineSolver.hpp` / `pthreadsStreamlineSolver.cpp` | Shared-memory implementation via pthreads |
| `mpiStreamlineSolver.hpp` / `mpiStreamlineSolver.cpp` | Distributed-memory implementation via MPI |
| `cudaStreamlineSolver.hpp` / `cudaStreamlineSolver.cu` | GPU solver entry point (delegates to `cuda::` kernels) |
| `cuda.hpp` / `cuda.cu` | GPU union-find connected-component kernels (`cuda::` namespace) |
| `streamWriter.hpp` / `streamWriter.cpp` | Writes traced streamlines to `streams.h5` |
