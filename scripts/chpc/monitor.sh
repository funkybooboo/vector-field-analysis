#!/usr/bin/env bash
# Monitor pipeline jobs: per-stem status, time estimates, and log tails.
# Run on the CHPC login node.
#
# Usage:
#   ./scripts/chpc/monitor.sh            # status table + last 20 lines of all logs
#   ./scripts/chpc/monitor.sh karman     # live tail a specific stem's log

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
LOG_DIR="$PROJECT_DIR/logs"
CONFIGS_DIR="$PROJECT_DIR/configs"
# shellcheck source=/dev/null
[[ -f "$PROJECT_DIR/.env" ]] && source "$PROJECT_DIR/.env"
JOB_NAME="${JOB_NAME:-vfa}"

# -- Live tail for a specific stem --------------------------------------------

if [[ $# -eq 1 ]]; then
	log="$LOG_DIR/$1/stdout.log"
	if [[ ! -f "$log" ]]; then
		echo "error: no log found at $log" >&2
		exit 1
	fi
	echo "==> tailing $log (Ctrl-C to stop)"
	tail -f "$log"
	exit 0
fi

# -- Per-stem status table ----------------------------------------------------

# Collect stems from configs/
STEMS=()
for f in "$CONFIGS_DIR"/*.toml; do
	STEMS+=("$(basename "$f" .toml)")
done

# Fetch active jobs (PENDING or RUNNING) into an associative array keyed by stem
declare -A JOB_STATE JOB_ELAPSED JOB_START

while IFS= read -r line; do
	[[ -z "$line" ]] && continue
	jobid=$(awk '{print $1}' <<<"$line")
	name=$(awk '{print $2}' <<<"$line")
	state=$(awk '{print $3}' <<<"$line")
	elapsed=$(awk '{print $4}' <<<"$line")
	# Strip job name prefix to recover stem
	stem="${name#"${JOB_NAME}"_}"
	JOB_STATE["$stem"]="$state"
	JOB_ELAPSED["$stem"]="$elapsed"
	# Estimated start for pending jobs
	if [[ "$state" == "PENDING" ]]; then
		est=$(squeue --start -j "$jobid" --format="%S" --noheader 2>/dev/null | head -1 || true)
		JOB_START["$stem"]="${est:-unknown}"
	fi
done < <(squeue -u "$USER" --format="%i %j %T %M %D" --noheader 2>/dev/null)

# Fetch recently completed jobs via sacct (today)
declare -A SACCT_STATE SACCT_ELAPSED

while IFS='|' read -r name state elapsed; do
	name="${name%%_[0-9]*}" # strip any array suffix
	stem="${name#"${JOB_NAME}"_}"
	[[ -z "$stem" || "$stem" == "$name" ]] && continue
	# Only record if not already seen as active
	if [[ -z "${JOB_STATE[$stem]:-}" ]]; then
		SACCT_STATE["$stem"]="$state"
		SACCT_ELAPSED["$stem"]="$elapsed"
	fi
done < <(sacct -u "$USER" --starttime=today \
	--format="JobName,State,Elapsed" --noheader --parsable2 2>/dev/null |
	grep "^${JOB_NAME}_" || true)

# Print table
printf "\n==> status\n"
printf "%-36s  %-12s  %-10s  %s\n" "STEM" "STATE" "ELAPSED" "NOTE"
printf "%-36s  %-12s  %-10s  %s\n" "----" "-----" "-------" "----"

for stem in "${STEMS[@]}"; do
	if [[ -n "${JOB_STATE[$stem]:-}" ]]; then
		state="${JOB_STATE[$stem]}"
		elapsed="${JOB_ELAPSED[$stem]}"
		note=""
		[[ "$state" == "PENDING" ]] && note="est. start: ${JOB_START[$stem]:-unknown}"
	elif [[ -n "${SACCT_STATE[$stem]:-}" ]]; then
		state="${SACCT_STATE[$stem]}"
		elapsed="${SACCT_ELAPSED[$stem]}"
		note=""
	elif [[ -f "$LOG_DIR/$stem/stdout.log" ]]; then
		state="UNKNOWN"
		elapsed="-"
		note="log exists but no job record found"
	else
		state="NOT RUN"
		elapsed="-"
		note=""
	fi
	printf "%-36s  %-12s  %-10s  %s\n" "$stem" "$state" "$elapsed" "$note"
done

# -- Log tails ----------------------------------------------------------------

echo ""
echo "==> logs"
for stem in "${STEMS[@]}"; do
	log="$LOG_DIR/$stem/stdout.log"
	[[ -f "$log" ]] || continue
	printf "\n--- %s ---\n" "$stem"
	tail -n 20 "$log"
done
