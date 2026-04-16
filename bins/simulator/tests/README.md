# simulator/tests

Catch2 tests for the `simulator` binary.

| File | Description |
|------|-------------|
| `test_simulator.cpp` | Tests for `FieldGenerator`, `ConfigParser`, and `FieldWriter`: all field types (vortex, uniform, source, sink, saddle, spiral, noise, custom), superposition, viscous decay, config parsing, and HDF5 output |

## Running

```sh
mise run test:simulator
```
