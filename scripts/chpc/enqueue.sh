#!/usr/bin/env bash
# Compile JOB_BIN for the configured arch and submit a batch job via job.sh.
# Run on the CHPC login node.
# Configure via .env at the project root.
#
# Examples:
#   ./scripts/chpc/enqueue.sh
#   CHPC_GPU=v100:1 CUDA_ARCH=sm_70 CUDA_MODULE=cuda/11.6.2 ./scripts/chpc/enqueue.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$SCRIPT_DIR/../validate.sh"

validate_or_die \
  _check_account _check_partition _check_gpu _check_time _check_ntasks \
  _check_cuda_arch _check_cuda_module \
  _check_job_name _check_job_bin _check_job_input _check_job_output

GPU_LABEL="${CHPC_GPU%%:*}"
SM_ARCH="$CUDA_ARCH"
LOG_DIR="$PROJECT_DIR/logs/$GPU_LABEL"
bin="$PROJECT_DIR/${JOB_BIN}_run"
cmake_bin="$PROJECT_DIR/build/bins/$JOB_BIN/$JOB_BIN"

cd "$PROJECT_DIR"

# --- Build via CMake ---
# Use the project's normal CMake build so all include paths, definitions, and
# link flags (HighFive/HDF5, toml++, MPI, etc.) are applied correctly.
echo "==> Building $JOB_BIN via CMake"
if [[ ! -d "$PROJECT_DIR/build" ]]; then
  echo "error: no cmake build directory found at $PROJECT_DIR/build" >&2
  echo "error: configure and build the project with cmake before running this script" >&2
  exit 1
fi
cmake --build "$PROJECT_DIR/build" --target "$JOB_BIN" -- -j"$(nproc)"
cp -f "$cmake_bin" "$bin"
echo "  staged ${JOB_BIN}_run"

# --- Submit ---
echo ""
echo "==> Submitting job"

mkdir -p "$LOG_DIR"
rm -f "$LOG_DIR/stdout.log" "$LOG_DIR/stderr.log"
base="${JOB_OUTPUT%.*}"; ext="${JOB_OUTPUT##*.}"
rm -f "${base}_${GPU_LABEL}.${ext}"

account_flag=()
if [[ "$CHPC_ACCOUNT" != "none" ]]; then
  account_flag=(--account="$CHPC_ACCOUNT")
fi

job_id=$(sbatch \
  --partition="$CHPC_PARTITION" \
  "${account_flag[@]}" \
  --gres="gpu:$CHPC_GPU" \
  --nodes=1 \
  --ntasks="$CHPC_NTASKS" \
  --time="$CHPC_TIME" \
  --job-name="${JOB_NAME}" \
  --output="$LOG_DIR/stdout.log" \
  --error="$LOG_DIR/stderr.log" \
  --export="GPU_LABEL=$GPU_LABEL,SM_ARCH=$SM_ARCH,PROJECT_DIR=$PROJECT_DIR,JOB_BIN=$JOB_BIN,JOB_INPUT=$JOB_INPUT,JOB_OUTPUT=$JOB_OUTPUT,CUDA_MODULE=$CUDA_MODULE" \
  "$SCRIPT_DIR/job.sh" \
  | awk '{print $NF}')

echo "  job $job_id -> ${base}_${GPU_LABEL}.${ext}"
echo ""
echo "Monitor:  squeue -u \$USER"
echo "Estimate: squeue --start -u \$USER"
echo "Logs:     $LOG_DIR/"
