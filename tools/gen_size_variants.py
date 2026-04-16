#!/usr/bin/env python3
"""Generate small/medium/large size variants for every config in configs/.

For each existing config, creates three files named {stem}_{W}x{H}.toml:
  - small:  half the current dimensions
  - medium: current dimensions (copy with updated filename references)
  - large:  double the current dimensions

Deletes the original file after creating all three variants.
"""

import os
import re
import sys

CONFIGS_DIR = os.path.join(os.path.dirname(__file__), '..', 'configs')
CONFIGS_DIR = os.path.normpath(CONFIGS_DIR)


def replace_dim(content: str, key: str, new_val: int) -> str:
    """Replace `key = <digits>` assignment lines (not comment lines)."""
    return re.sub(
        r'^(\s*' + key + r'\s*=\s*)\d+',
        lambda m: f'{m.group(1)}{new_val}',
        content,
        flags=re.MULTILINE,
    )


def update_filename_refs(content: str, old_stem: str, new_stem: str) -> str:
    """Update bare `# old_stem.toml` and `# Run: ... configs/old_stem.toml` comments."""
    # "# karman_street.toml" → "# karman_street_128x64.toml"
    content = re.sub(
        r'^(#\s*)' + re.escape(old_stem) + r'(\.toml)',
        lambda m: f'{m.group(1)}{new_stem}{m.group(2)}',
        content,
        flags=re.MULTILINE,
    )
    # "# Run: simulator configs/karman_street.toml" → new stem
    content = re.sub(
        r'^(#\s*[Rr]un:.*configs/)' + re.escape(old_stem) + r'(\.toml)',
        lambda m: f'{m.group(1)}{new_stem}{m.group(2)}',
        content,
        flags=re.MULTILINE,
    )
    return content


def process(filepath: str) -> None:
    stem = os.path.basename(filepath)[:-5]  # strip .toml

    with open(filepath) as f:
        original = f.read()

    w_m = re.search(r'^\s*width\s*=\s*(\d+)', original, re.MULTILINE)
    h_m = re.search(r'^\s*height\s*=\s*(\d+)', original, re.MULTILINE)
    if not w_m or not h_m:
        print(f'  SKIP {stem}: could not find width/height')
        return

    w = int(w_m.group(1))
    h = int(h_m.group(1))

    sizes = [
        (w // 2, h // 2),
        (w,      h),
        (w * 2,  h * 2),
    ]

    for nw, nh in sizes:
        new_stem = f'{stem}_{nw}x{nh}'
        content = replace_dim(original, 'width',  nw)
        content = replace_dim(content,  'height', nh)
        content = update_filename_refs(content, stem, new_stem)
        out = os.path.join(CONFIGS_DIR, f'{new_stem}.toml')
        with open(out, 'w') as f:
            f.write(content)
        print(f'  + {new_stem}.toml')

    os.remove(filepath)
    print(f'  - {stem}.toml')


def main() -> None:
    originals = sorted(
        os.path.join(CONFIGS_DIR, fn)
        for fn in os.listdir(CONFIGS_DIR)
        if fn.endswith('.toml')
    )
    print(f'Processing {len(originals)} configs in {CONFIGS_DIR}')
    for path in originals:
        stem = os.path.basename(path)[:-5]
        print(f'\n[{stem}]')
        process(path)
    print(f'\nDone. Created {len(originals) * 3} files, removed {len(originals)} originals.')


if __name__ == '__main__':
    main()
