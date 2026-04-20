#!/usr/bin/env bash
# Run configs through the full pipeline: simulator -> analyzer -> stats -> visualizer.
# Saves all stdout/stderr and visualizer output to data/<stem>/.
# Run from your local machine.
#
# Usage:
#   ./scripts/local/pipeline.sh                    # run all configs in configs/
#   ./scripts/local/pipeline.sh vortex hurricane   # run only named stems

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

# shellcheck source=/dev/null
[[ -f "$PROJECT_DIR/.env" ]] && source "$PROJECT_DIR/.env"

SIMULATOR="$PROJECT_DIR/build/bins/simulator/simulator"
ANALYZER="$PROJECT_DIR/build/bins/analyzer/analyzer"
STATS="$PROJECT_DIR/tools/stats.py"
VISUALIZER="$PROJECT_DIR/tools/visualize.py"
CONFIGS_DIR="$PROJECT_DIR/configs"
DATA_DIR="$PROJECT_DIR/data"

# -- Resolve stems ------------------------------------------------------------

if [[ $# -eq 0 ]]; then
	STEMS=()
	shopt -s nullglob
	for f in "$CONFIGS_DIR"/*.toml; do
		STEMS+=("$(basename "$f" .toml)")
	done
	shopt -u nullglob
	if [[ ${#STEMS[@]} -eq 0 ]]; then
		echo "ERROR: no *.toml configs found in $CONFIGS_DIR" >&2
		exit 1
	fi
else
	STEMS=("$@")
	for stem in "${STEMS[@]}"; do
		if [[ ! -f "$CONFIGS_DIR/$stem.toml" ]]; then
			echo "ERROR: no config for stem '$stem' in $CONFIGS_DIR" >&2
			exit 1
		fi
	done
fi

# -- Build --------------------------------------------------------------------

mkdir -p "$DATA_DIR"

echo "==> Building..."
configure_log=$(cmake -B "$PROJECT_DIR/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -S "$PROJECT_DIR" 2>&1) || {
	printf "%s\n" "$configure_log" >&2
	echo "ERROR: CMake configure failed. Aborting." >&2
	exit 1
}
if ! cmake --build "$PROJECT_DIR/build" --parallel 2>&1 | tee "$DATA_DIR/build_output.txt"; then
	echo "ERROR: Build failed. Aborting." >&2
	exit 1
fi
echo "==> Build OK"
echo

# -- Per-config pipeline ------------------------------------------------------

declare -A SIM_STATUS ANA_STATUS STATS_STATUS VIS_STATUS

for stem in "${STEMS[@]}"; do
	export STEM="$stem"
	config="$CONFIGS_DIR/$stem.toml"
	out="$DATA_DIR/$stem"
	mkdir -p "$out"

	printf "==> [%s]\n" "$stem"

	# Simulator
	if "$SIMULATOR" "$config" \
		> >(tee "$out/simulator_stdout.txt") \
		2> >(tee "$out/simulator_stderr.txt" >&2); then
		SIM_STATUS[$stem]="OK"
		printf "    simulator  OK\n"
	else
		SIM_STATUS[$stem]="FAIL"
		printf "    simulator  FAIL (exit %d)\n" "$?"
		ANA_STATUS[$stem]="SKIP"
		STATS_STATUS[$stem]="SKIP"
		VIS_STATUS[$stem]="SKIP"
		continue
	fi

	# Analyzer -- run each implementation separately, hash outputs, compare to sequential
	printf "    analyzer   running implementations...\n"
	ANA_STATUS[$stem]="OK"
	tmp_dir="$(mktemp -d)"

	# Strip existing [analyzer] section from config to build per-impl TOMLs from
	base_toml="$tmp_dir/base.toml"
	sed '/^\[analyzer\]/,$d' "$config" >"$base_toml"

	ref_hash=""
	ana_failed=0

	run_impl() {
		local label="$1" solver="$2" threads="$3" block_size="$4" mpi_n="$5"
		local tmp_toml="$tmp_dir/${label}.toml"
		local tmp_out="$tmp_dir/${label}.h5"
		local timing_name
		timing_name=$(printf '%s' "$label" | tr -cs 'a-zA-Z0-9' '_' | sed 's/__*/_/g;s/^_//;s/_$//')
		local timing_out="$out/timing_${timing_name}.txt"
		cp "$base_toml" "$tmp_toml"
		printf '\n[analyzer]\nsolver = "%s"\nthreads = %d\ncuda_block_size = %d\noutput = "%s"\ntiming_output = "%s"\n' \
			"$solver" "$threads" "$block_size" "$tmp_out" "$timing_out" >>"$tmp_toml"

		local stdout_capture="$tmp_dir/${label}_stdout.txt"
		if [[ "$mpi_n" -gt 0 ]]; then
			mpirun -n "$mpi_n" "$ANALYZER" "$tmp_toml" \
				>"$stdout_capture" 2>> >(tee -a "$out/analyzer_stderr.txt" >&2)
		else
			"$ANALYZER" "$tmp_toml" \
				>"$stdout_capture" 2>> >(tee -a "$out/analyzer_stderr.txt" >&2)
		fi
		local rc=$?
		cat "$stdout_capture" | tee -a "$out/analyzer_stdout.txt"
		if [[ $rc -ne 0 ]]; then
			printf "    %-30s FAIL\n" "$label"
			return 1
		fi
		if [[ ! -f "$tmp_out" ]]; then
			printf "    %-30s SKIP (no output file)\n" "$label"
			return 0
		fi
		local hash
		hash=$(h5dump "$tmp_out" | sha256sum | awk '{print $1}')
		if [[ -z "$ref_hash" ]]; then
			ref_hash="$hash"
			printf "    %-30s OK  (reference)\n" "$label"
		elif [[ "$hash" == "$ref_hash" ]]; then
			printf "    %-30s OK  (match)\n" "$label"
		else
			printf "    %-30s MISMATCH\n" "$label"
			return 1
		fi
		return 0
	}

	# sequential is always the reference
	run_impl "sequential" sequential 1 256 0 || { ana_failed=1; }

	if [[ $ana_failed -eq 0 ]]; then
		run_impl "pthreads (2t)" pthreads 2 256 0 || ana_failed=1
		run_impl "pthreads (4t)" pthreads 4 256 0 || ana_failed=1
		run_impl "pthreads (8t)" pthreads 8 256 0 || ana_failed=1
		run_impl "openmp (2t)" openmp 2 256 0 || ana_failed=1
		run_impl "openmp (4t)" openmp 4 256 0 || ana_failed=1
		run_impl "openmp (8t)" openmp 8 256 0 || ana_failed=1
		run_impl "mpi (2 ranks)" mpi 1 256 2 || ana_failed=1
		run_impl "mpi (4 ranks)" mpi 1 256 4 || ana_failed=1
		# CUDA is optional -- skip on failure (may not be built with ENABLE_CUDA)
		run_impl "cuda (blk=128)" cuda 1 128 0 || true
		run_impl "cuda (blk=256)" cuda 1 256 0 || true
		run_impl "cuda (blk=512)" cuda 1 512 0 || true
		# cudaMpi is optional -- requires CUDA+MPI
		run_impl "cudaMpi (2 ranks, blk=256)" cudaMpi 1 256 2 || true
		run_impl "cudaMpi (4 ranks, blk=256)" cudaMpi 1 256 4 || true
	fi

	# Copy sequential output as the canonical streams.h5 for downstream steps
	if [[ $ana_failed -eq 0 && -f "$tmp_dir/sequential.h5" ]]; then
		cp "$tmp_dir/sequential.h5" "$out/streams.h5"
	fi
	rm -rf "$tmp_dir"

	if [[ $ana_failed -eq 1 ]]; then
		ANA_STATUS[$stem]="FAIL"
		printf "    analyzer   FAIL\n"
		STATS_STATUS[$stem]="SKIP"
		VIS_STATUS[$stem]="SKIP"
		continue
	fi
	printf "    analyzer   OK\n"

	# Stats
	if uv run "$STATS" "$out/field.h5" "$out/streams.h5" \
		> >(tee "$out/stats_stdout.txt") \
		2> >(tee "$out/stats_stderr.txt" >&2); then
		STATS_STATUS[$stem]="OK"
		printf "    stats      OK\n"
	else
		STATS_STATUS[$stem]="FAIL"
		printf "    stats      FAIL (exit %d)\n" "$?"
	fi

	# Visualizer
	if uv run "$VISUALIZER" "$out/field.h5" \
		--streams "$out/streams.h5" \
		--save "$out/animation.gif" \
		> >(tee "$out/visualizer_stdout.txt") \
		2> >(tee "$out/visualizer_stderr.txt" >&2); then
		VIS_STATUS[$stem]="OK"
		printf "    visualizer OK\n"
	else
		VIS_STATUS[$stem]="FAIL"
		printf "    visualizer FAIL (exit %d)\n" "$?"
	fi
done

# -- Summary ------------------------------------------------------------------

echo
printf "%-40s  %-10s %-10s %-10s %-10s\n" "CONFIG" "SIMULATOR" "ANALYZER" "STATS" "VISUALIZER"
printf "%-40s  %-10s %-10s %-10s %-10s\n" "------" "---------" "--------" "-----" "----------"

for stem in "${STEMS[@]}"; do
	printf "%-40s  %-10s %-10s %-10s %-10s\n" \
		"$stem" \
		"${SIM_STATUS[$stem]:-SKIP}" \
		"${ANA_STATUS[$stem]:-SKIP}" \
		"${STATS_STATUS[$stem]:-SKIP}" \
		"${VIS_STATUS[$stem]:-SKIP}"
done

echo
