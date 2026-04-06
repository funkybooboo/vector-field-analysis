# analyzer/tests

Catch2 tests for the `analyzer` binary.

| File | Description |
|------|-------------|
| `test_vectorfield.cpp` | Tests for `FieldGrid`: `neighborInVectorDirection`, `traceStreamlineStep`, `joinStreamlines`, edge clamping |
| `test_fieldReader.cpp` | Tests for `FieldReader::read()`: dimensions, attributes, Vec2 values, error cases |

## Running

```sh
mise run test:bin:analyzer
```
