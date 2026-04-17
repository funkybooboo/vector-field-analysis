#!/usr/bin/env bash
# Run configs through the full pipeline: simulator -> analyzer -> stats -> visualizer.
# Saves all stdout/stderr and visualizer output to data/<stem>/.
# Run from the project root.
#
# Usage:
#   scripts/run_configs.sh                    # run all configs in configs/
#   scripts/run_configs.sh vortex hurricane   # run only named stems

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

SIMULATOR="$PROJECT_DIR/build/bins/simulator/simulator"
ANALYZER="$PROJECT_DIR/build/bins/analyzer/analyzer"
STATS="$PROJECT_DIR/tools/stats.py"
VISUALIZER="$PROJECT_DIR/tools/visualize.py"
CONFIGS_DIR="$PROJECT_DIR/configs"
DATA_DIR="$PROJECT_DIR/data"

# -- Resolve stems ------------------------------------------------------------

if [[ $# -eq 0 ]]; then
  STEMS=()
  for f in "$CONFIGS_DIR"/*.toml; do
    STEMS+=("$(basename "$f" .toml)")
  done
else
  STEMS=("$@")
  for stem in "${STEMS[@]}"; do
    if [[ ! -f "$CONFIGS_DIR/$stem.toml" ]]; then
      echo "ERROR: no config for stem '$stem' in $CONFIGS_DIR" >&2
      exit 1
    fi
  done
fi

# -- Build --------------------------------------------------------------------

echo "==> Building..."
if ! cmake -B "$PROJECT_DIR/build" -DCMAKE_BUILD_TYPE=Release -S "$PROJECT_DIR" \
     > /dev/null 2>&1; then
  echo "ERROR: CMake configure failed. Aborting." >&2
  exit 1
fi
if ! cmake --build "$PROJECT_DIR/build" --parallel; then
  echo "ERROR: Build failed. Aborting." >&2
  exit 1
fi
echo "==> Build OK"
echo

# -- Per-config pipeline ------------------------------------------------------

declare -A SIM_STATUS ANA_STATUS STATS_STATUS VIS_STATUS

for stem in "${STEMS[@]}"; do
  config="$CONFIGS_DIR/$stem.toml"
  out="$DATA_DIR/$stem"
  mkdir -p "$out"

  printf "==> [%s]\n" "$stem"

  # Simulator
  if "$SIMULATOR" "$config" \
       > "$out/simulator_stdout.txt" \
       2> "$out/simulator_stderr.txt"; then
    SIM_STATUS[$stem]="OK"
    printf "    simulator  OK\n"
  else
    SIM_STATUS[$stem]="FAIL"
    printf "    simulator  FAIL (exit %d)\n" "$?"
    ANA_STATUS[$stem]="SKIP"
    STATS_STATUS[$stem]="SKIP"
    VIS_STATUS[$stem]="SKIP"
    continue
  fi

  # Analyzer
  if "$ANALYZER" "$config" \
       > "$out/analyzer_stdout.txt" \
       2> "$out/analyzer_stderr.txt"; then
    ANA_STATUS[$stem]="OK"
    printf "    analyzer   OK\n"
  else
    ANA_STATUS[$stem]="FAIL"
    printf "    analyzer   FAIL (exit %d)\n" "$?"
    STATS_STATUS[$stem]="SKIP"
    VIS_STATUS[$stem]="SKIP"
    continue
  fi

  # Stats
  if uv run "$STATS" "$out/field.h5" "$out/streams.h5" \
       > "$out/stats_stdout.txt" \
       2> "$out/stats_stderr.txt"; then
    STATS_STATUS[$stem]="OK"
    printf "    stats      OK\n"
  else
    STATS_STATUS[$stem]="FAIL"
    printf "    stats      FAIL (exit %d)\n" "$?"
  fi

  # Visualizer
  if uv run "$VISUALIZER" "$out/field.h5" \
       --streams "$out/streams.h5" \
       --save "$out/animation.gif" \
       > "$out/visualizer_stdout.txt" \
       2> "$out/visualizer_stderr.txt"; then
    VIS_STATUS[$stem]="OK"
    printf "    visualizer OK\n"
  else
    VIS_STATUS[$stem]="FAIL"
    printf "    visualizer FAIL (exit %d)\n" "$?"
  fi
done

# -- Summary ------------------------------------------------------------------

echo
printf "%-40s  %-10s %-10s %-10s %-10s\n" "CONFIG" "SIMULATOR" "ANALYZER" "STATS" "VISUALIZER"
printf "%-40s  %-10s %-10s %-10s %-10s\n" "------" "---------" "--------" "-----" "----------"

for stem in "${STEMS[@]}"; do
  printf "%-40s  %-10s %-10s %-10s %-10s\n" \
    "$stem" \
    "${SIM_STATUS[$stem]:-SKIP}" \
    "${ANA_STATUS[$stem]:-SKIP}" \
    "${STATS_STATUS[$stem]:-SKIP}" \
    "${VIS_STATUS[$stem]:-SKIP}"
done

echo
