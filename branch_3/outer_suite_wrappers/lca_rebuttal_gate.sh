#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"
cd "$BRANCH_ROOT"
OUTER_ROOT="$(cd "$BRANCH_ROOT/.." && pwd -P)"
TOOLING_ROOT="$OUTER_ROOT/lca_tree_stress_v5/tooling"
export PYTHONDONTWRITEBYTECODE=1
SOLVER="$BRANCH_ROOT/boj28350_resume/solve"
SOURCE="$BRANCH_ROOT/boj28350_resume/boj28350_branch_3_solver.cpp"
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
BUILD_WRAPPER="$BRANCH_ROOT/build.sh"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
CERTIFY_HELPER="$BRANCH_ROOT/branch_certify_suite.py"
BRANCH_PRESET="$BRANCH_ROOT/suite_presets/rebuttal_gate.json"
OUTER_PRESET="$TOOLING_ROOT/suite_presets/rebuttal_gate.json"
LIMIT_SCALE="${2:-1.0}"
PRESET_SOURCE=""
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
LOCKDIR="$LOCK_ROOT/lca_rebuttal_gate"
LOCK_PID_FILE="$LOCKDIR/pid"
RUN_WORK_GLOB=".rebuttal_gate_in_progress.*"
RUN_WORK_TEMPLATE="lca_rebuttal_gate.run.XXXXXX"
OUTROOT=""
OUTPARENT=""
BACKUP_ROOT=""
WORKDIR=""
LOCK_HELD=0

fail() {
  echo "[lca_rebuttal_gate] $*" >&2
  exit 1
}

ensure_under_artifacts() {
  local path="$1"
  case "$path" in
    "$ARTIFACTS_ROOT"|"$ARTIFACTS_ROOT"/*)
      ;;
    *)
      fail "path escaped branch-local artifacts root: $path"
      ;;
  esac
}

usage() {
  echo "usage: ./outer_suite_wrappers/lca_rebuttal_gate.sh [artifact_subpath] [limit_scale]" >&2
  exit 2
}

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    fail "missing required tool: $1"
  fi
}

require_file() {
  local path="$1"
  local label="$2"
  if [[ ! -f "$path" ]]; then
    fail "missing ${label}: $path"
  fi
}

require_executable() {
  local path="$1"
  local label="$2"
  if [[ ! -x "$path" ]]; then
    fail "missing executable ${label}: $path"
  fi
}

resolve_preset() {
  if [[ -f "$BRANCH_PRESET" ]]; then
    printf '%s\n' "$BRANCH_PRESET"
    return
  fi
  if [[ -f "$OUTER_PRESET" ]]; then
    printf '%s\n' "$OUTER_PRESET"
    return
  fi
  fail "missing rebuttal gate preset: $BRANCH_PRESET or $OUTER_PRESET"
}

restore_previous_output() {
  if [[ -n "$BACKUP_ROOT" && -e "$BACKUP_ROOT" && ! -e "$OUTROOT" ]]; then
    mv "$BACKUP_ROOT" "$OUTROOT"
  fi
}

release_lock() {
  if (( LOCK_HELD )) && [[ -d "$LOCKDIR" ]]; then
    rm -rf "$LOCKDIR"
  fi
  LOCK_HELD=0
  rmdir "$LOCK_ROOT" 2>/dev/null || true
}

acquire_lock() {
  local holder=""
  mkdir -p "$LOCK_ROOT"
  while true; do
    if mkdir "$LOCKDIR" 2>/dev/null; then
      printf '%s\n' "$$" > "$LOCK_PID_FILE"
      LOCK_HELD=1
      return
    fi

    if [[ ! -f "$LOCK_PID_FILE" ]]; then
      sleep 0.05
      if [[ ! -f "$LOCK_PID_FILE" ]]; then
        rm -rf "$LOCKDIR"
      fi
      continue
    fi

    read -r holder < "$LOCK_PID_FILE" || holder=""
    if [[ -z "$holder" ]]; then
      sleep 0.05
      if [[ -f "$LOCK_PID_FILE" ]]; then
        continue
      fi
      rm -rf "$LOCKDIR"
      continue
    fi

    if kill -0 "$holder" 2>/dev/null; then
      fail "another lca_rebuttal_gate.sh run is active (pid $holder)"
    fi

    rm -rf "$LOCKDIR"
  done
}

clear_stale_state() {
  local stale
  restore_previous_output
  if [[ -d "$TMP_PARENT" ]]; then
    shopt -s nullglob
    for stale in "$TMP_PARENT"/lca_rebuttal_gate.run.*; do
      rm -rf "$stale"
    done
    shopt -u nullglob
  fi
  if [[ -d "$OUTPARENT" ]]; then
    shopt -s nullglob
    for stale in "$OUTPARENT"/$RUN_WORK_GLOB; do
      rm -rf "$stale"
    done
    shopt -u nullglob
  fi
  if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
    rm -rf "$BACKUP_ROOT"
  fi
}

publish_output() {
  local outleaf="${OUTROOT##*/}"

  if [[ ! -d "$WORKDIR" ]]; then
    fail "staging output directory disappeared before publish: $WORKDIR"
  fi

  mkdir -p "$OUTPARENT"
  if [[ -e "$OUTROOT" ]]; then
    rm -rf "$BACKUP_ROOT"
    mv "$OUTROOT" "$BACKUP_ROOT"
  fi
  mv "$WORKDIR" "$OUTPARENT/$outleaf"
  WORKDIR=""
  rm -rf "$BACKUP_ROOT"
}

cleanup() {
  local rc="${1:-$?}"
  trap - EXIT
  set +e
  if [[ -n "${WORKDIR:-}" && -e "$WORKDIR" ]]; then
    rm -rf "$WORKDIR"
  fi
  if (( rc != 0 )); then
    restore_previous_output
  fi
  if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
    rm -rf "$BACKUP_ROOT"
  fi
  release_lock
  rmdir "$TMP_PARENT" 2>/dev/null || true
  exit "$rc"
}

trap 'cleanup "$?"' EXIT

if (( $# > 2 )); then
  usage
fi

require_command python3
require_command mktemp
require_command dirname
require_file "$SOURCE" "solver source"
require_file "$ARTIFACT_RESOLVER" "artifact resolver"
require_file "$RELEASE_ENV" "release env wrapper"
require_file "$CERTIFY_HELPER" "branch-local certify helper"
require_executable "$BUILD_WRAPPER" "build wrapper"

OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_rebuttal_gate "${1:-}")"
OUTPARENT="$(dirname "$OUTROOT")"
BACKUP_ROOT="${OUTROOT}.previous"
PRESET_SOURCE="$(resolve_preset)"
source "$RELEASE_ENV"

ensure_under_artifacts "$ARTIFACTS_ROOT"
ensure_under_artifacts "$TMP_PARENT"
ensure_under_artifacts "$LOCK_ROOT"
ensure_under_artifacts "$OUTROOT"
ensure_under_artifacts "$OUTPARENT"
ensure_under_artifacts "$BACKUP_ROOT"
mkdir -p "$ARTIFACTS_ROOT"
acquire_lock
mkdir -p "$OUTPARENT"
clear_stale_state
mkdir -p "$TMP_PARENT"
WORKDIR="$(mktemp -d "$TMP_PARENT/$RUN_WORK_TEMPLATE")"
ensure_under_artifacts "$WORKDIR"

if [[ ! -x "$SOLVER" || "$SOURCE" -nt "$SOLVER" ]]; then
  "$BUILD_WRAPPER"
fi

BRANCH_CERTIFY_REPORT_OUTDIR="$OUTROOT" \
  python3 "$CERTIFY_HELPER" --solver "$SOLVER" --preset "$PRESET_SOURCE" --out "$WORKDIR" --limit-scale "$LIMIT_SCALE"
publish_output
