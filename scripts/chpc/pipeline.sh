#!/usr/bin/env bash
# Build simulator and analyzer, then submit one batch job per config stem.
# Run on the CHPC login node.
# Configure via .env at the project root.
#
# Usage:
#   ./scripts/chpc/pipeline.sh                    # all configs in configs/
#   ./scripts/chpc/pipeline.sh stem1 stem2        # named stems only

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
# shellcheck source=scripts/validate.sh
source "$SCRIPT_DIR/../validate.sh"
# shellcheck source=/dev/null
[[ -f "$PROJECT_DIR/.env" ]] && source "$PROJECT_DIR/.env"

validate_or_die \
	_check_account _check_partition _check_gpu _check_time _check_ntasks \
	_check_openmpi_module _check_hdf5_module _check_cuda_module

CONFIGS_DIR="$PROJECT_DIR/configs"
DATA_DIR="$PROJECT_DIR/data"
JOB_NAME="${JOB_NAME:-vfa}"

# -- Resolve stems ------------------------------------------------------------

if [[ $# -eq 0 ]]; then
	STEMS=()
	shopt -s nullglob
	for f in "$CONFIGS_DIR"/*.toml; do
		STEMS+=("$(basename "$f" .toml)")
	done
	shopt -u nullglob
	if [[ ${#STEMS[@]} -eq 0 ]]; then
		echo "error: no *.toml configs found in $CONFIGS_DIR" >&2
		exit 1
	fi
else
	STEMS=("$@")
	for stem in "${STEMS[@]}"; do
		if [[ ! -f "$CONFIGS_DIR/$stem.toml" ]]; then
			echo "error: no config for stem '$stem' in $CONFIGS_DIR" >&2
			exit 1
		fi
	done
fi

# -- Build --------------------------------------------------------------------

# shellcheck source=/dev/null
source /etc/profile.d/lmod.sh 2>/dev/null || true
module load "$OPENMPI_MODULE"
module load "$HDF5_MODULE"
module load "$CUDA_MODULE"

HDF5_ROOT="$(dirname "$(dirname "$(which h5dump)")")"
MPI_ROOT="$(dirname "$(dirname "$(which mpicc)")")"
CUDA_ROOT="$(dirname "$(dirname "$(which nvcc)")")"

echo "==> Configuring"
cmake -B "$PROJECT_DIR/build" \
	-DCMAKE_BUILD_TYPE=Release \
-DENABLE_CUDA=ON \
	-DCMAKE_CUDA_ARCHITECTURES="52;60;61;70;75;80;86" \
	-DHDF5_ROOT="$HDF5_ROOT" \
	-DHDF5_C_LIBRARY_hdf5="$HDF5_ROOT/lib/libhdf5.so" \
	-DCMAKE_INSTALL_RPATH="$HDF5_ROOT/lib;$CUDA_ROOT/lib64" \
	-DMPI_HOME="$MPI_ROOT" \
	-S "$PROJECT_DIR" \
	>/dev/null

echo "==> Building"
cmake --build "$PROJECT_DIR/build" --target simulator analyzer -- -j"$(nproc)"

# -- Submit -------------------------------------------------------------------

echo ""
echo "==> Submitting jobs"
for stem in "${STEMS[@]}"; do
	mkdir -p "$DATA_DIR/$stem"
	rm -f "$DATA_DIR/$stem/stdout.log" "$DATA_DIR/$stem/stderr.log"

	account_flag=()
	if [[ "$CHPC_ACCOUNT" != "none" ]]; then
		account_flag=(--account="$CHPC_ACCOUNT")
	fi

	exclude_flag=()
	if [[ -n "${CHPC_EXCLUDE:-}" ]]; then
		exclude_flag=(--exclude="$CHPC_EXCLUDE")
	fi

	job_id=$(sbatch \
		--partition="$CHPC_PARTITION" \
		"${account_flag[@]}" \
		"${exclude_flag[@]}" \
		--gres="gpu:$CHPC_GPU" \
		--nodes=1 \
		--ntasks="$CHPC_NTASKS" \
		--time="$CHPC_TIME" \
		--job-name="${JOB_NAME}_${stem}" \
		--output="$DATA_DIR/$stem/stdout.log" \
		--error="$DATA_DIR/$stem/stderr.log" \
		--export="STEM=$stem,PROJECT_DIR=$PROJECT_DIR,OPENMPI_MODULE=$OPENMPI_MODULE,HDF5_MODULE=$HDF5_MODULE,CUDA_MODULE=$CUDA_MODULE" \
		"$SCRIPT_DIR/pipeline-job.sh" |
		awk '{print $NF}')

	echo "  [$stem] job $job_id -> data/$stem/"
done

echo ""
