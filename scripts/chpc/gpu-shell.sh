#!/usr/bin/env bash
# Request an interactive GPU session via srun and drop into a shell.
# Run on the CHPC login node.
# Configure via .env at the project root.
#
# Examples:
#   ./scripts/chpc/gpu-shell.sh
#   CHPC_GPU=v100:1 ./scripts/chpc/gpu-shell.sh
#   CHPC_TIME=2:00:00 ./scripts/chpc/gpu-shell.sh
#   CHPC_PARTITION=notchpeak-shared-short CHPC_ACCOUNT=none ./scripts/chpc/gpu-shell.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$SCRIPT_DIR/../validate.sh"

validate_or_die _check_account _check_partition _check_gpu _check_time _check_ntasks

account_flag=()
if [[ "$CHPC_ACCOUNT" != "none" ]]; then
  account_flag=(--account="$CHPC_ACCOUNT")
fi

srun \
  --partition="$CHPC_PARTITION" \
  "${account_flag[@]}" \
  --gres="gpu:$CHPC_GPU" \
  --time="$CHPC_TIME" \
  --ntasks="$CHPC_NTASKS" \
  --pty bash
