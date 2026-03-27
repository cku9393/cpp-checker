#!/usr/bin/env bash
set -euo pipefail

SMOKE_EXIT_HARNESS_FAILURE=70
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
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
INNER_WRAPPER="$BRANCH_ROOT/outer_suite_wrappers/lca_smoke.sh"
BRANCH_ARTIFACTS_ROOT=""
ARTIFACTS_ROOT=""
TMP_PARENT=""
LOCK_ROOT=""
BASH_BIN="${BASH:-}"

fail() {
  echo "[lca_smoke] $*" >&2
  exit "$SMOKE_EXIT_HARNESS_FAILURE"
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

resolve_branch_local_roots() {
  BRANCH_ARTIFACTS_ROOT="$(python3 "$ARTIFACT_RESOLVER" --artifacts-root)"
  if [[ -z "$BRANCH_ARTIFACTS_ROOT" ]]; then
    fail "artifact resolver returned an empty branch artifacts root"
  fi

  case "$BRANCH_ARTIFACTS_ROOT" in
    "$BRANCH_ROOT"/artifacts)
      ;;
    *)
      fail "artifact resolver escaped repo-relative artifacts root: $BRANCH_ARTIFACTS_ROOT"
      ;;
  esac

  ARTIFACTS_ROOT="$BRANCH_ARTIFACTS_ROOT/lca_tree_stress_v5"
  TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
  LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
}

prepare_workdirs() {
  mkdir -p "$ARTIFACTS_ROOT" "$TMP_PARENT" "$LOCK_ROOT"
}

require_command bash
require_command python3
require_command mkdir
require_file "$INNER_WRAPPER" "outer smoke wrapper"
require_file "$BRANCH_ROOT/build.sh" "build wrapper"
require_file "$BRANCH_ROOT/solver_release_env.sh" "release env wrapper"
require_file "$ARTIFACT_RESOLVER" "artifact resolver"
require_file "$BRANCH_ROOT/boj28350_resume/boj28350_branch_3_solver.cpp" "solver source"
require_file "$BRANCH_ROOT/boj28350_resume/smoke_cases.tsv" "smoke case manifest"
resolve_bash_bin
resolve_branch_local_roots
prepare_workdirs

cd "$BRANCH_ROOT"
exec "$BASH_BIN" "$INNER_WRAPPER" "$@"
