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

# ── Resolve stems ─────────────────────────────────────────────────────────────

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

# ── Build ─────────────────────────────────────────────────────────────────────

echo "==> Building (with CUDA enabled)..."
if ! cmake -B "$PROJECT_DIR/build" -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=ON -S "$PROJECT_DIR" \
  >/dev/null 2>&1; then
  echo "ERROR: CMake configure failed. Aborting." >&2
  exit 1
fi
if ! cmake --build "$PROJECT_DIR/build" --parallel; then
  echo "ERROR: Build failed. Aborting." >&2
  exit 1
fi
echo "==> Build OK"
echo

# ── Per-config pipeline ───────────────────────────────────────────────────────

declare -A SIM_STATUS ANA_STATUS STATS_STATUS VIS_STATUS

for stem in "${STEMS[@]}"; do
  config="$CONFIGS_DIR/$stem.toml"
  out="$DATA_DIR/$stem"
  mkdir -p "$out"

  printf "==> [%s]\n" "$stem"

  # Simulator
  if "$SIMULATOR" "$config" \
    >"$out/simulator_stdout.txt" \
    2>"$out/simulator_stderr.txt"; then
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

  # Analyzer (Scaling Study)
  printf "    analyzer   scaling...\n"
  ANA_STATUS[$stem]="OK"

  # Function to run a specific variant and extract timing
  run_variant() {
    local solver=$1
    local workers=$2
    local variant_name="${solver}_${workers}"
    local log_file="$out/analyzer_${variant_name}.txt"
    local streams_out="$out/streams_${variant_name}.h5"
    
    # Create a temp config named exactly after the stem so main.cpp finds data/<stem>/field.h5
    local tmp_toml="${out}/${stem}.toml"
    sed '/^\[analyzer\]/,$d' "$config" >"$tmp_toml"
    
    if [[ "$solver" == "cuda_full" ]]; then
      # For cuda_full, 'workers' parameter is used as block size
      printf "\n[analyzer]\nsolver = \"cuda_full\"\ncuda_block_size = %d\noutput = \"%s\"\n" "$workers" "$streams_out" >>"$tmp_toml"
    else
      printf "\n[analyzer]\nsolver = \"%s\"\nthreads = %d\noutput = \"%s\"\n" "$solver" "$workers" "$streams_out" >>"$tmp_toml"
    fi
    
    local cmd=()
    if [[ "$solver" == "mpi" ]]; then
      cmd=(mpirun -n "$workers" --oversubscribe "$ANALYZER" "$tmp_toml")
    else
      cmd=("$ANALYZER" "$tmp_toml")
    fi

    if "${cmd[@]}" >"$log_file" 2>&1; then
      local ms=$(grep -E "^[a-z_]+.*[0-9.]+ ms" "$log_file" | awk '{print $(NF-1)}')
      printf "      %-15s %10s ms\n" "$variant_name" "$ms"
      # Link the last successful run to streams.h5 so stats/visualizer can find it
      ln -sf "$(basename "$streams_out")" "$out/streams.h5"
      rm -f "$tmp_toml"
      return 0
    else
      printf "      %-15s FAIL\n" "$variant_name"
      rm -f "$tmp_toml"
      return 1
    fi
  }

  # Sequential baseline
  run_variant "sequential" 1 || ANA_STATUS[$stem]="FAIL"

  # Pthreads Scaling
  for t in 2 4 8; do
    run_variant "pthreads" "$t" || ANA_STATUS[$stem]="FAIL"
  done

  # OpenMP Scaling
  for t in 2 4 8; do
    run_variant "openmp" "$t" || ANA_STATUS[$stem]="FAIL"
  done

  # MPI Scaling
  for p in 2 4; do
    run_variant "mpi" "$p" || ANA_STATUS[$stem]="FAIL"
  done

  # CUDA Full (Block Size Scaling)
  for block_size in 128 256 512; do
    run_variant "cuda_full" "$block_size" || ANA_STATUS[$stem]="FAIL"
  done

  if [[ "${ANA_STATUS[$stem]}" == "FAIL" ]]; then
    printf "    analyzer   FAIL (one or more variants failed)\n"
    STATS_STATUS[$stem]="SKIP"
    VIS_STATUS[$stem]="SKIP"
    continue
  fi

  # Stats
  if uv run "$STATS" "$out/field.h5" "$out/streams.h5" \
    >"$out/stats_stdout.txt" \
    2>"$out/stats_stderr.txt"; then
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
    >"$out/visualizer_stdout.txt" \
    2>"$out/visualizer_stderr.txt"; then
    VIS_STATUS[$stem]="OK"
    printf "    visualizer OK\n"
  else
    VIS_STATUS[$stem]="FAIL"
    printf "    visualizer FAIL (exit %d)\n" "$?"
  fi
done

# ── Summary ───────────────────────────────────────────────────────────────────

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
