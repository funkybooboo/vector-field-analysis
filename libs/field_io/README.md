# field_io

HDF5 read/write for `Field::TimeSeries`. Used by the simulator to write field data and by the analyzer to read it.

## Contents

- **FieldReader** — reads an HDF5 file into a `Field::TimeSeries`
- **FieldWriter** — writes a `Field::TimeSeries` to HDF5 with metadata (`typeLabel`, `dt`, `viscosity`)

## Usage

Link against the `field_io` CMake target.

```cpp
#include "fieldReader.hpp"
#include "fieldWriter.hpp"

auto field = FieldReader::read("field.h5");
FieldWriter::write("out.h5", field, "karman", dt, viscosity);
```
