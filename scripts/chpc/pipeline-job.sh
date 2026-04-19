#!/usr/bin/env bash
# SLURM batch worker -- submitted by pipeline.sh, do not run directly.
# Runs simulator -> analyzer for one config stem.
# Expects these env vars (set via sbatch --export):
#   STEM             - config stem (e.g. vortex_256x256)
#   PROJECT_DIR      - absolute path to the project root
#   CUDA_MODULE      - CUDA module to load (e.g. cuda/11.6.2)
#   OPENMPI_MODULE   - OpenMPI module to load (e.g. openmpi/5.0.8)
#   HDF5_MODULE      - HDF5 module to load (e.g. hdf5/1.14.6)

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

out="$PROJECT_DIR/data/$STEM"
tmp_dir="$(mktemp -d)"

base_toml="$tmp_dir/base.toml"
sed '/^\[analyzer\]/,$d' "$PROJECT_DIR/configs/$STEM.toml" >"$base_toml"

ref_file=""
ref_ms=""
ana_failed=0

run_impl() {
	local label="$1" solver="$2" threads="$3" block_size="$4" mpi_n="$5"
	local tmp_toml="$tmp_dir/$STEM.toml"
	local tmp_out="$tmp_dir/${label}.h5"
	cp "$base_toml" "$tmp_toml"
	printf '\n[analyzer]\nsolver = "%s"\nthreads = %d\ncuda_block_size = %d\noutput = "%s"\n' \
		"$solver" "$threads" "$block_size" "$tmp_out" >>"$tmp_toml"

	local stdout_capture="$tmp_dir/${label}_stdout.txt"
	if [[ "$mpi_n" -gt 0 ]]; then
		mpirun -np "$mpi_n" "$PROJECT_DIR/build/bins/analyzer/analyzer" "$tmp_toml" \
			>"$stdout_capture" 2>>"$out/analyzer_stderr.txt"
	else
		"$PROJECT_DIR/build/bins/analyzer/analyzer" "$tmp_toml" \
			>"$stdout_capture" 2>>"$out/analyzer_stderr.txt"
	fi
	local rc=$?
	cat "$stdout_capture" >>"$out/analyzer_stdout.txt"
	local ms
	ms=$(grep -m1 ' ms$' "$stdout_capture" 2>/dev/null | awk '{print $(NF-1)}' || true)
	local timing_col=""
	if [[ -n "$ms" ]]; then
		timing_col=$(printf "%10.3f ms" "$ms")
		if [[ -n "$ref_ms" && "$ms" != "0" ]]; then
			local speedup
			speedup=$(awk "BEGIN {printf \"%.2fx\", $ref_ms / $ms}")
			timing_col="$timing_col  ($speedup vs sequential)"
		fi
	fi
	if [[ $rc -ne 0 ]]; then
		echo "  FAIL: $label" | tee -a "$out/analyzer_stdout.txt"
		return 1
	fi
	if [[ ! -f "$tmp_out" ]]; then
		echo "  SKIP (no output file): $label"
		return 1
	fi
	if [[ -z "$ref_file" ]]; then
		ref_file="$tmp_out"
		ref_ms="$ms"
		printf "  OK (reference): %-30s %s\n" "$label" "$timing_col"
	elif h5diff -q "$ref_file" "$tmp_out"; then
		printf "  OK (match):     %-30s %s\n" "$label" "$timing_col"
	else
		printf "  MISMATCH:       %-30s %s\n" "$label" "$timing_col"
		return 1
	fi
	return 0
}

run_impl "sequential"         sequential  1  256  0 || ana_failed=1

if [[ $ana_failed -eq 0 ]]; then
	run_impl "pthreads (2t)"  pthreads    2  256  0 || ana_failed=1
	run_impl "pthreads (4t)"  pthreads    4  256  0 || ana_failed=1
	run_impl "pthreads (8t)"  pthreads    8  256  0 || ana_failed=1
	run_impl "openmp (2t)"    openmp      2  256  0 || ana_failed=1
	run_impl "openmp (4t)"    openmp      4  256  0 || ana_failed=1
	run_impl "openmp (8t)"    openmp      8  256  0 || ana_failed=1
	run_impl "mpi (2 ranks)"  mpi         1  256  2 || ana_failed=1
	run_impl "mpi (4 ranks)"  mpi         1  256  4 || ana_failed=1
	run_impl "cuda (blk=128)" cuda        1  128  0 || ana_failed=1
	run_impl "cuda (blk=256)" cuda        1  256  0 || ana_failed=1
	run_impl "cuda (blk=512)" cuda        1  512  0 || ana_failed=1
fi

if [[ $ana_failed -eq 0 && -f "$tmp_dir/sequential.h5" ]]; then
	cp "$tmp_dir/sequential.h5" "$out/streams.h5"
fi
rm -rf "$tmp_dir"

if [[ $ana_failed -eq 1 ]]; then
	echo "ERROR: analyzer failed" >&2
	exit 1
fi
