#!/usr/bin/env bash
# Shared validation helpers. Source this after loading .env.
# Each function prints a specific error and returns 1 on failure.
# Use validate_or_die to run a set of checks and exit if any fail.

# Runs all given validators, accumulates errors, exits if any failed.
# Usage: validate_or_die _check_user _check_host _check_partition
validate_or_die() {
  local failed=0
  for fn in "$@"; do
    $fn || failed=1
  done
  if [[ $failed -ne 0 ]]; then
    echo "" >&2
    echo "Fix the above in .env or export the variables before running." >&2
    exit 1
  fi
}

_check_user() {
  if [[ -z "${CHPC_USER:-}" ]]; then
    echo "error: CHPC_USER is not set (expected your uNID, e.g. u6074053)" >&2
    return 1
  fi
  if [[ ! "$CHPC_USER" =~ ^u[0-9]+$ ]]; then
    echo "error: CHPC_USER=\"$CHPC_USER\" does not look like a uNID (expected u followed by digits)" >&2
    return 1
  fi
}

_check_host() {
  if [[ -z "${CHPC_HOST:-}" ]]; then
    echo "error: CHPC_HOST is not set (e.g. notchpeak.chpc.utah.edu)" >&2
    return 1
  fi
  if [[ ! "$CHPC_HOST" =~ \. ]]; then
    echo "error: CHPC_HOST=\"$CHPC_HOST\" does not look like a hostname (expected a dot-separated name)" >&2
    return 1
  fi
}

_check_project() {
  if [[ -z "${CHPC_PROJECT:-}" ]]; then
    echo "error: CHPC_PROJECT is not set (path to the project on the remote, e.g. ~/projects/hw5.2)" >&2
    return 1
  fi
}

_check_partition() {
  if [[ -z "${CHPC_PARTITION:-}" ]]; then
    echo "error: CHPC_PARTITION is not set (e.g. notchpeak-gpu)" >&2
    return 1
  fi
}

_check_account() {
  if [[ -z "${CHPC_ACCOUNT:-}" ]]; then
    echo "error: CHPC_ACCOUNT is not set (use \"none\" if no account is required)" >&2
    return 1
  fi
}

_check_gpu() {
  if [[ -z "${CHPC_GPU:-}" ]]; then
    echo "error: CHPC_GPU is not set (e.g. a100:1)" >&2
    return 1
  fi
  if [[ ! "$CHPC_GPU" =~ ^[a-z0-9_]+:[0-9]+$ ]]; then
    echo "error: CHPC_GPU=\"$CHPC_GPU\" must be in type:count format (e.g. a100:1)" >&2
    return 1
  fi
}

_check_time() {
  if [[ -z "${CHPC_TIME:-}" ]]; then
    echo "error: CHPC_TIME is not set (e.g. 1:00:00)" >&2
    return 1
  fi
  if [[ ! "$CHPC_TIME" =~ ^[0-9]+:[0-5][0-9]:[0-5][0-9]$ ]]; then
    echo "error: CHPC_TIME=\"$CHPC_TIME\" must be in h:mm:ss format (e.g. 1:00:00)" >&2
    return 1
  fi
}

_check_ntasks() {
  if [[ -z "${CHPC_NTASKS:-}" ]]; then
    echo "error: CHPC_NTASKS is not set (e.g. 4)" >&2
    return 1
  fi
  if [[ ! "$CHPC_NTASKS" =~ ^[1-9][0-9]*$ ]]; then
    echo "error: CHPC_NTASKS=\"$CHPC_NTASKS\" must be a positive integer" >&2
    return 1
  fi
}

_check_cuda_arch() {
  if [[ -z "${CUDA_ARCH:-}" ]]; then
    echo "error: CUDA_ARCH is not set (e.g. sm_80 for A100)" >&2
    return 1
  fi
  if [[ ! "$CUDA_ARCH" =~ ^sm_[0-9]+$ ]]; then
    echo "error: CUDA_ARCH=\"$CUDA_ARCH\" must be in sm_XX format (e.g. sm_80)" >&2
    return 1
  fi
}

_check_job_name() {
  if [[ -z "${JOB_NAME:-}" ]]; then
    echo "error: JOB_NAME is not set (e.g. vfa)" >&2
    return 1
  fi
}

_check_job_bin() {
  if [[ -z "${JOB_BIN:-}" ]]; then
    echo "error: JOB_BIN is not set (e.g. analyzer)" >&2
    return 1
  fi
}

_check_job_input() {
  if [[ -z "${JOB_INPUT:-}" ]]; then
    echo "error: JOB_INPUT is not set (e.g. configs/source_grid_divergent_512x512.toml)" >&2
    return 1
  fi
}

_check_job_output() {
  if [[ -z "${JOB_OUTPUT:-}" ]]; then
    echo "error: JOB_OUTPUT is not set (e.g. output/gc_life.raw)" >&2
    return 1
  fi
}

_check_cuda_module() {
  if [[ -z "${CUDA_MODULE:-}" ]]; then
    echo "error: CUDA_MODULE is not set (e.g. cuda/12.1.0 -- run 'module spider cuda' to see options)" >&2
    return 1
  fi
  if [[ ! "$CUDA_MODULE" =~ ^cuda/ ]]; then
    echo "error: CUDA_MODULE=\"$CUDA_MODULE\" must be in cuda/X.Y.Z format (e.g. cuda/12.1.0)" >&2
    return 1
  fi
}
