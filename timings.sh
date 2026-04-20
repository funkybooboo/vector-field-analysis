#!/usr/bin/env bash
# Read timing files written by the analyzer and print a speedup report.
#
# Usage:
#   ./timings.sh                    # report for all stems in data/
#   ./timings.sh vortex_256x256     # report for one stem

set -euo pipefail

DATA_DIR="${DATA_DIR:-data}"

report_stem() {
    local stem="$1"
    local dir="$DATA_DIR/$stem"
    local -a files=()

    shopt -s nullglob
    files=("$dir"/timing_*.txt)
    shopt -u nullglob
    [[ ${#files[@]} -eq 0 ]] && return 0

    local -a labels=()
    local -a times=()
    local -a solvers=()
    local baseline_ms=""

    for f in "${files[@]}"; do
        local solver="" ms="" label=""
        while IFS='=' read -r key val; do
            case "$key" in
                solver) solver="$val" ;;
                ms)     ms="$val"     ;;
                label)  label="$val"  ;;
            esac
        done < "$f"
        [[ -z "$ms" ]] && continue
        labels+=("$label")
        times+=("$ms")
        solvers+=("$solver")
        [[ "$solver" == "sequential" ]] && baseline_ms="$ms"
    done

    [[ ${#times[@]} -eq 0 ]] && return 0

    # fall back: slowest = 1.00x baseline when sequential wasn't run
    if [[ -z "$baseline_ms" ]]; then
        baseline_ms=$(printf '%s\n' "${times[@]}" | sort -rn | head -1)
    fi

    echo ""
    echo "=== $stem ==="
    printf "%-36s %12s %10s\n" "Solver" "Time (ms)" "Speedup"
    printf "%-36s %12s %10s\n" "------" "---------" "-------"

    local n="${#times[@]}"
    local sorted
    sorted=$(for ((i=0; i<n; i++)); do printf '%s\t%d\n' "${times[$i]}" "$i"; done \
             | sort -t$'\t' -k1 -rn)

    while IFS=$'\t' read -r _ idx; do
        local speedup
        speedup=$(awk "BEGIN { printf \"%.2f\", $baseline_ms / ${times[$idx]} }")
        printf "%-36s %12.3f %9sx\n" "${labels[$idx]}" "${times[$idx]}" "$speedup"
    done <<< "$sorted"
}

if [[ $# -ge 1 ]]; then
    report_stem "$1"
else
    shopt -s nullglob
    for d in "$DATA_DIR"/*/; do
        [[ -d "$d" ]] || continue
        report_stem "$(basename "$d")"
    done
    shopt -u nullglob
fi
