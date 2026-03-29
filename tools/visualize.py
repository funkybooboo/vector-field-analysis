#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = ["h5py", "matplotlib", "numpy"]
# ///
"""Visualize vector field .h5 files produced by the simulator.

Usage:
    python tools/visualize.py field.h5 [--step N] [--stride S] [--save out.gif]

    --step N     show a single step N instead of animating (0-indexed)
    --stride S   subsample grid by S for less cluttered arrows (default: 4)
    --save FILE  save animation to a gif or mp4 instead of showing it
"""

import argparse
import sys

import h5py
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.animation import FuncAnimation


def load(path):
    with h5py.File(path, "r") as f:
        grp = f["field"]
        vx = grp["vx"][:]  # (steps, height, width)
        vy = grp["vy"][:]
        attrs = dict(grp.attrs)
    return vx, vy, attrs


def make_grid(vx, vy, attrs, stride):
    steps, height, width = vx.shape
    x = np.linspace(attrs.get("xMin", -1), attrs.get("xMax", 1), width)
    y = np.linspace(attrs.get("yMin", -1), attrs.get("yMax", 1), height)
    X, Y = np.meshgrid(x[::stride], y[::stride])
    return X, Y, steps


def speed(vx_s, vy_s):
    return np.sqrt(vx_s**2 + vy_s**2)


def plot_step(ax, vx, vy, attrs, stride, step):
    vx_s = vx[step, ::stride, ::stride]
    vy_s = vy[step, ::stride, ::stride]
    X, Y, _ = make_grid(vx, vy, attrs, stride)
    mag = speed(vx_s, vy_s)
    ax.clear()
    q = ax.quiver(X, Y, vx_s, vy_s, mag, cmap="plasma", pivot="mid", scale=None)
    field_type = attrs.get("type", "unknown")
    total = vx.shape[0]
    ax.set_title(f"{field_type}  |  step {step}/{total - 1}")
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_aspect("equal")
    return q


def show_single(vx, vy, attrs, stride, step):
    fig, ax = plt.subplots(figsize=(6, 6))
    q = plot_step(ax, vx, vy, attrs, stride, step)
    fig.colorbar(q, ax=ax, label="speed")
    plt.tight_layout()
    plt.show()


def animate(vx, vy, attrs, stride, save):
    steps = vx.shape[0]
    fig, ax = plt.subplots(figsize=(6, 6))
    cbar = None

    def update(frame):
        nonlocal cbar
        q = plot_step(ax, vx, vy, attrs, stride, frame)
        if cbar is None:
            cbar = fig.colorbar(q, ax=ax, label="speed")
        else:
            cbar.update_normal(q)
        return (q,)

    interval = max(1000 // steps, 40)  # aim for ~25 fps, floor at 40 ms
    anim = FuncAnimation(fig, update, frames=steps, interval=interval, blit=False)
    plt.tight_layout()

    if save:
        anim.save(save)
        print(f"Saved to {save}")
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(description="Visualize simulator .h5 output.")
    parser.add_argument("file", help="Path to .h5 file")
    parser.add_argument("--step", type=int, default=None, help="Show single step")
    parser.add_argument("--stride", type=int, default=4, help="Arrow subsampling (default: 4)")
    parser.add_argument("--save", metavar="FILE", help="Save animation to gif/mp4")
    args = parser.parse_args()

    try:
        vx, vy, attrs = load(args.file)
    except FileNotFoundError:
        print(f"Error: file not found: {args.file}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error reading {args.file}: {e}", file=sys.stderr)
        sys.exit(1)

    steps = vx.shape[0]
    print(f"Loaded {args.file}: {vx.shape[2]}x{vx.shape[1]}, {steps} steps, "
          f"type={attrs.get('type', 'unknown')}")

    if args.step is not None:
        if not 0 <= args.step < steps:
            print(f"Error: --step must be 0..{steps - 1}", file=sys.stderr)
            sys.exit(1)
        show_single(vx, vy, attrs, args.stride, args.step)
    else:
        animate(vx, vy, attrs, args.stride, args.save)


if __name__ == "__main__":
    main()
