# Analyzer

Parses vector field data and computes properties such as streamlines and flow paths.

## Responsibilities

- Ingest vector field data
- Compute streamlines via `flowFromVector`
- Merge and manage streamline paths across the field

## Dependencies

- [`libs/vector`](../../libs/vector) -> Vector and StreamLine types

## Tasks

```sh
mise run build:bin:analyzer   # build
mise run test:bin:analyzer    # test
mise run run:bin:analyzer     # run (reads field.h5)
```
