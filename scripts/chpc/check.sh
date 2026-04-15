#!/usr/bin/env bash
# Format, lint, and build checks.
# Run on the CHPC login node before requesting a GPU allocation.
# Usage: ./scripts/chpc/check.sh && ./scripts/chpc/gpu-shell.sh
# Configure via .env at the project root.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$SCRIPT_DIR/../validate.sh"

SOURCES=(
  "$PROJECT_DIR/bins/analyzer/src/main.cpp"
  "$PROJECT_DIR/bins/analyzer/src/vectorField.cpp"
  "$PROJECT_DIR/bins/analyzer/src/fieldReader.cpp"
  "$PROJECT_DIR/bins/analyzer/src/analyzerConfigParser.cpp"
  "$PROJECT_DIR/bins/analyzer/src/mpiCPU.cpp"
  "$PROJECT_DIR/bins/analyzer/src/openMP.cpp"
  "$PROJECT_DIR/bins/analyzer/src/pthreads.cpp"
  "$PROJECT_DIR/bins/analyzer/src/sequentialCPU.cpp"
  "$PROJECT_DIR/bins/analyzer/src/solverFactory.cpp"
  "$PROJECT_DIR/bins/analyzer/src/streamWriter.cpp"
  "$PROJECT_DIR/bins/simulator/src/main.cpp"
  "$PROJECT_DIR/bins/simulator/src/configParser.cpp"
  "$PROJECT_DIR/bins/simulator/src/fieldGenerator.cpp"
  "$PROJECT_DIR/bins/simulator/src/fieldWriter.cpp"
)

cd "$PROJECT_DIR"

echo "==> fmt"
fmt_failed=0
for f in "${SOURCES[@]}"; do
  if ! clang-format "$f" | diff -u "$f" - > /dev/null; then
    echo "  FAIL: $f is not formatted (run: clang-format -i $f)"
    fmt_failed=1
  fi
done
[[ $fmt_failed -eq 0 ]] || exit 1

echo "==> lint"
clang-tidy -p "$PROJECT_DIR/build" --config-file="$PROJECT_DIR/.clang-tidy" \
  --warnings-as-errors='*' "${SOURCES[@]}"

echo "==> build"
cmake --build build --parallel

echo ""
echo "Check passed. Run ./scripts/chpc/gpu-shell.sh to request a GPU node."
