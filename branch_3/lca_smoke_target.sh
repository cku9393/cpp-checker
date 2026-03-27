#!/usr/bin/env bash
set -euo pipefail

SMOKE_TARGET_EXIT_HARNESS_FAILURE=70
SCRIPT_SOURCE="${BASH_SOURCE[0]}"
SCRIPT_SOURCE_DIR="."
case "$SCRIPT_SOURCE" in
  */*)
    SCRIPT_SOURCE_DIR="${SCRIPT_SOURCE%/*}"
    ;;
esac
SCRIPT_DIR="$(
  unset CDPATH
  cd -- "$SCRIPT_SOURCE_DIR"
  pwd -P
)"
BRANCH_ROOT="$SCRIPT_DIR"
INNER_WRAPPER="$BRANCH_ROOT/outer_suite_wrappers/lca_smoke_target.sh"
BASH_BIN="${BASH:-}"

fail() {
  echo "[lca_smoke_target] $*" >&2
  exit "$SMOKE_TARGET_EXIT_HARNESS_FAILURE"
}

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    fail "missing required tool: $1"
  fi
}

require_file() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    fail "missing ${label}: $path"
  fi
  if [[ ! -f "$path" ]]; then
    fail "${label} is not a regular file: $path"
  fi
  if [[ ! -r "$path" ]]; then
    fail "${label} is not readable: $path"
  fi
}

resolve_bash_bin() {
  if [[ -n "$BASH_BIN" && -x "$BASH_BIN" ]]; then
    return
  fi
  BASH_BIN="$(command -v bash 2>/dev/null || true)"
  if [[ -z "$BASH_BIN" || ! -x "$BASH_BIN" ]]; then
    fail "unable to locate an executable bash interpreter"
  fi
}

require_command bash
require_file "$INNER_WRAPPER" "outer smoke target wrapper"
require_file "$BRANCH_ROOT/build.sh" "build wrapper"
require_file "$BRANCH_ROOT/solver_release_env.sh" "release env wrapper"
require_file "$BRANCH_ROOT/artifact_paths.py" "artifact resolver"
require_file "$BRANCH_ROOT/branch_run_case.py" "branch-local case helper"
require_file "$BRANCH_ROOT/boj28350_resume/smoke_cases.tsv" "smoke case manifest"
resolve_bash_bin

cd "$BRANCH_ROOT"
exec "$BASH_BIN" "$INNER_WRAPPER" "$@"
