# bins

Each binary has its own source, docs, tests, and build target.

| Binary | Description |
|--------|-------------|
| [`analyzer`](analyzer) | Reads `field.h5`, constructs a `Field::Grid` per time step, and traces streamlines across every cell |
| [`simulator`](simulator) | Generates configurable vector fields from a TOML config and writes them to `field.h5` |

## Running

```sh
mise run run:simulator   # generate field.h5
mise run run:analyzer    # read field.h5 and trace streamlines
```
