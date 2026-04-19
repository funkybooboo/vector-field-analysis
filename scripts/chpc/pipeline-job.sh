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

_seq_ms=""

run_variant() {
	local solver=$1 workers=$2
	local variant_name="${solver}_${workers}"
	local out="$PROJECT_DIR/data/$STEM"
	local log_file="$out/analyzer_${variant_name}.txt"
	local streams_out="$out/streams_${variant_name}.h5"

	local tmp_toml="$out/${STEM}.toml"
	sed '/^\[analyzer\]/,$d' "$PROJECT_DIR/configs/$STEM.toml" >"$tmp_toml"
	printf "\n[analyzer]\nsolver = \"%s\"\nthreads = %d\noutput = \"%s\"\n" \
		"$solver" "$workers" "$streams_out" >>"$tmp_toml"

	if [[ "$solver" == "mpi" ]]; then
		srun --mpi=pmix -n "$workers" "$PROJECT_DIR/analyzer_run" "$tmp_toml" \
			>"$log_file" 2>&1
	else
		"$PROJECT_DIR/analyzer_run" "$tmp_toml" >"$log_file" 2>&1
	fi
	local status=$?

	rm -f "$tmp_toml"
	if [[ $status -eq 0 ]]; then
		local ms
		ms=$(grep -oE '[0-9.]+ ms' "$log_file" | tail -1 | awk '{print $1}')
		if [[ "$solver" == "sequential" ]]; then
			_seq_ms="$ms"
			printf "  %-20s %10s ms\n" "$variant_name" "$ms"
		elif [[ -n "$_seq_ms" && -n "$ms" ]]; then
			local ratio
			ratio=$(awk "BEGIN {if ($ms > 0) printf \"%.2f\", $_seq_ms / $ms; else print \"?\"}")
			printf "  %-20s %10s ms  (${ratio}x vs sequential)\n" "$variant_name" "$ms"
		else
			printf "  %-20s %10s ms\n" "$variant_name" "$ms"
		fi
		ln -sf "$(basename "$streams_out")" "$out/streams.h5"
		return 0
	else
		printf "  %-20s FAIL\n" "$variant_name"
		return 1
	fi
}

mkdir -p "$PROJECT_DIR/data/$STEM"
run_variant "sequential" 1
for t in 2 4 8; do run_variant "pthreads" "$t"; done
for t in 2 4 8; do run_variant "openmp" "$t"; done
for p in 2 4; do run_variant "mpi" "$p"; done
