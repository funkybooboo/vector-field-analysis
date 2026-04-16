#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = ["h5py", "matplotlib", "numpy", "scipy"]
# ///
"""Visualize vector field output produced by the simulator.

Usage:
    python tools/visualize.py <file> [--step N] [--stride S] [--save out.gif]
                                     [--streams data/<name>/streams.h5]

    <file>           path to field.h5 (e.g. data/accretion_disk/field.h5)

    --step N         show a single step N instead of animating (0-indexed)
    --stride S       subsample grid by S for less cluttered arrows (default: 4)
    --save FILE      save animation to a gif or mp4 instead of showing it
    --streams FILE   overlay streamlines from the analyzer output streams.h5
"""

import argparse
import concurrent.futures
import multiprocessing
import os
import pickle
import sys
import threading

import h5py
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.animation import FuncAnimation
from scipy.integrate import solve_ivp
from scipy.interpolate import RegularGridInterpolator


def load(path):
    with h5py.File(path, "r") as f:
        fieldGroup = f["field"]
        vx = fieldGroup["vx"][:]  # (steps, height, width)
        vy = fieldGroup["vy"][:]
        attrs = dict(fieldGroup.attrs)
    return vx, vy, attrs


def load_streams(path):
    """Load streamline data written by the analyzer.

    Returns a list (one entry per time step) of lists of arrays. Each array
    has shape (path_len, 2) with integer (row, col) grid indices.
    """
    streamlines_by_step = []
    with h5py.File(path, "r") as f:
        streamsGroup = f["streams"]
        num_steps = int(streamsGroup.attrs["num_steps"])
        for s in range(num_steps):
            stepGroup = streamsGroup[f"step_{s}"]
            flat = stepGroup["paths_flat"][:]   # (N, 2) int32
            offsets = stepGroup["offsets"][:]   # (S+1,) int32
            streamlines = []
            for i in range(len(offsets) - 1):
                streamlines.append(flat[offsets[i]:offsets[i + 1]])
            streamlines_by_step.append(streamlines)
    return streamlines_by_step


def make_grid(vx, vy, attrs, stride):
    steps, height, width = vx.shape
    x = np.linspace(attrs.get("xMin", -1), attrs.get("xMax", 1), width)
    y = np.linspace(attrs.get("yMin", -1), attrs.get("yMax", 1), height)
    # Stride subsamples the grid to reduce arrow clutter -- only every S-th
    # cell in each axis is shown, keeping the plot readable at high resolutions.
    X, Y = np.meshgrid(x[::stride], y[::stride])
    return X, Y, steps


def speed(vx_step, vy_step):
    return np.sqrt(vx_step**2 + vy_step**2)


def precompute_quiver_data(vx, vy, attrs, stride):
    """Pre-compute the quiver inputs for every step.

    X, Y are constant across steps; vx_step/vy_step/mag are per-step numpy arrays.
    Returns (X, Y, vx_all, vy_all, mag_all) where *_all[s] is step s's data.
    """
    X, Y, steps = make_grid(vx, vy, attrs, stride)
    vx_all = vx[:, ::stride, ::stride]   # (steps, h', w') view
    vy_all = vy[:, ::stride, ::stride]
    mag_all = np.sqrt(vx_all**2 + vy_all**2)
    return X, Y, vx_all, vy_all, mag_all


_progress_queue: "multiprocessing.Queue | None" = None


def _init_worker(q: multiprocessing.Queue) -> None:
    global _progress_queue
    _progress_queue = q


def _integrate_step(streamlines_step, vx_step, vy_step, xMin, xMax, yMin, yMax):
    """Integrate one time step's streamlines with RK45.

    Returns:
        curves: list of (xs, ys) arrays, one per streamline
        arrows: list of (x0, y0, x1, y1) tuples or None, one per streamline
    """
    height, width = vx_step.shape
    x_grid = np.linspace(xMin, xMax, width)
    y_grid = np.linspace(yMin, yMax, height)

    interp_vyx = RegularGridInterpolator(
        (y_grid, x_grid), vx_step, method="linear",
        bounds_error=False, fill_value=0.0,
    )
    interp_vy = RegularGridInterpolator(
        (y_grid, x_grid), vy_step, method="linear",
        bounds_error=False, fill_value=0.0,
    )

    def field(t, state):
        pt = np.array([[state[1], state[0]]])  # (y, x) for RegularGridInterpolator
        return [float(interp_vyx(pt)[0]), float(interp_vy(pt)[0])]

    def field_bwd(t, state):
        fwd = field(t, state)
        return [-fwd[0], -fwd[1]]

    def out_of_domain(t, state):
        x, y = state
        margin = 1e-6
        return min(x - xMin, xMax - x, y - yMin, yMax - y) - margin
    out_of_domain.terminal = True
    out_of_domain.direction = -1

    # t_span: how far to integrate forward and backward from each seed.
    # Domain spans ~4 units wide, ~2 tall; speed ~1 → exit in 2–4 units.
    # 8.0 gives ~2 domain crossings before giving up, keeping paths visible
    # without the 30s worst-case for closed orbits.
    t_span = 8.0
    max_step = 0.05

    curves = []
    arrows = []

    for path in streamlines_step:
        if len(path) < 1:
            curves.append((np.array([]), np.array([])))
            arrows.append(None)
            continue

        row, col = int(path[0, 0]), int(path[0, 1])
        x0 = float(x_grid[col])
        y0 = float(y_grid[row])

        sol_fwd = solve_ivp(
            field, [0, t_span], [x0, y0], method="RK45",
            max_step=max_step, events=out_of_domain,
        )
        sol_bwd = solve_ivp(
            field_bwd, [0, t_span], [x0, y0], method="RK45",
            max_step=max_step, events=out_of_domain,
        )

        xs = np.concatenate([sol_bwd.y[0][::-1], sol_fwd.y[0]])
        ys = np.concatenate([sol_bwd.y[1][::-1], sol_fwd.y[1]])

        if len(xs) < 2:
            curves.append((xs, ys))
            arrows.append(None)
            continue

        curves.append((xs, ys))

        fwd_len = len(sol_fwd.y[0])
        if fwd_len >= 3:
            mid = fwd_len // 2
            arrow = (
                sol_fwd.y[0][mid], sol_fwd.y[1][mid],
                sol_fwd.y[0][mid + 1], sol_fwd.y[1][mid + 1],
            )
            arrows.append(arrow)
        else:
            arrows.append(None)

        if _progress_queue is not None:
            _progress_queue.put_nowait(1)

    return curves, arrows


def _curve_cache_path(streams_path):
    return os.path.join(os.path.dirname(streams_path), "vis_cache.pkl")


def _cache_is_valid(cache_path, *input_paths):
    """True if cache exists and is newer than all input files."""
    if not os.path.exists(cache_path):
        return False
    cache_mtime = os.path.getmtime(cache_path)
    return all(os.path.getmtime(p) <= cache_mtime for p in input_paths)


def precompute_stream_curves(streamlines_by_step, vx, vy, attrs, workers=None,
                             field_path=None, streams_path=None):
    """Pre-compute RK45 curves for all steps in parallel, with disk caching.

    If field_path and streams_path are given, results are cached next to the
    streams file and reused on subsequent runs as long as neither input changes.

    Returns:
        all_curves[step][i] = (xs, ys)
        all_arrows[step][i] = (x0, y0, x1, y1) or None
    """
    cache_path = None
    if field_path and streams_path:
        cache_path = _curve_cache_path(streams_path)
        if _cache_is_valid(cache_path, field_path, streams_path):
            print(f"Loading cached streamlines from {cache_path}")
            try:
                with open(cache_path, "rb") as f:
                    return pickle.load(f)
            except Exception as e:
                print(f"Warning: cache load failed ({e}), recomputing...", file=sys.stderr)

    xMin = float(attrs.get("xMin", -1))
    xMax = float(attrs.get("xMax", 1))
    yMin = float(attrs.get("yMin", -1))
    yMax = float(attrs.get("yMax", 1))

    num_steps = len(streamlines_by_step)
    nworkers = min(workers or os.cpu_count() or 1, num_steps)
    total_streams = sum(len(s) for s in streamlines_by_step)
    print(
        f"Pre-computing streamlines: {num_steps} steps, "
        f"{total_streams} streamlines, {nworkers} workers...",
        flush=True,
    )

    args = [
        (streamlines_by_step[s], vx[s], vy[s], xMin, xMax, yMin, yMax)
        for s in range(num_steps)
    ]

    q: multiprocessing.Queue = multiprocessing.Queue()
    done_streams = [0]
    stop_reader = [False]

    def _progress_reader() -> None:
        while not stop_reader[0]:
            try:
                q.get(timeout=0.1)
                done_streams[0] += 1
                print(f"  {done_streams[0]}/{total_streams} streamlines", flush=True)
            except Exception:
                pass
        # drain anything enqueued after stop signal
        while True:
            try:
                q.get_nowait()
                done_streams[0] += 1
            except Exception:
                break

    reader = threading.Thread(target=_progress_reader, daemon=True)
    reader.start()

    results = [None] * num_steps
    steps_done = 0
    try:
        with concurrent.futures.ProcessPoolExecutor(
            max_workers=nworkers,
            initializer=_init_worker,
            initargs=(q,),
        ) as pool:
            futures = {pool.submit(_integrate_step, *a): s for s, a in enumerate(args)}
            try:
                for fut in concurrent.futures.as_completed(futures):
                    s = futures[fut]
                    try:
                        results[s] = fut.result()
                    except Exception as e:
                        print(f"\nStep {s} failed: {e}", file=sys.stderr, flush=True)
                        raise
                    steps_done += 1
            except KeyboardInterrupt:
                print("\nInterrupted — cancelling workers...", file=sys.stderr, flush=True)
                for f in futures:
                    f.cancel()
                raise
    finally:
        stop_reader[0] = True
        reader.join(timeout=2)

    all_curves = [r[0] for r in results]
    all_arrows = [r[1] for r in results]

    if cache_path:
        try:
            with open(cache_path, "wb") as f:
                pickle.dump((all_curves, all_arrows), f)
            print(f"Cached streamlines to {cache_path}")
        except Exception as e:
            print(f"Warning: could not write cache ({e})", file=sys.stderr)

    return all_curves, all_arrows


def draw_loaded_streams(ax, curves_step, arrows_step):
    """Draw pre-computed streamline curves onto ax."""
    cmap = plt.cm.cool
    n = len(curves_step)
    for idx, (xs, ys) in enumerate(curves_step):
        if len(xs) < 2:
            continue
        color = cmap(idx / max(n - 1, 1))
        ax.plot(xs, ys, color=color, linewidth=1.5, alpha=0.85,
                solid_capstyle="round")
    for idx, arrow in enumerate(arrows_step):
        if arrow is None:
            continue
        color = cmap(idx / max(n - 1, 1))
        x0, y0, x1, y1 = arrow
        ax.annotate(
            "",
            xy=(x1, y1),
            xytext=(x0, y0),
            arrowprops=dict(arrowstyle="->", color=color, lw=1.0),
        )


def plot_step(ax, vx, vy, attrs, stride, step, curves=None, arrows=None,
              quiver_data=None):
    if quiver_data is not None:
        X, Y, vx_all, vy_all, mag_all = quiver_data
        vx_step, vy_step, mag = vx_all[step], vy_all[step], mag_all[step]
    else:
        vx_step = vx[step, ::stride, ::stride]
        vy_step = vy[step, ::stride, ::stride]
        X, Y, _ = make_grid(vx, vy, attrs, stride)
        mag = speed(vx_step, vy_step)
    ax.clear()
    # plasma is perceptually uniform, making speed differences easier to read
    # than with non-uniform colormaps like rainbow or jet.
    # scale=None lets matplotlib auto-scale arrow lengths to the data range,
    # preventing arrows from being invisible or overflowing the axes when
    # field magnitudes vary across steps.
    q = ax.quiver(X, Y, vx_step, vy_step, mag, cmap="plasma", pivot="mid", scale=None)
    if curves is not None:
        draw_loaded_streams(ax, curves[step], arrows[step])
    field_type = attrs.get("type", "unknown")
    total = vx.shape[0]
    ax.set_title(f"{field_type}  |  step {step}/{total - 1}")
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_aspect("equal")
    return q


def show_single(vx, vy, attrs, stride, step, streamlines=None, workers=None,
                field_path=None, streams_path=None):
    quiver_data = precompute_quiver_data(vx, vy, attrs, stride)

    curves = arrows = None
    if streamlines is not None:
        # Always pre-compute all steps so the cache covers future runs too.
        all_curves, all_arrows = precompute_stream_curves(
            streamlines, vx, vy, attrs, workers=workers,
            field_path=field_path, streams_path=streams_path,
        )
        curves = {s: all_curves[s] for s in range(len(all_curves))}
        arrows = {s: all_arrows[s] for s in range(len(all_arrows))}

    fig, ax = plt.subplots(figsize=(6, 6))
    q = plot_step(ax, vx, vy, attrs, stride, step, curves, arrows, quiver_data=quiver_data)
    fig.colorbar(q, ax=ax, label="speed")
    plt.tight_layout()
    plt.show()


def animate(vx, vy, attrs, stride, save, streamlines=None, workers=None,
            field_path=None, streams_path=None):
    steps = vx.shape[0]

    quiver_data = precompute_quiver_data(vx, vy, attrs, stride)

    curves = arrows = None
    if streamlines is not None:
        curves_list, arrows_list = precompute_stream_curves(
            streamlines, vx, vy, attrs, workers=workers,
            field_path=field_path, streams_path=streams_path,
        )
        # Index by step number (0..steps-1)
        curves = {s: curves_list[s] for s in range(steps)}
        arrows = {s: arrows_list[s] for s in range(steps)}

    fig, ax = plt.subplots(figsize=(6, 6))
    cbar = None

    def update(frame):
        nonlocal cbar
        q = plot_step(ax, vx, vy, attrs, stride, frame, curves, arrows, quiver_data=quiver_data)
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
        try:
            anim.save(save)
        except Exception as e:
            print(f"Error saving animation to {save}: {e}", file=sys.stderr)
            sys.exit(1)
        print(f"Saved to {save}")
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description="Visualize vector field output produced by the simulator.",
        epilog="Examples:\n"
               "  tools/visualize.py data/accretion_disk/field.h5\n"
               "  tools/visualize.py data/accretion_disk/field.h5 --streams data/accretion_disk/streams.h5",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "file",
        help="Path to the field HDF5 file (e.g. data/accretion_disk/field.h5).",
    )
    parser.add_argument("--step", type=int, default=None, help="Show single step (0-indexed)")
    parser.add_argument("--stride", type=int, default=4, help="Arrow subsampling stride (default: 4)")
    parser.add_argument("--save", metavar="FILE", help="Save animation to gif/mp4 instead of displaying")
    parser.add_argument(
        "--streams", metavar="FILE",
        help="Overlay streamlines from analyzer output (e.g. data/<name>/streams.h5)",
    )
    _default_workers = 4
    _env_val = os.environ.get("VISUALIZER_THREADS")
    if _env_val:
        try:
            _parsed = int(_env_val)
            if _parsed > 0:
                _default_workers = _parsed
        except ValueError:
            pass
    parser.add_argument(
        "--workers", type=int, default=_default_workers,
        help="Parallel workers for streamline pre-computation "
             "(default: $VISUALIZER_THREADS if set, else 4)",
    )
    args = parser.parse_args()

    field_path = args.file

    if args.stride < 1:
        print("Error: --stride must be >= 1", file=sys.stderr)
        sys.exit(1)

    if args.workers < 1:
        print("Error: --workers must be >= 1", file=sys.stderr)
        sys.exit(1)

    try:
        vx, vy, attrs = load(field_path)
    except FileNotFoundError:
        print(f"Error: field file not found: {field_path}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error reading {field_path}: {e}", file=sys.stderr)
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
    print(f"Loaded {field_path}: {vx.shape[2]}x{vx.shape[1]}, {steps} steps, "
          f"type={attrs.get('type', 'unknown')}")
    if streamlines is not None:
        print(f"Loaded streams: {args.streams}")

    if args.step is not None:
        if not 0 <= args.step < steps:
            print(f"Error: --step must be 0..{steps - 1}", file=sys.stderr)
            sys.exit(1)
        show_single(vx, vy, attrs, args.stride, args.step, streamlines, workers=args.workers,
                    field_path=field_path, streams_path=args.streams)
    else:
        animate(vx, vy, attrs, args.stride, args.save, streamlines, workers=args.workers,
                field_path=field_path, streams_path=args.streams)


if __name__ == "__main__":
    main()
