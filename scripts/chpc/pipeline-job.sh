#!/usr/bin/env bash
# SLURM batch worker -- submitted by pipeline.sh, do not run directly.
# Runs simulator -> analyzer for one config stem.
# Expects these env vars (set via sbatch --export):
#   STEM           - config stem (e.g. karman_street_128x64)
#   PROJECT_DIR    - absolute path to the project root
#   CUDA_MODULE    - CUDA module to load (e.g. cuda/11.6.2)
#   OPENMPI_MODULE - OpenMPI module to load (e.g. openmpi/5.0.8)
#   HDF5_MODULE    - HDF5 module to load (e.g. hdf5/1.14.6)

set -euo pipefail

# shellcheck source=/dev/null
source /etc/profile.d/lmod.sh 2>/dev/null || true
module load "$OPENMPI_MODULE"
module load "$HDF5_MODULE"
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
mpirun -n "$SLURM_NTASKS" "$PROJECT_DIR/analyzer_run" "configs/$STEM.toml"
