#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = ["h5py", "numpy"]
# ///
"""Print statistics for field and stream .h5 files produced by the simulator/analyzer.

Usage:
    python tools/stats.py field.h5
    python tools/stats.py field.streams.h5
    python tools/stats.py field.h5 field.streams.h5
"""

import argparse
import os
import sys

import h5py
import numpy as np


def file_size_str(path):
    size = os.path.getsize(path)
    for unit in ("B", "KB", "MB", "GB"):
        if size < 1024:
            return f"{size:.1f} {unit}"
        size /= 1024
    return f"{size:.1f} TB"


def stats_block(arr):
    """Return (min, max, mean, std) as formatted strings."""
    return arr.min(), arr.max(), arr.mean(), arr.std()


def print_field(path):
    with h5py.File(path, "r") as f:
        if "field" not in f:
            print(f"  [error] no 'field' group in {path}", file=sys.stderr)
            return
        g = f["field"]
        if "vx" not in g or "vy" not in g:
            print(f"  [error] missing 'vx' or 'vy' dataset in {path}", file=sys.stderr)
            return
        vx = g["vx"][:]  # (steps, height, width)
        vy = g["vy"][:]
        attrs = dict(g.attrs)

    if vx.ndim != 3:
        print(f"  [error] expected 3-D vx array, got shape {vx.shape} in {path}",
              file=sys.stderr)
        return
    steps, height, width = vx.shape
    speed = np.sqrt(vx**2 + vy**2)  # (steps, height, width)

    # Per-step speed stats for the summary table
    step_means = speed.reshape(steps, -1).mean(axis=1)
    step_maxes = speed.reshape(steps, -1).max(axis=1)

    print(f"{path}  ({file_size_str(path)})")
    print(f"  type:       {attrs.get('type', 'unknown')}")
    print(f"  grid:       {width} x {height}")
    print(f"  bounds:     x [{attrs.get('xMin', '?'):.4g}, {attrs.get('xMax', '?'):.4g}]"
          f"  y [{attrs.get('yMin', '?'):.4g}, {attrs.get('yMax', '?'):.4g}]")
    print(f"  steps:      {steps}  (dt={attrs.get('dt', '?'):.4g})")
    print(f"  viscosity:  {attrs.get('viscosity', '?'):.4g}")

    lo, hi, mean, std = stats_block(speed)
    print()
    print("  Speed (all steps, all cells):")
    print(f"    min    {lo:.4f}")
    print(f"    max    {hi:.4f}")
    print(f"    mean   {mean:.4f}")
    print(f"    std    {std:.4f}")

    lo2, hi2, mean2, std2 = stats_block(step_means)
    print()
    print("  Mean speed per step:")
    print(f"    min    {lo2:.4f}  (step {int(step_means.argmin())})")
    print(f"    max    {hi2:.4f}  (step {int(step_means.argmax())})")
    print(f"    mean   {mean2:.4f}")
    print(f"    std    {std2:.4f}")

    lo3, hi3, mean3, std3 = stats_block(step_maxes)
    print()
    print("  Peak speed per step:")
    print(f"    min    {lo3:.4f}  (step {int(step_maxes.argmin())})")
    print(f"    max    {hi3:.4f}  (step {int(step_maxes.argmax())})")
    print(f"    mean   {mean3:.4f}")
    print(f"    std    {std3:.4f}")


def print_streams(path):
    with h5py.File(path, "r") as f:
        if "streams" not in f:
            print(f"  [error] no 'streams' group in {path}", file=sys.stderr)
            return
        g = f["streams"]
        attrs = dict(g.attrs)
        if "num_steps" not in attrs:
            print(f"  [error] missing 'num_steps' attribute in {path}", file=sys.stderr)
            return
        num_steps = int(attrs["num_steps"])

        counts_per_step = []   # streamlines per step
        lengths = []           # all path lengths (in grid points) across all steps

        for s in range(num_steps):
            sg = g[f"step_{s}"]
            offsets = sg["offsets"][:]
            n = len(offsets) - 1
            counts_per_step.append(n)
            for i in range(n):
                lengths.append(int(offsets[i + 1] - offsets[i]))

    counts = np.array(counts_per_step, dtype=np.int64)
    lengths_arr = np.array(lengths, dtype=np.int64) if lengths else np.array([0], dtype=np.int64)

    total_streamlines = int(counts.sum())
    total_points = int(lengths_arr.sum())

    print(f"{path}  ({file_size_str(path)})")
    print(f"  grid:       {attrs.get('width', '?')} x {attrs.get('height', '?')}")
    print(f"  bounds:     x [{attrs.get('xMin', '?'):.4g}, {attrs.get('xMax', '?'):.4g}]"
          f"  y [{attrs.get('yMin', '?'):.4g}, {attrs.get('yMax', '?'):.4g}]")
    print(f"  steps:      {num_steps}")
    print(f"  streamlines:{total_streamlines}  ({total_streamlines / num_steps:.1f}/step avg)")
    print(f"  total pts:  {total_points}")

    print()
    print("  Streamlines per step:")
    print(f"    min    {int(counts.min())}  (step {int(counts.argmin())})")
    print(f"    max    {int(counts.max())}  (step {int(counts.argmax())})")
    print(f"    mean   {counts.mean():.2f}")
    print(f"    std    {counts.std():.2f}")

    print()
    print("  Path length (grid points):")
    print(f"    min    {int(lengths_arr.min())}")
    print(f"    max    {int(lengths_arr.max())}")
    print(f"    mean   {lengths_arr.mean():.2f}")
    print(f"    std    {lengths_arr.std():.2f}")


def detect_and_print(path):
    try:
        with h5py.File(path, "r") as f:
            keys = set(f.keys())
    except Exception as e:
        print(f"Error opening {path}: {e}", file=sys.stderr)
        return

    try:
        if "field" in keys:
            print_field(path)
        elif "streams" in keys:
            print_streams(path)
        else:
            print(f"Error: {path} has no 'field' or 'streams' group (keys: {sorted(keys)})",
                  file=sys.stderr)
    except Exception as e:
        print(f"Error processing {path}: {e}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description="Print stats for field or stream .h5 files.")
    parser.add_argument("files", nargs="+", metavar="FILE", help=".h5 file(s) to inspect")
    args = parser.parse_args()

    for i, path in enumerate(args.files):
        if i > 0:
            print()
        detect_and_print(path)


if __name__ == "__main__":
    main()
