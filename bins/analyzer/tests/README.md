# analyzer/tests

Catch2 tests for the `analyzer` binary.

| File | Description |
|------|-------------|
| `test_configParser.cpp` | Tests for `ConfigParser::parseAnalyzer()`: defaults, solver validation, thread count parsing, error cases |
| `test_implementations.cpp` | Tests all four solver implementations (sequential, openmp, pthreads, mpi) against the sequential reference |
| `test_solverFactory.cpp` | Tests `makeSolver()`: valid names, invalid names, thread count forwarding |
| `test_streamWriter.cpp` | Tests for `StreamWriter`: HDF5 output schema, path data, round-trip correctness |

## Running

```sh
mise run test:analyzer
```
