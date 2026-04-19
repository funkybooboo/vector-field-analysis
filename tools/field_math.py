#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = ["h5py", "numpy"]
# ///
"""Add or subtract two vector field .h5 files produced by the simulator.

Usage:
    python tools/field_math.py add  a.h5 b.h5 out.h5
    python tools/field_math.py sub  a.h5 b.h5 out.h5

Both files must have the same shape (steps, height, width).
Attributes (xMin, xMax, etc.) are taken from the first file.
"""

import argparse
import sys

import h5py


def load(path):
    with h5py.File(path, "r") as f:
        grp = f["field"]
        vx = grp["vx"][:]
        vy = grp["vy"][:]
        attrs = dict(grp.attrs)
    return vx, vy, attrs


def save(path, vx, vy, attrs):
    # Output mirrors the load() HDF5 schema so the result file is a drop-in
    # replacement usable by visualize.py and any other tool that reads .h5 fields.
    with h5py.File(path, "w") as f:
        grp = f.create_group("field")
        grp.create_dataset("vx", data=vx)
        grp.create_dataset("vy", data=vy)
        for k, v in attrs.items():
            grp.attrs[k] = v


def main():
    parser = argparse.ArgumentParser(
        description="Add or subtract two .h5 vector field files."
    )
    parser.add_argument("op", choices=["add", "sub"], help="Operation: add or sub")
    parser.add_argument("a", help="First .h5 file")
    parser.add_argument("b", help="Second .h5 file")
    parser.add_argument("out", help="Output .h5 file")
    args = parser.parse_args()

    try:
        vx_a, vy_a, attrs_a = load(args.a)
        vx_b, vy_b, attrs_b = load(args.b)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error reading files: {e}", file=sys.stderr)
        sys.exit(1)

    # Vector field superposition only makes physical sense when both fields share
    # the same grid geometry; a shape mismatch means the grids are incompatible.
    if vx_a.shape != vx_b.shape or vy_a.shape != vy_b.shape:
        print(
            f"Error: shape mismatch - {args.a} vx={vx_a.shape} vy={vy_a.shape},"
            f" {args.b} vx={vx_b.shape} vy={vy_b.shape}",
            file=sys.stderr,
        )
        sys.exit(1)

    if args.op == "add":
        vx_out = vx_a + vx_b
        vy_out = vy_a + vy_b
        op_symbol = "+"
    else:
        vx_out = vx_a - vx_b
        vy_out = vy_a - vy_b
        op_symbol = "-"

    # Output inherits spatial metadata (xMin/xMax etc.) from the first operand.
    # Merging attrs from two potentially different configs could produce an
    # inconsistent or ambiguous output file.
    try:
        save(args.out, vx_out, vy_out, attrs_a)
    except Exception as e:
        print(f"Error writing {args.out}: {e}", file=sys.stderr)
        sys.exit(1)

    steps, height, width = vx_out.shape
    print(
        f"{args.a} {op_symbol} {args.b} -> {args.out}  ({width}x{height}, {steps} steps)"
    )


if __name__ == "__main__":
    main()
