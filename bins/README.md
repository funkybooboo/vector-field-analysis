# bins

Each binary has its own source, docs, tests, and build target.

| Binary | Description |
|--------|-------------|
| [`analyzer`](analyzer) | Reads `field.h5`, constructs a `FieldGrid` per time step, and traces streamlines via `traceStreamlineStep` |
| [`simulator`](simulator) | Generates configurable vector fields from a TOML config and writes them to `field.h5` |

## Running

```sh
mise run run:bin:simulator   # generate field.h5
mise run run:bin:analyzer    # read field.h5 and trace streamlines
```
