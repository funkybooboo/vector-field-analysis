#!/usr/bin/env bash
# Format, lint, and build checks.
# Run on the CHPC login node before requesting a GPU allocation.
# Usage: ./scripts/chpc/check.sh && ./scripts/chpc/gpu-shell.sh
# Configure via .env at the project root.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$SCRIPT_DIR/../validate.sh"

# C++ sources: formatted + linted
CPP_SOURCES=(
  "$PROJECT_DIR/bins/analyzer/src/main.cpp"
  "$PROJECT_DIR/bins/analyzer/src/configParser.cpp"
  "$PROJECT_DIR/bins/analyzer/src/mpiStreamlineSolver.cpp"
  "$PROJECT_DIR/bins/analyzer/src/openMpStreamlineSolver.cpp"
  "$PROJECT_DIR/bins/analyzer/src/pthreadsStreamlineSolver.cpp"
  "$PROJECT_DIR/bins/analyzer/src/sequentialStreamlineSolver.cpp"
  "$PROJECT_DIR/bins/analyzer/src/solverFactory.cpp"
  "$PROJECT_DIR/bins/analyzer/src/streamWriter.cpp"
  "$PROJECT_DIR/bins/simulator/src/main.cpp"
  "$PROJECT_DIR/bins/simulator/src/configParser.cpp"
  "$PROJECT_DIR/bins/simulator/src/fieldGenerator.cpp"
)

# CUDA sources: formatted only (clang-tidy does not handle .cu without extra setup)
CUDA_SOURCES=(
  "$PROJECT_DIR/bins/analyzer/src/cudaFull.cu"
  "$PROJECT_DIR/bins/analyzer/src/cudaFullStreamlineSolver.cu"
  "$PROJECT_DIR/bins/analyzer/src/cudaStreamlineSolver.cu"
)

cd "$PROJECT_DIR"

echo "==> fmt"
fmt_failed=0
for f in "${CPP_SOURCES[@]}" "${CUDA_SOURCES[@]}"; do
  if ! clang-format "$f" | diff -u "$f" - > /dev/null; then
    echo "  FAIL: $f is not formatted (run: clang-format -i $f)"
    fmt_failed=1
  fi
done
[[ $fmt_failed -eq 0 ]] || exit 1

echo "==> lint"
clang-tidy -p "$PROJECT_DIR/build" --config-file="$PROJECT_DIR/.clang-tidy" \
  --warnings-as-errors='*' "${CPP_SOURCES[@]}"

echo "==> build"
cmake --build build --parallel

echo ""
echo "Check passed. Run ./scripts/chpc/gpu-shell.sh to request a GPU node."
