#!/usr/bin/env bash
# Rsync local project files to CHPC. Excludes build artifacts and output files.
# Run from your local machine.
# Configure via .env at the project root.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$SCRIPT_DIR/../validate.sh"
[[ -f "$PROJECT_DIR/.env" ]] && source "$PROJECT_DIR/.env"

validate_or_die _check_user _check_host _check_project

REMOTE="${CHPC_USER}@${CHPC_HOST}:${CHPC_PROJECT}"

rsync -avz --progress \
  --exclude='output/' \
  --exclude='logs/' \
  --exclude='build/' \
  --exclude='build-coverage/' \
  --exclude='.cache/' \
  --exclude='*.o' \
  --exclude='analyzer' \
  --exclude='analyzer_sm*' \
  --exclude='simulator' \
  --exclude='simulator_sm*' \
  --exclude='.env' \
  "$PROJECT_DIR/" "$REMOTE"
