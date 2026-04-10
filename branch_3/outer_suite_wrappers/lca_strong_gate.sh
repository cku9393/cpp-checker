#!/usr/bin/env bash
set -euo pipefail
trap '' HUP

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"
cd "$BRANCH_ROOT"
OUTER_ROOT="$(cd "$BRANCH_ROOT/.." && pwd -P)"
TOOLING_ROOT="$OUTER_ROOT/lca_tree_stress_v5/tooling"
export PYTHONDONTWRITEBYTECODE=1
BINARY="$BRANCH_ROOT/artifacts/boj28350_resume/build/solve"
SOLVER="$BINARY"
SOURCE="$BRANCH_ROOT/boj28350_resume/boj28350_branch_3_solver.cpp"
SOLVER_BUILD_METADATA="${BINARY}.build_meta.json"
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
BUILD_WRAPPER="$BRANCH_ROOT/build.sh"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
CERTIFY_HELPER="$BRANCH_ROOT/branch_certify_suite.py"
TOOLING_CERTIFY_SOURCE="$TOOLING_ROOT/certify_suite.py"
BRANCH_PRESET="$BRANCH_ROOT/suite_presets/strong_gate.json"
OUTER_PRESET="$TOOLING_ROOT/suite_presets/strong_gate.json"
OUTDIR=""
LIMIT_SCALE="${2:-1.0}"
PRESET=""
PRESET_SOURCE=""
PRESET_SOURCE_MATERIALIZED=""
STAGE_FILTER="${LCA_STAGE_FILTER:-}"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
SNAPSHOT_ROOT="$ARTIFACTS_ROOT/.solver_snapshots/lca_strong_gate"
PRESET_CACHE_ROOT="$ARTIFACTS_ROOT/.preset_cache"
PRESET_CACHE_PATH="$PRESET_CACHE_ROOT/lca_strong_gate.json"
LOCKDIR="$LOCK_ROOT/lca_strong_gate"
LOCK_PID_FILE="$LOCKDIR/pid"
LEGACY_RUN_WORK_GLOB=".strong_gate_in_progress.*"
RUN_WORK_TEMPLATE="lca_strong_gate.run.XXXXXX"
RUN_TMP_TEMPLATE="lca_strong_gate.env.XXXXXX"
OUTROOT=""
OUTPARENT=""
BACKUP_ROOT=""
FAILED_ROOT=""
FAILED_ARCHIVE_ROOT=""
WORKDIR=""
FILTERDIR=""
RUN_TMPDIR=""
CASE_RUN_TMP_ROOT=""
CASE_CACHE_ROOT=""
CASE_CACHE_TMP_ROOT=""
LOCK_HELD=0
HEARTBEAT_INTERVAL="${LCA_HEARTBEAT_INTERVAL:-25}"
STALE_LOCK_SECONDS="${LCA_STALE_LOCK_SECONDS:-60}"
CERTIFY_PID=""
SOLVER_SNAPSHOT=""
PRECHECK_MANIFEST=""
ENV_SNAPSHOT=""
BUILD_STDOUT_LOG=""
BUILD_STDERR_LOG=""
CERTIFY_STDOUT_LOG=""
CERTIFY_STDERR_LOG=""
FAILURE_SUMMARY_PATH=""
FAILURE_REPORT_PATH=""
PRESET_SNAPSHOT_PATH=""
BUILD_METADATA_SNAPSHOT=""
NON_ARTIFACT_BASELINE=""
NON_ARTIFACT_CURRENT=""
NON_ARTIFACT_REPORT=""
REPEATABILITY_MANIFEST_PATH=""

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

path_has_dataless_flag() {
  local path="$1"
  local flags=""
  if [[ -z "$path" || ! -e "$path" ]]; then
    return 1
  fi
  if ! flags="$(stat -f '%Sf' "$path" 2>/dev/null)"; then
    return 1
  fi
  [[ "$flags" == *dataless* ]]
}

path_is_readable_now() {
  local path="$1"
  python3 - "$path" <<'PY' >/dev/null 2>&1
from __future__ import annotations

import sys
from pathlib import Path

with Path(sys.argv[1]).open("rb") as f:
    f.read(1)
PY
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

artifact_preset_snapshot_is_full_gate() {
  local snapshot_path="$1"
  local runtime_env=""

  if [[ -z "$snapshot_path" || ! -f "$snapshot_path" ]]; then
    return 1
  fi
  if path_has_dataless_flag "$snapshot_path"; then
    return 1
  fi

  runtime_env="$(dirname "$snapshot_path")/runtime_env.txt"
  if [[ ! -f "$runtime_env" ]]; then
    return 1
  fi
  if path_has_dataless_flag "$runtime_env"; then
    return 1
  fi

  python3 - "$snapshot_path" "$runtime_env" <<'PY' >/dev/null 2>&1
from __future__ import annotations

import json
import sys
from pathlib import Path

snapshot_path = Path(sys.argv[1])
runtime_env_path = Path(sys.argv[2])
payload = json.loads(snapshot_path.read_text(encoding="utf-8"))
stages = payload.get("stages")
if not isinstance(stages, list) or not stages:
    raise SystemExit(1)

stage_filter = None
for line in runtime_env_path.read_text(encoding="utf-8").splitlines():
    if line.startswith("stage_filter="):
        stage_filter = line.split("=", 1)[1]
        break

# Artifact fallback must stay on a full-gate preset snapshot. Reusing an older
# stage-filtered probe snapshot would silently shrink the rerun surface and
# make repeated direct strong-gate invocations non-comparable.
if stage_filter is None or stage_filter:
    raise SystemExit(1)
PY
}

resolve_preset() {
  local candidate=""
  local preset_search_roots=()

  if [[ -f "$BRANCH_PRESET" ]] && ! path_has_dataless_flag "$BRANCH_PRESET"; then
    printf '%s\n' "$BRANCH_PRESET"
    return 0
  fi
  if [[ -f "$OUTER_PRESET" ]] && ! path_has_dataless_flag "$OUTER_PRESET"; then
    printf '%s\n' "$OUTER_PRESET"
    return 0
  fi
  if [[ -f "$PRESET_CACHE_PATH" ]] && ! path_has_dataless_flag "$PRESET_CACHE_PATH"; then
    printf '%s\n' "$PRESET_CACHE_PATH"
    return 0
  fi
  if [[ -d "$ARTIFACTS_ROOT/strong_gate" ]]; then
    preset_search_roots+=("$ARTIFACTS_ROOT/strong_gate")
  fi
  if [[ -d "$ARTIFACTS_ROOT/strong_gate.failure_archive" ]]; then
    preset_search_roots+=("$ARTIFACTS_ROOT/strong_gate.failure_archive")
  fi
  if (( ${#preset_search_roots[@]} > 0 )); then
    candidate="$(
      find "${preset_search_roots[@]}" \
        -type f -name 'selected_preset.json' \
        -print0 2>/dev/null |
        while IFS= read -r -d '' path; do
          [[ -n "$path" && -f "$path" ]] || continue
          printf '%s\t%s\n' "$(stat -f '%m' "$path" 2>/dev/null || printf '0')" "$path"
        done |
        sort -rn |
        cut -f2- |
        while IFS= read -r path; do
          [[ -n "$path" && -f "$path" ]] || continue
          if artifact_preset_snapshot_is_full_gate "$path"; then
            printf '%s\n' "$path"
            break
          fi
        done
    )"
  fi
  if [[ -n "$candidate" ]]; then
    printf '%s\n' "$candidate"
    return 0
  fi
  if [[ -f "$BRANCH_PRESET" ]]; then
    printf '%s\n' "$BRANCH_PRESET"
    return 0
  fi
  if [[ -f "$OUTER_PRESET" ]]; then
    printf '%s\n' "$OUTER_PRESET"
    return 0
  fi
  if [[ -f "$PRESET_CACHE_PATH" ]]; then
    printf '%s\n' "$PRESET_CACHE_PATH"
    return 0
  fi
  return 1
}

resolve_cached_preset_snapshot() {
  local candidate=""
  candidate="$(
    find "$ARTIFACTS_ROOT/strong_gate" "$ARTIFACTS_ROOT/strong_gate.failure_archive" \
      -type f -name 'selected_preset.json' \
      -print0 2>/dev/null |
      while IFS= read -r -d '' path; do
        [[ -n "$path" && -f "$path" ]] || continue
        printf '%s\t%s\n' "$(stat -f '%m' "$path" 2>/dev/null || printf '0')" "$path"
      done |
      sort -rn |
      cut -f2- |
      while IFS= read -r path; do
        [[ -n "$path" && -f "$path" ]] || continue
        if artifact_preset_snapshot_is_full_gate "$path"; then
          printf '%s\n' "$path"
          break
        fi
      done
  )"
  if [[ -n "$candidate" ]]; then
    printf '%s\n' "$candidate"
    return 0
  fi
  return 1
}

refresh_preset_cache_if_possible() {
  local candidate=""

  mkdir -p "$PRESET_CACHE_ROOT"
  ensure_under_artifacts "$PRESET_CACHE_ROOT"
  ensure_under_artifacts "$PRESET_CACHE_PATH"

  for candidate in "$BRANCH_PRESET" "$OUTER_PRESET"; do
    [[ -f "$candidate" ]] || continue
    if path_has_dataless_flag "$candidate"; then
      continue
    fi
    if snapshot_json_file "$candidate" "$PRESET_CACHE_PATH" >/dev/null 2>&1; then
      return 0
    fi
  done
  return 1
}

restore_previous_output() {
  if [[ -n "$BACKUP_ROOT" && -e "$BACKUP_ROOT" && ! -e "$OUTROOT" ]]; then
    if published_output_is_complete "$BACKUP_ROOT"; then
      mv "$BACKUP_ROOT" "$OUTROOT"
    else
      echo "[lca_strong_gate] skipping restore of incomplete previous published output: $BACKUP_ROOT" >&2
      remove_path_retry "$BACKUP_ROOT" || fail "failed to clear incomplete previous published output: $BACKUP_ROOT"
    fi
  fi
}

archive_previous_failure_output() {
  local archive_root=""
  local archive_path=""
  local archive_leaf=""
  local stamp=""
  local suffix=0

  if [[ -z "$FAILED_ROOT" || ! -d "$FAILED_ROOT" ]]; then
    return
  fi

  archive_root="$FAILED_ARCHIVE_ROOT"
  mkdir -p "$archive_root"
  stamp="$(date '+%Y%m%d_%H%M%S')"
  archive_leaf="$(basename "$FAILED_ROOT").$stamp"
  archive_path="$archive_root/$archive_leaf"
  while [[ -e "$archive_path" ]]; do
    suffix=$((suffix + 1))
    archive_path="$archive_root/${archive_leaf}_$suffix"
  done
  mv "$FAILED_ROOT" "$archive_path"
  echo "[lca_strong_gate] archived previous failure snapshot at $archive_path" >&2
}

published_output_is_complete() {
  local root="$1"
  local required_path=""

  [[ -n "$root" && -d "$root" ]] || return 1
  for required_path in \
    "$root/certify.json" \
    "$root/certify_summary.md" \
    "$root/runtime_env.txt" \
    "$root/preflight_manifest.tsv"; do
    [[ -s "$required_path" ]] || return 1
    if path_has_dataless_flag "$required_path" && ! path_is_readable_now "$required_path"; then
      return 1
    fi
  done
  return 0
}

archive_incomplete_published_output() {
  local archive_root=""
  local archive_path=""
  local archive_leaf=""
  local stamp=""
  local suffix=0

  if [[ -z "$OUTROOT" || ! -d "$OUTROOT" ]]; then
    return
  fi
  if published_output_is_complete "$OUTROOT"; then
    return
  fi

  archive_root="$FAILED_ARCHIVE_ROOT"
  mkdir -p "$archive_root"
  stamp="$(date '+%Y%m%d_%H%M%S')"
  archive_leaf="$(basename "$OUTROOT").incomplete_published.$stamp"
  archive_path="$archive_root/$archive_leaf"
  while [[ -e "$archive_path" ]]; do
    suffix=$((suffix + 1))
    archive_path="$archive_root/${archive_leaf}_$suffix"
  done
  mv "$OUTROOT" "$archive_path"
  echo "[lca_strong_gate] archived incomplete published output at $archive_path" >&2
}

clear_invalid_root_path() {
  local target="$1"
  local label="$2"
  if [[ -e "$target" && ! -d "$target" ]]; then
    remove_path_retry "$target" || fail "failed to clear stale ${label}: $target"
  fi
}

clear_solver_snapshot_artifacts() {
  local stale=""
  local snapshot_parent=""

  clear_invalid_root_path "$SNAPSHOT_ROOT" "solver snapshot root"
  if [[ -d "$SNAPSHOT_ROOT" ]]; then
    shopt -s nullglob
    for stale in "$SNAPSHOT_ROOT"/lca_strong_gate.solver.*; do
      remove_path_retry "$stale" || fail "failed to clear stale solver snapshot: $stale"
    done
    shopt -u nullglob
  fi

  snapshot_parent="$(dirname "$SNAPSHOT_ROOT")"
  ensure_under_artifacts "$snapshot_parent"
  if [[ -d "$snapshot_parent" ]]; then
    shopt -s nullglob
    for stale in "$snapshot_parent"/lca_strong_gate.solver.*; do
      remove_path_retry "$stale" || fail "failed to clear stale legacy solver snapshot: $stale"
    done
    shopt -u nullglob
  fi
}

prune_empty_solver_snapshot_roots() {
  local snapshot_parent=""

  clear_solver_snapshot_artifacts
  rmdir "$SNAPSHOT_ROOT" 2>/dev/null || true
  snapshot_parent="$(dirname "$SNAPSHOT_ROOT")"
  ensure_under_artifacts "$snapshot_parent"
  rmdir "$snapshot_parent" 2>/dev/null || true
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
  clear_invalid_root_path "$FAILED_ARCHIVE_ROOT" "failure archive root"
  clear_solver_snapshot_artifacts
  archive_previous_failure_output
  archive_incomplete_published_output
  restore_previous_output
  if [[ -d "$TMP_PARENT" ]]; then
    shopt -s nullglob
    for stale in "$TMP_PARENT"/lca_strong_gate.run.* "$TMP_PARENT"/lca_strong_gate.filter.* "$TMP_PARENT"/lca_strong_gate.env.*; do
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
  if [[ -e "$FAILED_ROOT" && ! -d "$FAILED_ROOT" ]]; then
    remove_path_retry "$FAILED_ROOT" || fail "failed to clear stale failure snapshot: $FAILED_ROOT"
  fi
}

configure_runtime_tmpdir() {
  RUN_TMPDIR="$(mktemp -d "$TMP_PARENT/$RUN_TMP_TEMPLATE")"
  if [[ -z "$RUN_TMPDIR" ]]; then
    fail "mktemp returned an empty strong gate runtime tmpdir"
  fi
  ensure_under_artifacts "$RUN_TMPDIR"
}

sanitize_solver_environment() {
  local env_var_name=""
  unset CC CXX CPPFLAGS CFLAGS CXXFLAGS LDFLAGS SDKROOT MACOSX_DEPLOYMENT_TARGET LOCAL_SKIP_SELF_TEST
  while IFS='=' read -r env_var_name _; do
    case "$env_var_name" in
      ENABLE_*|PROFILE_*|DENSE_*|RUN_TAG)
        unset "$env_var_name"
        ;;
    esac
  done < <(env)
}

apply_strong_gate_release_profile_overrides() {
  # AC3 correctness-fuzz currently fails in the 2s corridor on dense rect-family
  # cases even though the same solver validates them. Keep the branch-wide BOJ
  # release defaults intact, but let the strong-gate wrapper re-enable the
  # progress40 materialization path that improves this prerequisite gate.
  export ENABLE_STATE_LOAD_MATERIALIZATION_OPT=1
}

assert_runtime_environment() {
  local expected_home="$RUN_TMPDIR/home"
  local expected_config="$RUN_TMPDIR/xdg_config"
  local expected_cache="$RUN_TMPDIR/xdg_cache"
  local expected_state="$RUN_TMPDIR/xdg_state"
  local expected_pycache="$RUN_TMPDIR/pycache"
  local env_path=""

  if [[ -z "${RUN_TMPDIR:-}" || ! -d "$RUN_TMPDIR" ]]; then
    fail "strong gate runtime tmpdir is missing after release environment setup"
  fi

  for env_path in \
    "$BRANCH_ARTIFACT_TMP_ROOT" \
    "$TMPDIR" \
    "$TMP" \
    "$TEMP" \
    "$HOME" \
    "$XDG_CONFIG_HOME" \
    "$XDG_CACHE_HOME" \
    "$XDG_STATE_HOME" \
    "$PYTHONPYCACHEPREFIX"; do
    ensure_under_artifacts "$env_path"
  done

  [[ "$BRANCH_ARTIFACT_TMP_ROOT" == "$RUN_TMPDIR" ]] || fail "BRANCH_ARTIFACT_TMP_ROOT drifted from strong gate runtime tmpdir"
  [[ "$TMPDIR" == "$RUN_TMPDIR" ]] || fail "TMPDIR drifted from strong gate runtime tmpdir"
  [[ "$TMP" == "$RUN_TMPDIR" ]] || fail "TMP drifted from strong gate runtime tmpdir"
  [[ "$TEMP" == "$RUN_TMPDIR" ]] || fail "TEMP drifted from strong gate runtime tmpdir"
  [[ "$HOME" == "$expected_home" ]] || fail "HOME drifted from strong gate runtime home"
  [[ "$XDG_CONFIG_HOME" == "$expected_config" ]] || fail "XDG_CONFIG_HOME drifted from strong gate runtime config root"
  [[ "$XDG_CACHE_HOME" == "$expected_cache" ]] || fail "XDG_CACHE_HOME drifted from strong gate runtime cache root"
  [[ "$XDG_STATE_HOME" == "$expected_state" ]] || fail "XDG_STATE_HOME drifted from strong gate runtime state root"
  [[ "$PYTHONPYCACHEPREFIX" == "$expected_pycache" ]] || fail "PYTHONPYCACHEPREFIX drifted from strong gate runtime pycache root"
}

load_release_environment() {
  if [[ -z "${RUN_TMPDIR:-}" || ! -d "$RUN_TMPDIR" ]]; then
    fail "runtime tmpdir is unset before loading the strong gate release environment"
  fi

  export HOME="$RUN_TMPDIR/home"
  export XDG_CONFIG_HOME="$RUN_TMPDIR/xdg_config"
  export XDG_CACHE_HOME="$RUN_TMPDIR/xdg_cache"
  export XDG_STATE_HOME="$RUN_TMPDIR/xdg_state"
  export PYTHONPYCACHEPREFIX="$RUN_TMPDIR/pycache"
  mkdir -p "$HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_STATE_HOME" "$PYTHONPYCACHEPREFIX"
  export BRANCH_ARTIFACT_TMP_ROOT="$RUN_TMPDIR"
  export TMPDIR="$RUN_TMPDIR"
  export TMP="$RUN_TMPDIR"
  export TEMP="$RUN_TMPDIR"
  sanitize_solver_environment
  apply_strong_gate_release_profile_overrides
  source "$RELEASE_ENV"
  mkdir -p "$RUN_TMPDIR" "$HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_STATE_HOME" "$PYTHONPYCACHEPREFIX"
  export BRANCH_ARTIFACT_TMP_ROOT="$RUN_TMPDIR"
  export TMPDIR="$RUN_TMPDIR"
  export TMP="$RUN_TMPDIR"
  export TEMP="$RUN_TMPDIR"
  assert_runtime_environment
}

count_completed_cases() {
  local published_count=0
  local active_count=0

  if [[ -n "${WORKDIR:-}" && -d "$WORKDIR/runs" ]]; then
    published_count="$(
      find "$WORKDIR/runs" -type f \( -name 'run_case_result.json' -o -name 'time.txt' \) 2>/dev/null |
        sed 's#/[^/]*$##' |
        sort -u |
        wc -l |
        tr -d '[:space:]'
    )"
  fi

  if [[ -f "$LOCK_PID_FILE" && -n "${CASE_RUN_TMP_ROOT:-}" && -d "$CASE_RUN_TMP_ROOT" ]]; then
    active_count="$(
      find "$CASE_RUN_TMP_ROOT" -type f \( -name 'run_case_result.json' -o -name 'time.txt' \) -newer "$LOCK_PID_FILE" 2>/dev/null |
        sed 's#/[^/]*$##' |
        sort -u |
        wc -l |
        tr -d '[:space:]'
    )"
  fi

  printf '%s\n' "$(( published_count + active_count ))"
}

lock_has_recent_activity() {
  local path
  if [[ ! -f "$LOCK_PID_FILE" ]]; then
    return 1
  fi

  for path in "${CASE_RUN_TMP_ROOT:-}" "${CASE_CACHE_TMP_ROOT:-}"; do
    if [[ -d "$path" ]] && find "$path" -mindepth 1 -newer "$LOCK_PID_FILE" -print -quit 2>/dev/null | grep -q .; then
      return 0
    fi
  done

  if [[ -d "$TMP_PARENT" ]] && find "$TMP_PARENT" -type f \( -path '*/.case_runs_tmp/*' -o -path '*/.case_cache_tmp/*' \) -newer "$LOCK_PID_FILE" -print -quit 2>/dev/null | grep -q .; then
    return 0
  fi
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

  : > "$CERTIFY_STDOUT_LOG"
  : > "$CERTIFY_STDERR_LOG"
  start_ts="$(date +%s)"
  echo "[lca_strong_gate] certify start preset=$PRESET out=$OUTROOT workdir=$WORKDIR" >&2

  BRANCH_CERTIFY_CASE_RUN_TMP_ROOT="$CASE_RUN_TMP_ROOT" \
    BRANCH_CERTIFY_CASE_CACHE_ROOT="$CASE_CACHE_ROOT" \
    BRANCH_CERTIFY_CASE_CACHE_TMP_ROOT="$CASE_CACHE_TMP_ROOT" \
    BRANCH_CERTIFY_REPORT_OUTDIR="$OUTROOT" \
    python3 "$CERTIFY_HELPER" --solver "$SOLVER_SNAPSHOT" --preset "$PRESET" --out "$WORKDIR" --limit-scale "$LIMIT_SCALE" >"$CERTIFY_STDOUT_LOG" 2>"$CERTIFY_STDERR_LOG" &
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

normalize_script_log() {
  local path="$1"
  python3 - "$path" <<'PY' >/dev/null 2>&1 || true
from pathlib import Path
import sys

path = Path(sys.argv[1])
try:
    data = path.read_text(encoding="utf-8", errors="replace")
except OSError:
    raise SystemExit(0)
data = data.replace("\r", "").replace("\x04", "").replace("\x08", "")
path.write_text(data, encoding="utf-8")
PY
}

run_build_wrapper() {
  local rc=0
  : > "$BUILD_STDOUT_LOG"
  : > "$BUILD_STDERR_LOG"
  if script -q /dev/null "$BUILD_WRAPPER" >"$BUILD_STDOUT_LOG" 2>"$BUILD_STDERR_LOG"; then
    rc=0
  else
    rc=$?
  fi
  normalize_script_log "$BUILD_STDOUT_LOG"
  return "$rc"
}

init_runtime_artifacts() {
  bind_runtime_artifacts_to_root "$WORKDIR"
  printf 'kind\tlabel\tstatus\tvalue\n' > "$PRECHECK_MANIFEST"
}

bind_runtime_artifacts_to_root() {
  local root="$1"
  mkdir -p "$root"
  ensure_under_artifacts "$root"
  CASE_RUN_TMP_ROOT="$root/.case_runs_tmp"
  CASE_CACHE_ROOT="$root/.case_cache"
  CASE_CACHE_TMP_ROOT="$root/.case_cache_tmp"
  PRECHECK_MANIFEST="$root/preflight_manifest.tsv"
  ENV_SNAPSHOT="$root/runtime_env.txt"
  BUILD_STDOUT_LOG="$root/build.stdout.txt"
  BUILD_STDERR_LOG="$root/build.stderr.txt"
  CERTIFY_STDOUT_LOG="$root/certify.stdout.txt"
  CERTIFY_STDERR_LOG="$root/certify.stderr.txt"
  FAILURE_SUMMARY_PATH="$root/failure_summary.txt"
  FAILURE_REPORT_PATH="$root/latest_failure_report.md"
  PRESET_SNAPSHOT_PATH="$root/selected_preset.json"
  BUILD_METADATA_SNAPSHOT="$root/solver_build_meta.json"
  NON_ARTIFACT_BASELINE="$root/non_artifact_tree_baseline.json"
  NON_ARTIFACT_CURRENT="$root/non_artifact_tree_current.json"
  NON_ARTIFACT_REPORT="$root/non_artifact_tree_report.txt"
  REPEATABILITY_MANIFEST_PATH="$root/repeatability_gate_manifest.txt"

  ensure_under_artifacts "$CASE_RUN_TMP_ROOT"
  ensure_under_artifacts "$CASE_CACHE_ROOT"
  ensure_under_artifacts "$CASE_CACHE_TMP_ROOT"
  ensure_under_artifacts "$PRECHECK_MANIFEST"
  ensure_under_artifacts "$ENV_SNAPSHOT"
  ensure_under_artifacts "$BUILD_STDOUT_LOG"
  ensure_under_artifacts "$BUILD_STDERR_LOG"
  ensure_under_artifacts "$CERTIFY_STDOUT_LOG"
  ensure_under_artifacts "$CERTIFY_STDERR_LOG"
  ensure_under_artifacts "$FAILURE_SUMMARY_PATH"
  ensure_under_artifacts "$FAILURE_REPORT_PATH"
  ensure_under_artifacts "$PRESET_SNAPSHOT_PATH"
  ensure_under_artifacts "$BUILD_METADATA_SNAPSHOT"
  ensure_under_artifacts "$NON_ARTIFACT_BASELINE"
  ensure_under_artifacts "$NON_ARTIFACT_CURRENT"
  ensure_under_artifacts "$NON_ARTIFACT_REPORT"
  ensure_under_artifacts "$REPEATABILITY_MANIFEST_PATH"
}

ensure_workdir_runtime_artifacts_bound() {
  # Rebind runtime artifact paths to the live staging workdir if a previous
  # failure archive or inherited shell state left them pointing at an older run.
  if [[ -z "${WORKDIR:-}" ]]; then
    return 0
  fi
  mkdir -p "$WORKDIR"
  bind_runtime_artifacts_to_root "$WORKDIR"
  if [[ ! -f "$PRECHECK_MANIFEST" ]]; then
    printf 'kind\tlabel\tstatus\tvalue\n' > "$PRECHECK_MANIFEST"
  fi
}

recover_staging_workdir_after_loss() {
  local phase="$1"
  if [[ -z "${WORKDIR:-}" ]]; then
    fail "cannot recover missing strong gate staging workdir during $phase"
  fi
  echo "[lca_strong_gate] rebuilding staging artifacts after $phase removed $WORKDIR" >&2
  init_runtime_artifacts
  : > "$BUILD_STDOUT_LOG"
  : > "$BUILD_STDERR_LOG"
  : > "$CERTIFY_STDOUT_LOG"
  : > "$CERTIFY_STDERR_LOG"
  if [[ -z "${RUN_TMPDIR:-}" || ! -d "$RUN_TMPDIR" ]]; then
    configure_runtime_tmpdir
    load_release_environment
  fi
  PRESET_SOURCE_MATERIALIZED=""
  if check_selected_preset_source_ready; then
    :
  else
    return $?
  fi
  init_output_locality_scan
  if prepare_selected_preset; then
    :
  else
    return $?
  fi
  run_setup_preflight
}

record_preflight_check() {
  local kind="$1"
  local label="$2"
  local status="$3"
  local value="${4-}"
  ensure_workdir_runtime_artifacts_bound
  mkdir -p "$(dirname "$PRECHECK_MANIFEST")"
  printf '%s\t%s\t%s\t%s\n' "$kind" "$label" "$status" "$value" >> "$PRECHECK_MANIFEST"
}

write_repeatability_manifest() {
  python3 - "$REPEATABILITY_MANIFEST_PATH" "$WORKDIR/certify.json" "$WORKDIR/certify_summary.md" <<'PY'
from __future__ import annotations

import hashlib
import pathlib
import sys
import os

manifest_path = pathlib.Path(sys.argv[1])
certify_json = pathlib.Path(sys.argv[2])
certify_summary = pathlib.Path(sys.argv[3])


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


lines = [
    "gate=lca_strong_gate",
    "wrapper=./outer_suite_wrappers/lca_strong_gate.sh",
    f"repeatability_run_token={os.environ.get('LCA_REPEATABILITY_RUN_TOKEN', '')}",
    f"repeatability_cycle={os.environ.get('LCA_REPEATABILITY_CYCLE', '')}",
    f"repeatability_gate_label={os.environ.get('LCA_REPEATABILITY_GATE_LABEL', '')}",
    f"certify_json_sha256={sha256(certify_json)}",
    f"certify_summary_sha256={sha256(certify_summary)}",
]
manifest_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
PY
}

check_required_command_recorded() {
  local label="$1"
  local resolved=""
  if resolved="$(command -v "$label" 2>/dev/null)"; then
    record_preflight_check "command" "$label" "ok" "$resolved"
    return 0
  fi
  record_preflight_check "command" "$label" "missing" "-"
  return 1
}

check_required_file_recorded() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    record_preflight_check "file" "$label" "missing" "$path"
    return 1
  fi
  if [[ ! -f "$path" ]]; then
    record_preflight_check "file" "$label" "non_regular" "$path"
    return 1
  fi
  if [[ ! -r "$path" ]]; then
    record_preflight_check "file" "$label" "unreadable" "$path"
    return 1
  fi
  record_preflight_check "file" "$label" "ok" "$path"
  return 0
}

check_required_executable_recorded() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    record_preflight_check "executable" "$label" "missing" "$path"
    return 1
  fi
  if [[ ! -f "$path" ]]; then
    record_preflight_check "executable" "$label" "non_regular" "$path"
    return 1
  fi
  if [[ ! -r "$path" ]]; then
    record_preflight_check "executable" "$label" "unreadable" "$path"
    return 1
  fi
  if [[ ! -x "$path" ]]; then
    record_preflight_check "executable" "$label" "non_executable" "$path"
    return 1
  fi
  record_preflight_check "executable" "$label" "ok" "$path"
  return 0
}

check_not_dataless_recorded() {
  local path="$1"
  local label="$2"
  if path_has_dataless_flag "$path"; then
    record_preflight_check "materialization" "$label" "dataless" "$path"
    return 1
  fi
  record_preflight_check "materialization" "$label" "ok" "$path"
  return 0
}

materialize_selected_preset_source() {
  local source_path="$1"
  local staged_dir=""
  local staged_path=""

  if [[ -z "${RUN_TMPDIR:-}" || ! -d "$RUN_TMPDIR" ]]; then
    return 1
  fi

  staged_dir="$RUN_TMPDIR/preset_source"
  staged_path="$staged_dir/${source_path##*/}"
  mkdir -p "$staged_dir"
  ensure_under_artifacts "$staged_dir"
  ensure_under_artifacts "$staged_path"

  if python3 - "$source_path" "$staged_path" <<'PY' >/dev/null 2>&1
from __future__ import annotations

import json
import sys
from pathlib import Path

src = Path(sys.argv[1])
dst = Path(sys.argv[2])
data = src.read_text(encoding="utf-8")
json.loads(data)
dst.write_text(data, encoding="utf-8")
PY
  then
    PRESET_SOURCE_MATERIALIZED="$staged_path"
    return 0
  fi

  PRESET_SOURCE_MATERIALIZED=""
  return 1
}

snapshot_json_file() {
  local source_path="$1"
  local snapshot_path="$2"

  python3 - "$source_path" "$snapshot_path" <<'PY'
from __future__ import annotations

import json
import sys
from pathlib import Path

src = Path(sys.argv[1])
dst = Path(sys.argv[2])
data = src.read_text(encoding="utf-8")
json.loads(data)
dst.parent.mkdir(parents=True, exist_ok=True)
dst.write_text(data, encoding="utf-8")
PY
}

check_json_file_recorded() {
  local path="$1"
  local label="$2"
  check_not_dataless_recorded "$path" "$label materialization" || return 1
  if python3 - "$path" <<'PY' >/dev/null 2>&1
from __future__ import annotations

import json
import sys
from pathlib import Path

with Path(sys.argv[1]).open("r", encoding="utf-8") as f:
    json.load(f)
PY
  then
    record_preflight_check "json" "$label" "ok" "$path"
    return 0
  fi
  record_preflight_check "json" "$label" "invalid" "$path"
  return 1
}

check_selected_preset_source_ready() {
  local rc=0
  local fallback_source=""
  local cached_source=""

  if [[ -z "$PRESET_SOURCE" ]]; then
    record_preflight_check "file" "selected preset source" "missing" "$BRANCH_PRESET | $OUTER_PRESET"
    return 2
  fi

  check_required_file_recorded "$PRESET_SOURCE" "selected preset source" || rc=2
  if path_has_dataless_flag "$PRESET_SOURCE"; then
    if [[ "$PRESET_SOURCE" != "$PRESET_CACHE_PATH" ]] && [[ -f "$PRESET_CACHE_PATH" ]] && ! path_has_dataless_flag "$PRESET_CACHE_PATH"; then
      PRESET_SOURCE="$PRESET_CACHE_PATH"
      PRESET_SOURCE_MATERIALIZED=""
      record_preflight_check "file" "selected preset cached mirror" "ok" "$PRESET_SOURCE"
      record_preflight_check "materialization" "selected preset source" "recovered" "$PRESET_SOURCE"
    elif materialize_selected_preset_source "$PRESET_SOURCE"; then
      record_preflight_check "materialization" "selected preset source" "recovered" "$PRESET_SOURCE -> $PRESET_SOURCE_MATERIALIZED"
    else
      record_preflight_check "materialization" "selected preset source" "dataless" "$PRESET_SOURCE"
      rc=2
    fi
  else
    PRESET_SOURCE_MATERIALIZED=""
    record_preflight_check "materialization" "selected preset source" "ok" "$PRESET_SOURCE"
  fi

  if (( rc != 0 )) && [[ "$PRESET_SOURCE" == "$BRANCH_PRESET" ]] && [[ -f "$OUTER_PRESET" ]]; then
    fallback_source="$OUTER_PRESET"
    PRESET_SOURCE="$fallback_source"
    PRESET_SOURCE_MATERIALIZED=""
    rc=0
    record_preflight_check "file" "selected preset source fallback" "ok" "$fallback_source"
    check_required_file_recorded "$PRESET_SOURCE" "selected preset source fallback" || rc=2
    if path_has_dataless_flag "$PRESET_SOURCE"; then
      if materialize_selected_preset_source "$PRESET_SOURCE"; then
        record_preflight_check "materialization" "selected preset source fallback" "recovered" "$PRESET_SOURCE -> $PRESET_SOURCE_MATERIALIZED"
      else
        record_preflight_check "materialization" "selected preset source fallback" "dataless" "$PRESET_SOURCE"
        rc=2
      fi
    else
      record_preflight_check "materialization" "selected preset source fallback" "ok" "$PRESET_SOURCE"
    fi
  fi

  if (( rc != 0 )) && cached_source="$(resolve_cached_preset_snapshot)"; then
    PRESET_SOURCE="$cached_source"
    PRESET_SOURCE_MATERIALIZED=""
    rc=0
    record_preflight_check "file" "selected preset cached fallback" "ok" "$cached_source"
    check_required_file_recorded "$PRESET_SOURCE" "selected preset cached fallback" || rc=2
    if path_has_dataless_flag "$PRESET_SOURCE"; then
      record_preflight_check "materialization" "selected preset cached fallback" "dataless" "$PRESET_SOURCE"
      rc=2
    else
      record_preflight_check "materialization" "selected preset cached fallback" "ok" "$PRESET_SOURCE"
    fi
  fi
  return "$rc"
}

check_python_entrypoint_recorded() {
  local path="$1"
  local label="$2"
  if python3 - "$path" <<'PY' >/dev/null 2>&1
from __future__ import annotations

import py_compile
import sys

# `runpy.run_path(...)` can block on this branch's iCloud-backed workspace even
# for helpers that later execute normally. Preflight only needs a lightweight
# structural gate here, so prefer bytecode compilation to prove the entrypoint
# is syntactically importable without re-executing its runtime side effects.
py_compile.compile(sys.argv[1], doraise=True)
PY
  then
    record_preflight_check "python_entrypoint" "$label" "ok" "$path"
    return 0
  fi
  record_preflight_check "python_entrypoint" "$label" "broken" "$path"
  return 1
}

check_artifact_path_recorded() {
  local path="$1"
  local label="$2"
  local resolved=""
  if [[ -z "$path" ]]; then
    record_preflight_check "artifact_path" "$label" "missing" "-"
    return 1
  fi
  if resolved="$(python3 "$ARTIFACT_RESOLVER" --ensure "$path" 2>/dev/null)"; then
    record_preflight_check "artifact_path" "$label" "ok" "$resolved"
    return 0
  fi
  record_preflight_check "artifact_path" "$label" "escaped" "$path"
  return 1
}

write_runtime_environment_snapshot() {
  ensure_workdir_runtime_artifacts_bound
  if [[ -n "${PRESET:-}" && -f "$PRESET" ]]; then
    snapshot_json_file "$PRESET" "$PRESET_SNAPSHOT_PATH"
  fi

  {
    echo "script=./outer_suite_wrappers/lca_strong_gate.sh"
    echo "script_dir=$SCRIPT_DIR"
    echo "branch_root=$BRANCH_ROOT"
    echo "outer_root=$OUTER_ROOT"
    echo "tooling_root=$TOOLING_ROOT"
    echo "tooling_certify_source=$TOOLING_CERTIFY_SOURCE"
    echo "artifacts_root=$ARTIFACTS_ROOT"
    echo "preset_cache_root=$PRESET_CACHE_ROOT"
    echo "preset_cache_path=$PRESET_CACHE_PATH"
    echo "tmp_parent=$TMP_PARENT"
    echo "lock_root=$LOCK_ROOT"
    echo "snapshot_root=$SNAPSHOT_ROOT"
    echo "output_root=$OUTROOT"
    echo "failure_root=$FAILED_ROOT"
    echo "workdir=$WORKDIR"
    echo "case_run_tmp_root=$CASE_RUN_TMP_ROOT"
    echo "case_cache_root=$CASE_CACHE_ROOT"
    echo "case_cache_tmp_root=$CASE_CACHE_TMP_ROOT"
    echo "build_wrapper=$BUILD_WRAPPER"
    echo "release_env=$RELEASE_ENV"
    echo "certify_helper=$CERTIFY_HELPER"
    echo "selected_preset_source=$PRESET_SOURCE"
    echo "selected_preset_source_materialized=${PRESET_SOURCE_MATERIALIZED:-}"
    echo "selected_preset_path=$PRESET"
    echo "selected_preset_snapshot=$PRESET_SNAPSHOT_PATH"
    echo "stage_filter=${STAGE_FILTER:-}"
    echo "limit_scale=$LIMIT_SCALE"
    echo "heartbeat_interval=$HEARTBEAT_INTERVAL"
    echo "stale_lock_seconds=$STALE_LOCK_SECONDS"
    echo "solver_source=$SOURCE"
    echo "solver_binary=$BINARY"
    echo "solver_build_metadata=$SOLVER_BUILD_METADATA"
    echo "solver_build_metadata_snapshot=$BUILD_METADATA_SNAPSHOT"
    echo "build_stdout=$BUILD_STDOUT_LOG"
    echo "build_stderr=$BUILD_STDERR_LOG"
    echo "branch_artifact_tmp_root=${BRANCH_ARTIFACT_TMP_ROOT:-}"
    echo "tmpdir=${TMPDIR:-}"
    echo "tmp=${TMP:-}"
    echo "temp=${TEMP:-}"
    echo "home=${HOME:-}"
    echo "xdg_config_home=${XDG_CONFIG_HOME:-}"
    echo "xdg_cache_home=${XDG_CACHE_HOME:-}"
    echo "xdg_state_home=${XDG_STATE_HOME:-}"
    echo "pythonpycacheprefix=${PYTHONPYCACHEPREFIX:-}"
    echo "pythondontwritebytecode=${PYTHONDONTWRITEBYTECODE:-}"
    echo "cc=${CC:-}"
    echo "cxx=${CXX:-}"
    echo "profile_mode=${PROFILE_MODE:-}"
    echo "local_skip_self_test=${LOCAL_SKIP_SELF_TEST:-}"
    echo "enable_state_load_materialization_opt=${ENABLE_STATE_LOAD_MATERIALIZATION_OPT:-}"
    echo "enable_prev_state_writeback_opt=${ENABLE_PREV_STATE_WRITEBACK_OPT:-}"
    echo "enable_layout_signature_gate_opt=${ENABLE_LAYOUT_SIGNATURE_GATE_OPT:-}"
    echo "enable_layout_reuse_zero_elision_opt=${ENABLE_LAYOUT_REUSE_ZERO_ELISION_OPT:-}"
    echo "strong_gate_release_profile=progress40_defaults+ac3_state_materialization"
    echo "solver_env_scrub=compiler_and_ENABLE_PROFILE_DENSE_RUN_TAG"
  } > "$ENV_SNAPSHOT"
}

prepare_selected_preset() {
  local effective_preset_source="${PRESET_SOURCE_MATERIALIZED:-$PRESET_SOURCE}"

  PRESET="$effective_preset_source"
  if [[ -z "$STAGE_FILTER" || -z "$effective_preset_source" ]]; then
    return 0
  fi

  FILTERDIR="$WORKDIR/.stage_filter"
  mkdir -p "$FILTERDIR"
  ensure_under_artifacts "$FILTERDIR"
  PRESET="$FILTERDIR/preset.json"
  ensure_under_artifacts "$PRESET"
  python3 - "$effective_preset_source" "$STAGE_FILTER" "$PRESET" <<'PY'
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
}

run_setup_preflight() {
  local preflight_rc=0
  local env_var_name=""
  local env_var_value=""

  record_preflight_check "path" "script_dir" "ok" "$SCRIPT_DIR"
  record_preflight_check "path" "branch_root" "ok" "$BRANCH_ROOT"
  record_preflight_check "path" "outer_root" "ok" "$OUTER_ROOT"
  record_preflight_check "path" "tooling_root" "ok" "$TOOLING_ROOT"
  record_preflight_check "path" "artifacts_root" "ok" "$ARTIFACTS_ROOT"
  record_preflight_check "path" "tmp_parent" "ok" "$TMP_PARENT"
  record_preflight_check "path" "lock_root" "ok" "$LOCK_ROOT"
  record_preflight_check "path" "output_root" "ok" "$OUTROOT"
  record_preflight_check "path" "failure_root" "ok" "$FAILED_ROOT"
  record_preflight_check "path" "branch_preset_candidate" "ok" "$BRANCH_PRESET"
  record_preflight_check "path" "outer_preset_candidate" "ok" "$OUTER_PRESET"
  record_preflight_check "path" "preset_cache_path" "ok" "$PRESET_CACHE_PATH"
  record_preflight_check "path" "selected_preset_source" "ok" "$PRESET_SOURCE"
  if [[ -n "$PRESET_SOURCE_MATERIALIZED" ]]; then
    record_preflight_check "path" "selected_preset_source_materialized" "ok" "$PRESET_SOURCE_MATERIALIZED"
  fi
  record_preflight_check "path" "selected_preset" "ok" "$PRESET"
  record_preflight_check "value" "stage_filter" "ok" "${STAGE_FILTER:-}"

  check_required_command_recorded bash || preflight_rc=2
  check_required_command_recorded python3 || preflight_rc=2
  check_required_command_recorded mktemp || preflight_rc=2
  check_required_command_recorded dirname || preflight_rc=2
  check_required_command_recorded script || preflight_rc=2
  check_required_command_recorded cp || preflight_rc=2
  check_required_command_recorded mv || preflight_rc=2
  check_required_command_recorded rm || preflight_rc=2
  check_required_command_recorded grep || preflight_rc=2
  check_required_command_recorded sleep || preflight_rc=2
  check_required_command_recorded tail || preflight_rc=2

  check_required_file_recorded "$SOURCE" "solver source" || preflight_rc=2
  check_required_file_recorded "$ARTIFACT_RESOLVER" "artifact resolver" || preflight_rc=2
  check_required_file_recorded "$RELEASE_ENV" "release env wrapper" || preflight_rc=2
  check_required_file_recorded "$CERTIFY_HELPER" "branch-local certify helper" || preflight_rc=2
  check_required_file_recorded "$TOOLING_CERTIFY_SOURCE" "outer certify helper" || preflight_rc=2
  check_required_file_recorded "$BRANCH_ROOT/branch_gen_case.py" "case generator helper" || preflight_rc=2
  check_required_file_recorded "$BRANCH_ROOT/branch_validator.py" "validator helper" || preflight_rc=2
  check_required_file_recorded "$BRANCH_ROOT/boj28350_resume.py" "resume helper" || preflight_rc=2
  check_required_executable_recorded "$BUILD_WRAPPER" "build wrapper" || preflight_rc=2

  if [[ -n "$PRESET" ]]; then
    check_required_file_recorded "$PRESET" "selected preset" || preflight_rc=2
    check_json_file_recorded "$PRESET" "selected preset json" || preflight_rc=2
  else
    record_preflight_check "file" "selected preset" "missing" "$BRANCH_PRESET | $OUTER_PRESET"
    preflight_rc=2
  fi
  check_python_entrypoint_recorded "$CERTIFY_HELPER" "branch-local certify helper imports" || preflight_rc=2
  check_python_entrypoint_recorded "$BRANCH_ROOT/boj28350_resume.py" "resume helper imports" || preflight_rc=2

  check_artifact_path_recorded "$ARTIFACTS_ROOT" "artifacts_root" || preflight_rc=2
  check_artifact_path_recorded "$TMP_PARENT" "tmp_parent" || preflight_rc=2
  check_artifact_path_recorded "$LOCK_ROOT" "lock_root" || preflight_rc=2
  check_artifact_path_recorded "$SNAPSHOT_ROOT" "snapshot_root" || preflight_rc=2
  check_artifact_path_recorded "$PRESET_CACHE_ROOT" "preset_cache_root" || preflight_rc=2
  check_artifact_path_recorded "$PRESET_CACHE_PATH" "preset_cache_path" || preflight_rc=2
  check_artifact_path_recorded "$OUTROOT" "output_root" || preflight_rc=2
  check_artifact_path_recorded "$OUTPARENT" "output_parent" || preflight_rc=2
  check_artifact_path_recorded "$FAILED_ROOT" "failure_root" || preflight_rc=2
  check_artifact_path_recorded "$WORKDIR" "workdir" || preflight_rc=2
  check_artifact_path_recorded "$CASE_RUN_TMP_ROOT" "case_run_tmp_root" || preflight_rc=2
  check_artifact_path_recorded "$CASE_CACHE_ROOT" "case_cache_root" || preflight_rc=2
  check_artifact_path_recorded "$CASE_CACHE_TMP_ROOT" "case_cache_tmp_root" || preflight_rc=2
  check_artifact_path_recorded "$RUN_TMPDIR" "runtime_tmpdir" || preflight_rc=2
  if [[ -n "$PRESET_SOURCE_MATERIALIZED" ]]; then
    check_artifact_path_recorded "$PRESET_SOURCE_MATERIALIZED" "selected_preset_source_materialized" || preflight_rc=2
  fi
  if [[ -n "$FILTERDIR" ]]; then
    check_artifact_path_recorded "$FILTERDIR" "filterdir" || preflight_rc=2
  fi

  for env_var_name in \
    BRANCH_ARTIFACT_TMP_ROOT \
    TMPDIR \
    TMP \
    TEMP \
    HOME \
    XDG_CONFIG_HOME \
    XDG_CACHE_HOME \
    XDG_STATE_HOME \
    PYTHONPYCACHEPREFIX; do
    eval "env_var_value=\${$env_var_name-}"
    check_artifact_path_recorded "$env_var_value" "$env_var_name" || preflight_rc=2
  done

  write_runtime_environment_snapshot

  if (( preflight_rc != 0 )); then
    write_failure_summary "preflight" "$preflight_rc" "required strong gate dependency or runtime path check failed"
    report_failure_context "preflight" "$preflight_rc" "required strong gate dependency or runtime path check failed"
  fi
  return "$preflight_rc"
}

init_output_locality_scan() {
  if ! python3 "$ARTIFACT_RESOLVER" --snapshot-non-artifact-tree "$NON_ARTIFACT_BASELINE" >/dev/null; then
    fail "failed to capture the non-artifact tree baseline"
  fi
}

verify_output_locality() {
  local rc=0

  if [[ -z "${NON_ARTIFACT_BASELINE:-}" || ! -f "$NON_ARTIFACT_BASELINE" ]]; then
    return 0
  fi

  if python3 "$ARTIFACT_RESOLVER" --verify-non-artifact-tree \
    "$NON_ARTIFACT_BASELINE" "$NON_ARTIFACT_CURRENT" "$NON_ARTIFACT_REPORT" >/dev/null; then
    return 0
  fi
  rc=$?
  if (( rc == 3 )); then
    echo "[lca_strong_gate] non-artifact output locality violation report: $NON_ARTIFACT_REPORT" >&2
    return "$rc"
  fi
  fail "failed to verify non-artifact output locality"
}

strip_internal_state() {
  rm -rf "$WORKDIR/.case_cache" "$WORKDIR/.case_cache_tmp" "$WORKDIR/.case_runs_tmp" "$WORKDIR/.stage_filter"
}

write_failure_summary() {
  local phase="$1"
  local exit_code="$2"
  local message="$3"
  local summary_root="$WORKDIR"
  local failure_summary_path=""
  local failure_report_path=""
  local certify_json="$WORKDIR/certify.json"
  local certify_summary="$WORKDIR/certify_summary.md"
  local final_precheck_manifest="$FAILED_ROOT/$(basename "$PRECHECK_MANIFEST")"
  local final_env_snapshot="$FAILED_ROOT/$(basename "$ENV_SNAPSHOT")"
  local final_build_stdout="$FAILED_ROOT/$(basename "$BUILD_STDOUT_LOG")"
  local final_build_stderr="$FAILED_ROOT/$(basename "$BUILD_STDERR_LOG")"
  local final_certify_stdout="$FAILED_ROOT/$(basename "$CERTIFY_STDOUT_LOG")"
  local final_certify_stderr="$FAILED_ROOT/$(basename "$CERTIFY_STDERR_LOG")"
  local final_preset_snapshot="$FAILED_ROOT/$(basename "$PRESET_SNAPSHOT_PATH")"
  local final_build_metadata="$FAILED_ROOT/$(basename "$BUILD_METADATA_SNAPSHOT")"
  local final_certify_json="$FAILED_ROOT/$(basename "$certify_json")"
  local final_certify_summary="$FAILED_ROOT/$(basename "$certify_summary")"
  local final_solver_snapshot="$FAILED_ROOT/solver_snapshot"
  local final_non_artifact_current="$FAILED_ROOT/$(basename "$NON_ARTIFACT_CURRENT")"
  local final_non_artifact_report="$FAILED_ROOT/$(basename "$NON_ARTIFACT_REPORT")"
  local locality_rc=0

  if [[ -z "${summary_root:-}" || ! -d "$summary_root" ]]; then
    summary_root="$FAILED_ROOT"
    bind_runtime_artifacts_to_root "$summary_root"
  fi
  mkdir -p "$summary_root"
  failure_summary_path="$summary_root/$(basename "$FAILURE_SUMMARY_PATH")"
  failure_report_path="$summary_root/$(basename "$FAILURE_REPORT_PATH")"
  ensure_under_artifacts "$failure_summary_path"
  ensure_under_artifacts "$failure_report_path"

  if [[ -n "${ENV_SNAPSHOT:-}" && ! -f "$ENV_SNAPSHOT" ]]; then
    write_runtime_environment_snapshot || true
  fi

  if verify_output_locality; then
    :
  else
    locality_rc=$?
    if (( locality_rc != 3 )); then
      echo "[lca_strong_gate] warning: failed to refresh non-artifact output locality report" >&2
    fi
  fi

  {
    echo "script=./outer_suite_wrappers/lca_strong_gate.sh"
    echo "failure_stage=$phase"
    echo "exit_code=$exit_code"
    echo "message=$message"
    echo "output_root=$OUTROOT"
    echo "failure_root=$FAILED_ROOT"
    echo "workdir=$WORKDIR"
    echo "selected_preset_source=$PRESET_SOURCE"
    echo "selected_preset_source_materialized=${PRESET_SOURCE_MATERIALIZED:-}"
    echo "selected_preset_snapshot=$final_preset_snapshot"
    echo "preflight_manifest=$final_precheck_manifest"
    echo "runtime_env=$final_env_snapshot"
    echo "build_stdout=$final_build_stdout"
    echo "build_stderr=$final_build_stderr"
    echo "certify_stdout=$final_certify_stdout"
    echo "certify_stderr=$final_certify_stderr"
    echo "solver_build_metadata=$final_build_metadata"
    echo "certify_json=$final_certify_json"
    echo "certify_summary=$final_certify_summary"
    echo "solver_snapshot=$final_solver_snapshot"
    echo "non_artifact_tree_current=$final_non_artifact_current"
    echo "non_artifact_tree_report=$final_non_artifact_report"
  } > "$failure_summary_path"

  {
    echo "# lca_strong_gate Failure Report"
    echo
    echo "- Stage: \`$phase\`"
    echo "- Exit code: \`$exit_code\`"
    echo "- Message: \`$message\`"
    echo "- Output root: \`$OUTROOT\`"
    echo "- Failure root: \`$FAILED_ROOT\`"
    echo "- Workdir: \`$WORKDIR\`"
    echo "- Selected preset source: \`$PRESET_SOURCE\`"
    if [[ -n "$PRESET_SOURCE_MATERIALIZED" ]]; then
      echo "- Materialized preset source: \`$PRESET_SOURCE_MATERIALIZED\`"
    fi
    echo "- Selected preset snapshot: \`$final_preset_snapshot\`"
    echo "- Solver binary: \`$BINARY\`"
    echo "- Solver build metadata: \`$final_build_metadata\`"
    echo "- Solver snapshot: \`$final_solver_snapshot\`"
    echo
    echo "## Recorded Artifacts"
    echo
    echo "- Preflight manifest: \`$final_precheck_manifest\`"
    echo "- Runtime env snapshot: \`$final_env_snapshot\`"
    echo "- Build stdout: \`$final_build_stdout\`"
    echo "- Build stderr: \`$final_build_stderr\`"
    echo "- Certify stdout: \`$final_certify_stdout\`"
    echo "- Certify stderr: \`$final_certify_stderr\`"
    echo "- Certify JSON: \`$final_certify_json\`"
    echo "- Certify summary: \`$final_certify_summary\`"
    echo "- Non-artifact tree state: \`$final_non_artifact_current\`"
    echo "- Non-artifact tree report: \`$final_non_artifact_report\`"
    if command -v tail >/dev/null 2>&1 && [[ -s "$BUILD_STDERR_LOG" ]]; then
      echo
      echo "## Build stderr tail"
      echo
      echo '```text'
      tail -n 40 "$BUILD_STDERR_LOG"
      echo '```'
    fi
    if command -v tail >/dev/null 2>&1 && [[ -s "$certify_summary" ]]; then
      echo
      echo "## Certify Summary Tail"
      echo
      echo '```text'
      tail -n 60 "$certify_summary"
      echo '```'
    fi
    if command -v tail >/dev/null 2>&1 && [[ -s "$CERTIFY_STDERR_LOG" ]]; then
      echo
      echo "## Certify stderr tail"
      echo
      echo '```text'
      tail -n 60 "$CERTIFY_STDERR_LOG"
      echo '```'
    fi
  } > "$failure_report_path"
}

report_failure_context() {
  local phase="$1"
  local exit_code="$2"
  local message="$3"
  echo "[lca_strong_gate] stage=$phase exit_code=$exit_code message=$message" >&2
  echo "[lca_strong_gate] failure snapshot root after cleanup: $FAILED_ROOT" >&2
  echo "[lca_strong_gate] preflight manifest after cleanup: $FAILED_ROOT/$(basename "$PRECHECK_MANIFEST")" >&2
  echo "[lca_strong_gate] runtime env snapshot after cleanup: $FAILED_ROOT/$(basename "$ENV_SNAPSHOT")" >&2
  echo "[lca_strong_gate] selected preset snapshot after cleanup: $FAILED_ROOT/$(basename "$PRESET_SNAPSHOT_PATH")" >&2
  echo "[lca_strong_gate] build stdout after cleanup: $FAILED_ROOT/$(basename "$BUILD_STDOUT_LOG")" >&2
  echo "[lca_strong_gate] build stderr after cleanup: $FAILED_ROOT/$(basename "$BUILD_STDERR_LOG")" >&2
  echo "[lca_strong_gate] certify stdout after cleanup: $FAILED_ROOT/$(basename "$CERTIFY_STDOUT_LOG")" >&2
  echo "[lca_strong_gate] certify stderr after cleanup: $FAILED_ROOT/$(basename "$CERTIFY_STDERR_LOG")" >&2
  echo "[lca_strong_gate] solver build metadata after cleanup: $FAILED_ROOT/$(basename "$BUILD_METADATA_SNAPSHOT")" >&2
  echo "[lca_strong_gate] failure summary after cleanup: $FAILED_ROOT/$(basename "$FAILURE_SUMMARY_PATH")" >&2
  echo "[lca_strong_gate] failure report after cleanup: $FAILED_ROOT/$(basename "$FAILURE_REPORT_PATH")" >&2
  echo "[lca_strong_gate] non-artifact output locality report after cleanup: $FAILED_ROOT/$(basename "$NON_ARTIFACT_REPORT")" >&2
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
  local locality_rc=0
  trap - EXIT
  set +e
  if [[ -n "${CERTIFY_PID:-}" ]] && kill -0 "$CERTIFY_PID" 2>/dev/null; then
    kill "$CERTIFY_PID" 2>/dev/null || true
    wait "$CERTIFY_PID" 2>/dev/null || true
  fi
  CERTIFY_PID=""
  if [[ -n "${WORKDIR:-}" && -d "$WORKDIR" ]]; then
    if verify_output_locality; then
      :
    else
      locality_rc=$?
      if (( rc == 0 )); then
        write_failure_summary "output_locality" "$locality_rc" "non-artifact output locality verification failed"
        report_failure_context "output_locality" "$locality_rc" "non-artifact output locality verification failed"
      fi
      if (( rc == 0 )); then
        rc=$locality_rc
      fi
    fi
  fi
  if (( rc != 0 )); then
    preserve_failed_output
    restore_previous_output
    if [[ -n "${SOLVER_SNAPSHOT:-}" && -f "$SOLVER_SNAPSHOT" ]]; then
      mkdir -p "$FAILED_ROOT"
      cp "$SOLVER_SNAPSHOT" "$FAILED_ROOT/solver_snapshot" 2>/dev/null || true
    fi
  fi
  if [[ -n "${WORKDIR:-}" && -e "$WORKDIR" ]]; then
    remove_path_retry "$WORKDIR"
  fi
  if [[ -n "${FILTERDIR:-}" && -e "$FILTERDIR" ]]; then
    remove_path_retry "$FILTERDIR"
  fi
  if [[ -n "${RUN_TMPDIR:-}" && -e "$RUN_TMPDIR" ]]; then
    remove_path_retry "$RUN_TMPDIR"
  fi
  if [[ -n "${SOLVER_SNAPSHOT:-}" && -e "$SOLVER_SNAPSHOT" ]]; then
    remove_path_retry "$SOLVER_SNAPSHOT"
  fi
  if (( LOCK_HELD )); then
    prune_empty_solver_snapshot_roots
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
require_command script
require_file "$ARTIFACT_RESOLVER" "artifact resolver"

OUTDIR="$(python3 "$ARTIFACT_RESOLVER" lca_strong_gate "${1:-}")"
OUTROOT="$OUTDIR"
OUTPARENT="$(dirname "$OUTROOT")"
BACKUP_ROOT="${OUTROOT}.previous"
FAILED_ROOT="${OUTROOT}.latest_failure"
FAILED_ARCHIVE_ROOT="${OUTROOT}.failure_archive"

ensure_under_artifacts "$ARTIFACTS_ROOT"
ensure_under_artifacts "$TMP_PARENT"
ensure_under_artifacts "$LOCK_ROOT"
ensure_under_artifacts "$SNAPSHOT_ROOT"
ensure_under_artifacts "$PRESET_CACHE_ROOT"
ensure_under_artifacts "$PRESET_CACHE_PATH"
ensure_under_artifacts "$OUTROOT"
ensure_under_artifacts "$OUTPARENT"
ensure_under_artifacts "$BACKUP_ROOT"
ensure_under_artifacts "$FAILED_ROOT"
ensure_under_artifacts "$FAILED_ARCHIVE_ROOT"
mkdir -p "$ARTIFACTS_ROOT"
acquire_lock
mkdir -p "$OUTPARENT"
clear_stale_state
mkdir -p "$SNAPSHOT_ROOT"
refresh_preset_cache_if_possible || true
PRESET_SOURCE="$(resolve_preset || true)"

if [[ -e "$OUTROOT" && ! -d "$OUTROOT" ]]; then
  fail "output path exists but is not a directory: $OUTROOT"
fi
if [[ -e "$BACKUP_ROOT" && ! -d "$BACKUP_ROOT" ]]; then
  fail "backup path exists but is not a directory: $BACKUP_ROOT"
fi

mkdir -p "$TMP_PARENT"
WORKDIR="$(mktemp -d "$TMP_PARENT/$RUN_WORK_TEMPLATE")"
ensure_under_artifacts "$WORKDIR"
init_runtime_artifacts
init_output_locality_scan
configure_runtime_tmpdir

if check_selected_preset_source_ready; then
  :
else
  preset_source_rc=$?
  write_failure_summary "preset_source" "$preset_source_rc" "selected preset source is missing or not locally materialized"
  report_failure_context "preset_source" "$preset_source_rc" "selected preset source is missing or not locally materialized"
  exit "$preset_source_rc"
fi

if prepare_selected_preset; then
  :
else
  preset_rc=$?
  write_failure_summary "preset_filter" "$preset_rc" "failed to materialize the selected preset"
  report_failure_context "preset_filter" "$preset_rc" "failed to materialize the selected preset"
  exit "$preset_rc"
fi

if load_release_environment; then
  :
else
  release_env_rc=$?
  write_failure_summary "release_env" "$release_env_rc" "failed to source release env wrapper"
  report_failure_context "release_env" "$release_env_rc" "failed to source release env wrapper"
  exit "$release_env_rc"
fi

if run_setup_preflight; then
  :
else
  preflight_rc=$?
  exit "$preflight_rc"
fi

if run_build_wrapper; then
  :
else
  build_rc=$?
  write_failure_summary "build" "$build_rc" "build wrapper failed"
  report_failure_context "build" "$build_rc" "build wrapper failed"
  exit "$build_rc"
fi

if [[ ! -d "$WORKDIR" ]]; then
  if recover_staging_workdir_after_loss "build"; then
    :
  else
    recovery_rc=$?
    write_failure_summary "staging_recovery" "$recovery_rc" "failed to rebuild the strong gate staging workdir after build"
    report_failure_context "staging_recovery" "$recovery_rc" "failed to rebuild the strong gate staging workdir after build"
    exit "$recovery_rc"
  fi
fi

require_executable "$BINARY" "built solver binary"
SOLVER_SNAPSHOT="$(mktemp "$SNAPSHOT_ROOT/lca_strong_gate.solver.XXXXXX")"
ensure_under_artifacts "$SOLVER_SNAPSHOT"
cp "$BINARY" "$SOLVER_SNAPSHOT"
chmod +x "$SOLVER_SNAPSHOT"

if [[ ! -f "$SOLVER_BUILD_METADATA" ]]; then
  metadata_rc=2
  write_failure_summary "build_metadata" "$metadata_rc" "build metadata missing after build"
  report_failure_context "build_metadata" "$metadata_rc" "build metadata missing after build"
  exit "$metadata_rc"
fi
if cp "$SOLVER_BUILD_METADATA" "$BUILD_METADATA_SNAPSHOT"; then
  :
else
  metadata_rc=$?
  write_failure_summary "build_metadata" "$metadata_rc" "failed to snapshot build metadata"
  report_failure_context "build_metadata" "$metadata_rc" "failed to snapshot build metadata"
  exit "$metadata_rc"
fi

if run_certify_suite; then
  :
else
  certify_rc=$?
  write_failure_summary "certify" "$certify_rc" "certify suite failed"
  report_failure_context "certify" "$certify_rc" "certify suite failed"
  exit "$certify_rc"
fi
if write_repeatability_manifest; then
  :
else
  manifest_rc=$?
  write_failure_summary "repeatability_manifest" "$manifest_rc" "failed to materialize current-run repeatability manifest"
  report_failure_context "repeatability_manifest" "$manifest_rc" "failed to materialize current-run repeatability manifest"
  exit "$manifest_rc"
fi
if verify_output_locality; then
  :
else
  locality_rc=$?
  write_failure_summary "output_locality" "$locality_rc" "non-artifact output locality verification failed"
  report_failure_context "output_locality" "$locality_rc" "non-artifact output locality verification failed"
  exit "$locality_rc"
fi
publish_output
