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
BRANCH_PRESET="$BRANCH_ROOT/suite_presets/strong_gate.json"
OUTER_PRESET="$TOOLING_ROOT/suite_presets/strong_gate.json"
OUTDIR=""
LIMIT_SCALE="${2:-1.0}"
PRESET=""
PRESET_SOURCE=""
STAGE_FILTER="${LCA_STAGE_FILTER:-}"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
LOCKDIR="$LOCK_ROOT/lca_strong_gate"
LOCK_PID_FILE="$LOCKDIR/pid"
LEGACY_RUN_WORK_GLOB=".strong_gate_in_progress.*"
RUN_WORK_TEMPLATE="lca_strong_gate.run.XXXXXX"
OUTROOT=""
OUTPARENT=""
BACKUP_ROOT=""
FAILED_ROOT=""
WORKDIR=""
FILTERDIR=""
LOCK_HELD=0
HEARTBEAT_INTERVAL="${LCA_HEARTBEAT_INTERVAL:-25}"
STALE_LOCK_SECONDS="${LCA_STALE_LOCK_SECONDS:-60}"
CERTIFY_PID=""

fail() {
  echo "[lca_strong_gate] $*" >&2
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
  echo "usage: ./outer_suite_wrappers/lca_strong_gate.sh [artifact_subpath] [limit_scale]" >&2
  echo "[lca_strong_gate] stage filtering remains available via LCA_STAGE_FILTER" >&2
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

remove_path_retry() {
  local target="$1"
  local attempt
  for attempt in 1 2 3 4 5; do
    if [[ ! -e "$target" ]]; then
      return 0
    fi
    if [[ -d "$target" && ! -L "$target" ]]; then
      rm -rf "$target" 2>/dev/null || true
      python3 - "$target" <<'PY' >/dev/null 2>&1 || true
import os
import shutil
import sys

path = sys.argv[1]
try:
    if os.path.islink(path) or os.path.isfile(path):
        os.unlink(path)
    elif os.path.isdir(path):
        shutil.rmtree(path, ignore_errors=True)
except FileNotFoundError:
    pass
PY
    else
      rm -f "$target" 2>/dev/null || true
    fi
    if [[ ! -e "$target" ]]; then
      return 0
    fi
    sleep 0.1
  done
  return 1
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
  fail "missing strong gate preset: $BRANCH_PRESET or $OUTER_PRESET"
}

restore_previous_output() {
  if [[ -n "$BACKUP_ROOT" && -e "$BACKUP_ROOT" && ! -e "$OUTROOT" ]]; then
    mv "$BACKUP_ROOT" "$OUTROOT"
  fi
}

clear_invalid_root_path() {
  local target="$1"
  local label="$2"
  if [[ -e "$target" && ! -d "$target" ]]; then
    remove_path_retry "$target" || fail "failed to clear stale ${label}: $target"
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
  local age_seconds=""
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
      age_seconds="$(lock_age_seconds 2>/dev/null || printf '0\n')"
      if [[ "$age_seconds" =~ ^[0-9]+$ ]] && (( age_seconds >= STALE_LOCK_SECONDS )) && ! lock_has_recent_activity; then
        echo "[lca_strong_gate] clearing stale lock held by pid $holder after ${age_seconds}s without branch-local temp activity" >&2
        rm -rf "$LOCKDIR"
        continue
      fi
      fail "another lca_strong_gate.sh run is active (pid $holder)"
    fi

    rm -rf "$LOCKDIR"
  done
}

clear_stale_state() {
  local stale
  clear_invalid_root_path "$OUTROOT" "output root"
  clear_invalid_root_path "$BACKUP_ROOT" "backup root"
  clear_invalid_root_path "$FAILED_ROOT" "failure snapshot"
  restore_previous_output
  if [[ -d "$TMP_PARENT" ]]; then
    shopt -s nullglob
    for stale in "$TMP_PARENT"/lca_strong_gate.run.* "$TMP_PARENT"/lca_strong_gate.filter.*; do
      remove_path_retry "$stale" || fail "failed to clear stale temp path: $stale"
    done
    shopt -u nullglob
  fi
  if [[ -d "$OUTPARENT" ]]; then
    shopt -s nullglob
    for stale in "$OUTPARENT"/$LEGACY_RUN_WORK_GLOB; do
      remove_path_retry "$stale" || fail "failed to clear stale legacy work path: $stale"
    done
    shopt -u nullglob
  fi
  if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
    remove_path_retry "$BACKUP_ROOT" || fail "failed to clear stale backup path: $BACKUP_ROOT"
  fi
  if [[ -e "$FAILED_ROOT" ]]; then
    remove_path_retry "$FAILED_ROOT" || fail "failed to clear stale failure snapshot: $FAILED_ROOT"
  fi
}

count_completed_cases() {
  if [[ -f "$LOCK_PID_FILE" && -d "$TMP_PARENT/case_runs" ]]; then
    find "$TMP_PARENT/case_runs" -type f -name 'time.txt' -newer "$LOCK_PID_FILE" 2>/dev/null | wc -l | tr -d '[:space:]'
    return
  fi

  if [[ -z "${WORKDIR:-}" || ! -d "$WORKDIR/runs" ]]; then
    printf '0\n'
    return
  fi

  find "$WORKDIR/runs" -type f -name 'time.txt' 2>/dev/null | wc -l | tr -d '[:space:]'
}

lock_has_recent_activity() {
  local path
  if [[ ! -f "$LOCK_PID_FILE" ]]; then
    return 1
  fi
  for path in "$TMP_PARENT/case_runs" "$TMP_PARENT/case_cache_tmp"; do
    if [[ -d "$path" ]] && find "$path" -mindepth 1 -newer "$LOCK_PID_FILE" -print -quit 2>/dev/null | grep -q .; then
      return 0
    fi
  done
  return 1
}

lock_age_seconds() {
  python3 - "$LOCK_PID_FILE" <<'PY'
from __future__ import annotations

import sys
import time
from pathlib import Path

lock_path = Path(sys.argv[1])
print(max(0, int(time.time() - lock_path.stat().st_mtime)))
PY
}

run_certify_suite() {
  local start_ts now elapsed completed rc

  start_ts="$(date +%s)"
  echo "[lca_strong_gate] certify start preset=$PRESET_SOURCE out=$OUTROOT workdir=$WORKDIR" >&2

  BRANCH_CERTIFY_REPORT_OUTDIR="$OUTROOT" \
    python3 "$CERTIFY_HELPER" --solver "$SOLVER" --preset "$PRESET" --out "$WORKDIR" --limit-scale "$LIMIT_SCALE" &
  CERTIFY_PID=$!

  while kill -0 "$CERTIFY_PID" 2>/dev/null; do
    sleep "$HEARTBEAT_INTERVAL"
    if ! kill -0 "$CERTIFY_PID" 2>/dev/null; then
      break
    fi
    now="$(date +%s)"
    elapsed=$(( now - start_ts ))
    completed="$(count_completed_cases)"
    echo "[lca_strong_gate] heartbeat elapsed=${elapsed}s completed_cases=${completed} workdir=$WORKDIR" >&2
  done

  if wait "$CERTIFY_PID"; then
    rc=0
  else
    rc=$?
  fi
  CERTIFY_PID=""
  return "$rc"
}

strip_internal_state() {
  rm -rf "$WORKDIR/.case_cache" "$WORKDIR/.case_cache_tmp" "$WORKDIR/.case_runs_tmp" "$WORKDIR/.stage_filter"
}

publish_output() {
  local outleaf="${OUTROOT##*/}"

  if [[ ! -d "$WORKDIR" ]]; then
    fail "staging output directory disappeared before publish: $WORKDIR"
  fi

  strip_internal_state
  mkdir -p "$OUTPARENT"
  if [[ -e "$OUTROOT" ]]; then
    remove_path_retry "$BACKUP_ROOT" || fail "failed to clear backup path before publish: $BACKUP_ROOT"
    mv "$OUTROOT" "$BACKUP_ROOT"
  fi
  mv "$WORKDIR" "$OUTPARENT/$outleaf"
  WORKDIR=""
  remove_path_retry "$BACKUP_ROOT" || fail "failed to clear backup path after publish: $BACKUP_ROOT"
}

preserve_failed_output() {
  if [[ -z "${WORKDIR:-}" || ! -e "$WORKDIR" || -z "${FAILED_ROOT:-}" ]]; then
    return
  fi

  mkdir -p "$OUTPARENT"
  remove_path_retry "$FAILED_ROOT" || fail "failed to clear previous failure snapshot: $FAILED_ROOT"
  mv "$WORKDIR" "$FAILED_ROOT"
  WORKDIR=""
  echo "[lca_strong_gate] preserved failed staging output at $FAILED_ROOT" >&2
}

cleanup() {
  local rc="${1:-$?}"
  trap - EXIT
  set +e
  if [[ -n "${CERTIFY_PID:-}" ]] && kill -0 "$CERTIFY_PID" 2>/dev/null; then
    kill "$CERTIFY_PID" 2>/dev/null || true
    wait "$CERTIFY_PID" 2>/dev/null || true
  fi
  CERTIFY_PID=""
  if (( rc != 0 )); then
    preserve_failed_output
    restore_previous_output
  fi
  if [[ -n "${WORKDIR:-}" && -e "$WORKDIR" ]]; then
    remove_path_retry "$WORKDIR"
  fi
  if [[ -n "${FILTERDIR:-}" && -e "$FILTERDIR" ]]; then
    remove_path_retry "$FILTERDIR"
  fi
  if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
    remove_path_retry "$BACKUP_ROOT"
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

OUTDIR="$(python3 "$ARTIFACT_RESOLVER" lca_strong_gate "${1:-}")"
OUTROOT="$OUTDIR"
OUTPARENT="$(dirname "$OUTROOT")"
BACKUP_ROOT="${OUTROOT}.previous"
FAILED_ROOT="${OUTROOT}.latest_failure"
PRESET_SOURCE="$(resolve_preset)"
PRESET="$PRESET_SOURCE"
source "$RELEASE_ENV"

ensure_under_artifacts "$ARTIFACTS_ROOT"
ensure_under_artifacts "$TMP_PARENT"
ensure_under_artifacts "$LOCK_ROOT"
ensure_under_artifacts "$OUTROOT"
ensure_under_artifacts "$OUTPARENT"
ensure_under_artifacts "$BACKUP_ROOT"
ensure_under_artifacts "$FAILED_ROOT"
mkdir -p "$ARTIFACTS_ROOT"
acquire_lock
mkdir -p "$OUTPARENT"
clear_stale_state

if [[ -e "$OUTROOT" && ! -d "$OUTROOT" ]]; then
  fail "output path exists but is not a directory: $OUTROOT"
fi
if [[ -e "$BACKUP_ROOT" && ! -d "$BACKUP_ROOT" ]]; then
  fail "backup path exists but is not a directory: $BACKUP_ROOT"
fi

mkdir -p "$TMP_PARENT"
WORKDIR="$(mktemp -d "$TMP_PARENT/$RUN_WORK_TEMPLATE")"
ensure_under_artifacts "$WORKDIR"

if [[ -n "$STAGE_FILTER" ]]; then
  FILTERDIR="$(mktemp -d "$TMP_PARENT/lca_strong_gate.filter.XXXXXX")"
  ensure_under_artifacts "$FILTERDIR"
  PRESET="$FILTERDIR/preset.json"
  ensure_under_artifacts "$PRESET"
  python3 - "$PRESET_SOURCE" "$STAGE_FILTER" "$PRESET" <<'PY'
import json
import pathlib
import sys

src = pathlib.Path(sys.argv[1])
stage_name = sys.argv[2]
dst = pathlib.Path(sys.argv[3])

with src.open(encoding="utf-8") as f:
    preset = json.load(f)

stages = [stage for stage in preset.get("stages", []) if stage.get("name") == stage_name]
if not stages:
    raise SystemExit(f"[lca_strong_gate] unknown stage filter: {stage_name}")

preset["stages"] = stages
with dst.open("w", encoding="utf-8") as f:
    json.dump(preset, f, indent=2)
    f.write("\n")
PY
fi

if [[ ! -x "$SOLVER" || "$SOURCE" -nt "$SOLVER" ]]; then
  "$BUILD_WRAPPER"
fi
require_executable "$SOLVER" "solver wrapper"

run_certify_suite
publish_output
