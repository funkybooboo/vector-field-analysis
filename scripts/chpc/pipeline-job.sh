#!/usr/bin/env bash
# SLURM batch worker -- submitted by pipeline.sh, do not run directly.
# Runs simulator -> analyzer for one config stem.
# Expects these env vars (set via sbatch --export):
#   STEM             - config stem (e.g. vortex_256x256)
#   PROJECT_DIR      - absolute path to the project root
#   CUDA_MODULE      - CUDA module to load (e.g. cuda/11.6.2)
#   OPENMPI_MODULE   - OpenMPI module to load (e.g. openmpi/5.0.8)
#   HDF5_MODULE      - HDF5 module to load (e.g. hdf5/1.14.6)
#   CUDA_BLOCK_SIZE  - CUDA threads per block (default: 256)

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

mkdir -p "$PROJECT_DIR/data/$STEM"

echo "=== simulator ==="
"$PROJECT_DIR/build/bins/simulator/simulator" "configs/$STEM.toml" \
	>"$PROJECT_DIR/data/$STEM/simulator_stdout.txt" \
	2>"$PROJECT_DIR/data/$STEM/simulator_stderr.txt"

echo ""
echo "=== analyzer ==="

tmp_toml="$PROJECT_DIR/data/$STEM/${STEM}_benchmark.toml"
sed '/^\[analyzer\]/,$d' "$PROJECT_DIR/configs/$STEM.toml" >"$tmp_toml"
printf '\n[analyzer]\nsolver = "benchmark"\noutput = "%s"\n' \
	"$PROJECT_DIR/data/$STEM/streams.h5" >>"$tmp_toml"
srun --mpi=pmix -n "$SLURM_NTASKS" "$PROJECT_DIR/build/bins/analyzer/analyzer" "$tmp_toml" \
	>"$PROJECT_DIR/data/$STEM/analyzer_stdout.txt" \
	2>"$PROJECT_DIR/data/$STEM/analyzer_stderr.txt"
rm -f "$tmp_toml"
