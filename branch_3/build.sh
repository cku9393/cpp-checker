#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

# Keep release builds reproducible by clearing ambient compiler/profile knobs
# before the branch-local runtime envelope rehydrates its artifact-rooted env.
unset CXX || true
unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true
export PROFILE_MODE="${PROFILE_MODE:-PROFILE_BASE}"
export BRANCH_NON_ARTIFACT_BYTECODE_PURGED=1

source "$SCRIPT_DIR/solver_release_env.sh"

unset CXX || true
unset ENABLE_STATE_LOAD_MATERIALIZATION_OPT || true
export ENABLE_STATE_LOAD_MATERIALIZATION_OPT="${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-0}"
export PROFILE_MODE="${PROFILE_MODE:-PROFILE_BASE}"
export BRANCH_NON_ARTIFACT_BYTECODE_PURGED=1

BUILD_WRAPPER_CWD="$(python3 "$SCRIPT_DIR/artifact_paths.py" boj28350_build)"
mkdir -p "$BUILD_WRAPPER_CWD"
cd "$BUILD_WRAPPER_CWD"

if [[ -f "$SCRIPT_DIR/build.py" ]]; then
  exec python3 "$SCRIPT_DIR/build.py" "$@"
fi

exec python3 "$SCRIPT_DIR/boj28350_resume.py" build "$@"
