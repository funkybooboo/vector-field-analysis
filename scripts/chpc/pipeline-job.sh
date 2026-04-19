#!/usr/bin/env bash
# SLURM batch worker -- submitted by pipeline.sh, do not run directly.
# Runs simulator -> analyzer for one config stem.
# Expects these env vars (set via sbatch --export):
#   STEM         - config stem (e.g. karman_street_128x64)
#   PROJECT_DIR  - absolute path to the project root
#   CUDA_MODULE  - CUDA module to load (e.g. cuda/12.1.0)

set -euo pipefail

# shellcheck source=/dev/null
source /etc/profile.d/lmod.sh 2>/dev/null || true
module load "$CUDA_MODULE"

cd "$PROJECT_DIR"

echo "=== job info ==="
date
echo "host: $(hostname)"
echo "stem: $STEM"
echo ""

echo "=== simulator ==="
"$PROJECT_DIR/simulator_run" "configs/$STEM.toml"

echo ""
echo "=== analyzer ==="
srun "$PROJECT_DIR/analyzer_run" "configs/$STEM.toml"
