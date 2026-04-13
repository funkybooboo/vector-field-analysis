#!/usr/bin/env bash
# Submit benchmark jobs across all configured GPU node types.
# Run from anywhere on a CHPC login node -- no interactive session needed.
# Configure via .env at the project root.
#
# What it does:
#   1. Compiles a separate binary for each distinct SM arch (login node, no GPU required)
#   2. Submits one sbatch job per GPU config (via job.sh)
#   3. Each job runs JOB_BIN, saving stdout/stderr to logs/
#
# Logs:  logs/<gpu_label>/stdout.log  and  logs/<gpu_label>/stderr.log
# Output files: derived from JOB_OUTPUT with _<gpu_label> inserted before extension
#
# Usage:
#   ./scripts/chpc/benchmark.sh
#   BENCHMARK_GPUS="a100" ./scripts/chpc/benchmark.sh
#
# To add or remove GPU targets, edit the CONFIGS array below.
# Verify GPU type strings with: freegpus  or  sinfo -o "%G" -p notchpeak-gpu

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$SCRIPT_DIR/../validate.sh"

validate_or_die _check_account _check_partition _check_time _check_cuda_module \
  _check_job_name _check_job_bin _check_job_src _check_job_input _check_job_output

BATCH_SCRIPT="$SCRIPT_DIR/job.sh"
LOG_DIR="$PROJECT_DIR/logs"
BENCHMARK_GPUS="${BENCHMARK_GPUS:-}"

cd "$PROJECT_DIR"

# --- GPU configurations ---
# Format per entry (space-separated): label gres partition account sm_arch
# Account typically matches partition name. Verify with:
#   sacctmgr show associations user=$USER format=account,partition,cluster
#
# Notchpeak entries use $CHPC_PARTITION/$CHPC_ACCOUNT from .env, so switching
# those two vars redirects the entire notchpeak block at once.
CONFIGS=(
  # notchpeak -- requires allocation
  "a100   gpu:a100:1    $CHPC_PARTITION  $CHPC_ACCOUNT  sm_80"
  "v100   gpu:v100:1    $CHPC_PARTITION  $CHPC_ACCOUNT  sm_70"
  "2080ti gpu:2080ti:1  $CHPC_PARTITION  $CHPC_ACCOUNT  sm_75"
  "p40    gpu:p40:1     $CHPC_PARTITION  $CHPC_ACCOUNT  sm_61"
  # granite freecycle -- account required (granite-gpu-freecycle), preemptable
  # "l40s   gpu:l40s:1    granite-gpu-freecycle  granite-gpu-freecycle  sm_89"
  # "h100   gpu:h100:1    granite-gpu-freecycle  granite-gpu-freecycle  sm_90"
  # lonepeak -- account required (lonepeak-gpu)
  # "1080ti gpu:1080ti:1  lonepeak-gpu           lonepeak-gpu           sm_61"
)

# --- Filter by BENCHMARK_GPUS if set ---
if [[ -n "$BENCHMARK_GPUS" ]]; then
  filtered=()
  for config in "${CONFIGS[@]}"; do
    read -r label _ <<< "$config"
    for wanted in $BENCHMARK_GPUS; do
      if [[ "$label" == "$wanted" ]]; then
        filtered+=("$config")
        break
      fi
    done
  done
  CONFIGS=("${filtered[@]}")
  if [[ ${#CONFIGS[@]} -eq 0 ]]; then
    echo "error: BENCHMARK_GPUS=\"$BENCHMARK_GPUS\" matched no configured targets" >&2
    exit 1
  fi
fi

# --- Build via CMake ---
# Use the project's normal CMake build so all include paths, definitions, and
# link flags (HighFive/HDF5, toml++, MPI, etc.) are applied correctly.
cmake_bin="$PROJECT_DIR/build/bins/$JOB_BIN/$JOB_BIN"

echo "==> Building $JOB_BIN via CMake"
if [[ ! -d "$PROJECT_DIR/build" ]]; then
  echo "error: no cmake build directory found at $PROJECT_DIR/build" >&2
  echo "error: configure and build the project with cmake before running this script" >&2
  exit 1
fi
cmake --build "$PROJECT_DIR/build" --target "$JOB_BIN" -- -j"$(nproc)"

# --- Stage one binary per unique arch label ---
# The arch suffix is used as an identifier by job.sh; each config gets its own copy.
echo "==> Staging binaries"
declare -A staged

for config in "${CONFIGS[@]}"; do
  read -r label gres partition account arch <<< "$config"
  if [[ -z "${staged[$arch]+x}" ]]; then
    job_bin="$PROJECT_DIR/${JOB_BIN}_$arch"
    cp -f "$cmake_bin" "$job_bin"
    echo "  $arch: staged $JOB_BIN"
    staged[$arch]=1
  fi
done

# --- Submit one job per GPU config ---
echo ""
echo "==> Submitting jobs"
for config in "${CONFIGS[@]}"; do
  read -r label gres partition account arch <<< "$config"

  mkdir -p "$LOG_DIR/$label"

  account_flag=()
  if [[ "$account" != "none" ]]; then
    account_flag=(--account="$account")
  fi

  job_id=$(sbatch \
    --partition="$partition" \
    "${account_flag[@]}" \
    --gres="$gres" \
    --nodes=1 \
    --ntasks=1 \
    --time="$CHPC_TIME" \
    --job-name="${JOB_NAME}_$label" \
    --output="$LOG_DIR/$label/stdout.log" \
    --error="$LOG_DIR/$label/stderr.log" \
    --export="GPU_LABEL=$label,SM_ARCH=$arch,PROJECT_DIR=$PROJECT_DIR,JOB_BIN=$JOB_BIN,JOB_INPUT=$JOB_INPUT,JOB_OUTPUT=$JOB_OUTPUT,CUDA_MODULE=$CUDA_MODULE" \
    "$BATCH_SCRIPT" \
    | awk '{print $NF}')

  echo "  [$label] job $job_id -> logs/$label/"
done

echo ""
echo "Monitor: squeue -u \$USER"
echo "Logs:    $LOG_DIR/"
