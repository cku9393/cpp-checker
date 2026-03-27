#!/usr/bin/env bash
__dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
if [[ -z "${BRANCH_ARTIFACT_TMP_ROOT:-}" ]]; then
  export BRANCH_ARTIFACT_TMP_ROOT="$__dir/artifacts/lca_tree_stress_v5/.tmp/release_env"
fi
mkdir -p "$BRANCH_ARTIFACT_TMP_ROOT"
export TMPDIR="$BRANCH_ARTIFACT_TMP_ROOT"
export TMP="$BRANCH_ARTIFACT_TMP_ROOT"
export TEMP="$BRANCH_ARTIFACT_TMP_ROOT"
if [[ -z "${HOME:-}" ]]; then export HOME="$BRANCH_ARTIFACT_TMP_ROOT/home"; fi
if [[ -z "${XDG_CONFIG_HOME:-}" ]]; then export XDG_CONFIG_HOME="$BRANCH_ARTIFACT_TMP_ROOT/xdg_config"; fi
if [[ -z "${XDG_CACHE_HOME:-}" ]]; then export XDG_CACHE_HOME="$BRANCH_ARTIFACT_TMP_ROOT/xdg_cache"; fi
if [[ -z "${XDG_STATE_HOME:-}" ]]; then export XDG_STATE_HOME="$BRANCH_ARTIFACT_TMP_ROOT/xdg_state"; fi
if [[ -z "${PYTHONPYCACHEPREFIX:-}" ]]; then export PYTHONPYCACHEPREFIX="$BRANCH_ARTIFACT_TMP_ROOT/pycache"; fi
mkdir -p "$HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_STATE_HOME" "$PYTHONPYCACHEPREFIX"
