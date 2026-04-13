#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = ["h5py", "matplotlib", "numpy"]
# ///
"""Visualize vector field .h5 files produced by the simulator.

Usage:
    python tools/visualize.py field.h5 [--step N] [--stride S] [--save out.gif]
                                       [--streams field.streams.h5]

    --step N         show a single step N instead of animating (0-indexed)
    --stride S       subsample grid by S for less cluttered arrows (default: 4)
    --save FILE      save animation to a gif or mp4 instead of showing it
    --streams FILE   overlay streamlines from an analyzer .streams.h5 file
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


def load_streams(path):
    """Load streamline data written by the analyzer.

    Returns a list (one entry per time step) of lists of arrays. Each array
    has shape (path_len, 2) with integer (row, col) grid indices.
    """
    all_steps = []
    with h5py.File(path, "r") as f:
        grp = f["streams"]
        num_steps = int(grp.attrs["num_steps"])
        for s in range(num_steps):
            step_grp = grp[f"step_{s}"]
            flat = step_grp["paths_flat"][:]   # (N, 2) int32
            offsets = step_grp["offsets"][:]   # (S+1,) int32
            streamlines = []
            for i in range(len(offsets) - 1):
                streamlines.append(flat[offsets[i]:offsets[i + 1]])
            all_steps.append(streamlines)
    return all_steps


def make_grid(vx, vy, attrs, stride):
    steps, height, width = vx.shape
    x = np.linspace(attrs.get("xMin", -1), attrs.get("xMax", 1), width)
    y = np.linspace(attrs.get("yMin", -1), attrs.get("yMax", 1), height)
    # Stride subsamples the grid to reduce arrow clutter -- only every S-th
    # cell in each axis is shown, keeping the plot readable at high resolutions.
    X, Y = np.meshgrid(x[::stride], y[::stride])
    return X, Y, steps


def speed(vx_s, vy_s):
    return np.sqrt(vx_s**2 + vy_s**2)


def draw_loaded_streams(ax, streamlines_step, attrs, shape):
    """Render pre-computed streamlines loaded from a .streams.h5 file.

    Converts each path's (row, col) grid indices to (x, y) coordinates using
    the field extents stored in attrs, then plots each path as a line.
    """
    height, width = shape
    x_coords = np.linspace(attrs.get("xMin", -1), attrs.get("xMax", 1), width)
    y_coords = np.linspace(attrs.get("yMin", -1), attrs.get("yMax", 1), height)
    for path in streamlines_step:
        if len(path) < 2:
            continue
        xs = x_coords[path[:, 1]]  # col index -> x coordinate
        ys = y_coords[path[:, 0]]  # row index -> y coordinate
        ax.plot(xs, ys, color="cyan", linewidth=0.8, alpha=0.7)


def plot_step(ax, vx, vy, attrs, stride, step, streamlines=None):
    vx_s = vx[step, ::stride, ::stride]
    vy_s = vy[step, ::stride, ::stride]
    X, Y, _ = make_grid(vx, vy, attrs, stride)
    mag = speed(vx_s, vy_s)
    ax.clear()
    # plasma is perceptually uniform, making speed differences easier to read
    # than with non-uniform colormaps like rainbow or jet.
    # scale=None lets matplotlib auto-scale arrow lengths to the data range,
    # preventing arrows from being invisible or overflowing the axes when
    # field magnitudes vary across steps.
    q = ax.quiver(X, Y, vx_s, vy_s, mag, cmap="plasma", pivot="mid", scale=None)
    if streamlines is not None:
        draw_loaded_streams(ax, streamlines[step], attrs, vx[step].shape)
    field_type = attrs.get("type", "unknown")
    total = vx.shape[0]
    ax.set_title(f"{field_type}  |  step {step}/{total - 1}")
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_aspect("equal")
    return q


def show_single(vx, vy, attrs, stride, step, streamlines=None):
    fig, ax = plt.subplots(figsize=(6, 6))
    q = plot_step(ax, vx, vy, attrs, stride, step, streamlines)
    fig.colorbar(q, ax=ax, label="speed")
    plt.tight_layout()
    plt.show()


def animate(vx, vy, attrs, stride, save, streamlines=None):
    steps = vx.shape[0]
    fig, ax = plt.subplots(figsize=(6, 6))
    cbar = None

    def update(frame):
        nonlocal cbar
        q = plot_step(ax, vx, vy, attrs, stride, frame, streamlines)
        if cbar is None:
            cbar = fig.colorbar(q, ax=ax, label="speed")
        else:
            cbar.update_normal(q)
        return (q,)

    interval = max(1000 // steps, 40)  # aim for ~25 fps, floor at 40 ms
    # blit=False because the colorbar is redrawn every frame when the speed
    # range changes -- blitting only re-renders explicitly marked artists and
    # would leave the colorbar stale.
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
    parser.add_argument("--streams", metavar="FILE", help="Overlay streamlines from .streams.h5")
    args = parser.parse_args()

    try:
        vx, vy, attrs = load(args.file)
    except FileNotFoundError:
        print(f"Error: file not found: {args.file}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error reading {args.file}: {e}", file=sys.stderr)
        sys.exit(1)

    streamlines = None
    if args.streams:
        try:
            streamlines = load_streams(args.streams)
        except FileNotFoundError:
            print(f"Error: streams file not found: {args.streams}", file=sys.stderr)
            sys.exit(1)
        except Exception as e:
            print(f"Error reading {args.streams}: {e}", file=sys.stderr)
            sys.exit(1)

    steps = vx.shape[0]
    print(f"Loaded {args.file}: {vx.shape[2]}x{vx.shape[1]}, {steps} steps, "
          f"type={attrs.get('type', 'unknown')}")
    if streamlines is not None:
        print(f"Loaded streams: {args.streams}")

    if args.step is not None:
        if not 0 <= args.step < steps:
            print(f"Error: --step must be 0..{steps - 1}", file=sys.stderr)
            sys.exit(1)
        show_single(vx, vy, attrs, args.stride, args.step, streamlines)
    else:
        animate(vx, vy, attrs, args.stride, args.save, streamlines)


if __name__ == "__main__":
    main()
