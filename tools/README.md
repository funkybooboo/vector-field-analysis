# Tools

Python scripts for working with simulator output.

## visualize.py

Animates or plots a single step from a simulator `.h5` file as a quiver (arrow) plot.
Arrows are colored by speed. Dependencies are managed automatically by `uv`.

### Usage

```sh
# animate all steps
uv run tools/visualize.py field.h5

# show a single step (0-indexed)
uv run tools/visualize.py field.h5 --step 0

# save animation to gif or mp4
uv run tools/visualize.py field.h5 --save out.gif
uv run tools/visualize.py field.h5 --save out.mp4

# denser arrows (lower stride = more arrows)
uv run tools/visualize.py field.h5 --stride 2
```

### Options

| Option | Default | Description |
|--------|---------|-------------|
| `--step N` | - | Show step N instead of animating |
| `--stride S` | `4` | Subsample grid by S for arrow density |
| `--save FILE` | - | Save animation to gif or mp4 instead of showing |

### Dependencies

- Python >= 3.11
- `h5py`, `matplotlib`, `numpy` (fetched automatically by `uv run`)
