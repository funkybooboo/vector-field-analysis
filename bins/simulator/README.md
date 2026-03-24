# Simulator

Generates vector fields for use in research and testing.

## Responsibilities

- Procedurally produce vector fields with configurable properties
- Output fields in formats consumable by the analyzer

## Dependencies

- [`libs/vector`](../../libs/vector) -> Vector and StreamLine types

## Tasks

```sh
mise run build:bin:simulator  # build
mise run test:bin:simulator   # test
mise run run:bin:simulator    # run (writes field.h5)
```
