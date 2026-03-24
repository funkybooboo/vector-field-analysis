# analyzer/src

Source and header files for the `analyzer` binary.

| File | Description |
|---|---|
| `main.cpp` | Entry point -> reads HDF5, constructs `VectorField`, calls `flowFromVector` |
| `vectorField.hpp` / `vectorField.cpp` | `VectorField` type with `pointsTo`, `flowFromVector`, and `mergeStreamLines` |
