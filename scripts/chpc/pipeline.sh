#!/usr/bin/env bash
# Build simulator and analyzer, then submit one batch job per config stem.
# Run on the CHPC login node.
# Configure via .env at the project root.
#
# Usage:
#   ./scripts/chpc/pipeline.sh                    # all configs in configs/
#   ./scripts/chpc/pipeline.sh stem1 stem2        # named stems only

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$SCRIPT_DIR/../validate.sh"
[[ -f "$PROJECT_DIR/.env" ]] && source "$PROJECT_DIR/.env"

validate_or_die \
  _check_account _check_partition _check_gpu _check_time _check_ntasks \
  _check_openmpi_module _check_hdf5_module _check_cuda_module

CONFIGS_DIR="$PROJECT_DIR/configs"
LOG_DIR="$PROJECT_DIR/logs"
JOB_NAME="${JOB_NAME:-vfa}"

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
      echo "error: no config for stem '$stem' in $CONFIGS_DIR" >&2
      exit 1
    fi
  done
fi

# -- Build --------------------------------------------------------------------

# shellcheck source=/dev/null
source /etc/profile.d/lmod.sh 2>/dev/null || true
module load "$OPENMPI_MODULE"
module load "$HDF5_MODULE"
module load "$CUDA_MODULE"

echo "==> Configuring"
cmake -B "$PROJECT_DIR/build" \
  -DCMAKE_BUILD_TYPE=Release \
  -S "$PROJECT_DIR" \
  > /dev/null

echo "==> Building"
cmake --build "$PROJECT_DIR/build" --target simulator analyzer -- -j"$(nproc)"

echo "==> Staging binaries"
cp -f "$PROJECT_DIR/build/bins/simulator/simulator" "$PROJECT_DIR/simulator_run"
cp -f "$PROJECT_DIR/build/bins/analyzer/analyzer"   "$PROJECT_DIR/analyzer_run"
echo "  staged simulator_run  analyzer_run"

# -- Submit -------------------------------------------------------------------

echo ""
echo "==> Submitting jobs"
for stem in "${STEMS[@]}"; do
  mkdir -p "$LOG_DIR/$stem"
  rm -f "$LOG_DIR/$stem/stdout.log" "$LOG_DIR/$stem/stderr.log"

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
    --job-name="${JOB_NAME}_${stem}" \
    --output="$LOG_DIR/$stem/stdout.log" \
    --error="$LOG_DIR/$stem/stderr.log" \
    --export="STEM=$stem,PROJECT_DIR=$PROJECT_DIR,OPENMPI_MODULE=$OPENMPI_MODULE,HDF5_MODULE=$HDF5_MODULE,CUDA_MODULE=$CUDA_MODULE" \
    "$SCRIPT_DIR/pipeline-job.sh" \
    | awk '{print $NF}')

  echo "  [$stem] job $job_id -> logs/$stem/"
done

echo ""
