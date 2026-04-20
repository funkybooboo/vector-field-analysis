#!/usr/bin/env bash
# Print all .log and .txt files under data/ with their paths as headers.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)/data"

find "$DATA_DIR" -type f \( -name "*.log" -o -name "*.txt" \) | sort | while read -r f; do
	echo "=== $f ==="
	cat "$f"
	echo ""
done
