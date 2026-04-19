#!/usr/bin/env bash
# Cancel all running/pending pipeline jobs for this user.
# Run on the CHPC login node.
#
# Usage:
#   ./scripts/chpc/stop.sh          # cancel all your jobs
#   ./scripts/chpc/stop.sh karman   # cancel jobs matching a stem name

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
[[ -f "$PROJECT_DIR/.env" ]] && source "$PROJECT_DIR/.env"

JOB_NAME="${JOB_NAME:-vfa}"

if [[ $# -eq 1 ]]; then
  pattern="${JOB_NAME}_$1"
else
  pattern="${JOB_NAME}_"
fi

mapfile -t job_ids < <(
  squeue -u "$USER" --format="%i %j" --noheader \
    | awk -v pat="$pattern" '$2 ~ pat {print $1}'
)

if [[ ${#job_ids[@]} -eq 0 ]]; then
  echo "No matching jobs found."
  exit 0
fi

for id in "${job_ids[@]}"; do
  name=$(squeue -j "$id" --format="%j" --noheader 2>/dev/null || echo "unknown")
  scancel "$id"
  echo "cancelled job $id ($name)"
done
