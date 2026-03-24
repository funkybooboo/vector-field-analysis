# bins

Each binary has its own source, docs, tests, and build target.

| Binary | Description |
|---|---|
| [`analyzer`](analyzer) | Reads `field.h5`, constructs a `VectorField`, and traces streamlines via `flowFromVector` |
| [`simulator`](simulator) | Generates a 64x64 vortex field and writes it to `field.h5` |

## Running

```sh
mise run run:bin:simulator   # generate field.h5
mise run run:bin:analyzer    # read field.h5 and trace streamlines
```
