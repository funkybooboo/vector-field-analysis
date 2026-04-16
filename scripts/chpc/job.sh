#!/usr/bin/env bash
# SLURM batch worker -- submitted by enqueue.sh and benchmark.sh, do not run directly.
# Expects these env vars (set via sbatch --export):
#   GPU_LABEL        - human label for this run (e.g. "a100")
#   SM_ARCH          - node SM arch label, used for logging only (e.g. "sm_80")
#   PROJECT_DIR      - absolute path to the project root
#   JOB_BIN          - binary name (staged as ${JOB_BIN}_run in PROJECT_DIR by benchmark.sh)
#   JOB_INPUT        - TOML config path (relative to project root)
#   JOB_OUTPUT       - base output path (relative to project root, _<gpu_label> inserted before extension)
#   CUDA_MODULE      - CUDA module to load (e.g. cuda/12.1.0)

set -euo pipefail

# Initialize LMOD so the module command is available and functional.
# SLURM batch jobs run in a non-login, non-interactive shell that does not source
# /etc/profile, so the module function is not set up by default.
# shellcheck source=/dev/null
source /etc/profile.d/lmod.sh 2>/dev/null || true

module load "$CUDA_MODULE"

cd "$PROJECT_DIR"

# Derive per-run output path: output/streams.h5 -> output/streams_a100.h5
base="${JOB_OUTPUT%.*}"
ext="${JOB_OUTPUT##*.}"
gpu_output="${base}_${GPU_LABEL}.${ext}"

echo "=== job info ==="
date
echo "host: $(hostname)"
echo "node: $GPU_LABEL ($SM_ARCH)"

echo ""
echo "=== $JOB_BIN ==="
mkdir -p "$(dirname "$gpu_output")"

# Inject the per-run output path into a temp TOML so the analyzer writes the
# correct file without modifying the shared base config.
tmp_toml=$(mktemp /tmp/vfa_job_XXXXXX.toml)
trap 'rm -f "$tmp_toml"' EXIT
# Strip any existing output key, then insert output = "..." after [analyzer].
sed '/^\s*output\s*=/d' "$JOB_INPUT" \
  | sed '/^\[analyzer\]/a output = "'"$gpu_output"'"' \
  > "$tmp_toml"

srun "$PROJECT_DIR/${JOB_BIN}_run" "$tmp_toml"
