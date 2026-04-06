# simulator/src

Source and header files for the `simulator` binary.

| File | Description |
|------|-------------|
| `main.cpp` | Entry point - parses args, calls ConfigParser -> FieldGenerator -> FieldWriter |
| `simulatorConfig.hpp` | `SimulatorConfig` and `FieldLayerConfig` types; `FieldType` enum |
| `configParser.hpp` / `configParser.cpp` | Parses a TOML file into `SimulatorConfig` |
| `fieldGenerator.hpp` / `fieldGenerator.cpp` | Generates a `FieldTimeSeries` from a `SimulatorConfig` |
| `fieldWriter.hpp` / `fieldWriter.cpp` | Writes a `FieldTimeSeries` to an HDF5 file |
