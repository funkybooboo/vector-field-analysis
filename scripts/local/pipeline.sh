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

	# Analyzer (benchmark -- all variants in one call)
	printf "    analyzer   benchmarking...\n"
	ANA_STATUS[$stem]="OK"
	tmp_dir="$(mktemp -d)"
	tmp_toml="$tmp_dir/${stem}.toml"
	sed '/^\[analyzer\]/,$d' "$config" >"$tmp_toml"
	printf '\n[analyzer]\nsolver = "benchmark"\noutput = "%s"\n' "$out/streams.h5" >>"$tmp_toml"
	if "$ANALYZER" "$tmp_toml" \
		> >(tee "$out/analyzer_stdout.txt") \
		2> >(tee "$out/analyzer_stderr.txt" >&2); then
		printf "    analyzer   OK\n"
	else
		ANA_STATUS[$stem]="FAIL"
		printf "    analyzer   FAIL\n"
		rm -rf "$tmp_dir"
		STATS_STATUS[$stem]="SKIP"
		VIS_STATUS[$stem]="SKIP"
		continue
	fi
	rm -rf "$tmp_dir"

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
