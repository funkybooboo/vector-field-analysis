# analyzer/src

Source and header files for the `analyzer` binary.

| File | Description |
|------|-------------|
| `main.cpp` | Entry point - reads HDF5, constructs a `FieldGrid` per step, calls `traceStreamlineStep` |
| `fieldReader.hpp` / `fieldReader.cpp` | Reads an HDF5 file into a `FieldTimeSeries` (3D grid of `Vec2`) |
| `vectorField.hpp` / `vectorField.cpp` | `FieldGrid` with `neighborInVectorDirection`, `traceStreamlineStep`, and `joinStreamlines` |
