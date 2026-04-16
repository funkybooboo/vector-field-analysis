#!/usr/bin/env bash
# SSH into a CHPC cluster login node.
# Run from your local machine.
# Configure via .env at the project root.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../validate.sh"

validate_or_die _check_user _check_host

ssh "${CHPC_USER}@${CHPC_HOST}"
