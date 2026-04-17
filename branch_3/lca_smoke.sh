#!/usr/bin/env bash
set -Eeuo pipefail

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

if [[ "${LCA_SMOKE_ENABLE_XTRACE:-0}" == "1" ]]; then
  export PS4='+x:${LINENO}: '
  # Keep shell tracing branch-local even if the wrapper is launched from a
  # different cwd.
  mkdir -p "$SCRIPT_DIR/artifacts" 2>/dev/null || true
  exec 9>>"$SCRIPT_DIR/artifacts/trace.log"
  export BASH_XTRACEFD=9
  set -x
fi

SMOKE_EXIT_SOLVER_FAILURE=1
SMOKE_EXIT_USAGE=2
SMOKE_EXIT_SOLVER_TIMEOUT=124
SMOKE_EXIT_SOLVER_RUNTIME_FAILURE=125
SMOKE_EXIT_HARNESS_FAILURE=70
LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG="LCA_SMOKE_LAUNCHER_CLEAN_ENV_READY"
LCA_SMOKE_INNER_CLEAN_ENV_FLAG="LCA_SMOKE_CLEAN_ENV_READY"
LCA_SMOKE_LAUNCHER_REEXEC_ARG="--__lca_smoke_launcher_clean_env_reexec"
LCA_SMOKE_CLEAN_PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"
SELF_PATH="$SCRIPT_DIR/${SCRIPT_SOURCE##*/}"
BRANCH_ROOT="$SCRIPT_DIR"
OUTER_SUITE_WRAPPERS_DIR="$BRANCH_ROOT/outer_suite_wrappers"
RESUME_WORKSPACE_DIR="$BRANCH_ROOT/boj28350_resume"
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
INNER_WRAPPER="$BRANCH_ROOT/outer_suite_wrappers/lca_smoke.sh"
BUILD_WRAPPER="$BRANCH_ROOT/build.sh"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
RUN_CASE_HELPER="$BRANCH_ROOT/branch_run_case.py"
CHECKER_HELPER="$BRANCH_ROOT/branch_validator.py"
CHECKER_HELPER_LOCAL="$BRANCH_ROOT/branch_validator_local.py"
BUILD_HELPER="$BRANCH_ROOT/build.py"
RESUME_HELPER="$BRANCH_ROOT/boj28350_resume.py"
SMOKE_TARGET_WRAPPER="$BRANCH_ROOT/lca_smoke_target.sh"
SOURCE="$BRANCH_ROOT/boj28350_resume/boj28350_branch_3_solver.cpp"
SMOKE_CASES_SOURCE_DEFAULT="$BRANCH_ROOT/boj28350_resume/smoke_cases.tsv"
SMOKE_CASES_SOURCE="${LCA_SMOKE_DEBUG_MANIFEST:-$SMOKE_CASES_SOURCE_DEFAULT}"
SMOKE_MANIFEST_INPUT_POLICY="branch_local_smoke_manifest"
if [[ -n "${LCA_SMOKE_DEBUG_MANIFEST:-}" ]]; then
  SMOKE_MANIFEST_INPUT_POLICY="debug_manifest_override"
fi
if [[ -f "$CHECKER_HELPER_LOCAL" && -r "$CHECKER_HELPER_LOCAL" ]]; then
  CHECKER_HELPER="$CHECKER_HELPER_LOCAL"
fi
BRANCH_ARTIFACTS_ROOT=""
ARTIFACTS_ROOT=""
TMP_PARENT=""
LOCK_ROOT=""
LAUNCHER_LOCKDIR=""
LAUNCHER_LOCK_PID_FILE=""
LAUNCHER_LOCK_HELD=0
LAUNCHER_TMPDIR=""
LAUNCHER_TMPDIR_PARENT=""
LAUNCHER_PREFLIGHT_ROOT=""
LAUNCHER_PREFLIGHT_MANIFEST_PATH=""
LAUNCHER_PREFLIGHT_ENV_SNAPSHOT_PATH=""
LAUNCHER_HOME=""
LAUNCHER_XDG_CONFIG_HOME=""
LAUNCHER_XDG_CACHE_HOME=""
LAUNCHER_XDG_STATE_HOME=""
LAUNCHER_PYCACHE_ROOT=""
LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION=""
LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR=""
LAUNCHER_DISPATCH_MARKER=""
BASH_BIN="${BASH:-}"
SMOKE_OUTPUT_ROOT_DEFAULT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke"
SMOKE_FAILURE_ROOT_DEFAULT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_failure"
LAUNCHER_FAILURE_ROOT_DEFAULT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_launcher_latest_failure"
LAUNCHER_FAILURE_ROOT=""
LAUNCHER_FAILURE_SUMMARY=""
LAUNCHER_FAILURE_REPORT=""
LAUNCHER_FAILURE_ENV_SNAPSHOT=""
LAUNCHER_FAILURE_PREFLIGHT_MANIFEST=""
LAUNCHER_FAILURE_INVOCATION_COMMAND_PATH=""
LAUNCHER_FAILURE_DISPATCH_COMMAND_PATH=""
LAUNCHER_FAILURE_RERUN_COMMAND_PATH=""
LAUNCHER_FAILURE_REASON_PATH=""
LAUNCHER_FAILURE_COMMAND_PATH=""
LAUNCHER_FAILURE_ARTIFACT_MANIFEST=""
LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION=""
LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR=""
LAUNCHER_FAILURE_STAGE="bootstrap"
LAUNCHER_FAILURE_BUNDLE_ACTIVE=0
LAUNCHER_INVOCATION_COMMAND=""
LAUNCHER_DISPATCH_COMMAND=""
LAUNCHER_ORIGINAL_PWD=""
LAUNCHER_FAILURE_MESSAGE=""
LAUNCHER_FAILURE_RC=0
LAUNCHER_FAILURE_COMMAND=""
LAUNCHER_FAILURE_LINE=""
LAUNCHER_LAST_CHECK_KIND=""
LAUNCHER_LAST_CHECK_LABEL=""
LAUNCHER_LAST_CHECK_STATUS=""
LAUNCHER_LAST_CHECK_DETAIL=""
LAUNCHER_LAST_CHECK_ARTIFACT=""
LAUNCHER_DISPATCH_STARTED_NS=""
SMOKE_OUTPUT_ROOT=""
SMOKE_FAILURE_ROOT=""
LAUNCHER_INNER_LEGACY_OUTPUT_GLOB=".lca_smoke_in_progress.*"
LAUNCHER_INNER_BUILD_TMP_GLOB="boj28350_branch_3_solver-*.o"
LAUNCHER_INNER_BUILD_TMP_TMP_GLOB="boj28350_branch_3_solver-*.o.tmp"
LAUNCHER_STATUS_ROOT_DEFAULT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/smoke_latest_status"
LAUNCHER_STATUS_ROOT=""
LAUNCHER_STATUS_SUMMARY=""
LAUNCHER_STATUS_REPORT=""
LAUNCHER_STATUS_ITERATION_EVIDENCE=""
LAUNCHER_STATUS_RETRY_LOOP_CONTROL=""
LAUNCHER_STATUS_ARTIFACT_MANIFEST=""
LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST=""
LAUNCHER_STATUS_RUN_RECORD=""
LAUNCHER_STATUS_RUN_COMPARISON=""
LAUNCHER_RUN_HISTORY_ROOT=""
LAUNCHER_RUN_HISTORY_INDEX=""
LAUNCHER_RUN_ARCHIVE_ROOT=""
LAUNCHER_RUN_EXPORT_ROOT=""
LAUNCHER_RUN_EXPORT_ALIAS_ROOT=""
LAUNCHER_RUN_CONSOLE_LOG=""
LAUNCHER_RUN_STATUS_SUMMARY_PATH=""
LAUNCHER_RUN_STATUS_REPORT_PATH=""
LAUNCHER_RUN_STATUS_ITERATION_EVIDENCE_PATH=""
LAUNCHER_RUN_STATUS_RETRY_LOOP_CONTROL_PATH=""
LAUNCHER_RUN_STATUS_DIAGNOSTICS_PATH=""
LAUNCHER_RUN_STATUS_ARTIFACT_MANIFEST_PATH=""
LAUNCHER_RUN_STATUS_RUN_RECORD_PATH=""
LAUNCHER_RUN_STATUS_RUN_COMPARISON_PATH=""
LAUNCHER_RUN_PREFLIGHT_ROOT=""
LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT=""
LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH=""
LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT=""
LAUNCHER_RUN_ARTIFACT_MANIFEST=""
LAUNCHER_RUN_ID=""
LAUNCHER_RUN_STARTED_AT_UTC=""
LAUNCHER_RUN_FINISHED_AT_UTC=""
LAUNCHER_RUN_STARTED_SECONDS=-1
LAUNCHER_RUN_ELAPSED_SECONDS=0
LAUNCHER_RUN_COMPARISON_SUMMARY=""
LAUNCHER_RUN_COMPARISON_CHANGED_FIELDS=""
LAUNCHER_RUN_HISTORY_HAS_INDEX=0
LAUNCHER_RUN_HISTORY_MAX_SEQ=0
LAUNCHER_RECORDED_RUN_IDS=$'\n'
LAUNCHER_PREVIOUS_RUN_ID=""
LAUNCHER_PREVIOUS_RUN_ARCHIVE_ROOT=""
LAUNCHER_PREVIOUS_RUN_PUBLIC_STATUS=""
LAUNCHER_PREVIOUS_RUN_RESULT_FAMILY=""
LAUNCHER_PREVIOUS_RUN_NORMALIZED_OUTCOME=""
LAUNCHER_PREVIOUS_RUN_STAGE_LABEL=""
LAUNCHER_PREVIOUS_RUN_SOURCE_FAILURE_CASE=""
LAUNCHER_PREVIOUS_RUN_STATUS_SUMMARY_PATH=""
LAUNCHER_PREVIOUS_RUN_ITERATION_EVIDENCE_PATH=""
LAUNCHER_STATUS_OUTCOME=""
LAUNCHER_STATUS_SOURCE=""
LAUNCHER_STATUS_MESSAGE=""
LAUNCHER_STATUS_PUBLIC_STATUS=""
LAUNCHER_STATUS_RESULT_FAMILY=""
LAUNCHER_STATUS_NORMALIZED_RC=0
LAUNCHER_STATUS_RAW_RC=0
LAUNCHER_STATUS_SOURCE_ROOT=""
LAUNCHER_STATUS_SOURCE_SUMMARY=""
LAUNCHER_STATUS_SOURCE_REPORT=""
LAUNCHER_STATUS_SUITE_CONFIG_PATH=""
LAUNCHER_STATUS_SUITE_PLAN_PATH=""
LAUNCHER_STATUS_ENV_VALIDATION_REPORT=""
LAUNCHER_STATUS_ENV_MANIFEST_PATH=""
LAUNCHER_STATUS_ENV_SETUP_ENV_PATH=""
LAUNCHER_STATUS_ENV_BUILD_COMMAND_PATH=""
LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SNAPSHOT_PATH=""
LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SELECTION_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_MANIFEST_SELECTION_PATH=""
LAUNCHER_REPLAY_SUMMARY=""
LAUNCHER_REPLAY_CASE_TAG=""
LAUNCHER_REPLAY_STAGE=""
LAUNCHER_REPLAY_MODE=""
LAUNCHER_REPLAY_N=""
LAUNCHER_REPLAY_SEED=""
LAUNCHER_REPLAY_SHUFFLE_LABELS=""
LAUNCHER_REPLAY_SHUFFLE_QUERIES=""
LAUNCHER_REPLAY_TIMEOUT_S=""
LAUNCHER_REPLAY_FAILURE_ROOT=""
LAUNCHER_REPLAY_FAILURE_CASE_DIR=""
LAUNCHER_REPLAY_COMMANDS_PATH=""
LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH=""
LAUNCHER_REPLAY_RERUN_COMMAND_PATH=""
LAUNCHER_REPLAY_EXACT_SEED_PATH=""
LAUNCHER_REPLAY_EXACT_INPUT_PATH=""
LAUNCHER_REPLAY_EXACT_OUTPUT_PATH=""
LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH=""
LAUNCHER_REPLAY_INVOKED_COMMAND_PATH=""
LAUNCHER_REPLAY_ACTIVE_SCRIPT=""
LAUNCHER_REPLAY_COMMAND=""
LAUNCHER_SOURCE_FAILURE_KIND=""
LAUNCHER_SOURCE_FAILURE_ORIGIN=""
LAUNCHER_SOURCE_FAILURE_RETRYABLE=""
LAUNCHER_SOURCE_FAILURE_STAGE=""
LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS=""
LAUNCHER_SOURCE_FAILURE_REPORTING_WARNING=""
LAUNCHER_SOURCE_HELPER_STDOUT=""
LAUNCHER_SOURCE_HELPER_STDERR=""
LAUNCHER_SOURCE_HELPER_RESULT_JSON=""
LAUNCHER_SOURCE_CHECKER_RESULT_PATH=""
LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH=""
LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH=""
LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH=""
LAUNCHER_SOURCE_RETRY_LOG_PATH=""
LAUNCHER_SOURCE_ENV_VALIDATION_REPORT_PATH=""
LAUNCHER_SOURCE_RUNTIME_ENV_PATH=""
LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH=""
LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH=""
LAUNCHER_SOURCE_SETUP_ENV_PATH=""
LAUNCHER_SOURCE_BUILD_COMMAND_PATH=""
LAUNCHER_SOURCE_BUILD_STDOUT_PATH=""
LAUNCHER_SOURCE_BUILD_STDERR_PATH=""
LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH=""
LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH=""
LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH=""
LAUNCHER_SOURCE_SUITE_CONFIG_PATH=""
LAUNCHER_SOURCE_SUITE_PLAN_PATH=""
LAUNCHER_SOURCE_CHECKER_SCRIPT=""
LAUNCHER_SOURCE_SEED_REPRO_SCRIPT=""
LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT=""
LAUNCHER_SOURCE_GATE_SCRIPT=""
LAUNCHER_SOURCE_GATE_WRAPPER_STAGE=""
LAUNCHER_SOURCE_GATE_EXACT_FAILURE_STAGE=""
LAUNCHER_SOURCE_GATE_PRESET_PATH=""
LAUNCHER_SOURCE_GATE_PRESET_SNAPSHOT_PATH=""
LAUNCHER_SOURCE_GATE_CERTIFY_SUMMARY_PATH=""
LAUNCHER_SOURCE_GATE_CERTIFY_ROWS_PATH=""
LAUNCHER_SOURCE_GATE_CERTIFY_FAILURE_DETAILS_PATH=""
LAUNCHER_SOURCE_GATE_FAILURE_SOURCE=""
LAUNCHER_SOURCE_GATE_PRIMARY_FAILED_STAGE=""
LAUNCHER_SOURCE_GATE_FAILED_STAGES=""
LAUNCHER_SOURCE_GATE_FAILURE_REASONS=""
LAUNCHER_SOURCE_GATE_PRIMARY_STAGE_STATUS=""
LAUNCHER_SOURCE_GATE_PRIMARY_STAGE_CASES=""
LAUNCHER_SOURCE_GATE_PRIMARY_STAGE_TIMEOUTS=""
LAUNCHER_SOURCE_GATE_PRIMARY_STAGE_RE_WA=""
LAUNCHER_RETRY_LOOP_ACTION=""
LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND=""
LAUNCHER_RETRY_LOOP_LAUNCH_COMMAND=""
LAUNCHER_RETRY_LOOP_DIRECT_COMMAND=""
LAUNCHER_RETRY_LOOP_HINT=""
LAUNCHER_RETRY_LOOP_LOG_PATH=""
LAUNCHER_STATUS_WRITTEN=0
LAUNCHER_SKIP_FAILURE_BUNDLE=0
LAUNCHER_STATUS_SKIP_SHARED_ARCHIVE=0
LAUNCHER_DISPATCH_TIMEOUT_S_DEFAULT="600"
LAUNCHER_DISPATCH_KILL_GRACE_S="0.2"
LAUNCHER_DISPATCH_TIMEOUT_S=""
LAUNCHER_LOCK_WAIT_TIMEOUT_S_DEFAULT="15"
LAUNCHER_LOCK_WAIT_TIMEOUT_S=""
LAUNCHER_LOCK_RETRY_SLEEP_S="0.05"
LAUNCHER_DISPATCH_RESULT_PATH=""
LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED=0
LAUNCHER_DISPATCH_RAW_RC=0
LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL=0
LAUNCHER_ACTIVE_DISPATCH_MONITOR_PID=""
LAUNCHER_DISPATCH_STATE_PATH=""
RETRY_LOOP_LAUNCH_WRAPPER_REL=".ouroboros/launch_retry_loop.sh"
RETRY_LOOP_RUNNER_REL=".ouroboros/run_until_pass_progress40.sh"
RETRY_LOOP_SOLVER_SEED_REL=".ouroboros/seed_branch3_progress40_research_loop.yaml"
RETRY_LOOP_ANALYSIS_SEED_REL=".ouroboros/seed_branch3_failure_analysis.yaml"
RETRY_LOOP_LAUNCH_LOG_NAME="smoke_latest_status_retry_loop.log"
RETRY_LOOP_NEXT_GATE_COMMAND="./lca_strong_gate.sh"

fail() {
  LAUNCHER_FAILURE_MESSAGE="$*"
  LAUNCHER_FAILURE_RC="$SMOKE_EXIT_HARNESS_FAILURE"
  exit "$SMOKE_EXIT_HARNESS_FAILURE"
}

sanitize_shell_state() {
  unset CDPATH BASH_ENV ENV GLOBIGNORE
  unalias -a 2>/dev/null || true
  set +f
  shopt -u dotglob extglob failglob nocaseglob nullglob
}

enter_branch_root() {
  if ! cd "$BRANCH_ROOT"; then
    set_launcher_last_check "working_directory" "branch root" "unreachable" "$BRANCH_ROOT"
    fail "failed to enter branch root: $BRANCH_ROOT"
  fi
  set_launcher_last_check "working_directory" "branch root" "ok" "$BRANCH_ROOT"
}

normalize_existing_path() {
  local raw="$1"
  local label="$2"
  local normalized=""

  if ! normalized="$(
    python3 - "$raw" <<'PY' 2>/dev/null
from __future__ import annotations

import os
import sys

print(os.path.realpath(sys.argv[1]))
PY
  )"; then
    fail "failed to normalize ${label}: $raw"
  fi
  if [[ -z "$normalized" ]]; then
    fail "normalized ${label} resolved to an empty path: $raw"
  fi
  printf '%s\n' "$normalized"
}

normalize_branch_local_path() {
  local raw="$1"
  local label="$2"
  local normalized=""

  normalized="$(normalize_existing_path "$raw" "$label")"
  case "$normalized" in
    "$BRANCH_ROOT"|"$BRANCH_ROOT"/*)
      ;;
    *)
      fail "${label} escaped the branch root after normalization: $normalized"
      ;;
  esac
  printf '%s\n' "$normalized"
}

normalize_branch_artifact_path() {
  local raw="$1"
  local label="$2"
  local normalized=""

  if [[ -z "$raw" ]]; then
    printf '%s\n' ""
    return 0
  fi

  if ! normalized="$(
    python3 - "$BRANCH_ROOT" "$BRANCH_ARTIFACTS_ROOT" "$raw" <<'PY' 2>/dev/null
from __future__ import annotations

import sys
from pathlib import Path

branch_root = Path(sys.argv[1]).resolve()
artifacts_root = Path(sys.argv[2]).resolve()
raw = Path(sys.argv[3]).expanduser()

if raw.is_absolute():
    candidate = raw
else:
    parts = [part for part in raw.parts if part not in ("", ".")]
    artifact_rooted = False
    if len(parts) >= 2 and parts[0] == branch_root.name and parts[1] == artifacts_root.name:
        artifact_rooted = True
        parts = parts[1:]
    while parts and parts[0] == artifacts_root.name:
        artifact_rooted = True
        parts = parts[1:]
    candidate = artifacts_root.joinpath(*parts) if artifact_rooted else branch_root.joinpath(*parts)

print(candidate.resolve())
PY
  )"; then
    fail "failed to normalize ${label}: $raw"
  fi
  if [[ -z "$normalized" ]]; then
    fail "normalized ${label} resolved to an empty path: $raw"
  fi
  case "$normalized" in
    "$BRANCH_ARTIFACTS_ROOT"|"$BRANCH_ARTIFACTS_ROOT"/*)
      ;;
    *)
      fail "${label} escaped the branch-local artifacts root after normalization: $normalized"
      ;;
  esac
  printf '%s\n' "$normalized"
}

normalize_launcher_prerequisite_paths() {
  INNER_WRAPPER="$(normalize_branch_local_path "$INNER_WRAPPER" "outer smoke wrapper")"
  ARTIFACT_RESOLVER="$(normalize_branch_local_path "$ARTIFACT_RESOLVER" "artifact resolver")"
  BUILD_WRAPPER="$(normalize_branch_local_path "$BUILD_WRAPPER" "build wrapper")"
  RELEASE_ENV="$(normalize_branch_local_path "$RELEASE_ENV" "release env wrapper")"
  RUN_CASE_HELPER="$(normalize_branch_local_path "$RUN_CASE_HELPER" "branch-local case helper")"
  CHECKER_HELPER="$(normalize_branch_local_path "$CHECKER_HELPER" "branch-local validator")"
  BUILD_HELPER="$(normalize_branch_local_path "$BUILD_HELPER" "build helper")"
  RESUME_HELPER="$(normalize_branch_local_path "$RESUME_HELPER" "resume helper")"
  SMOKE_TARGET_WRAPPER="$(normalize_branch_local_path "$SMOKE_TARGET_WRAPPER" "smoke target wrapper")"
  SOURCE="$(normalize_branch_local_path "$SOURCE" "solver source")"
  SMOKE_CASES_SOURCE="$(normalize_branch_local_path "$SMOKE_CASES_SOURCE" "smoke case manifest")"
}

require_command() {
  local resolved=""
  if ! resolved="$(command -v "$1" 2>/dev/null)"; then
    set_launcher_last_check "command" "$1" "missing" "-"
    fail "missing required tool: $1"
  fi
  set_launcher_last_check "command" "$1" "ok" "$resolved"
}

require_build_compiler() {
  local candidate=""
  local resolved=""

  for candidate in clang++ g++ c++; do
    if resolved="$(command -v "$candidate" 2>/dev/null)"; then
      set_launcher_last_check "compiler" "$candidate" "ok" "$resolved"
      return 0
    fi
  done
  set_launcher_last_check "compiler" "clang++|g++|c++" "missing" "-"
  fail "missing required C++ compiler: expected one of clang++, g++, c++"
}

require_directory() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    set_launcher_last_check "directory" "$label" "missing" "$path"
    fail "missing ${label}: $path"
  fi
  if [[ ! -d "$path" ]]; then
    set_launcher_last_check "directory" "$label" "not_directory" "$path"
    fail "${label} is not a directory: $path"
  fi
  if [[ ! -r "$path" ]]; then
    set_launcher_last_check "directory" "$label" "not_readable" "$path"
    fail "${label} is not readable: $path"
  fi
  set_launcher_last_check "directory" "$label" "ok" "$path"
}

require_file() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    set_launcher_last_check "file" "$label" "missing" "$path"
    fail "missing ${label}: $path"
  fi
  if [[ ! -f "$path" ]]; then
    set_launcher_last_check "file" "$label" "not_regular_file" "$path"
    fail "${label} is not a regular file: $path"
  fi
  if [[ ! -r "$path" ]]; then
    set_launcher_last_check "file" "$label" "not_readable" "$path"
    fail "${label} is not readable: $path"
  fi
  set_launcher_last_check "file" "$label" "ok" "$path"
}

require_executable() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    set_launcher_last_check "executable" "$label" "missing" "$path"
    fail "missing executable ${label}: $path"
  fi
  if [[ ! -f "$path" ]]; then
    set_launcher_last_check "executable" "$label" "not_regular_file" "$path"
    fail "executable ${label} is not a regular file: $path"
  fi
  if [[ ! -r "$path" ]]; then
    set_launcher_last_check "executable" "$label" "not_readable" "$path"
    fail "executable ${label} is not readable: $path"
  fi
  if [[ ! -x "$path" ]]; then
    set_launcher_last_check "executable" "$label" "not_executable" "$path"
    fail "missing executable ${label}: $path"
  fi
  set_launcher_last_check "executable" "$label" "ok" "$path"
}

validate_launcher_repo_root_layout() {
  require_directory "$BRANCH_ROOT" "branch root directory"
  require_executable "$SELF_PATH" "launcher entrypoint"
  require_directory "$OUTER_SUITE_WRAPPERS_DIR" "outer suite wrappers directory"
  require_directory "$RESUME_WORKSPACE_DIR" "resume workspace directory"
}

check_shell_syntax() {
  local path="$1"
  local label="$2"
  local stderr_path=""
  local sanitized_label="${label// /_}"

  resolve_launcher_failure_root
  mkdir -p "$LAUNCHER_FAILURE_ROOT" || fail "failed to prepare launcher failure root: $LAUNCHER_FAILURE_ROOT"
  stderr_path="$LAUNCHER_FAILURE_ROOT/${sanitized_label//\//_}.stderr.txt"
  if "$BASH_BIN" -n "$path" >/dev/null 2>"$stderr_path"; then
    set_launcher_last_check "shell_syntax" "$label" "ok" "$path" "$stderr_path"
    return 0
  fi
  set_launcher_last_check "shell_syntax" "$label" "broken" "$path" "$stderr_path"
  fail "broken ${label}: $path"
}

check_python_entrypoint() {
  local path="$1"
  local label="$2"
  local stderr_path=""
  local sanitized_label="${label// /_}"

  resolve_launcher_failure_root
  mkdir -p "$LAUNCHER_FAILURE_ROOT" || fail "failed to prepare launcher failure root: $LAUNCHER_FAILURE_ROOT"
  stderr_path="$LAUNCHER_FAILURE_ROOT/${sanitized_label//\//_}.stderr.txt"
  if python3 - "$path" <<'PY' >/dev/null 2>"$stderr_path"
from __future__ import annotations

import runpy
import sys

runpy.run_path(sys.argv[1], run_name="__lca_smoke_launcher_preflight__")
PY
  then
    set_launcher_last_check "python_entrypoint" "$label" "ok" "$path" "$stderr_path"
    return
  fi
  set_launcher_last_check "python_entrypoint" "$label" "broken" "$path" "$stderr_path"
  fail "broken ${label}: $path"
}

remove_path_retry() {
  local target="$1"
  local attempt

  for attempt in 1 2 3 4 5; do
    if [[ ! -e "$target" ]]; then
      return 0
    fi
    rm -rf "$target" 2>/dev/null || true
    if [[ ! -e "$target" ]]; then
      return 0
    fi
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
    if [[ ! -e "$target" ]]; then
      return 0
    fi
    sleep 0.1
  done
  return 1
}

ensure_launcher_directory() {
  local path="$1"
  local label="$2"

  if [[ -L "$path" || ( -e "$path" && ! -d "$path" ) ]]; then
    remove_path_retry "$path" || return 1
  fi
  mkdir -p "$path" || return 1
}

prepare_launcher_artifact_namespace() {
  local branch_artifacts_root="$BRANCH_ROOT/artifacts"
  local smoke_artifacts_root="$branch_artifacts_root/lca_tree_stress_v5"

  ensure_launcher_directory "$branch_artifacts_root" "branch artifacts root" \
    || fail "failed to prepare branch artifacts root: $branch_artifacts_root"
  branch_artifacts_root="$(normalize_branch_local_path "$branch_artifacts_root" "branch artifacts root")"
  if [[ "$branch_artifacts_root" != "$BRANCH_ROOT/artifacts" ]]; then
    fail "branch artifacts root must stay pinned to $BRANCH_ROOT/artifacts (got: $branch_artifacts_root)"
  fi

  ensure_launcher_directory "$smoke_artifacts_root" "launcher smoke artifact namespace" \
    || fail "failed to prepare launcher smoke artifact namespace: $smoke_artifacts_root"
  smoke_artifacts_root="$(normalize_branch_local_path "$smoke_artifacts_root" "launcher smoke artifact namespace")"
  if [[ "$smoke_artifacts_root" != "$BRANCH_ROOT/artifacts/lca_tree_stress_v5" ]]; then
    fail "launcher smoke artifact namespace must stay pinned to $BRANCH_ROOT/artifacts/lca_tree_stress_v5 (got: $smoke_artifacts_root)"
  fi
}

allocate_launcher_run_archive_root() {
  local next_seq=0
  local candidate_run_id=""
  local candidate_root=""

  if [[ -z "$LAUNCHER_RUN_HISTORY_ROOT" ]]; then
    return 1
  fi

  scan_launcher_run_history_root || return 1
  next_seq=$(( LAUNCHER_RUN_HISTORY_MAX_SEQ + 1 ))
  while :; do
    printf -v candidate_run_id 'run.%06d' "$next_seq"
    candidate_root="$LAUNCHER_RUN_HISTORY_ROOT/$candidate_run_id"
    if mkdir "$candidate_root" 2>/dev/null; then
      LAUNCHER_RUN_ID="$candidate_run_id"
      LAUNCHER_RUN_ARCHIVE_ROOT="$candidate_root"
      LAUNCHER_RUN_EXPORT_ALIAS_ROOT="$LAUNCHER_RUN_EXPORT_ROOT/run-${candidate_run_id#run.}"
      return 0
    fi
    next_seq=$(( next_seq + 1 ))
    if (( next_seq > 999999 )); then
      return 1
    fi
  done
}

reset_launcher_run_history_scan_state() {
  LAUNCHER_RUN_HISTORY_HAS_INDEX=0
  LAUNCHER_RUN_HISTORY_MAX_SEQ=0
  LAUNCHER_RECORDED_RUN_IDS=$'\n'
}

launcher_run_id_is_recorded() {
  local run_id="$1"
  case "$LAUNCHER_RECORDED_RUN_IDS" in
    *$'\n'"$run_id"$'\n'*)
      return 0
      ;;
  esac
  return 1
}

launcher_run_archive_is_complete() {
  local archive_root="$1"
  [[ -d "$archive_root" && -f "$archive_root/summary.txt" && -f "$archive_root/run_record.json" ]]
}

launcher_run_id_to_alias_name() {
  local run_id="$1"
  printf 'run-%s\n' "${run_id#run.}"
}

launcher_run_alias_name_to_id() {
  local alias_name="$1"
  local alias_seq=0

  if [[ ! "$alias_name" =~ ^run-([0-9]+)$ ]]; then
    return 1
  fi
  alias_seq=$((10#${BASH_REMATCH[1]}))
  printf 'run.%06d\n' "$alias_seq"
}

scan_launcher_run_history_root() {
  local run_id=""
  local entry=""
  local entry_name=""
  local entry_seq=0
  local nullglob_was_on=0

  reset_launcher_run_history_scan_state
  if [[ -f "$LAUNCHER_RUN_HISTORY_INDEX" ]]; then
    LAUNCHER_RUN_HISTORY_HAS_INDEX=1
    while IFS=$'\t' read -r run_id _ || [[ -n "$run_id" ]]; do
      if [[ -z "$run_id" || "$run_id" == "run_id" ]]; then
        continue
      fi
      if [[ "$run_id" =~ ^run\.([0-9]+)$ ]]; then
        entry_seq=$((10#${BASH_REMATCH[1]}))
        LAUNCHER_RECORDED_RUN_IDS+="$run_id"$'\n'
        if (( entry_seq > LAUNCHER_RUN_HISTORY_MAX_SEQ )); then
          LAUNCHER_RUN_HISTORY_MAX_SEQ=$entry_seq
        fi
      fi
    done < "$LAUNCHER_RUN_HISTORY_INDEX"
  elif [[ -e "$LAUNCHER_RUN_HISTORY_INDEX" ]]; then
    remove_path_retry "$LAUNCHER_RUN_HISTORY_INDEX" || return 1
  fi

  if shopt -q nullglob; then
    nullglob_was_on=1
  fi
  shopt -s nullglob
  for entry in "$LAUNCHER_RUN_HISTORY_ROOT"/run.*; do
    [[ -e "$entry" || -L "$entry" ]] || continue
    entry_name="${entry##*/}"
    if [[ ! "$entry_name" =~ ^run\.([0-9]+)$ ]]; then
      continue
    fi
    entry_seq=$((10#${BASH_REMATCH[1]}))
    if (( LAUNCHER_RUN_HISTORY_HAS_INDEX == 1 )); then
      if launcher_run_id_is_recorded "$entry_name"; then
        continue
      fi
      remove_path_retry "$entry" || return 1
      continue
    fi
    if launcher_run_archive_is_complete "$entry"; then
      LAUNCHER_RECORDED_RUN_IDS+="$entry_name"$'\n'
      if (( entry_seq > LAUNCHER_RUN_HISTORY_MAX_SEQ )); then
        LAUNCHER_RUN_HISTORY_MAX_SEQ=$entry_seq
      fi
      continue
    fi
    remove_path_retry "$entry" || return 1
  done
  if (( nullglob_was_on == 0 )); then
    shopt -u nullglob
  fi
}

reconcile_launcher_run_export_alias() {
  local run_id="$1"
  local expected_archive="$LAUNCHER_RUN_HISTORY_ROOT/$run_id"
  local alias_path="$LAUNCHER_RUN_EXPORT_ROOT/$(launcher_run_id_to_alias_name "$run_id")"
  local resolved_alias=""

  if ! launcher_run_archive_is_complete "$expected_archive"; then
    if [[ -e "$alias_path" || -L "$alias_path" ]]; then
      remove_path_retry "$alias_path" || return 1
    fi
    return 0
  fi

  if [[ -L "$alias_path" ]]; then
    resolved_alias="$(normalize_existing_path "$alias_path" "launcher run export alias")"
    if [[ "$resolved_alias" == "$expected_archive" ]]; then
      return 0
    fi
  fi

  if [[ -e "$alias_path" || -L "$alias_path" ]]; then
    remove_path_retry "$alias_path" || return 1
  fi
  ln -s "$expected_archive" "$alias_path" || return 1
}

scan_launcher_run_export_root() {
  local entry=""
  local entry_name=""
  local run_id=""
  local nullglob_was_on=0

  if [[ -z "$LAUNCHER_RUN_EXPORT_ROOT" || ! -d "$LAUNCHER_RUN_EXPORT_ROOT" ]]; then
    return 0
  fi

  if shopt -q nullglob; then
    nullglob_was_on=1
  fi
  shopt -s nullglob
  for entry in "$LAUNCHER_RUN_EXPORT_ROOT"/run-*; do
    [[ -e "$entry" || -L "$entry" ]] || continue
    entry_name="${entry##*/}"
    if ! run_id="$(launcher_run_alias_name_to_id "$entry_name")"; then
      continue
    fi
    if ! launcher_run_id_is_recorded "$run_id"; then
      remove_path_retry "$entry" || return 1
      continue
    fi
    reconcile_launcher_run_export_alias "$run_id" || return 1
  done
  if (( nullglob_was_on == 0 )); then
    shopt -u nullglob
  fi

  while IFS= read -r run_id || [[ -n "$run_id" ]]; do
    if [[ -z "$run_id" ]]; then
      continue
    fi
    reconcile_launcher_run_export_alias "$run_id" || return 1
  done <<< "$LAUNCHER_RECORDED_RUN_IDS"
}

parse_positive_decimal_setting() {
  local raw="$1"
  local label="$2"

  python3 - "$raw" "$label" <<'PY'
from __future__ import annotations

import sys

raw = sys.argv[1]
label = sys.argv[2]
try:
    value = float(raw)
except ValueError:
    print(f"{label} must be a positive decimal (got: {raw})", file=sys.stderr)
    raise SystemExit(1)

if value <= 0.0:
    print(f"{label} must be > 0 (got: {raw})", file=sys.stderr)
    raise SystemExit(1)

print(raw)
PY
}

quote_command() {
  local quoted=""
  local word=""
  for word in "$@"; do
    printf -v quoted '%s%q ' "$quoted" "$word"
  done
  printf '%s\n' "${quoted% }"
}

capture_original_launcher_context() {
  local original_pwd=""
  local -a launcher_args=("$@")

  if [[ "${launcher_args[0]:-}" == "$LCA_SMOKE_LAUNCHER_REEXEC_ARG" ]]; then
    launcher_args=("${launcher_args[@]:1}")
    if [[ -n "${LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND:-}" && -n "${LCA_SMOKE_LAUNCHER_ORIGINAL_PWD:-}" ]]; then
      return
    fi
  fi

  export LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND
  if ((${#launcher_args[@]} > 0)); then
    LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND="$(quote_command "$0" "${launcher_args[@]}")"
  else
    LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND="$(quote_command "$0")"
  fi
  export LCA_SMOKE_LAUNCHER_ORIGINAL_PWD
  original_pwd="$(pwd -P 2>/dev/null || pwd)"
  LCA_SMOKE_LAUNCHER_ORIGINAL_PWD="$original_pwd"
}

set_launcher_last_check() {
  LAUNCHER_LAST_CHECK_KIND="$1"
  LAUNCHER_LAST_CHECK_LABEL="$2"
  LAUNCHER_LAST_CHECK_STATUS="$3"
  LAUNCHER_LAST_CHECK_DETAIL="$4"
  LAUNCHER_LAST_CHECK_ARTIFACT="${5:-}"
}

record_launcher_invocation() {
  LAUNCHER_INVOCATION_COMMAND="${LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND:-$(quote_command "$SELF_PATH" "$@")}"
  LAUNCHER_ORIGINAL_PWD="${LCA_SMOKE_LAUNCHER_ORIGINAL_PWD:-$(pwd -P 2>/dev/null || pwd)}"
  LAUNCHER_DISPATCH_COMMAND="$(quote_command "${BASH_BIN:-bash}" "$INNER_WRAPPER" "$@")"
}

set_launcher_failure_stage() {
  LAUNCHER_FAILURE_STAGE="$1"
}

resolve_launcher_failure_root() {
  local resolved_root="$LAUNCHER_FAILURE_ROOT_DEFAULT"
  local candidate=""

  if command -v python3 >/dev/null 2>&1 && [[ -f "$ARTIFACT_RESOLVER" && -r "$ARTIFACT_RESOLVER" ]]; then
    candidate="$(PYTHONDONTWRITEBYTECODE=1 PYTHONNOUSERSITE=1 python3 -B "$ARTIFACT_RESOLVER" --artifacts-root 2>/dev/null || true)"
    case "$candidate" in
      "$BRANCH_ROOT"/artifacts)
        resolved_root="$candidate/lca_tree_stress_v5/smoke_launcher_latest_failure"
        ;;
    esac
  fi

  LAUNCHER_FAILURE_ROOT="$resolved_root"
  LAUNCHER_FAILURE_SUMMARY="$LAUNCHER_FAILURE_ROOT/failure_summary.txt"
  LAUNCHER_FAILURE_REPORT="$LAUNCHER_FAILURE_ROOT/latest_failure_report.md"
  LAUNCHER_FAILURE_ENV_SNAPSHOT="$LAUNCHER_FAILURE_ROOT/launcher_env.txt"
  LAUNCHER_FAILURE_PREFLIGHT_MANIFEST="$LAUNCHER_FAILURE_ROOT/preflight_manifest.tsv"
  LAUNCHER_FAILURE_INVOCATION_COMMAND_PATH="$LAUNCHER_FAILURE_ROOT/invocation_command.txt"
  LAUNCHER_FAILURE_DISPATCH_COMMAND_PATH="$LAUNCHER_FAILURE_ROOT/dispatch_command.txt"
  LAUNCHER_FAILURE_RERUN_COMMAND_PATH="$LAUNCHER_FAILURE_ROOT/rerun_command.txt"
  LAUNCHER_FAILURE_REASON_PATH="$LAUNCHER_FAILURE_ROOT/failure_reason.txt"
  LAUNCHER_FAILURE_COMMAND_PATH="$LAUNCHER_FAILURE_ROOT/failing_command.txt"
  LAUNCHER_FAILURE_ARTIFACT_MANIFEST="$LAUNCHER_FAILURE_ROOT/artifact_manifest.tsv"
  LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION="$LAUNCHER_FAILURE_ROOT/smoke_manifest_selection.txt"
  LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR="$LAUNCHER_FAILURE_ROOT/smoke_manifest_check.stderr.txt"
}

resolve_launcher_status_root() {
  local resolved_artifacts_root="${ARTIFACTS_ROOT:-$BRANCH_ROOT/artifacts/lca_tree_stress_v5}"

  if [[ -n "${ARTIFACTS_ROOT:-}" ]]; then
    SMOKE_OUTPUT_ROOT="${SMOKE_OUTPUT_ROOT:-$resolved_artifacts_root/smoke}"
    SMOKE_FAILURE_ROOT="${SMOKE_FAILURE_ROOT:-$resolved_artifacts_root/smoke_latest_failure}"
  else
    SMOKE_OUTPUT_ROOT="${SMOKE_OUTPUT_ROOT:-$SMOKE_OUTPUT_ROOT_DEFAULT}"
    SMOKE_FAILURE_ROOT="${SMOKE_FAILURE_ROOT:-$SMOKE_FAILURE_ROOT_DEFAULT}"
  fi
  LAUNCHER_STATUS_ROOT="$resolved_artifacts_root/smoke_latest_status"
  LAUNCHER_STATUS_SUMMARY="$LAUNCHER_STATUS_ROOT/summary.txt"
  LAUNCHER_STATUS_REPORT="$LAUNCHER_STATUS_ROOT/latest_status_report.md"
  LAUNCHER_STATUS_ITERATION_EVIDENCE="$LAUNCHER_STATUS_ROOT/iteration_evidence.txt"
  LAUNCHER_STATUS_RETRY_LOOP_CONTROL="$LAUNCHER_STATUS_ROOT/retry_loop_control.json"
  LAUNCHER_STATUS_ARTIFACT_MANIFEST="$LAUNCHER_STATUS_ROOT/artifact_manifest.tsv"
  LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST="$LAUNCHER_STATUS_ROOT/diagnostics_manifest.tsv"
  LAUNCHER_RUN_HISTORY_ROOT="$resolved_artifacts_root/smoke_run_history"
  LAUNCHER_RUN_HISTORY_INDEX="$LAUNCHER_RUN_HISTORY_ROOT/history.tsv"
  LAUNCHER_STATUS_RUN_RECORD="$LAUNCHER_STATUS_ROOT/run_record.json"
  LAUNCHER_STATUS_RUN_COMPARISON="$LAUNCHER_STATUS_ROOT/run_comparison.json"
  LAUNCHER_RUN_EXPORT_ROOT="$resolved_artifacts_root/smoke_runs"
  LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SELECTION_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/environment_validation/smoke_manifest_selection.txt}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/summary.txt}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/status_report.md}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/failure_report.md}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/iteration_evidence.txt}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/retry_loop_control.json}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/diagnostics_manifest.tsv}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/standard_gap.json}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/run_record.json}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/run_comparison.json}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_MANIFEST_SELECTION_PATH="$LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SELECTION_PATH"
}

ensure_launcher_run_archive_root() {
  local effective_artifacts_root=""

  if [[ -n "$LAUNCHER_RUN_ARCHIVE_ROOT" && -d "$LAUNCHER_RUN_ARCHIVE_ROOT" ]]; then
    return 0
  fi

  resolve_launcher_status_root
  effective_artifacts_root="${ARTIFACTS_ROOT:-$BRANCH_ROOT/artifacts/lca_tree_stress_v5}"
  ensure_launcher_directory "$LAUNCHER_RUN_HISTORY_ROOT" "launcher run history root" || return 1
  ensure_launcher_directory "$LAUNCHER_RUN_EXPORT_ROOT" "launcher run export root" || return 1
  case "$LAUNCHER_RUN_HISTORY_ROOT" in
    "$effective_artifacts_root"|"$effective_artifacts_root"/*)
      ;;
    *)
      fail "launcher run history root escaped branch-local artifacts root: $LAUNCHER_RUN_HISTORY_ROOT"
      ;;
  esac
  case "$LAUNCHER_RUN_EXPORT_ROOT" in
    "$effective_artifacts_root"|"$effective_artifacts_root"/*)
      ;;
    *)
      fail "launcher run export root escaped branch-local artifacts root: $LAUNCHER_RUN_EXPORT_ROOT"
      ;;
  esac
  scan_launcher_run_history_root || return 1
  scan_launcher_run_export_root || return 1
  allocate_launcher_run_archive_root || return 1
  case "$LAUNCHER_RUN_ARCHIVE_ROOT" in
    "$effective_artifacts_root"|"$effective_artifacts_root"/*)
      ;;
    *)
      fail "launcher run archive root escaped branch-local artifacts root: $LAUNCHER_RUN_ARCHIVE_ROOT"
      ;;
  esac
  LAUNCHER_RUN_CONSOLE_LOG="$LAUNCHER_RUN_ARCHIVE_ROOT/console.stderr.txt"
  LAUNCHER_RUN_STATUS_SUMMARY_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/summary.txt"
  LAUNCHER_RUN_STATUS_REPORT_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/latest_status_report.md"
  LAUNCHER_RUN_STATUS_ITERATION_EVIDENCE_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/iteration_evidence.txt"
  LAUNCHER_RUN_STATUS_RETRY_LOOP_CONTROL_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/retry_loop_control.json"
  LAUNCHER_RUN_STATUS_DIAGNOSTICS_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/diagnostics_manifest.tsv"
  LAUNCHER_RUN_STATUS_ARTIFACT_MANIFEST_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/status_artifact_manifest.tsv"
  LAUNCHER_RUN_STATUS_RUN_RECORD_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/run_record.json"
  LAUNCHER_RUN_STATUS_RUN_COMPARISON_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/run_comparison.json"
  LAUNCHER_RUN_PREFLIGHT_ROOT="$LAUNCHER_RUN_ARCHIVE_ROOT/launcher_preflight"
  LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT="$LAUNCHER_RUN_ARCHIVE_ROOT/source_root_snapshot"
  LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/source_failure_snapshot_manifest.tsv"
  LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT="$LAUNCHER_RUN_ARCHIVE_ROOT/launcher_failure_root_snapshot"
  LAUNCHER_RUN_ARTIFACT_MANIFEST="$LAUNCHER_RUN_ARCHIVE_ROOT/artifact_manifest.tsv"
  LAUNCHER_RUN_STARTED_AT_UTC="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  LAUNCHER_RUN_FINISHED_AT_UTC=""
  LAUNCHER_RUN_STARTED_SECONDS=$SECONDS
  LAUNCHER_RUN_ELAPSED_SECONDS=0
  LAUNCHER_RUN_COMPARISON_SUMMARY=""
  LAUNCHER_RUN_COMPARISON_CHANGED_FIELDS=""
  : > "$LAUNCHER_RUN_CONSOLE_LOG" || return 1
}

record_launcher_console_line() {
  local line="$1"

  if [[ -z "$LAUNCHER_RUN_CONSOLE_LOG" ]]; then
    return 0
  fi
  printf '%s\n' "$line" >> "$LAUNCHER_RUN_CONSOLE_LOG" 2>/dev/null || true
}

emit_launcher_context_line() {
  local line="$1"

  printf '%s\n' "$line" >&2
  record_launcher_console_line "$line"
}

copy_launcher_run_path() {
  local source_path="$1"
  local target_path="$2"

  if [[ -z "$source_path" || ! -e "$source_path" ]]; then
    return 0
  fi
  if [[ -e "$target_path" ]]; then
    remove_path_retry "$target_path" || return 1
  fi
  if [[ -d "$source_path" ]]; then
    cp -R "$source_path" "$target_path" || return 1
  else
    cp "$source_path" "$target_path" || return 1
  fi
}

launcher_snapshot_equivalent_path() {
  local live_path="$1"
  local source_root="${LAUNCHER_STATUS_SOURCE_ROOT:-}"

  if [[ -z "$live_path" || -z "$source_root" || -z "$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT" ]]; then
    printf '%s\n' ""
    return 0
  fi

  case "$live_path" in
    "$source_root")
      printf '%s\n' "$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT"
      return 0
      ;;
    "$source_root"/*)
      printf '%s\n' "$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT/${live_path#"$source_root"/}"
      return 0
      ;;
  esac

  printf '%s\n' ""
}

launcher_snapshot_preferred_path() {
  local live_path="$1"
  local snapshot_path=""

  if [[ -z "$live_path" ]]; then
    printf '%s\n' ""
    return 0
  fi

  snapshot_path="$(launcher_snapshot_equivalent_path "$live_path")"
  if [[ -n "$snapshot_path" ]]; then
    printf '%s\n' "$snapshot_path"
    return 0
  fi

  printf '%s\n' "$live_path"
}

append_launcher_run_archived_artifact_row() {
  local label="$1"
  local live_path="$2"
  local snapshot_path=""

  if [[ -z "$label" || -z "$live_path" || -z "$LAUNCHER_RUN_ARTIFACT_MANIFEST" ]]; then
    return 0
  fi

  snapshot_path="$(launcher_snapshot_equivalent_path "$live_path")"
  if [[ -z "$snapshot_path" || ! -e "$snapshot_path" ]]; then
    return 0
  fi

  printf '%s\t%s\tcopy_of_%s\n' "$label" "$snapshot_path" "$live_path" >> "$LAUNCHER_RUN_ARTIFACT_MANIFEST"
}

append_launcher_run_source_snapshot_row() {
  local label="$1"
  local live_path="$2"
  local snapshot_path=""
  local exists="0"

  if [[ -z "$label" || -z "$live_path" || -z "$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH" ]]; then
    return 0
  fi

  snapshot_path="$(launcher_snapshot_equivalent_path "$live_path")"
  if [[ -n "$snapshot_path" && -e "$snapshot_path" ]]; then
    exists="1"
  fi

  printf '%s\t%s\t%s\t%s\n' "$label" "$live_path" "${snapshot_path:--}" "$exists" >> "$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH"
}

write_launcher_run_source_failure_snapshot_manifest() {
  local case_dir="${LAUNCHER_REPLAY_FAILURE_CASE_DIR:-}"

  if [[ -z "$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH" || -z "$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT" ]]; then
    return 0
  fi
  if [[ ! -d "$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT" ]]; then
    return 0
  fi

  {
    printf 'label\tlive_path\tsnapshot_path\texists\n'
  } > "$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH"

  append_launcher_run_source_snapshot_row "source_root" "$LAUNCHER_STATUS_SOURCE_ROOT"
  append_launcher_run_source_snapshot_row "source_summary" "$LAUNCHER_STATUS_SOURCE_SUMMARY"
  append_launcher_run_source_snapshot_row "source_report" "$LAUNCHER_STATUS_SOURCE_REPORT"
  append_launcher_run_source_snapshot_row "failure_root" "$LAUNCHER_REPLAY_FAILURE_ROOT"
  append_launcher_run_source_snapshot_row "failure_case_dir" "$case_dir"
  append_launcher_run_source_snapshot_row "commands" "$LAUNCHER_REPLAY_COMMANDS_PATH"
  append_launcher_run_source_snapshot_row "artifact_manifest" "$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH"
  append_launcher_run_source_snapshot_row "rerun_command" "$LAUNCHER_REPLAY_RERUN_COMMAND_PATH"
  append_launcher_run_source_snapshot_row "exact_seed" "$LAUNCHER_REPLAY_EXACT_SEED_PATH"
  append_launcher_run_source_snapshot_row "exact_input" "$LAUNCHER_REPLAY_EXACT_INPUT_PATH"
  append_launcher_run_source_snapshot_row "exact_output" "$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH"
  append_launcher_run_source_snapshot_row "expected_output" "$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH"
  append_launcher_run_source_snapshot_row "invoked_command" "$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH"
  append_launcher_run_source_snapshot_row "helper_stdout" "$LAUNCHER_SOURCE_HELPER_STDOUT"
  append_launcher_run_source_snapshot_row "helper_stderr" "$LAUNCHER_SOURCE_HELPER_STDERR"
  append_launcher_run_source_snapshot_row "helper_result_json" "$LAUNCHER_SOURCE_HELPER_RESULT_JSON"
  append_launcher_run_source_snapshot_row "checker_result" "$LAUNCHER_SOURCE_CHECKER_RESULT_PATH"
  append_launcher_run_source_snapshot_row "checker_replay_stdout" "$LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH"
  append_launcher_run_source_snapshot_row "checker_replay_stderr" "$LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH"
  append_launcher_run_source_snapshot_row "mismatch_summary" "$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH"
  append_launcher_run_source_snapshot_row "retry_log" "$LAUNCHER_SOURCE_RETRY_LOG_PATH"
  append_launcher_run_source_snapshot_row "environment_validation" "$LAUNCHER_SOURCE_ENV_VALIDATION_REPORT_PATH"
  append_launcher_run_source_snapshot_row "runtime_env" "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH"
  append_launcher_run_source_snapshot_row "runtime_env_exports" "$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH"
  append_launcher_run_source_snapshot_row "preflight_manifest" "$LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH"
  append_launcher_run_source_snapshot_row "setup_env" "$LAUNCHER_SOURCE_SETUP_ENV_PATH"
  append_launcher_run_source_snapshot_row "build_command" "$LAUNCHER_SOURCE_BUILD_COMMAND_PATH"
  append_launcher_run_source_snapshot_row "build_stdout" "$LAUNCHER_SOURCE_BUILD_STDOUT_PATH"
  append_launcher_run_source_snapshot_row "build_stderr" "$LAUNCHER_SOURCE_BUILD_STDERR_PATH"
  append_launcher_run_source_snapshot_row "structured_context" "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH"
  append_launcher_run_source_snapshot_row "manifest_snapshot" "$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH"
  append_launcher_run_source_snapshot_row "failed_case_row" "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH"
  append_launcher_run_source_snapshot_row "suite_config" "$LAUNCHER_SOURCE_SUITE_CONFIG_PATH"
  append_launcher_run_source_snapshot_row "suite_plan" "$LAUNCHER_SOURCE_SUITE_PLAN_PATH"
  append_launcher_run_source_snapshot_row "checker_script" "$LAUNCHER_SOURCE_CHECKER_SCRIPT"
  append_launcher_run_source_snapshot_row "seed_repro_script" "$LAUNCHER_SOURCE_SEED_REPRO_SCRIPT"
  append_launcher_run_source_snapshot_row "preserved_input_replay_script" "$LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT"
  append_launcher_run_source_snapshot_row "active_solver_replay_script" "$LAUNCHER_REPLAY_ACTIVE_SCRIPT"
  append_launcher_run_source_snapshot_row "case_input" "${case_dir:+$case_dir/in.txt}"
  append_launcher_run_source_snapshot_row "case_meta" "${case_dir:+$case_dir/meta.json}"
  append_launcher_run_source_snapshot_row "case_hidden_parent" "${case_dir:+$case_dir/hidden_parent.txt}"
  append_launcher_run_source_snapshot_row "case_output" "${case_dir:+$case_dir/out.txt}"
  append_launcher_run_source_snapshot_row "case_time" "${case_dir:+$case_dir/time.txt}"
  append_launcher_run_source_snapshot_row "case_solver_stderr" "${case_dir:+$case_dir/solver_stderr.txt}"
}

write_launcher_run_artifact_manifest() {
  {
    printf 'artifact\tpath\tprovenance\n'
    printf 'console_stderr\t%s\tlauncher_console_transcript\n' "$LAUNCHER_RUN_CONSOLE_LOG"
    printf 'dispatch_result\t%s\tlauncher_dispatch_result_snapshot\n' "$LAUNCHER_DISPATCH_RESULT_PATH"
    printf 'status_summary\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_STATUS_SUMMARY_PATH" "$LAUNCHER_STATUS_SUMMARY"
    printf 'status_report\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_STATUS_REPORT_PATH" "$LAUNCHER_STATUS_REPORT"
    printf 'status_iteration_evidence\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_STATUS_ITERATION_EVIDENCE_PATH" "$LAUNCHER_STATUS_ITERATION_EVIDENCE"
    printf 'status_retry_loop_control\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_STATUS_RETRY_LOOP_CONTROL_PATH" "$LAUNCHER_STATUS_RETRY_LOOP_CONTROL"
    printf 'status_run_record\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_STATUS_RUN_RECORD_PATH" "$LAUNCHER_STATUS_RUN_RECORD"
    printf 'status_run_comparison\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_STATUS_RUN_COMPARISON_PATH" "$LAUNCHER_STATUS_RUN_COMPARISON"
    printf 'status_diagnostics_manifest\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_STATUS_DIAGNOSTICS_PATH" "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST"
    printf 'status_artifact_manifest\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_STATUS_ARTIFACT_MANIFEST_PATH" "$LAUNCHER_STATUS_ARTIFACT_MANIFEST"
    if [[ -d "$LAUNCHER_RUN_PREFLIGHT_ROOT" ]]; then
      printf 'launcher_preflight\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_PREFLIGHT_ROOT" "$LAUNCHER_PREFLIGHT_ROOT"
    fi
    if [[ -e "$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT" ]]; then
      printf 'source_root_snapshot\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT" "$LAUNCHER_STATUS_SOURCE_ROOT"
    fi
    if [[ -f "$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH" ]]; then
      printf 'source_failure_snapshot_manifest\t%s\timmutable_snapshot_index_for_%s\n' "$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH" "$LAUNCHER_STATUS_SOURCE_ROOT"
    fi
    if [[ -e "$LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT" ]]; then
      printf 'launcher_failure_root_snapshot\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT" "$LAUNCHER_FAILURE_ROOT"
    fi
  } > "$LAUNCHER_RUN_ARTIFACT_MANIFEST"

  append_launcher_run_archived_artifact_row "source_summary" "$LAUNCHER_STATUS_SOURCE_SUMMARY"
  append_launcher_run_archived_artifact_row "source_report" "$LAUNCHER_STATUS_SOURCE_REPORT"
  append_launcher_run_archived_artifact_row "source_failure_root" "$LAUNCHER_REPLAY_FAILURE_ROOT"
  append_launcher_run_archived_artifact_row "source_failure_case_dir" "$LAUNCHER_REPLAY_FAILURE_CASE_DIR"
  append_launcher_run_archived_artifact_row "source_failure_commands" "$LAUNCHER_REPLAY_COMMANDS_PATH"
  append_launcher_run_archived_artifact_row "source_failure_artifact_manifest" "$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH"
  append_launcher_run_archived_artifact_row "source_failure_rerun_command" "$LAUNCHER_REPLAY_RERUN_COMMAND_PATH"
  append_launcher_run_archived_artifact_row "source_failure_exact_seed" "$LAUNCHER_REPLAY_EXACT_SEED_PATH"
  append_launcher_run_archived_artifact_row "source_failure_exact_input" "$LAUNCHER_REPLAY_EXACT_INPUT_PATH"
  append_launcher_run_archived_artifact_row "source_failure_exact_output" "$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH"
  append_launcher_run_archived_artifact_row "source_failure_expected_output" "$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH"
  append_launcher_run_archived_artifact_row "source_failure_invoked_command" "$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH"
  append_launcher_run_archived_artifact_row "source_failure_active_solver_replay_script" "$LAUNCHER_REPLAY_ACTIVE_SCRIPT"
  append_launcher_run_archived_artifact_row "source_failure_structured_context" "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH"
  append_launcher_run_archived_artifact_row "source_failure_runtime_env" "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH"
  append_launcher_run_archived_artifact_row "source_failure_runtime_env_exports" "$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH"
  append_launcher_run_archived_artifact_row "source_failure_failed_case_row" "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH"
  append_launcher_run_archived_artifact_row "source_failure_manifest_snapshot" "$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH"
  append_launcher_run_archived_artifact_row "source_failure_suite_config" "$LAUNCHER_SOURCE_SUITE_CONFIG_PATH"
  append_launcher_run_archived_artifact_row "source_failure_suite_plan" "$LAUNCHER_SOURCE_SUITE_PLAN_PATH"
  append_launcher_run_archived_artifact_row "source_failure_helper_stderr" "$LAUNCHER_SOURCE_HELPER_STDERR"
  append_launcher_run_archived_artifact_row "source_failure_mismatch_summary" "$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH"
  append_launcher_run_archived_artifact_row "source_failure_retry_log" "$LAUNCHER_SOURCE_RETRY_LOG_PATH"
}

archive_launcher_run_bundle() {
  ensure_launcher_run_archive_root || return 1
  cp "$LAUNCHER_STATUS_SUMMARY" "$LAUNCHER_RUN_STATUS_SUMMARY_PATH" || return 1
  cp "$LAUNCHER_STATUS_REPORT" "$LAUNCHER_RUN_STATUS_REPORT_PATH" || return 1
  cp "$LAUNCHER_STATUS_ITERATION_EVIDENCE" "$LAUNCHER_RUN_STATUS_ITERATION_EVIDENCE_PATH" || return 1
  cp "$LAUNCHER_STATUS_RETRY_LOOP_CONTROL" "$LAUNCHER_RUN_STATUS_RETRY_LOOP_CONTROL_PATH" || return 1
  cp "$LAUNCHER_STATUS_RUN_RECORD" "$LAUNCHER_RUN_STATUS_RUN_RECORD_PATH" || return 1
  cp "$LAUNCHER_STATUS_RUN_COMPARISON" "$LAUNCHER_RUN_STATUS_RUN_COMPARISON_PATH" || return 1
  cp "$LAUNCHER_STATUS_ARTIFACT_MANIFEST" "$LAUNCHER_RUN_STATUS_ARTIFACT_MANIFEST_PATH" || return 1
  if [[ -d "$LAUNCHER_PREFLIGHT_ROOT" ]]; then
    mkdir -p "$LAUNCHER_RUN_PREFLIGHT_ROOT" || return 1
    cp -R "$LAUNCHER_PREFLIGHT_ROOT"/. "$LAUNCHER_RUN_PREFLIGHT_ROOT"/ || return 1
  fi
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" != "pass" && -n "$LAUNCHER_STATUS_SOURCE_ROOT" ]]; then
    copy_launcher_run_path "$LAUNCHER_STATUS_SOURCE_ROOT" "$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT" || return 1
    write_launcher_run_source_failure_snapshot_manifest || return 1
  fi
  if [[ -n "$LAUNCHER_FAILURE_ROOT" && "$LAUNCHER_FAILURE_ROOT" != "${LAUNCHER_STATUS_SOURCE_ROOT:-}" ]]; then
    copy_launcher_run_path "$LAUNCHER_FAILURE_ROOT" "$LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT" || return 1
  fi
  write_launcher_run_artifact_manifest
  write_launcher_status_diagnostics_manifest
  cp "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST" "$LAUNCHER_RUN_STATUS_DIAGNOSTICS_PATH" || return 1
  if [[ -n "$LAUNCHER_RUN_EXPORT_ALIAS_ROOT" ]]; then
    if [[ -e "$LAUNCHER_RUN_EXPORT_ALIAS_ROOT" || -L "$LAUNCHER_RUN_EXPORT_ALIAS_ROOT" ]]; then
      remove_path_retry "$LAUNCHER_RUN_EXPORT_ALIAS_ROOT" || return 1
    fi
    ln -s "$LAUNCHER_RUN_ARCHIVE_ROOT" "$LAUNCHER_RUN_EXPORT_ALIAS_ROOT" || return 1
  fi
}

set_launcher_status() {
  LAUNCHER_STATUS_OUTCOME="$1"
  LAUNCHER_STATUS_NORMALIZED_RC="$2"
  LAUNCHER_STATUS_RAW_RC="$3"
  LAUNCHER_STATUS_SOURCE="$4"
  LAUNCHER_STATUS_MESSAGE="$5"
  LAUNCHER_STATUS_SOURCE_ROOT="${6:-}"
  LAUNCHER_STATUS_SOURCE_SUMMARY="${7:-}"
  LAUNCHER_STATUS_SOURCE_REPORT="${8:-}"
  case "$LAUNCHER_STATUS_OUTCOME" in
    pass)
      LAUNCHER_STATUS_PUBLIC_STATUS="PASS"
      LAUNCHER_STATUS_RESULT_FAMILY="none"
      ;;
    reproducible_solver_failure)
      LAUNCHER_STATUS_PUBLIC_STATUS="FAIL"
      LAUNCHER_STATUS_RESULT_FAMILY="solver"
      ;;
    reproducible_stress_gate_failure)
      LAUNCHER_STATUS_PUBLIC_STATUS="FAIL"
      LAUNCHER_STATUS_RESULT_FAMILY="stress_gate"
      ;;
    harness_infrastructure_failure)
      LAUNCHER_STATUS_PUBLIC_STATUS="FAIL"
      LAUNCHER_STATUS_RESULT_FAMILY="harness"
      ;;
    *)
      LAUNCHER_STATUS_PUBLIC_STATUS="FAIL"
      LAUNCHER_STATUS_RESULT_FAMILY="unknown"
      ;;
  esac
}

launcher_failure_partition_key() {
  case "${LAUNCHER_STATUS_RESULT_FAMILY:-unknown}" in
    none)
      printf 'pass\n'
      ;;
    harness)
      printf 'harness_setup\n'
      ;;
    solver|stress_gate)
      printf 'solver_test\n'
      ;;
    *)
      printf 'unknown\n'
      ;;
  esac
}

launcher_failure_partition_label() {
  local partition_key="${1:-$(launcher_failure_partition_key)}"

  case "$partition_key" in
    pass)
      printf 'pass\n'
      ;;
    harness_setup)
      printf 'harness/setup\n'
      ;;
    solver_test)
      printf 'solver/test\n'
      ;;
    *)
      printf 'unknown\n'
      ;;
  esac
}

clear_launcher_source_failure_details() {
  LAUNCHER_REPLAY_SUMMARY=""
  LAUNCHER_REPLAY_CASE_TAG=""
  LAUNCHER_REPLAY_STAGE=""
  LAUNCHER_REPLAY_MODE=""
  LAUNCHER_REPLAY_N=""
  LAUNCHER_REPLAY_SEED=""
  LAUNCHER_REPLAY_SHUFFLE_LABELS=""
  LAUNCHER_REPLAY_SHUFFLE_QUERIES=""
  LAUNCHER_REPLAY_TIMEOUT_S=""
  LAUNCHER_REPLAY_FAILURE_ROOT=""
  LAUNCHER_REPLAY_FAILURE_CASE_DIR=""
  LAUNCHER_REPLAY_COMMANDS_PATH=""
  LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH=""
  LAUNCHER_REPLAY_RERUN_COMMAND_PATH=""
  LAUNCHER_REPLAY_EXACT_SEED_PATH=""
  LAUNCHER_REPLAY_EXACT_INPUT_PATH=""
  LAUNCHER_REPLAY_EXACT_OUTPUT_PATH=""
  LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH=""
  LAUNCHER_REPLAY_INVOKED_COMMAND_PATH=""
  LAUNCHER_REPLAY_ACTIVE_SCRIPT=""
  LAUNCHER_REPLAY_COMMAND=""
  LAUNCHER_SOURCE_FAILURE_KIND=""
  LAUNCHER_SOURCE_FAILURE_ORIGIN=""
  LAUNCHER_SOURCE_FAILURE_RETRYABLE=""
  LAUNCHER_SOURCE_FAILURE_STAGE=""
  LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS=""
  LAUNCHER_SOURCE_FAILURE_REPORTING_WARNING=""
  LAUNCHER_SOURCE_HELPER_STDOUT=""
  LAUNCHER_SOURCE_HELPER_STDERR=""
  LAUNCHER_SOURCE_HELPER_RESULT_JSON=""
  LAUNCHER_SOURCE_CHECKER_RESULT_PATH=""
  LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH=""
  LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH=""
  LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH=""
  LAUNCHER_SOURCE_RETRY_LOG_PATH=""
  LAUNCHER_SOURCE_ENV_VALIDATION_REPORT_PATH=""
  LAUNCHER_SOURCE_RUNTIME_ENV_PATH=""
  LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH=""
  LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH=""
  LAUNCHER_SOURCE_SETUP_ENV_PATH=""
  LAUNCHER_SOURCE_BUILD_COMMAND_PATH=""
  LAUNCHER_SOURCE_BUILD_STDOUT_PATH=""
  LAUNCHER_SOURCE_BUILD_STDERR_PATH=""
  LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH=""
  LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH=""
  LAUNCHER_SOURCE_SUITE_CONFIG_PATH=""
  LAUNCHER_SOURCE_SUITE_PLAN_PATH=""
  LAUNCHER_SOURCE_CHECKER_SCRIPT=""
  LAUNCHER_SOURCE_SEED_REPRO_SCRIPT=""
  LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT=""
}

launcher_set_source_failure_path_if_missing() {
  local variable_name="$1"
  local candidate_raw="$2"
  local label="$3"
  local normalized=""

  if [[ -n "${!variable_name:-}" || -z "$candidate_raw" ]]; then
    return 0
  fi
  normalized="$(normalize_branch_artifact_path "$candidate_raw" "$label")"
  if [[ -e "$normalized" ]]; then
    printf -v "$variable_name" '%s' "$normalized"
  fi
}

launcher_read_first_nonempty_line() {
  local path="$1"

  if [[ -z "$path" || ! -f "$path" ]]; then
    printf '\n'
    return 0
  fi
  python3 - "$path" <<'PY'
from __future__ import annotations

import sys
from pathlib import Path

for raw_line in Path(sys.argv[1]).read_text(encoding="utf-8").splitlines():
    line = raw_line.strip()
    if line:
        print(line)
        break
PY
}

launcher_parse_failed_case_row() {
  local row_path="$1"

  if [[ -z "$row_path" || ! -f "$row_path" ]]; then
    return 0
  fi
  python3 - "$row_path" <<'PY'
from __future__ import annotations

import sys
from pathlib import Path

path = Path(sys.argv[1])
lines = [line.rstrip("\n") for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
if not lines:
    raise SystemExit(0)

default_header = ["stage", "mode", "n", "seed", "shuffle_labels", "shuffle_queries", "timeout_s"]
first_fields = lines[0].split("\t")
if len(lines) >= 2 and {"stage", "mode", "n", "seed"}.intersection(first_fields):
    header = first_fields
    row = lines[1].split("\t")
else:
    header = default_header
    row = first_fields

for key, value in zip(header, row):
    print(f"{key}={value}")
PY
}

launcher_backfill_source_failure_details() {
  local fallback_failure_root=""
  local line=""
  local replay_case_descriptor=""

  fallback_failure_root="$LAUNCHER_REPLAY_FAILURE_ROOT"
  if [[ -z "$fallback_failure_root" && -n "$SMOKE_FAILURE_ROOT" ]]; then
    fallback_failure_root="$(normalize_branch_artifact_path "$SMOKE_FAILURE_ROOT" "source failure root")"
  fi

  if [[ -n "$fallback_failure_root" && -d "$fallback_failure_root" ]]; then
    LAUNCHER_REPLAY_FAILURE_ROOT="$fallback_failure_root"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_ENV_VALIDATION_REPORT_PATH \
      "$fallback_failure_root/environment_validation.txt" \
      "source failure environment validation report path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH \
      "$fallback_failure_root/environment_validation/preflight_manifest.tsv" \
      "source failure preflight manifest path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_SETUP_ENV_PATH \
      "$fallback_failure_root/environment_validation/setup_env.txt" \
      "source failure setup env path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_BUILD_COMMAND_PATH \
      "$fallback_failure_root/environment_validation/build.command.txt" \
      "source failure build command path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH \
      "$fallback_failure_root/setup_build/preflight_manifest.tsv" \
      "source failure setup-build preflight manifest path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_SETUP_ENV_PATH \
      "$fallback_failure_root/setup_build/setup_env.txt" \
      "source failure setup-build env path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_BUILD_COMMAND_PATH \
      "$fallback_failure_root/setup_build/build.command.txt" \
      "source failure setup-build command path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_BUILD_STDOUT_PATH \
      "$fallback_failure_root/setup_build/build.stdout.txt" \
      "source failure build stdout path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_BUILD_STDERR_PATH \
      "$fallback_failure_root/setup_build/build.stderr.txt" \
      "source failure build stderr path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH \
      "$fallback_failure_root/failed_case_row.tsv" \
      "source failure failed case row path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH \
      "$fallback_failure_root/mismatch_summary.txt" \
      "source failure mismatch summary path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_HELPER_STDERR \
      "$fallback_failure_root/helper.stderr.txt" \
      "source failure helper stderr path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_REPLAY_COMMANDS_PATH \
      "$fallback_failure_root/commands.txt" \
      "source failure commands path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH \
      "$fallback_failure_root/artifact_manifest.tsv" \
      "source failure artifact manifest path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_REPLAY_RERUN_COMMAND_PATH \
      "$fallback_failure_root/rerun_command.txt" \
      "source failure rerun command path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH \
      "$fallback_failure_root/expected_output.txt" \
      "source failure expected output path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_REPLAY_INVOKED_COMMAND_PATH \
      "$fallback_failure_root/invoked_command.txt" \
      "source failure invoked command path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_REPLAY_ACTIVE_SCRIPT \
      "$fallback_failure_root/replay_active_manifest_case.sh" \
      "source failure active replay script"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_SUITE_PLAN_PATH \
      "$fallback_failure_root/suite_plan.tsv" \
      "source failure suite plan path"
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_SOURCE_SUITE_CONFIG_PATH \
      "$fallback_failure_root/suite_config.txt" \
      "source failure suite config path"
    if [[ -z "$LAUNCHER_REPLAY_FAILURE_CASE_DIR" && -n "$LAUNCHER_REPLAY_CASE_TAG" ]]; then
      launcher_set_source_failure_path_if_missing \
        LAUNCHER_REPLAY_FAILURE_CASE_DIR \
        "$fallback_failure_root/$LAUNCHER_REPLAY_CASE_TAG" \
        "source failure case dir"
    fi
  fi

  if [[ -n "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH" && -f "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH" ]]; then
    while IFS= read -r line || [[ -n "$line" ]]; do
      case "$line" in
        case_tag=*)
          if [[ -z "$LAUNCHER_REPLAY_CASE_TAG" ]]; then
            LAUNCHER_REPLAY_CASE_TAG="${line#*=}"
          fi
          ;;
        stage=*)
          if [[ -z "$LAUNCHER_REPLAY_STAGE" ]]; then
            LAUNCHER_REPLAY_STAGE="${line#*=}"
          fi
          ;;
        mode=*)
          if [[ -z "$LAUNCHER_REPLAY_MODE" ]]; then
            LAUNCHER_REPLAY_MODE="${line#*=}"
          fi
          ;;
        n=*)
          if [[ -z "$LAUNCHER_REPLAY_N" ]]; then
            LAUNCHER_REPLAY_N="${line#*=}"
          fi
          ;;
        seed=*)
          if [[ -z "$LAUNCHER_REPLAY_SEED" ]]; then
            LAUNCHER_REPLAY_SEED="${line#*=}"
          fi
          ;;
      esac
    done < <(launcher_parse_failed_case_row "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH")
  fi

  if [[ -z "$LAUNCHER_SOURCE_FAILURE_STAGE" && -n "$LAUNCHER_REPLAY_STAGE" ]]; then
    LAUNCHER_SOURCE_FAILURE_STAGE="$LAUNCHER_REPLAY_STAGE"
  fi

  if [[ -z "$LAUNCHER_REPLAY_FAILURE_CASE_DIR" && -n "$LAUNCHER_REPLAY_FAILURE_ROOT" && -n "$LAUNCHER_REPLAY_CASE_TAG" ]]; then
    launcher_set_source_failure_path_if_missing \
      LAUNCHER_REPLAY_FAILURE_CASE_DIR \
      "$LAUNCHER_REPLAY_FAILURE_ROOT/$LAUNCHER_REPLAY_CASE_TAG" \
      "source failure case dir"
  fi

  if [[ -z "$LAUNCHER_REPLAY_SUMMARY" && -n "$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH" ]]; then
    LAUNCHER_REPLAY_SUMMARY="$(launcher_read_first_nonempty_line "$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH")"
  fi
  if [[ -z "$LAUNCHER_REPLAY_SUMMARY" && -n "$LAUNCHER_SOURCE_HELPER_STDERR" ]]; then
    LAUNCHER_REPLAY_SUMMARY="$(launcher_read_first_nonempty_line "$LAUNCHER_SOURCE_HELPER_STDERR")"
  fi
  if [[ -z "$LAUNCHER_REPLAY_SUMMARY" ]]; then
    replay_case_descriptor="$(launcher_replay_case_descriptor)"
    if [[ -n "$replay_case_descriptor" ]]; then
      LAUNCHER_REPLAY_SUMMARY="preserved failing smoke case: $replay_case_descriptor"
    fi
  fi
}

capture_launcher_source_failure_details() {
  local source_summary="$1"
  local line=""
  local env_validation_report_path=""
  local env_validation_preflight_manifest_path=""
  local env_validation_setup_env_path=""
  local env_validation_build_command_path=""
  local setup_build_preflight_manifest_path=""
  local setup_build_setup_env_path=""
  local setup_build_build_command_path=""
  local setup_build_build_stdout_path=""
  local setup_build_build_stderr_path=""

  clear_launcher_source_failure_details
  if [[ -n "$source_summary" && -f "$source_summary" ]]; then
    while IFS= read -r line || [[ -n "$line" ]]; do
      line="${line#"${line%%[![:space:]]*}"}"
      case "$line" in
        failure_summary=*)
          LAUNCHER_REPLAY_SUMMARY="${line#*=}"
          ;;
        message=*)
          if [[ -z "$LAUNCHER_REPLAY_SUMMARY" ]]; then
            LAUNCHER_REPLAY_SUMMARY="${line#*=}"
          fi
          ;;
        failure_kind=*)
          LAUNCHER_SOURCE_FAILURE_KIND="${line#*=}"
          ;;
        failure_origin=*)
          LAUNCHER_SOURCE_FAILURE_ORIGIN="${line#*=}"
          ;;
        failure_retryable=*)
          LAUNCHER_SOURCE_FAILURE_RETRYABLE="${line#*=}"
          ;;
        failure_stage=*)
          LAUNCHER_SOURCE_FAILURE_STAGE="${line#*=}"
          ;;
        failure_reporting_status=*)
          LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS="${line#*=}"
          ;;
        failure_reporting_warning=*)
          LAUNCHER_SOURCE_FAILURE_REPORTING_WARNING="${line#*=}"
          ;;
        failed_case_tag=*)
          LAUNCHER_REPLAY_CASE_TAG="${line#*=}"
          ;;
        failed_stage=*)
          LAUNCHER_REPLAY_STAGE="${line#*=}"
          ;;
        failed_mode=*)
          LAUNCHER_REPLAY_MODE="${line#*=}"
          ;;
        failed_n=*)
          LAUNCHER_REPLAY_N="${line#*=}"
          ;;
        failed_seed=*)
          LAUNCHER_REPLAY_SEED="${line#*=}"
          ;;
        failure_root=*)
          LAUNCHER_REPLAY_FAILURE_ROOT="${line#*=}"
          ;;
        failure_case_dir=*)
          LAUNCHER_REPLAY_FAILURE_CASE_DIR="${line#*=}"
          ;;
        commands_path=*)
          LAUNCHER_REPLAY_COMMANDS_PATH="${line#*=}"
          ;;
        artifact_manifest_path=*)
          LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH="${line#*=}"
          ;;
        structured_context_path=*)
          LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH="${line#*=}"
          ;;
        rerun_command_path=*)
          LAUNCHER_REPLAY_RERUN_COMMAND_PATH="${line#*=}"
          ;;
        exact_seed_path=*)
          LAUNCHER_REPLAY_EXACT_SEED_PATH="${line#*=}"
          ;;
        exact_input_path=*)
          LAUNCHER_REPLAY_EXACT_INPUT_PATH="${line#*=}"
          ;;
        exact_output_path=*)
          LAUNCHER_REPLAY_EXACT_OUTPUT_PATH="${line#*=}"
          ;;
        expected_output_path=*)
          LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH="${line#*=}"
          ;;
        invoked_command_path=*)
          LAUNCHER_REPLAY_INVOKED_COMMAND_PATH="${line#*=}"
          ;;
        active_solver_replay_script=*)
          LAUNCHER_REPLAY_ACTIVE_SCRIPT="${line#*=}"
          ;;
        active_solver_replay_command=*)
          LAUNCHER_REPLAY_COMMAND="${line#*=}"
          ;;
        helper_stdout=*)
          LAUNCHER_SOURCE_HELPER_STDOUT="${line#*=}"
          ;;
        helper_stderr=*)
          LAUNCHER_SOURCE_HELPER_STDERR="${line#*=}"
          ;;
        helper_result_json=*)
          LAUNCHER_SOURCE_HELPER_RESULT_JSON="${line#*=}"
          ;;
        checker_result_path=*)
          LAUNCHER_SOURCE_CHECKER_RESULT_PATH="${line#*=}"
          ;;
        checker_replay_stdout_path=*)
          LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH="${line#*=}"
          ;;
        checker_replay_stderr_path=*)
          LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH="${line#*=}"
          ;;
        mismatch_summary_path=*)
          LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH="${line#*=}"
          ;;
        retry_log_path=*)
          LAUNCHER_SOURCE_RETRY_LOG_PATH="${line#*=}"
          ;;
        environment_validation_report_path=*)
          env_validation_report_path="${line#*=}"
          ;;
        environment_validation_preflight_manifest_path=*)
          env_validation_preflight_manifest_path="${line#*=}"
          ;;
        environment_validation_setup_env_path=*)
          env_validation_setup_env_path="${line#*=}"
          ;;
        environment_validation_build_command_path=*)
          env_validation_build_command_path="${line#*=}"
          ;;
        runtime_env_path=*)
          LAUNCHER_SOURCE_RUNTIME_ENV_PATH="${line#*=}"
          ;;
        runtime_env_exports_path=*)
          LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH="${line#*=}"
          ;;
        preflight_manifest=*)
          LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH="${line#*=}"
          ;;
        setup_env=*)
          LAUNCHER_SOURCE_SETUP_ENV_PATH="${line#*=}"
          ;;
        build_command=*)
          LAUNCHER_SOURCE_BUILD_COMMAND_PATH="${line#*=}"
          ;;
        build_stdout=*)
          LAUNCHER_SOURCE_BUILD_STDOUT_PATH="${line#*=}"
          ;;
        build_stderr=*)
          LAUNCHER_SOURCE_BUILD_STDERR_PATH="${line#*=}"
          ;;
        setup_build_preflight_manifest_path=*)
          setup_build_preflight_manifest_path="${line#*=}"
          ;;
        setup_build_setup_env_path=*)
          setup_build_setup_env_path="${line#*=}"
          ;;
        setup_build_build_command_path=*)
          setup_build_build_command_path="${line#*=}"
          ;;
        setup_build_build_stdout_path=*)
          setup_build_build_stdout_path="${line#*=}"
          ;;
        setup_build_build_stderr_path=*)
          setup_build_build_stderr_path="${line#*=}"
          ;;
        manifest_snapshot_path=*)
          LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH="${line#*=}"
          ;;
        failed_case_row_path=*)
          LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH="${line#*=}"
          ;;
        suite_config_path=*)
          LAUNCHER_SOURCE_SUITE_CONFIG_PATH="${line#*=}"
          ;;
        suite_plan_path=*)
          LAUNCHER_SOURCE_SUITE_PLAN_PATH="${line#*=}"
          ;;
        checker_script=*)
          LAUNCHER_SOURCE_CHECKER_SCRIPT="${line#*=}"
          ;;
        seed_repro_script=*)
          LAUNCHER_SOURCE_SEED_REPRO_SCRIPT="${line#*=}"
          ;;
        preserved_input_replay_script=*)
          LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT="${line#*=}"
          ;;
      esac
    done < "$source_summary"
  fi

  if [[ -z "$LAUNCHER_SOURCE_ENV_VALIDATION_REPORT_PATH" ]]; then
    LAUNCHER_SOURCE_ENV_VALIDATION_REPORT_PATH="$env_validation_report_path"
  fi
  if [[ -z "$LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH" ]]; then
    if [[ -n "$env_validation_preflight_manifest_path" ]]; then
      LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH="$env_validation_preflight_manifest_path"
    else
      LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH="$setup_build_preflight_manifest_path"
    fi
  fi
  if [[ -z "$LAUNCHER_SOURCE_SETUP_ENV_PATH" ]]; then
    if [[ -n "$env_validation_setup_env_path" ]]; then
      LAUNCHER_SOURCE_SETUP_ENV_PATH="$env_validation_setup_env_path"
    else
      LAUNCHER_SOURCE_SETUP_ENV_PATH="$setup_build_setup_env_path"
    fi
  fi
  if [[ -z "$LAUNCHER_SOURCE_BUILD_COMMAND_PATH" ]]; then
    if [[ -n "$env_validation_build_command_path" ]]; then
      LAUNCHER_SOURCE_BUILD_COMMAND_PATH="$env_validation_build_command_path"
    else
      LAUNCHER_SOURCE_BUILD_COMMAND_PATH="$setup_build_build_command_path"
    fi
  fi
  if [[ -z "$LAUNCHER_SOURCE_BUILD_STDOUT_PATH" ]]; then
    LAUNCHER_SOURCE_BUILD_STDOUT_PATH="$setup_build_build_stdout_path"
  fi
  if [[ -z "$LAUNCHER_SOURCE_BUILD_STDERR_PATH" ]]; then
    LAUNCHER_SOURCE_BUILD_STDERR_PATH="$setup_build_build_stderr_path"
  fi

  LAUNCHER_SOURCE_ENV_VALIDATION_REPORT_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_ENV_VALIDATION_REPORT_PATH" "source failure environment validation report path")"
  LAUNCHER_REPLAY_FAILURE_ROOT="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_FAILURE_ROOT" "source failure root")"
  LAUNCHER_REPLAY_FAILURE_CASE_DIR="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_FAILURE_CASE_DIR" "source failure case dir")"
  LAUNCHER_REPLAY_COMMANDS_PATH="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_COMMANDS_PATH" "source failure commands path")"
  LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH" "source failure artifact manifest path")"
  LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH" "source failure structured context path")"
  LAUNCHER_REPLAY_RERUN_COMMAND_PATH="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_RERUN_COMMAND_PATH" "source failure rerun command path")"
  LAUNCHER_REPLAY_EXACT_SEED_PATH="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_EXACT_SEED_PATH" "source failure exact seed path")"
  LAUNCHER_REPLAY_EXACT_INPUT_PATH="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_EXACT_INPUT_PATH" "source failure exact input path")"
  LAUNCHER_REPLAY_EXACT_OUTPUT_PATH="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH" "source failure exact output path")"
  LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH" "source failure expected output path")"
  LAUNCHER_REPLAY_INVOKED_COMMAND_PATH="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH" "source failure invoked command path")"
  LAUNCHER_REPLAY_ACTIVE_SCRIPT="$(normalize_branch_artifact_path "$LAUNCHER_REPLAY_ACTIVE_SCRIPT" "source failure active replay script")"
  LAUNCHER_SOURCE_HELPER_STDOUT="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_HELPER_STDOUT" "source failure helper stdout path")"
  LAUNCHER_SOURCE_HELPER_STDERR="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_HELPER_STDERR" "source failure helper stderr path")"
  LAUNCHER_SOURCE_HELPER_RESULT_JSON="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_HELPER_RESULT_JSON" "source failure helper result path")"
  LAUNCHER_SOURCE_CHECKER_RESULT_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_CHECKER_RESULT_PATH" "source failure checker result path")"
  LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH" "source failure checker replay stdout path")"
  LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH" "source failure checker replay stderr path")"
  LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH" "source failure mismatch summary path")"
  LAUNCHER_SOURCE_RETRY_LOG_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_RETRY_LOG_PATH" "source failure retry log path")"
  LAUNCHER_SOURCE_RUNTIME_ENV_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH" "source failure runtime env path")"
  LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH" "source failure runtime env exports path")"
  LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH" "source failure preflight manifest path")"
  LAUNCHER_SOURCE_SETUP_ENV_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_SETUP_ENV_PATH" "source failure setup env path")"
  LAUNCHER_SOURCE_BUILD_COMMAND_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_BUILD_COMMAND_PATH" "source failure build command path")"
  LAUNCHER_SOURCE_BUILD_STDOUT_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_BUILD_STDOUT_PATH" "source failure build stdout path")"
  LAUNCHER_SOURCE_BUILD_STDERR_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_BUILD_STDERR_PATH" "source failure build stderr path")"
  LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH" "source failure manifest snapshot path")"
  LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH" "source failure failed case row path")"
  LAUNCHER_SOURCE_SUITE_CONFIG_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_SUITE_CONFIG_PATH" "source failure suite config path")"
  LAUNCHER_SOURCE_SUITE_PLAN_PATH="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_SUITE_PLAN_PATH" "source failure suite plan path")"
  LAUNCHER_SOURCE_CHECKER_SCRIPT="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_CHECKER_SCRIPT" "source failure checker script path")"
  LAUNCHER_SOURCE_SEED_REPRO_SCRIPT="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_SEED_REPRO_SCRIPT" "source failure seed repro script path")"
  LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT="$(normalize_branch_artifact_path "$LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT" "source failure preserved-input replay script path")"
  launcher_backfill_source_failure_details

  if [[ -z "$LAUNCHER_REPLAY_COMMAND" && -n "$LAUNCHER_REPLAY_COMMANDS_PATH" && -f "$LAUNCHER_REPLAY_COMMANDS_PATH" ]]; then
    while IFS= read -r line || [[ -n "$line" ]]; do
      case "$line" in
        active_solver_replay_command=*)
          LAUNCHER_REPLAY_COMMAND="${line#*=}"
          break
          ;;
        seed_repro_command=*|preserved_input_command=*|executed_command=*)
          if [[ -z "$LAUNCHER_REPLAY_COMMAND" ]]; then
            LAUNCHER_REPLAY_COMMAND="${line#*=}"
          fi
          ;;
      esac
    done < "$LAUNCHER_REPLAY_COMMANDS_PATH"
  fi

  if [[ -n "$LAUNCHER_REPLAY_ACTIVE_SCRIPT" ]]; then
    # Keep the published status/report command aligned with the preserved
    # artifact path while keeping the replay command shell-safe for branch
    # roots that contain spaces.
    LAUNCHER_REPLAY_COMMAND="$(quote_command "${BASH_BIN:-bash}" "$LAUNCHER_REPLAY_ACTIVE_SCRIPT")"
  fi
}

launcher_source_failure_message() {
  local default_message="$1"

  if [[ -n "$LAUNCHER_REPLAY_SUMMARY" && -n "$LAUNCHER_SOURCE_FAILURE_STAGE" ]]; then
    printf 'inner smoke wrapper failed at stage %s: %s\n' "$LAUNCHER_SOURCE_FAILURE_STAGE" "$LAUNCHER_REPLAY_SUMMARY"
    return 0
  fi
  if [[ -n "$LAUNCHER_REPLAY_SUMMARY" ]]; then
    printf 'inner smoke wrapper failure detail: %s\n' "$LAUNCHER_REPLAY_SUMMARY"
    return 0
  fi
  printf '%s\n' "$default_message"
}

refresh_launcher_status_diagnostics_paths() {
  LAUNCHER_STATUS_SUITE_CONFIG_PATH="${LAUNCHER_SOURCE_SUITE_CONFIG_PATH:-${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/suite_config.txt}}"
  LAUNCHER_STATUS_SUITE_PLAN_PATH="${LAUNCHER_SOURCE_SUITE_PLAN_PATH:-${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/suite_plan.tsv}}"
  LAUNCHER_STATUS_ENV_VALIDATION_REPORT="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/environment_validation.txt}"
  LAUNCHER_STATUS_ENV_MANIFEST_PATH="${LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH:-${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/environment_validation/preflight_manifest.tsv}}"
  LAUNCHER_STATUS_ENV_SETUP_ENV_PATH="${LAUNCHER_SOURCE_SETUP_ENV_PATH:-${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/environment_validation/setup_env.txt}}"
  LAUNCHER_STATUS_ENV_BUILD_COMMAND_PATH="${LAUNCHER_SOURCE_BUILD_COMMAND_PATH:-${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/environment_validation/build.command.txt}}"
  LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SNAPSHOT_PATH="${LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH:-${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/environment_validation/smoke_cases.snapshot.tsv}}"
  LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SELECTION_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/environment_validation/smoke_manifest_selection.txt}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_MANIFEST_SELECTION_PATH="$LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SELECTION_PATH"
}

join_launcher_paths() {
  local joined=""
  local path=""

  for path in "$@"; do
    if [[ -z "$path" ]]; then
      continue
    fi
    joined="${joined:+$joined | }$path"
  done
  printf '%s\n' "$joined"
}

build_branch_root_shell_command() {
  local command_body="$1"
  local branch_root_q=""

  printf -v branch_root_q '%q' "$BRANCH_ROOT"
  printf 'cd %s && %s\n' "$branch_root_q" "$command_body"
}

launcher_acceptance_signal_status() {
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf 'PASS\n'
  else
    printf 'FAIL\n'
  fi
}

launcher_acceptance_failure_is_retryable() {
  case "${LAUNCHER_STATUS_RESULT_FAMILY:-unknown}" in
    solver|stress_gate)
      printf '1\n'
      ;;
    *)
      printf '0\n'
      ;;
  esac
}

launcher_acceptance_signal_summary() {
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf 'smoke accepted on this working tree; AC2 now has fresh same-worktree pass evidence for later gates\n'
  elif [[ "$(launcher_acceptance_failure_is_retryable)" == "1" ]]; then
    printf 'smoke did not satisfy AC2 on this working tree; keep the failure visible and do not treat this run as formal gate closure\n'
  else
    printf 'smoke did not satisfy AC2 because the launcher or smoke harness failed before acceptance evidence was trustworthy; keep later gates blocked and rerun smoke after repairing the wrapper path\n'
  fi
}

launcher_iteration_support_status() {
  printf 'ACTIONABLE\n'
}

launcher_iteration_support_next_step() {
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf 'gate_escalation\n'
  elif [[ "$(launcher_acceptance_failure_is_retryable)" == "1" ]]; then
    printf 'retry\n'
  else
    printf 'repair_then_retry\n'
  fi
}

launcher_iteration_support_summary() {
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf 'stable smoke status outputs are published; proceed to ./lca_strong_gate.sh on the same working tree\n'
  elif [[ "$(launcher_acceptance_failure_is_retryable)" == "1" ]]; then
    printf 'stable smoke status and retry artifacts are published despite the failed acceptance signal; inspect them and continue the next debugging pass\n'
  else
    printf 'stable smoke failure status is published, but this run stopped before acceptance-grade evidence was trustworthy; repair the smoke launcher or harness path, then rerun ./lca_smoke.sh before later gates\n'
  fi
}

launcher_gate_chain_status() {
  local ac_id="$1"

  case "$ac_id" in
    2)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'satisfied\n'
      else
        printf 'failed\n'
      fi
      ;;
    3)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'ready_to_run\n'
      else
        printf 'blocked_by_ac2\n'
      fi
      ;;
    4)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'pending_after_ac3\n'
      else
        printf 'blocked_by_ac2\n'
      fi
      ;;
    5)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'blocked_by_ac3\n'
      else
        printf 'blocked_by_ac2\n'
      fi
      ;;
    6)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'blocked_by_ac5\n'
      else
        printf 'blocked_by_ac2\n'
      fi
      ;;
    *)
      printf 'unknown\n'
      ;;
  esac
}

launcher_gate_chain_dependency() {
  local ac_id="$1"

  case "$ac_id" in
    2) printf 'none\n' ;;
    3) printf 'AC2\n' ;;
    4) printf 'AC3\n' ;;
    5) printf 'AC3\n' ;;
    6) printf 'AC5\n' ;;
    *) printf '\n' ;;
  esac
}

launcher_gate_chain_command() {
  local ac_id="$1"

  case "$ac_id" in
    2) printf './lca_smoke.sh\n' ;;
    3) printf './lca_strong_gate.sh\n' ;;
    4) printf './lca_strong_gate.sh && ./lca_strong_gate.sh\n' ;;
    5) printf './lca_boj3s_gate.sh\n' ;;
    6) printf './lca_boj3s_gate.sh && ./lca_boj3s_gate.sh\n' ;;
    *) printf '\n' ;;
  esac
}

launcher_gate_chain_summary() {
  local ac_id="$1"
  local message="${LAUNCHER_STATUS_MESSAGE:-launcher status was not initialized}"

  case "$ac_id" in
    2)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'fresh smoke evidence published for this working tree\n'
      else
        printf 'smoke is the active blocker: %s\n' "$message"
      fi
      ;;
    3)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'smoke is green; run the required prerequisite gate next on the same working tree\n'
      else
        printf 'strong gate is intentionally blocked until smoke publishes a fresh same-worktree pass\n'
      fi
      ;;
    4)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'strong-gate repeatability remains pending until AC3 records a fresh same-worktree pass\n'
      else
        printf 'strong-gate repeatability is intentionally blocked until AC3 has fresh same-worktree pass evidence\n'
      fi
      ;;
    5)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'boj3s gate remains blocked until AC3 records a fresh same-worktree pass\n'
      else
        printf 'boj3s gate is intentionally blocked until smoke and AC3 produce fresh same-worktree pass evidence\n'
      fi
      ;;
    6)
      if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
        printf 'boj3s repeatability remains blocked until AC5 records a fresh same-worktree pass\n'
      else
        printf 'boj3s repeatability is intentionally blocked until AC5 has fresh same-worktree pass evidence\n'
      fi
      ;;
    *)
      printf '\n'
      ;;
  esac
}

launcher_gate_chain_overview() {
  printf 'AC2=%s AC3=%s AC4=%s AC5=%s AC6=%s\n' \
    "$(launcher_gate_chain_status 2)" \
    "$(launcher_gate_chain_status 3)" \
    "$(launcher_gate_chain_status 4)" \
    "$(launcher_gate_chain_status 5)" \
    "$(launcher_gate_chain_status 6)"
}

launcher_command_control_mode() {
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf 'gate_escalation\n'
  elif [[ "$(launcher_acceptance_failure_is_retryable)" == "1" ]]; then
    printf 'acceptance_failure_retry\n'
  else
    printf 'smoke_repair_retry\n'
  fi
}

launcher_command_control_preferred_command_kind() {
  case "$(launcher_command_control_mode)" in
    gate_escalation)
      printf 'gate\n'
      ;;
    acceptance_failure_retry)
      printf 'retry_loop\n'
      ;;
    *)
      printf 'smoke_rerun\n'
      ;;
  esac
}

launcher_should_resume_retry_loop() {
  if [[ "$(launcher_command_control_mode)" == "acceptance_failure_retry" ]]; then
    printf '1\n'
  else
    printf '0\n'
  fi
}

launcher_should_retry_smoke_directly() {
  if [[ "$(launcher_command_control_mode)" == "smoke_repair_retry" ]]; then
    printf '1\n'
  else
    printf '0\n'
  fi
}

launcher_failure_is_terminal() {
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf '0\n'
  elif [[ -n "${LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND:-}" ]]; then
    printf '0\n'
  else
    printf '1\n'
  fi
}

launcher_next_gate_status() {
  launcher_gate_chain_status 3
}

launcher_next_gate_dependency() {
  launcher_gate_chain_dependency 3
}

launcher_next_gate_summary() {
  launcher_gate_chain_summary 3
}

launcher_gate_escalation_allowed() {
  if [[ "$(launcher_next_gate_status)" == "ready_to_run" ]]; then
    printf '1\n'
  else
    printf '0\n'
  fi
}

write_launcher_gate_chain_report() {
  local ac_id=""
  local status=""
  local dependency=""
  local command_text=""
  local summary=""

  for ac_id in 2 3 4 5 6; do
    status="$(launcher_gate_chain_status "$ac_id")"
    dependency="$(launcher_gate_chain_dependency "$ac_id")"
    command_text="$(launcher_gate_chain_command "$ac_id")"
    summary="$(launcher_gate_chain_summary "$ac_id")"
    echo "- AC${ac_id} (\`$command_text\`): status=\`$status\`; depends_on=\`$dependency\`; summary=\`$summary\`"
  done
}

refresh_launcher_retry_loop_control() {
  local launch_body=""
  local direct_body=""

  launch_body="$(quote_command \
    zsh \
    "$RETRY_LOOP_LAUNCH_WRAPPER_REL" \
    "$RETRY_LOOP_LAUNCH_LOG_NAME" \
    "$RETRY_LOOP_SOLVER_SEED_REL" \
    "$RETRY_LOOP_ANALYSIS_SEED_REL"
  )"
  direct_body="$(quote_command \
    zsh \
    "$RETRY_LOOP_RUNNER_REL" \
    "$RETRY_LOOP_SOLVER_SEED_REL" \
    "$RETRY_LOOP_ANALYSIS_SEED_REL"
  )"
  LAUNCHER_RETRY_LOOP_LAUNCH_COMMAND="$(build_branch_root_shell_command "$launch_body")"
  LAUNCHER_RETRY_LOOP_DIRECT_COMMAND="$(build_branch_root_shell_command "$direct_body")"
  LAUNCHER_RETRY_LOOP_LOG_PATH="$BRANCH_ROOT/artifacts/lca_tree_stress_v5/retry_loop/$RETRY_LOOP_LAUNCH_LOG_NAME"
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    LAUNCHER_RETRY_LOOP_ACTION="escalate_to_strong_gate"
    LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND="$RETRY_LOOP_NEXT_GATE_COMMAND"
    LAUNCHER_RETRY_LOOP_HINT="smoke passed; escalate to ./lca_strong_gate.sh on the same working tree for the next required gate"
    return 0
  fi
  if [[ "$(launcher_acceptance_failure_is_retryable)" == "1" ]]; then
    LAUNCHER_RETRY_LOOP_ACTION="resume_progress40_retry_loop"
    LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND="$LAUNCHER_RETRY_LOOP_LAUNCH_COMMAND"
    LAUNCHER_RETRY_LOOP_HINT="after inspecting the smoke failure handoff, relaunch the branch-local retry loop so the next solver iteration starts with fresh same-worktree artifacts"
    return 0
  fi
  LAUNCHER_RETRY_LOOP_ACTION="repair_and_rerun_smoke"
  LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND="./lca_smoke.sh"
  LAUNCHER_RETRY_LOOP_HINT="repair the smoke launcher or harness failure first, then rerun ./lca_smoke.sh before attempting later gates"
}

reset_launcher_previous_run_context() {
  LAUNCHER_PREVIOUS_RUN_ID=""
  LAUNCHER_PREVIOUS_RUN_ARCHIVE_ROOT=""
  LAUNCHER_PREVIOUS_RUN_PUBLIC_STATUS=""
  LAUNCHER_PREVIOUS_RUN_RESULT_FAMILY=""
  LAUNCHER_PREVIOUS_RUN_NORMALIZED_OUTCOME=""
  LAUNCHER_PREVIOUS_RUN_STAGE_LABEL=""
  LAUNCHER_PREVIOUS_RUN_SOURCE_FAILURE_CASE=""
  LAUNCHER_PREVIOUS_RUN_STATUS_SUMMARY_PATH=""
  LAUNCHER_PREVIOUS_RUN_ITERATION_EVIDENCE_PATH=""
}

load_launcher_previous_run_context() {
  local previous_row=""

  reset_launcher_previous_run_context
  if [[ -z "$LAUNCHER_RUN_HISTORY_INDEX" ]]; then
    return 0
  fi
  if [[ -e "$LAUNCHER_RUN_HISTORY_INDEX" && ! -f "$LAUNCHER_RUN_HISTORY_INDEX" ]]; then
    remove_path_retry "$LAUNCHER_RUN_HISTORY_INDEX" || return 1
    return 0
  fi
  if [[ ! -f "$LAUNCHER_RUN_HISTORY_INDEX" ]]; then
    return 0
  fi

previous_row="$(
    python3 - "$LAUNCHER_RUN_HISTORY_INDEX" "$LAUNCHER_RUN_ID" <<'PY'
from __future__ import annotations

import csv
import re
import sys
from pathlib import Path

history_path = Path(sys.argv[1])
current_run_id = sys.argv[2]
if not history_path.is_file():
    raise SystemExit(0)

rows = []
with history_path.open(encoding="utf-8", newline="") as handle:
    reader = csv.DictReader(handle, delimiter="\t")
    for row in reader:
        if row.get("run_id"):
            rows.append(row)

if not rows:
    raise SystemExit(0)

def run_sort_key(row: dict[str, str]) -> tuple[int, int | str]:
    run_id = row.get("run_id", "")
    match = re.fullmatch(r"run\.(\d+)", run_id)
    if match is None:
        return (1, run_id)
    return (0, int(match.group(1)))


canonical_rows: dict[str, dict[str, str]] = {}
for row in rows:
    canonical_rows[row["run_id"]] = row

ordered_rows = sorted(canonical_rows.values(), key=run_sort_key)
last = ordered_rows[-1]
if last.get("run_id") == current_run_id:
    if len(ordered_rows) < 2:
        raise SystemExit(0)
    last = ordered_rows[-2]
fields = [
    "run_id",
    "run_archive_root",
    "public_status",
    "result_family",
    "normalized_outcome",
    "stage_label",
    "source_failure_case",
    "status_summary_path",
    "iteration_evidence_path",
]
print("\x1f".join(last.get(field, "") for field in fields))
PY
  )"
  if [[ -z "$previous_row" ]]; then
    return 0
  fi
  IFS=$'\x1f' read -r \
    LAUNCHER_PREVIOUS_RUN_ID \
    LAUNCHER_PREVIOUS_RUN_ARCHIVE_ROOT \
    LAUNCHER_PREVIOUS_RUN_PUBLIC_STATUS \
    LAUNCHER_PREVIOUS_RUN_RESULT_FAMILY \
    LAUNCHER_PREVIOUS_RUN_NORMALIZED_OUTCOME \
    LAUNCHER_PREVIOUS_RUN_STAGE_LABEL \
    LAUNCHER_PREVIOUS_RUN_SOURCE_FAILURE_CASE \
    LAUNCHER_PREVIOUS_RUN_STATUS_SUMMARY_PATH \
    LAUNCHER_PREVIOUS_RUN_ITERATION_EVIDENCE_PATH <<< "$previous_row"
}

finalize_launcher_run_identity() {
  if [[ -z "$LAUNCHER_RUN_FINISHED_AT_UTC" ]]; then
    LAUNCHER_RUN_FINISHED_AT_UTC="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  fi
  if (( LAUNCHER_RUN_STARTED_SECONDS >= 0 )); then
    LAUNCHER_RUN_ELAPSED_SECONDS=$(( SECONDS - LAUNCHER_RUN_STARTED_SECONDS ))
  else
    LAUNCHER_RUN_ELAPSED_SECONDS=0
  fi
}

summarize_launcher_run_comparison() {
  local current_public_status="$1"
  local current_result_family="$2"
  local current_normalized_outcome="$3"
  local current_stage_label="$4"
  local current_failure_case="$5"
  local changed=()

  LAUNCHER_RUN_COMPARISON_CHANGED_FIELDS=""
  if [[ -z "$LAUNCHER_PREVIOUS_RUN_ID" ]]; then
    LAUNCHER_RUN_COMPARISON_SUMMARY="first recorded smoke run under $LAUNCHER_RUN_HISTORY_INDEX; no previous iteration is available for comparison"
    return 0
  fi

  if [[ "$LAUNCHER_PREVIOUS_RUN_PUBLIC_STATUS" != "$current_public_status" ]]; then
    changed+=("public_status")
  fi
  if [[ "$LAUNCHER_PREVIOUS_RUN_RESULT_FAMILY" != "$current_result_family" ]]; then
    changed+=("result_family")
  fi
  if [[ "$LAUNCHER_PREVIOUS_RUN_NORMALIZED_OUTCOME" != "$current_normalized_outcome" ]]; then
    changed+=("normalized_outcome")
  fi
  if [[ "$LAUNCHER_PREVIOUS_RUN_STAGE_LABEL" != "$current_stage_label" ]]; then
    changed+=("stage_label")
  fi
  if [[ "$LAUNCHER_PREVIOUS_RUN_SOURCE_FAILURE_CASE" != "$current_failure_case" ]]; then
    changed+=("source_failure_case")
  fi

  if (( ${#changed[@]} == 0 )); then
    LAUNCHER_RUN_COMPARISON_SUMMARY="same normalized outcome as previous run $LAUNCHER_PREVIOUS_RUN_ID ($LAUNCHER_PREVIOUS_RUN_NORMALIZED_OUTCOME at $LAUNCHER_PREVIOUS_RUN_STAGE_LABEL)"
    return 0
  fi

  LAUNCHER_RUN_COMPARISON_CHANGED_FIELDS="$(IFS=,; printf '%s' "${changed[*]}")"
  LAUNCHER_RUN_COMPARISON_SUMMARY="changed $LAUNCHER_RUN_COMPARISON_CHANGED_FIELDS relative to previous run $LAUNCHER_PREVIOUS_RUN_ID"
}

write_launcher_iteration_evidence() {
  local triage_scope="${1:-completed}"
  local triage_stage="${2:-completed}"
  local triage_primary_summary="${3:-}"
  local triage_primary_report="${4:-}"
  local triage_primary_manifest="${5:-}"
  local triage_first_artifacts="${6:-}"
  local triage_retry_command="${7:-}"
  local triage_retry_hint="${8:-}"
  local stage_label="$triage_scope:$triage_stage"

  {
    echo "script=./lca_smoke.sh"
    echo "run_id=$LAUNCHER_RUN_ID"
    echo "run_started_at_utc=$LAUNCHER_RUN_STARTED_AT_UTC"
    echo "run_finished_at_utc=$LAUNCHER_RUN_FINISHED_AT_UTC"
    echo "run_elapsed_seconds=$LAUNCHER_RUN_ELAPSED_SECONDS"
    echo "public_status=${LAUNCHER_STATUS_PUBLIC_STATUS:-FAIL}"
    echo "result_family=${LAUNCHER_STATUS_RESULT_FAMILY:-unknown}"
    echo "failure_partition=$(launcher_failure_partition_key)"
    echo "failure_partition_label=$(launcher_failure_partition_label)"
    echo "normalized_outcome=${LAUNCHER_STATUS_OUTCOME:-harness_infrastructure_failure}"
    echo "normalized_exit_code=${LAUNCHER_STATUS_NORMALIZED_RC:-$SMOKE_EXIT_HARNESS_FAILURE}"
    echo "raw_exit_code=${LAUNCHER_STATUS_RAW_RC:-${LAUNCHER_STATUS_NORMALIZED_RC:-$SMOKE_EXIT_HARNESS_FAILURE}}"
    echo "outcome_source=${LAUNCHER_STATUS_SOURCE:-launcher}"
    echo "acceptance_signal_status=$(launcher_acceptance_signal_status)"
    echo "acceptance_signal_summary=$(launcher_acceptance_signal_summary)"
    echo "iteration_support_status=$(launcher_iteration_support_status)"
    echo "iteration_support_next_step=$(launcher_iteration_support_next_step)"
    echo "iteration_support_summary=$(launcher_iteration_support_summary)"
    echo "run_history_index_path=$LAUNCHER_RUN_HISTORY_INDEX"
    echo "run_record_path=$LAUNCHER_STATUS_RUN_RECORD"
    echo "run_comparison_path=$LAUNCHER_STATUS_RUN_COMPARISON"
    echo "run_dispatch_result_path=$LAUNCHER_DISPATCH_RESULT_PATH"
    echo "run_comparison_summary=$LAUNCHER_RUN_COMPARISON_SUMMARY"
    echo "run_comparison_changed_fields=$LAUNCHER_RUN_COMPARISON_CHANGED_FIELDS"
    echo "previous_run_id=$LAUNCHER_PREVIOUS_RUN_ID"
    echo "previous_run_archive_root=$LAUNCHER_PREVIOUS_RUN_ARCHIVE_ROOT"
    echo "previous_run_public_status=$LAUNCHER_PREVIOUS_RUN_PUBLIC_STATUS"
    echo "previous_run_result_family=$LAUNCHER_PREVIOUS_RUN_RESULT_FAMILY"
    echo "previous_run_normalized_outcome=$LAUNCHER_PREVIOUS_RUN_NORMALIZED_OUTCOME"
    echo "previous_run_stage_label=$LAUNCHER_PREVIOUS_RUN_STAGE_LABEL"
    echo "previous_run_source_failure_case=$LAUNCHER_PREVIOUS_RUN_SOURCE_FAILURE_CASE"
    echo "previous_run_status_summary_path=$LAUNCHER_PREVIOUS_RUN_STATUS_SUMMARY_PATH"
    echo "previous_run_iteration_evidence_path=$LAUNCHER_PREVIOUS_RUN_ITERATION_EVIDENCE_PATH"
    echo "stage_scope=$triage_scope"
    echo "stage=$triage_stage"
    echo "stage_label=$stage_label"
    echo "primary_summary=$triage_primary_summary"
    echo "primary_report=$triage_primary_report"
    echo "primary_manifest=$triage_primary_manifest"
    echo "inspect_first=$triage_first_artifacts"
    echo "retry_command=$triage_retry_command"
    echo "retry_hint=$triage_retry_hint"
    echo "retry_loop_action=$LAUNCHER_RETRY_LOOP_ACTION"
    echo "retry_loop_preferred_command=$LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND"
    echo "retry_loop_launch_command=$LAUNCHER_RETRY_LOOP_LAUNCH_COMMAND"
    echo "retry_loop_direct_command=$LAUNCHER_RETRY_LOOP_DIRECT_COMMAND"
    echo "retry_loop_hint=$LAUNCHER_RETRY_LOOP_HINT"
    echo "retry_loop_log_path=$LAUNCHER_RETRY_LOOP_LOG_PATH"
    echo "retry_loop_control_path=$LAUNCHER_STATUS_RETRY_LOOP_CONTROL"
    echo "published_smoke_retry_loop_control_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH"
    echo "next_gate_command=$RETRY_LOOP_NEXT_GATE_COMMAND"
    echo "retry_loop_solver_seed_file=$RETRY_LOOP_SOLVER_SEED_REL"
    echo "retry_loop_analysis_seed_file=$RETRY_LOOP_ANALYSIS_SEED_REL"
    echo "status_summary_path=$LAUNCHER_STATUS_SUMMARY"
    echo "status_report_path=$LAUNCHER_STATUS_REPORT"
    echo "diagnostics_manifest_path=$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST"
    echo "run_archive_root=$LAUNCHER_RUN_ARCHIVE_ROOT"
    echo "run_archive_source_root_snapshot_path=$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT"
    echo "run_archive_source_failure_snapshot_manifest_path=$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH"
    echo "run_archive_summary_path=$LAUNCHER_RUN_STATUS_SUMMARY_PATH"
    echo "run_archive_status_report_path=$LAUNCHER_RUN_STATUS_REPORT_PATH"
    echo "run_archive_iteration_evidence_path=$LAUNCHER_RUN_STATUS_ITERATION_EVIDENCE_PATH"
    echo "run_archive_run_record_path=$LAUNCHER_RUN_STATUS_RUN_RECORD_PATH"
    echo "run_archive_run_comparison_path=$LAUNCHER_RUN_STATUS_RUN_COMPARISON_PATH"
    echo "run_console_stderr_path=$LAUNCHER_RUN_CONSOLE_LOG"
    echo "source_root=$LAUNCHER_STATUS_SOURCE_ROOT"
    echo "source_summary=$LAUNCHER_STATUS_SOURCE_SUMMARY"
    echo "source_report=$LAUNCHER_STATUS_SOURCE_REPORT"
    echo "published_smoke_summary_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH"
    echo "published_smoke_status_report_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH"
    echo "published_smoke_failure_report_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH"
    echo "published_smoke_iteration_evidence_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH"
    echo "published_smoke_diagnostics_manifest_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH"
    echo "published_smoke_standard_gap_json_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH"
    echo "published_smoke_run_record_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH"
    echo "published_smoke_run_comparison_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH"
  } > "$LAUNCHER_STATUS_ITERATION_EVIDENCE"
}

launcher_triage_stage_scope() {
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf 'completed\n'
    return 0
  fi
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "launcher" ]]; then
    case "${LAUNCHER_FAILURE_STAGE:-}" in
      dispatch|dispatch_monitor|dispatch_result_capture|status_normalization)
        printf 'launcher_dispatch\n'
        ;;
      *)
        printf 'launcher_pre_dispatch\n'
        ;;
    esac
    return 0
  fi
  if [[ -n "$LAUNCHER_REPLAY_CASE_TAG" ]]; then
    printf 'inner_wrapper_case\n'
    return 0
  fi
  if [[ -n "$LAUNCHER_SOURCE_FAILURE_STAGE" ]]; then
    printf 'inner_wrapper_setup_build\n'
    return 0
  fi
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "inner_wrapper" ]]; then
    printf 'inner_wrapper_bundle_validation\n'
    return 0
  fi
  printf 'unknown\n'
}

launcher_triage_stage_name() {
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf 'completed\n'
    return 0
  fi
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "launcher" ]]; then
    printf '%s\n' "${LAUNCHER_FAILURE_STAGE:-unknown}"
    return 0
  fi
  if [[ -n "$LAUNCHER_REPLAY_CASE_TAG" && -n "$LAUNCHER_REPLAY_STAGE" ]]; then
    printf '%s\n' "$LAUNCHER_REPLAY_STAGE"
    return 0
  fi
  if [[ -n "$LAUNCHER_SOURCE_FAILURE_STAGE" ]]; then
    printf '%s\n' "$LAUNCHER_SOURCE_FAILURE_STAGE"
    return 0
  fi
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "inner_wrapper" ]]; then
    printf 'bundle_validation\n'
    return 0
  fi
  printf 'unknown\n'
}

launcher_triage_primary_summary() {
  if [[ -n "${LAUNCHER_STATUS_SOURCE_SUMMARY:-}" ]]; then
    printf '%s\n' "$LAUNCHER_STATUS_SOURCE_SUMMARY"
    return 0
  fi
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "launcher" && -n "${LAUNCHER_FAILURE_SUMMARY:-}" ]]; then
    printf '%s\n' "$LAUNCHER_FAILURE_SUMMARY"
    return 0
  fi
  printf '%s\n' "$LAUNCHER_STATUS_SUMMARY"
}

launcher_triage_primary_report() {
  if [[ -n "${LAUNCHER_STATUS_SOURCE_REPORT:-}" ]]; then
    printf '%s\n' "$LAUNCHER_STATUS_SOURCE_REPORT"
    return 0
  fi
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "launcher" && -n "${LAUNCHER_FAILURE_REPORT:-}" ]]; then
    printf '%s\n' "$LAUNCHER_FAILURE_REPORT"
    return 0
  fi
  printf '%s\n' "$LAUNCHER_STATUS_REPORT"
}

launcher_triage_primary_manifest() {
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "launcher" && -n "${LAUNCHER_FAILURE_PREFLIGHT_MANIFEST:-}" ]]; then
    printf '%s\n' "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
    return 0
  fi
  printf '%s\n' "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST"
}

launcher_triage_first_artifacts() {
  local primary_summary=""
  local primary_report=""
  local primary_manifest=""

  primary_summary="$(launcher_triage_primary_summary)"
  primary_report="$(launcher_triage_primary_report)"
  primary_manifest="$(launcher_triage_primary_manifest)"
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "launcher" ]]; then
    join_launcher_paths \
      "$primary_summary" \
      "$primary_report" \
      "$primary_manifest" \
      "${LAUNCHER_FAILURE_REASON_PATH:-}"
    return 0
  fi
  join_launcher_paths \
    "$primary_summary" \
    "$primary_report" \
    "$primary_manifest" \
    "${LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH:-}" \
    "${LAUNCHER_REPLAY_COMMANDS_PATH:-}" \
    "${LAUNCHER_REPLAY_RERUN_COMMAND_PATH:-}" \
    "${LAUNCHER_REPLAY_ACTIVE_SCRIPT:-}" \
    "${LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH:-}" \
    "${LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH:-}" \
    "${LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH:-}" \
    "${LAUNCHER_REPLAY_EXACT_SEED_PATH:-}" \
    "${LAUNCHER_REPLAY_INVOKED_COMMAND_PATH:-}" \
    "${LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH:-}" \
    "${LAUNCHER_SOURCE_RETRY_LOG_PATH:-}" \
    "${LAUNCHER_SOURCE_HELPER_STDERR:-}" \
    "${LAUNCHER_SOURCE_ENV_VALIDATION_REPORT_PATH:-}" \
    "${LAUNCHER_SOURCE_BUILD_STDERR_PATH:-}" \
    "${LAUNCHER_SOURCE_SETUP_ENV_PATH:-}" \
    "${LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH:-}"
}

launcher_triage_retry_command() {
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf '\n'
    return 0
  fi
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "launcher" ]]; then
    printf './lca_smoke.sh\n'
    return 0
  fi
  if [[ -n "${LAUNCHER_REPLAY_COMMAND:-}" ]]; then
    printf '%s\n' "$LAUNCHER_REPLAY_COMMAND"
    return 0
  fi
  printf './lca_smoke.sh\n'
}

launcher_triage_retry_hint() {
  local triage_stage=""
  local triage_first_artifacts=""
  local retry_command=""

  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    printf 'smoke passed; no retry is needed before escalating beyond ./lca_smoke.sh\n'
    return 0
  fi
  triage_stage="$(launcher_triage_stage_name)"
  triage_first_artifacts="$(launcher_triage_first_artifacts)"
  retry_command="$(launcher_triage_retry_command)"
  if [[ "${LAUNCHER_STATUS_SOURCE:-}" == "launcher" ]]; then
    printf 'fix the launcher/preflight failure at stage %s, inspect %s, then rerun %s\n' \
      "$triage_stage" \
      "$triage_first_artifacts" \
      "$retry_command"
    return 0
  fi
  if [[ ("${LAUNCHER_STATUS_RESULT_FAMILY:-}" == "solver" || "${LAUNCHER_STATUS_RESULT_FAMILY:-}" == "stress_gate") && -n "${LAUNCHER_REPLAY_COMMAND:-}" ]]; then
    printf 'reproduce the preserved failing smoke case at stage %s via %s, inspect %s, then rerun ./lca_smoke.sh after the fix\n' \
      "$triage_stage" \
      "$retry_command" \
      "$triage_first_artifacts"
    return 0
  fi
  printf 'inspect %s for the smoke failure at stage %s, then rerun %s\n' \
    "$triage_first_artifacts" \
    "$triage_stage" \
    "$retry_command"
}

launcher_replay_case_descriptor() {
  local detail=""

  if [[ -n "$LAUNCHER_REPLAY_CASE_TAG" ]]; then
    detail="tag=$LAUNCHER_REPLAY_CASE_TAG"
  fi
  if [[ -n "$LAUNCHER_REPLAY_STAGE" ]]; then
    detail="${detail:+$detail }stage=$LAUNCHER_REPLAY_STAGE"
  fi
  if [[ -n "$LAUNCHER_REPLAY_MODE" ]]; then
    detail="${detail:+$detail }mode=$LAUNCHER_REPLAY_MODE"
  fi
  if [[ -n "$LAUNCHER_REPLAY_N" ]]; then
    detail="${detail:+$detail }n=$LAUNCHER_REPLAY_N"
  fi
  if [[ -n "$LAUNCHER_REPLAY_SEED" ]]; then
    detail="${detail:+$detail }seed=$LAUNCHER_REPLAY_SEED"
  fi
  printf '%s\n' "$detail"
}

launcher_replay_artifact_descriptor() {
  local detail=""

  if [[ -n "$LAUNCHER_REPLAY_FAILURE_ROOT" ]]; then
    detail="failure_root=$LAUNCHER_REPLAY_FAILURE_ROOT"
  fi
  if [[ -n "$LAUNCHER_REPLAY_FAILURE_CASE_DIR" ]]; then
    detail="${detail:+$detail }case_dir=$LAUNCHER_REPLAY_FAILURE_CASE_DIR"
  fi
  if [[ -n "$LAUNCHER_REPLAY_COMMANDS_PATH" ]]; then
    detail="${detail:+$detail }commands=$LAUNCHER_REPLAY_COMMANDS_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_RERUN_COMMAND_PATH" ]]; then
    detail="${detail:+$detail }rerun_commands=$LAUNCHER_REPLAY_RERUN_COMMAND_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH" ]]; then
    detail="${detail:+$detail }artifact_manifest=$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH" ]]; then
    detail="${detail:+$detail }failed_case_row=$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH" ]]; then
    detail="${detail:+$detail }manifest_snapshot=$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_SUITE_CONFIG_PATH" ]]; then
    detail="${detail:+$detail }suite_config=$LAUNCHER_SOURCE_SUITE_CONFIG_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_SUITE_PLAN_PATH" ]]; then
    detail="${detail:+$detail }suite_plan=$LAUNCHER_SOURCE_SUITE_PLAN_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_EXACT_SEED_PATH" ]]; then
    detail="${detail:+$detail }exact_seed=$LAUNCHER_REPLAY_EXACT_SEED_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_EXACT_INPUT_PATH" ]]; then
    detail="${detail:+$detail }exact_input=$LAUNCHER_REPLAY_EXACT_INPUT_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH" ]]; then
    detail="${detail:+$detail }exact_output=$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH" ]]; then
    detail="${detail:+$detail }expected_output=$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH" ]]; then
    detail="${detail:+$detail }invoked_command=$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_ACTIVE_SCRIPT" ]]; then
    detail="${detail:+$detail }active_solver_replay_script=$LAUNCHER_REPLAY_ACTIVE_SCRIPT"
  fi
  if [[ -n "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH" ]]; then
    detail="${detail:+$detail }runtime_env=$LAUNCHER_SOURCE_RUNTIME_ENV_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH" ]]; then
    detail="${detail:+$detail }runtime_env_exports=$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH" ]]; then
    detail="${detail:+$detail }structured_context=$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH"
  fi
  printf '%s\n' "$detail"
}

append_launcher_status_diagnostic_entry() {
  local artifact="$1"
  local path="$2"
  local note="$3"
  local exists="0"

  if [[ -z "$path" ]]; then
    return 0
  fi
  if [[ -e "$path" ]]; then
    exists="1"
  fi
  printf '%s\t%s\t%s\t%s\n' "$artifact" "$path" "$exists" "$note" >> "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST"
}

write_launcher_status_diagnostics_manifest() {
  printf 'artifact\tpath\texists\tnote\n' > "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST"
  append_launcher_status_diagnostic_entry "status_summary" "$LAUNCHER_STATUS_SUMMARY" "stable machine-readable status summary"
  append_launcher_status_diagnostic_entry "status_report" "$LAUNCHER_STATUS_REPORT" "human-readable status report"
  append_launcher_status_diagnostic_entry "status_iteration_evidence" "$LAUNCHER_STATUS_ITERATION_EVIDENCE" "stable key-value iteration handoff with stage labels and retry paths"
  append_launcher_status_diagnostic_entry "status_retry_loop_control" "$LAUNCHER_STATUS_RETRY_LOOP_CONTROL" "machine-readable retry-loop and next-gate continuation control"
  append_launcher_status_diagnostic_entry "status_run_record" "$LAUNCHER_STATUS_RUN_RECORD" "machine-readable record for the current smoke invocation"
  append_launcher_status_diagnostic_entry "status_run_comparison" "$LAUNCHER_STATUS_RUN_COMPARISON" "machine-readable comparison between the current run and the previous archived run"
  append_launcher_status_diagnostic_entry "run_history_root" "$LAUNCHER_RUN_HISTORY_ROOT" "parent directory containing per-run smoke archives"
  append_launcher_status_diagnostic_entry "run_history_index" "$LAUNCHER_RUN_HISTORY_INDEX" "append-only ledger of smoke outcomes across iterations"
  append_launcher_status_diagnostic_entry "run_archive_root" "$LAUNCHER_RUN_ARCHIVE_ROOT" "per-run immutable archive for this launcher invocation"
  append_launcher_status_diagnostic_entry "run_console_stderr" "$LAUNCHER_RUN_CONSOLE_LOG" "launcher stderr transcript captured for this invocation"
  append_launcher_status_diagnostic_entry "run_dispatch_result" "$LAUNCHER_DISPATCH_RESULT_PATH" "launcher dispatch result snapshot captured for this invocation"
  append_launcher_status_diagnostic_entry "run_archive_manifest" "$LAUNCHER_RUN_ARTIFACT_MANIFEST" "inventory of copied artifacts preserved for this invocation"
  append_launcher_status_diagnostic_entry "smoke_output_root" "$SMOKE_OUTPUT_ROOT" "published smoke output root for passing runs"
  append_launcher_status_diagnostic_entry "smoke_failure_root" "$SMOKE_FAILURE_ROOT" "stable preserved smoke failure root"
  append_launcher_status_diagnostic_entry "source_root" "$LAUNCHER_STATUS_SOURCE_ROOT" "root bundle behind the normalized outcome"
  append_launcher_status_diagnostic_entry "source_summary" "$LAUNCHER_STATUS_SOURCE_SUMMARY" "inner-wrapper source summary when available"
  append_launcher_status_diagnostic_entry "source_report" "$LAUNCHER_STATUS_SOURCE_REPORT" "inner-wrapper source report when available"
  append_launcher_status_diagnostic_entry "smoke_suite_config" "$LAUNCHER_STATUS_SUITE_CONFIG_PATH" "suite policy and retry/build configuration"
  append_launcher_status_diagnostic_entry "smoke_suite_plan" "$LAUNCHER_STATUS_SUITE_PLAN_PATH" "deterministic smoke case plan"
  append_launcher_status_diagnostic_entry "smoke_environment_validation" "$LAUNCHER_STATUS_ENV_VALIDATION_REPORT" "environment validation report from the smoke run"
  append_launcher_status_diagnostic_entry "smoke_environment_preflight_manifest" "$LAUNCHER_STATUS_ENV_MANIFEST_PATH" "setup preflight manifest from the smoke run"
  append_launcher_status_diagnostic_entry "smoke_environment_setup_env" "$LAUNCHER_STATUS_ENV_SETUP_ENV_PATH" "setup environment snapshot from the smoke run"
  append_launcher_status_diagnostic_entry "smoke_environment_build_command" "$LAUNCHER_STATUS_ENV_BUILD_COMMAND_PATH" "isolated smoke build command snapshot"
  append_launcher_status_diagnostic_entry "smoke_manifest_snapshot" "$LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SNAPSHOT_PATH" "frozen smoke manifest snapshot for this run"
  append_launcher_status_diagnostic_entry "published_smoke_summary" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH" "stable smoke-root summary for downstream standard-gap capture"
  append_launcher_status_diagnostic_entry "published_smoke_status_report" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH" "stable smoke-root human-readable report"
  append_launcher_status_diagnostic_entry "published_smoke_failure_report" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH" "stable smoke-root failure report for failed smoke runs"
  append_launcher_status_diagnostic_entry "published_smoke_iteration_evidence" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH" "stable smoke-root copy of the iteration handoff"
  append_launcher_status_diagnostic_entry "published_smoke_retry_loop_control" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH" "stable smoke-root copy of the retry-loop continuation control"
  append_launcher_status_diagnostic_entry "published_smoke_diagnostics_manifest" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH" "stable smoke-root diagnostics manifest mirror"
  append_launcher_status_diagnostic_entry "published_smoke_standard_gap_json" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH" "machine-readable explanation of how this run relates to lca_tree_stress_v5"
  append_launcher_status_diagnostic_entry "published_smoke_run_record" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH" "stable smoke-root copy of the current run record"
  append_launcher_status_diagnostic_entry "published_smoke_run_comparison" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH" "stable smoke-root copy of the current-vs-previous run comparison"
  append_launcher_status_diagnostic_entry "source_failure_commands" "$LAUNCHER_REPLAY_COMMANDS_PATH" "recorded helper and replay commands for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_rerun_commands" "$LAUNCHER_REPLAY_RERUN_COMMAND_PATH" "single-file rerun command bundle"
  append_launcher_status_diagnostic_entry "source_failure_helper_stdout" "$LAUNCHER_SOURCE_HELPER_STDOUT" "preserved helper stdout from the failing case"
  append_launcher_status_diagnostic_entry "source_failure_helper_stderr" "$LAUNCHER_SOURCE_HELPER_STDERR" "preserved helper stderr from the failing case"
  append_launcher_status_diagnostic_entry "source_failure_helper_result_json" "$LAUNCHER_SOURCE_HELPER_RESULT_JSON" "preserved helper result json from the failing case"
  append_launcher_status_diagnostic_entry "source_failure_checker_result" "$LAUNCHER_SOURCE_CHECKER_RESULT_PATH" "direct checker replay result for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_checker_replay_stdout" "$LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH" "checker replay stdout for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_checker_replay_stderr" "$LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH" "checker replay stderr for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_mismatch_summary" "$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH" "parsed mismatch summary for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_retry_log" "$LAUNCHER_SOURCE_RETRY_LOG_PATH" "retry ledger leading into the preserved failure"
  append_launcher_status_diagnostic_entry "source_failure_runtime_env" "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH" "runtime environment snapshot for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_runtime_env_exports" "$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH" "shell exports for reproducing the failing case env"
  append_launcher_status_diagnostic_entry "source_failure_preflight_manifest" "$LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH" "setup/build preflight manifest from the failing smoke run"
  append_launcher_status_diagnostic_entry "source_failure_setup_env" "$LAUNCHER_SOURCE_SETUP_ENV_PATH" "setup/build environment snapshot from the failing smoke run"
  append_launcher_status_diagnostic_entry "source_failure_build_command" "$LAUNCHER_SOURCE_BUILD_COMMAND_PATH" "setup/build command snapshot from the failing smoke run"
  append_launcher_status_diagnostic_entry "source_failure_build_stdout" "$LAUNCHER_SOURCE_BUILD_STDOUT_PATH" "setup/build stdout from the failing smoke run"
  append_launcher_status_diagnostic_entry "source_failure_build_stderr" "$LAUNCHER_SOURCE_BUILD_STDERR_PATH" "setup/build stderr from the failing smoke run"
  append_launcher_status_diagnostic_entry "source_failure_structured_context" "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH" "machine-readable failure handoff for the next retry iteration"
  append_launcher_status_diagnostic_entry "source_failure_failed_case_row" "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH" "frozen manifest row for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_manifest_snapshot" "$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH" "frozen smoke manifest copied into the failure bundle"
  append_launcher_status_diagnostic_entry "source_failure_checker_script" "$LAUNCHER_SOURCE_CHECKER_SCRIPT" "recheck preserved output script"
  append_launcher_status_diagnostic_entry "source_failure_seed_repro_script" "$LAUNCHER_SOURCE_SEED_REPRO_SCRIPT" "seed-based repro helper script"
  append_launcher_status_diagnostic_entry "source_failure_preserved_input_replay_script" "$LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT" "preserved-input replay helper script"
  append_launcher_status_diagnostic_entry "source_failure_active_solver_replay_script" "$LAUNCHER_REPLAY_ACTIVE_SCRIPT" "active-branch solver replay helper script"
  append_launcher_status_diagnostic_entry "source_failure_artifact_manifest" "$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH" "artifact inventory from the preserved failure bundle"
  append_launcher_status_diagnostic_entry "source_failure_exact_seed" "$LAUNCHER_REPLAY_EXACT_SEED_PATH" "exact seed snapshot for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_exact_input" "$LAUNCHER_REPLAY_EXACT_INPUT_PATH" "exact input snapshot for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_exact_output" "$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH" "exact solver output snapshot for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_expected_output" "$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH" "expected output snapshot for the failing case"
  append_launcher_status_diagnostic_entry "source_failure_invoked_command" "$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH" "single-file helper invocation snapshot"
  append_launcher_status_diagnostic_entry "run_archive_iteration_evidence" "$LAUNCHER_RUN_STATUS_ITERATION_EVIDENCE_PATH" "copy of the stage-labeled iteration handoff preserved under the per-run archive"
  append_launcher_status_diagnostic_entry "run_archive_run_record" "$LAUNCHER_RUN_STATUS_RUN_RECORD_PATH" "copy of the current run record preserved under the per-run archive"
  append_launcher_status_diagnostic_entry "run_archive_run_comparison" "$LAUNCHER_RUN_STATUS_RUN_COMPARISON_PATH" "copy of the current-vs-previous run comparison preserved under the per-run archive"
  append_launcher_status_diagnostic_entry "run_archive_source_root_snapshot" "$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT" "copy of the failure/source bundle preserved under the per-run archive"
  append_launcher_status_diagnostic_entry "run_archive_source_failure_snapshot_manifest" "$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH" "immutable per-run index that remaps source failure artifacts into the archived snapshot tree"
append_launcher_status_diagnostic_entry "run_archive_launcher_failure_snapshot" "$LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT" "copy of the launcher failure bundle preserved under the per-run archive"
}

append_launcher_manifest_row() {
  local manifest_path="$1"
  local kind="$2"
  local label="$3"
  local status="$4"
  local detail="$5"
  local artifact="${6:--}"

  if [[ -z "$manifest_path" ]]; then
    return 0
  fi
  printf '%s\t%s\t%s\t%s\t%s\n' "$kind" "$label" "$status" "$detail" "$artifact" >> "$manifest_path"
}

append_launcher_manifest_command_status() {
  local manifest_path="$1"
  local name="$2"
  local status="missing"
  local detail="-"

  if detail="$(command -v "$name" 2>/dev/null)"; then
    status="ok"
  else
    detail="-"
  fi

  append_launcher_manifest_row "$manifest_path" "command" "$name" "$status" "$detail"
}

append_launcher_manifest_compiler_status() {
  local manifest_path="$1"
  local candidate=""
  local resolved=""

  for candidate in clang++ g++ c++; do
    if resolved="$(command -v "$candidate" 2>/dev/null)"; then
      append_launcher_manifest_row "$manifest_path" "compiler" "$candidate" "ok" "$resolved"
      return 0
    fi
  done

  append_launcher_manifest_row "$manifest_path" "compiler" "clang++|g++|c++" "missing" "-"
}

append_launcher_manifest_path_status() {
  local manifest_path=""
  local kind=""
  local label=""
  local path=""
  local status="missing"

  case "$#" in
    3)
      manifest_path="${LAUNCHER_FAILURE_PREFLIGHT_MANIFEST:-$LAUNCHER_PREFLIGHT_MANIFEST_PATH}"
      kind="$1"
      label="$2"
      path="$3"
      ;;
    4)
      manifest_path="$1"
      kind="$2"
      label="$3"
      path="$4"
      ;;
    *)
      fail "append_launcher_manifest_path_status expected 3 or 4 arguments (got: $#)"
      ;;
  esac

  if [[ -e "$path" ]]; then
    status="ok"
    case "$kind" in
      directory)
        if [[ ! -d "$path" ]]; then
          status="not_directory"
        elif [[ ! -r "$path" ]]; then
          status="not_readable"
        fi
        ;;
      file|executable)
        if [[ ! -f "$path" ]]; then
          status="not_regular_file"
        elif [[ ! -r "$path" ]]; then
          status="not_readable"
        elif [[ "$kind" == "executable" && ! -x "$path" ]]; then
          status="not_executable"
        fi
        ;;
      *)
        status="unknown_kind"
        ;;
    esac
  fi

  append_launcher_manifest_row "$manifest_path" "$kind" "$label" "$status" "$path"
}

append_launcher_manifest_value_status() {
  local manifest_path="$1"
  local kind="$2"
  local label="$3"
  local value="$4"

  append_launcher_manifest_row "$manifest_path" "$kind" "$label" "ok" "${value:--}"
}

write_launcher_environment_snapshot() {
  local target_path="$1"
  local working_directory=""

  if [[ -z "$target_path" ]]; then
    return 0
  fi
  mkdir -p "$(dirname "$target_path")" || return 1
  working_directory="$(pwd -P 2>/dev/null || pwd)"
  {
    echo "PWD=$working_directory"
    echo "ORIGINAL_LAUNCH_PWD=$LAUNCHER_ORIGINAL_PWD"
    echo "BRANCH_ROOT=$BRANCH_ROOT"
    echo "BRANCH_ARTIFACTS_ROOT=${BRANCH_ARTIFACTS_ROOT:-}"
    echo "ARTIFACTS_ROOT=${ARTIFACTS_ROOT:-}"
    echo "SMOKE_OUTPUT_ROOT=${SMOKE_OUTPUT_ROOT:-}"
    echo "SMOKE_FAILURE_ROOT=${SMOKE_FAILURE_ROOT:-}"
    echo "LAUNCHER_STATUS_ROOT=${LAUNCHER_STATUS_ROOT:-}"
    echo "LAUNCHER_RUN_HISTORY_ROOT=${LAUNCHER_RUN_HISTORY_ROOT:-}"
    echo "LAUNCHER_RUN_EXPORT_ROOT=${LAUNCHER_RUN_EXPORT_ROOT:-}"
    echo "TMP_PARENT=${TMP_PARENT:-}"
    echo "LOCK_ROOT=${LOCK_ROOT:-}"
    echo "PATH=${PATH:-}"
    echo "HOME=${HOME:-}"
    echo "TERM=${TERM:-}"
    echo "TMPDIR=${TMPDIR:-}"
    echo "BRANCH_ARTIFACT_TMP_ROOT=${BRANCH_ARTIFACT_TMP_ROOT:-}"
    echo "LCA_SMOKE_EXPORT_SNAPSHOT_ROOT=${LCA_SMOKE_EXPORT_SNAPSHOT_ROOT:-}"
    echo "LCA_SMOKE_DEBUG_MANIFEST=${LCA_SMOKE_DEBUG_MANIFEST:-}"
    echo "LCA_SMOKE_BUILD_TIMEOUT_S=${LCA_SMOKE_BUILD_TIMEOUT_S:-}"
    echo "LCA_SMOKE_LAUNCHER_TIMEOUT_S=${LCA_SMOKE_LAUNCHER_TIMEOUT_S:-}"
    echo "LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND=${LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND:-}"
    echo "LCA_SMOKE_LAUNCHER_ORIGINAL_PWD=${LCA_SMOKE_LAUNCHER_ORIGINAL_PWD:-}"
    echo "SMOKE_CASES_SOURCE=${SMOKE_CASES_SOURCE:-}"
    echo "SMOKE_MANIFEST_INPUT_POLICY=${SMOKE_MANIFEST_INPUT_POLICY:-}"
    echo "LAUNCHER_DISPATCH_TIMEOUT_S=${LAUNCHER_DISPATCH_TIMEOUT_S:-}"
    echo "$LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG=${!LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG:-0}"
    echo "$LCA_SMOKE_INNER_CLEAN_ENV_FLAG=${!LCA_SMOKE_INNER_CLEAN_ENV_FLAG:-0}"
    echo "launcher_tmpdir=${LAUNCHER_TMPDIR:-}"
    echo "launcher_home=${LAUNCHER_HOME:-}"
    echo "launcher_preflight_root=${LAUNCHER_PREFLIGHT_ROOT:-}"
    echo
    env | LC_ALL=C sort
  } > "$target_path"
}

write_launcher_preflight_artifacts() {
  mkdir -p "$LAUNCHER_PREFLIGHT_ROOT" || fail "failed to prepare launcher preflight root: $LAUNCHER_PREFLIGHT_ROOT"
  write_launcher_environment_snapshot "$LAUNCHER_PREFLIGHT_ENV_SNAPSHOT_PATH" \
    || fail "failed to record launcher bootstrap environment snapshot: $LAUNCHER_PREFLIGHT_ENV_SNAPSHOT_PATH"
  write_launcher_preflight_manifest "$LAUNCHER_PREFLIGHT_MANIFEST_PATH"
}

write_launcher_preflight_manifest() {
  local manifest_path="${1:-$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST}"

  if [[ -z "$manifest_path" ]]; then
    fail "missing launcher preflight manifest path"
  fi
  mkdir -p "$(dirname "$manifest_path")" || fail "failed to prepare launcher preflight manifest parent: $manifest_path"
  printf 'kind\tlabel\tstatus\tdetail\tartifact\n' > "$manifest_path"
  append_launcher_manifest_command_status "$manifest_path" bash
  append_launcher_manifest_command_status "$manifest_path" python3
  append_launcher_manifest_command_status "$manifest_path" mkdir
  append_launcher_manifest_command_status "$manifest_path" mktemp
  append_launcher_manifest_command_status "$manifest_path" dirname
  append_launcher_manifest_command_status "$manifest_path" chmod
  append_launcher_manifest_command_status "$manifest_path" cp
  append_launcher_manifest_command_status "$manifest_path" mv
  append_launcher_manifest_command_status "$manifest_path" rm
  append_launcher_manifest_command_status "$manifest_path" rmdir
  append_launcher_manifest_command_status "$manifest_path" kill
  append_launcher_manifest_command_status "$manifest_path" tail
  append_launcher_manifest_command_status "$manifest_path" sleep
  append_launcher_manifest_command_status "$manifest_path" grep
  append_launcher_manifest_command_status "$manifest_path" sort
  append_launcher_manifest_command_status "$manifest_path" date
  append_launcher_manifest_command_status "$manifest_path" ln
  append_launcher_manifest_compiler_status "$manifest_path"
  append_launcher_manifest_path_status directory "branch root directory" "$BRANCH_ROOT"
  append_launcher_manifest_path_status executable "launcher entrypoint" "$SELF_PATH"
  append_launcher_manifest_path_status directory "outer suite wrappers directory" "$OUTER_SUITE_WRAPPERS_DIR"
  append_launcher_manifest_path_status directory "resume workspace directory" "$RESUME_WORKSPACE_DIR"
  append_launcher_manifest_path_status executable "outer smoke wrapper" "$INNER_WRAPPER"
  append_launcher_manifest_path_status file "release env wrapper" "$RELEASE_ENV"
  append_launcher_manifest_path_status file "artifact resolver" "$ARTIFACT_RESOLVER"
  append_launcher_manifest_path_status file "branch-local case helper" "$RUN_CASE_HELPER"
  append_launcher_manifest_path_status file "branch-local validator" "$CHECKER_HELPER"
  append_launcher_manifest_path_status file "build helper" "$BUILD_HELPER"
  append_launcher_manifest_path_status file "resume helper" "$RESUME_HELPER"
  append_launcher_manifest_path_status file "solver source" "$SOURCE"
  append_launcher_manifest_path_status file "smoke case manifest" "$SMOKE_CASES_SOURCE"
  append_launcher_manifest_path_status executable "build wrapper" "$BUILD_WRAPPER"
  append_launcher_manifest_path_status executable "smoke target wrapper" "$SMOKE_TARGET_WRAPPER"
  append_launcher_manifest_path_status directory "branch artifacts root" "${BRANCH_ARTIFACTS_ROOT:-$BRANCH_ROOT/artifacts}"
  append_launcher_manifest_path_status directory "launcher artifacts root" "${ARTIFACTS_ROOT:-$BRANCH_ROOT/artifacts/lca_tree_stress_v5}"
  append_launcher_manifest_path_status directory "smoke output root" "${SMOKE_OUTPUT_ROOT:-$SMOKE_OUTPUT_ROOT_DEFAULT}"
  append_launcher_manifest_path_status directory "smoke failure root" "${SMOKE_FAILURE_ROOT:-$SMOKE_FAILURE_ROOT_DEFAULT}"
  append_launcher_manifest_path_status directory "launcher failure root" "$LAUNCHER_FAILURE_ROOT"
  append_launcher_manifest_path_status directory "launcher status root" "${LAUNCHER_STATUS_ROOT:-}"
  append_launcher_manifest_path_status directory "launcher run history root" "${LAUNCHER_RUN_HISTORY_ROOT:-}"
  append_launcher_manifest_path_status directory "launcher run export root" "${LAUNCHER_RUN_EXPORT_ROOT:-}"
  append_launcher_manifest_path_status directory "launcher tmp parent" "${TMP_PARENT:-}"
  append_launcher_manifest_path_status directory "launcher lock root" "${LOCK_ROOT:-}"
  append_launcher_manifest_path_status directory "launcher tmpdir" "${LAUNCHER_TMPDIR:-}"
  append_launcher_manifest_path_status directory "launcher preflight root" "${LAUNCHER_PREFLIGHT_ROOT:-}"
  append_launcher_manifest_path_status directory "launcher home root" "${LAUNCHER_HOME:-}"
  append_launcher_manifest_path_status directory "launcher xdg config root" "${LAUNCHER_XDG_CONFIG_HOME:-}"
  append_launcher_manifest_path_status directory "launcher xdg cache root" "${LAUNCHER_XDG_CACHE_HOME:-}"
  append_launcher_manifest_path_status directory "launcher xdg state root" "${LAUNCHER_XDG_STATE_HOME:-}"
  append_launcher_manifest_path_status directory "launcher pycache root" "${LAUNCHER_PYCACHE_ROOT:-}"
  append_launcher_manifest_value_status "$manifest_path" "working_directory" "original launch working directory" "${LAUNCHER_ORIGINAL_PWD:-}"
  append_launcher_manifest_value_status "$manifest_path" "working_directory" "branch root" "$BRANCH_ROOT"
  append_launcher_manifest_value_status "$manifest_path" "path_policy" "clean path" "$LCA_SMOKE_CLEAN_PATH"
  append_launcher_manifest_value_status "$manifest_path" "policy" "manifest_input_policy" "$SMOKE_MANIFEST_INPUT_POLICY"
  append_launcher_manifest_value_status "$manifest_path" "policy" "manifest_selection_policy" "manifest_row_order"
  append_launcher_manifest_value_status "$manifest_path" "policy" "seed_policy" "manifest_seed"
  append_launcher_manifest_value_status "$manifest_path" "setting" "smoke_manifest_path" "$SMOKE_CASES_SOURCE"
  append_launcher_manifest_value_status "$manifest_path" "setting" "build_timeout_override" "${LCA_SMOKE_BUILD_TIMEOUT_S:-}"
  append_launcher_manifest_value_status "$manifest_path" "setting" "launcher_dispatch_timeout_s" "${LAUNCHER_DISPATCH_TIMEOUT_S:-}"
}

write_launcher_artifact_manifest() {
  {
    printf 'artifact\tpath\n'
    printf 'failure_summary\t%s\n' "$LAUNCHER_FAILURE_SUMMARY"
    printf 'failure_report\t%s\n' "$LAUNCHER_FAILURE_REPORT"
    printf 'failure_reason\t%s\n' "$LAUNCHER_FAILURE_REASON_PATH"
    printf 'invocation_command\t%s\n' "$LAUNCHER_FAILURE_INVOCATION_COMMAND_PATH"
    printf 'dispatch_command\t%s\n' "$LAUNCHER_FAILURE_DISPATCH_COMMAND_PATH"
    printf 'rerun_command\t%s\n' "$LAUNCHER_FAILURE_RERUN_COMMAND_PATH"
    printf 'launcher_env_snapshot\t%s\n' "$LAUNCHER_FAILURE_ENV_SNAPSHOT"
    printf 'preflight_manifest\t%s\n' "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
    if [[ -f "$LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION" ]]; then
      printf 'smoke_manifest_selection\t%s\n' "$LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION"
    fi
    if [[ -f "$LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR" ]]; then
      printf 'smoke_manifest_check_stderr\t%s\n' "$LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR"
    fi
    if [[ -f "$LAUNCHER_FAILURE_COMMAND_PATH" ]]; then
      printf 'failing_command\t%s\n' "$LAUNCHER_FAILURE_COMMAND_PATH"
    fi
  } > "$LAUNCHER_FAILURE_ARTIFACT_MANIFEST"
}

write_launcher_status_artifact_manifest() {
  {
    printf 'artifact\tpath\n'
    printf 'status_summary\t%s\n' "$LAUNCHER_STATUS_SUMMARY"
    printf 'status_report\t%s\n' "$LAUNCHER_STATUS_REPORT"
    printf 'status_iteration_evidence\t%s\n' "$LAUNCHER_STATUS_ITERATION_EVIDENCE"
    printf 'status_retry_loop_control\t%s\n' "$LAUNCHER_STATUS_RETRY_LOOP_CONTROL"
    printf 'status_diagnostics_manifest\t%s\n' "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST"
    printf 'status_run_record\t%s\n' "$LAUNCHER_STATUS_RUN_RECORD"
    printf 'status_run_comparison\t%s\n' "$LAUNCHER_STATUS_RUN_COMPARISON"
    if [[ -n "$LAUNCHER_RUN_HISTORY_INDEX" ]]; then
      printf 'run_history_index\t%s\n' "$LAUNCHER_RUN_HISTORY_INDEX"
    fi
    if [[ -n "$LAUNCHER_RUN_ARCHIVE_ROOT" ]]; then
      printf 'run_archive_root\t%s\n' "$LAUNCHER_RUN_ARCHIVE_ROOT"
    fi
    if [[ -n "$LAUNCHER_RUN_CONSOLE_LOG" ]]; then
      printf 'run_console_stderr\t%s\n' "$LAUNCHER_RUN_CONSOLE_LOG"
    fi
    if [[ -n "$LAUNCHER_DISPATCH_RESULT_PATH" ]]; then
      printf 'run_dispatch_result\t%s\n' "$LAUNCHER_DISPATCH_RESULT_PATH"
    fi
    if [[ -n "$LAUNCHER_RUN_ARTIFACT_MANIFEST" ]]; then
      printf 'run_archive_manifest\t%s\n' "$LAUNCHER_RUN_ARTIFACT_MANIFEST"
    fi
    if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH" ]]; then
      printf 'published_smoke_summary\t%s\n' "$LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH"
    fi
    if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH" ]]; then
      printf 'published_smoke_status_report\t%s\n' "$LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH"
    fi
    if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH" ]]; then
      printf 'published_smoke_failure_report\t%s\n' "$LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH"
    fi
    if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH" ]]; then
      printf 'published_smoke_iteration_evidence\t%s\n' "$LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH"
    fi
    if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH" ]]; then
      printf 'published_smoke_retry_loop_control\t%s\n' "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH"
    fi
    if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH" ]]; then
      printf 'published_smoke_diagnostics_manifest\t%s\n' "$LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH"
    fi
    if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH" ]]; then
      printf 'published_smoke_standard_gap_json\t%s\n' "$LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH"
    fi
    if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH" ]]; then
      printf 'published_smoke_run_record\t%s\n' "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH"
    fi
    if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH" ]]; then
      printf 'published_smoke_run_comparison\t%s\n' "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH"
    fi
    if [[ -n "$LAUNCHER_STATUS_SOURCE_SUMMARY" ]]; then
      printf 'source_summary\t%s\n' "$LAUNCHER_STATUS_SOURCE_SUMMARY"
    fi
    if [[ -n "$LAUNCHER_STATUS_SOURCE_REPORT" ]]; then
      printf 'source_report\t%s\n' "$LAUNCHER_STATUS_SOURCE_REPORT"
    fi
    if [[ -n "$LAUNCHER_STATUS_SOURCE_ROOT" ]]; then
      printf 'source_root\t%s\n' "$LAUNCHER_STATUS_SOURCE_ROOT"
    fi
    if [[ -n "$SMOKE_OUTPUT_ROOT" ]]; then
      printf 'smoke_output_root\t%s\n' "$SMOKE_OUTPUT_ROOT"
    fi
    if [[ -n "$SMOKE_FAILURE_ROOT" ]]; then
      printf 'smoke_failure_root\t%s\n' "$SMOKE_FAILURE_ROOT"
    fi
    if [[ -n "$LAUNCHER_FAILURE_ROOT" ]]; then
      printf 'launcher_failure_root\t%s\n' "$LAUNCHER_FAILURE_ROOT"
    fi
  } > "$LAUNCHER_STATUS_ARTIFACT_MANIFEST"
}

write_launcher_smoke_standard_gap_json() {
  local output_path="$1"

  python3 - "$LAUNCHER_STATUS_SUMMARY" "$output_path" <<'PY'
from __future__ import annotations

import json
import sys
from pathlib import Path

summary_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])

summary: dict[str, str] = {}
for raw_line in summary_path.read_text(encoding="utf-8").splitlines():
    if "=" not in raw_line:
        continue
    key, value = raw_line.split("=", 1)
    summary[key] = value


def as_int(value: str | None) -> int | None:
    if value is None or value == "":
        return None
    try:
        return int(value)
    except ValueError:
        return None


def as_bool(value: str | None) -> bool | None:
    if value is None or value == "":
        return None
    lowered = value.lower()
    if lowered in {"1", "true", "yes"}:
        return True
    if lowered in {"0", "false", "no"}:
        return False
    return None


def split_artifacts(value: str | None) -> list[str]:
    if value is None or value == "":
        return []
    return [part for part in value.split(" | ") if part]


def run_sort_key(run_id: str) -> tuple[int, int | str]:
    import re

    match = re.fullmatch(r"run\.(\d+)", run_id)
    if match is None:
        return (1, run_id)
    return (0, int(match.group(1)))


def normalize_history_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    canonical_rows: dict[str, dict[str, str]] = {}
    for row in rows:
        run_id = row.get("run_id", "")
        if run_id == "":
            continue
        canonical_rows[run_id] = row
    return [canonical_rows[run_id] for run_id in sorted(canonical_rows, key=run_sort_key)]


def build_gate_chain(summary: dict[str, str], *, summary_path: Path) -> list[dict[str, object]]:
    requirements: list[tuple[int, str, str, list[int]]] = [
        (2, "smoke", "./lca_smoke.sh", []),
        (3, "strong_gate", "./lca_strong_gate.sh", [2]),
        (4, "strong_gate_repeatability", "./lca_strong_gate.sh && ./lca_strong_gate.sh", [3]),
        (5, "boj3s_gate", "./lca_boj3s_gate.sh", [3]),
        (6, "boj3s_gate_repeatability", "./lca_boj3s_gate.sh && ./lca_boj3s_gate.sh", [5]),
    ]
    evidence_path = summary.get("published_smoke_summary_path") or str(summary_path)
    gate_chain: list[dict[str, object]] = []
    for ac, name, command, depends_on in requirements:
        gate_chain.append(
            {
                "ac": ac,
                "name": name,
                "command": command,
                "depends_on": depends_on,
                "status": summary.get(f"gate_chain_ac{ac}_status"),
                "summary": summary.get(f"gate_chain_ac{ac}_summary"),
                "evidence_path": evidence_path,
            }
        )
    return gate_chain


public_status = summary.get("public_status", "FAIL")
misses_standard = public_status != "PASS"
payload = {
    "required_standard": summary.get("required_standard", "lca_tree_stress_v5"),
    "standard_signal_role": summary.get("standard_signal_role", "smoke_wrapper_early_signal"),
    "standard_gap_status": summary.get(
        "standard_gap_status",
        "smoke_blocker_detected" if misses_standard else "ready_for_gate_escalation",
    ),
    "misses_standard": misses_standard,
    "public_status": public_status,
    "result_family": summary.get("result_family"),
    "failure_partition": summary.get("failure_partition"),
    "failure_partition_label": summary.get("failure_partition_label"),
    "normalized_outcome": summary.get("normalized_outcome"),
    "normalized_exit_code": as_int(summary.get("normalized_exit_code")),
    "raw_exit_code": as_int(summary.get("raw_exit_code")),
    "outcome_summary": summary.get("outcome_summary"),
    "standard_gap_summary": summary.get("standard_gap_summary"),
    "acceptance_signal": {
        "status": summary.get("acceptance_signal_status"),
        "summary": summary.get("acceptance_signal_summary"),
    },
    "iteration_support": {
        "status": summary.get("iteration_support_status"),
        "next_step": summary.get("iteration_support_next_step"),
        "summary": summary.get("iteration_support_summary"),
    },
    "command_control": {
        "mode": summary.get("command_control_mode"),
        "preferred_command_kind": summary.get("command_control_preferred_command_kind"),
        "should_resume_retry_loop": as_bool(summary.get("should_resume_retry_loop")),
        "should_retry_smoke_directly": as_bool(summary.get("should_retry_smoke_directly")),
        "failure_is_terminal": as_bool(summary.get("failure_is_terminal")),
        "gate_escalation_allowed": as_bool(summary.get("gate_escalation_allowed")),
        "next_gate": {
            "command": summary.get("next_gate_command"),
            "status": summary.get("next_gate_status"),
            "dependency": summary.get("next_gate_dependency"),
            "summary": summary.get("next_gate_summary"),
        },
    },
    "triage": {
        "scope": summary.get("triage_stage_scope"),
        "stage": summary.get("triage_stage"),
        "stage_label": summary.get("triage_stage_label"),
        "primary_summary": summary.get("triage_primary_summary"),
        "primary_report": summary.get("triage_primary_report"),
        "primary_manifest": summary.get("triage_primary_manifest"),
        "iteration_evidence_path": summary.get("iteration_evidence_path"),
        "first_artifacts": split_artifacts(summary.get("triage_first_artifacts")),
        "retry_command": summary.get("triage_retry_command"),
        "retry_hint": summary.get("triage_retry_hint"),
    },
    "retry_loop": {
        "action": summary.get("retry_loop_action"),
        "preferred_command": summary.get("retry_loop_preferred_command"),
        "launch_command": summary.get("retry_loop_launch_command"),
        "direct_command": summary.get("retry_loop_direct_command"),
        "hint": summary.get("retry_loop_hint"),
        "log_path": summary.get("retry_loop_log_path"),
        "control_path": summary.get("retry_loop_control_path"),
        "published_control_path": summary.get("published_smoke_retry_loop_control_path"),
        "next_gate_command": summary.get("next_gate_command"),
        "solver_seed_file": summary.get("retry_loop_solver_seed_file"),
        "analysis_seed_file": summary.get("retry_loop_analysis_seed_file"),
    },
    "source_failure": {
        "summary": summary.get("source_failure_summary"),
        "case": summary.get("source_failure_case"),
        "seed": as_int(summary.get("source_failure_seed")),
        "stage": summary.get("source_failure_stage"),
        "kind": summary.get("source_failure_kind"),
        "origin": summary.get("source_failure_origin"),
        "retryable": as_bool(summary.get("source_failure_retryable")),
        "reporting_status": summary.get("source_failure_reporting_status"),
        "reporting_warning": summary.get("source_failure_reporting_warning"),
        "replay_command": summary.get("source_failure_replay_command"),
        "artifacts": {
            "failure_root_path": summary.get("source_failure_root_path"),
            "failure_case_dir_path": summary.get("source_failure_case_dir_path"),
            "commands_path": summary.get("source_failure_commands_path"),
            "artifact_manifest_path": summary.get("source_failure_artifact_manifest_path"),
            "rerun_command_path": summary.get("source_failure_rerun_command_path"),
            "exact_seed_path": summary.get("source_failure_exact_seed_path"),
            "exact_input_path": summary.get("source_failure_exact_input_path"),
            "exact_output_path": summary.get("source_failure_exact_output_path"),
            "expected_output_path": summary.get("source_failure_expected_output_path"),
            "invoked_command_path": summary.get("source_failure_invoked_command_path"),
            "helper_stdout_path": summary.get("source_failure_helper_stdout_path"),
            "helper_stderr_path": summary.get("source_failure_helper_stderr_path"),
            "helper_result_json_path": summary.get("source_failure_helper_result_json_path"),
            "checker_result_path": summary.get("source_failure_checker_result_path"),
            "checker_replay_stdout_path": summary.get("source_failure_checker_replay_stdout_path"),
            "checker_replay_stderr_path": summary.get("source_failure_checker_replay_stderr_path"),
            "mismatch_summary_path": summary.get("source_failure_mismatch_summary_path"),
            "retry_log_path": summary.get("source_failure_retry_log_path"),
            "runtime_env_path": summary.get("source_failure_runtime_env_path"),
            "runtime_env_exports_path": summary.get("source_failure_runtime_env_exports_path"),
            "preflight_manifest_path": summary.get("source_failure_preflight_manifest_path"),
            "setup_env_path": summary.get("source_failure_setup_env_path"),
            "build_command_path": summary.get("source_failure_build_command_path"),
            "build_stdout_path": summary.get("source_failure_build_stdout_path"),
            "build_stderr_path": summary.get("source_failure_build_stderr_path"),
            "structured_context_path": summary.get("source_failure_structured_context_path"),
            "manifest_snapshot_path": summary.get("source_failure_manifest_snapshot_path"),
            "failed_case_row_path": summary.get("source_failure_failed_case_row_path"),
            "suite_config_path": summary.get("source_failure_suite_config_path"),
            "suite_plan_path": summary.get("source_failure_suite_plan_path"),
            "checker_script_path": summary.get("source_failure_checker_script"),
            "seed_repro_script_path": summary.get("source_failure_seed_repro_script"),
            "preserved_input_replay_script_path": summary.get("source_failure_preserved_input_replay_script"),
            "active_solver_replay_script_path": summary.get("source_failure_active_solver_replay_script"),
        },
    },
    "published_artifacts": {
        "summary_path": summary.get("published_smoke_summary_path"),
        "status_report_path": summary.get("published_smoke_status_report_path"),
        "failure_report_path": summary.get("published_smoke_failure_report_path"),
        "iteration_evidence_path": summary.get("published_smoke_iteration_evidence_path"),
        "diagnostics_manifest_path": summary.get("published_smoke_diagnostics_manifest_path"),
        "standard_gap_json_path": summary.get("published_smoke_standard_gap_json_path"),
    },
    "gate_chain": build_gate_chain(summary, summary_path=summary_path),
    "summary_fields": summary,
}
output_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
}

write_launcher_retry_loop_control_json() {
  local output_path="$1"

  python3 - "$LAUNCHER_STATUS_SUMMARY" "$output_path" <<'PY'
from __future__ import annotations

import json
import sys
from pathlib import Path

summary_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])

summary: dict[str, str] = {}
for raw_line in summary_path.read_text(encoding="utf-8").splitlines():
    if "=" not in raw_line:
        continue
    key, value = raw_line.split("=", 1)
    summary[key] = value


def as_bool(value: str | None) -> bool | None:
    if value is None or value == "":
        return None
    lowered = value.lower()
    if lowered in {"1", "true", "yes"}:
        return True
    if lowered in {"0", "false", "no"}:
        return False
    return None


def build_gate_chain(summary: dict[str, str], *, summary_path: Path) -> list[dict[str, object]]:
    requirements: list[tuple[int, str, str, list[int]]] = [
        (2, "smoke", "./lca_smoke.sh", []),
        (3, "strong_gate", "./lca_strong_gate.sh", [2]),
        (4, "strong_gate_repeatability", "./lca_strong_gate.sh && ./lca_strong_gate.sh", [3]),
        (5, "boj3s_gate", "./lca_boj3s_gate.sh", [3]),
        (6, "boj3s_gate_repeatability", "./lca_boj3s_gate.sh && ./lca_boj3s_gate.sh", [5]),
    ]
    evidence_path = summary.get("published_smoke_summary_path") or str(summary_path)
    gate_chain: list[dict[str, object]] = []
    for ac, name, command, depends_on in requirements:
        gate_chain.append(
            {
                "ac": ac,
                "name": name,
                "command": command,
                "depends_on": depends_on,
                "status": summary.get(f"gate_chain_ac{ac}_status"),
                "summary": summary.get(f"gate_chain_ac{ac}_summary"),
                "evidence_path": evidence_path,
            }
        )
    return gate_chain

public_status = summary.get("public_status", "FAIL")
payload = {
    "script": summary.get("script", "./lca_smoke.sh"),
    "public_status": public_status,
    "normalized_outcome": summary.get("normalized_outcome"),
    "outcome_summary": summary.get("outcome_summary"),
    "acceptance_signal_status": summary.get("acceptance_signal_status"),
    "acceptance_signal_summary": summary.get("acceptance_signal_summary"),
    "iteration_support_status": summary.get("iteration_support_status"),
    "iteration_support_next_step": summary.get("iteration_support_next_step"),
    "iteration_support_summary": summary.get("iteration_support_summary"),
    "retry_loop_action": summary.get("retry_loop_action"),
    "preferred_command": summary.get("retry_loop_preferred_command"),
    "launch_command": summary.get("retry_loop_launch_command"),
    "direct_command": summary.get("retry_loop_direct_command"),
    "hint": summary.get("retry_loop_hint"),
    "log_path": summary.get("retry_loop_log_path"),
    "should_resume_retry_loop": as_bool(summary.get("should_resume_retry_loop")),
    "should_retry_smoke_directly": as_bool(summary.get("should_retry_smoke_directly")),
    "failure_is_terminal": as_bool(summary.get("failure_is_terminal")),
    "smoke_retry_command": summary.get("triage_retry_command"),
    "smoke_retry_hint": summary.get("triage_retry_hint"),
    "next_gate_command": summary.get("next_gate_command"),
    "next_gate_status": summary.get("next_gate_status"),
    "next_gate_dependency": summary.get("next_gate_dependency"),
    "next_gate_summary": summary.get("next_gate_summary"),
    "gate_escalation_allowed": as_bool(summary.get("gate_escalation_allowed")),
    "command_control": {
        "mode": summary.get("command_control_mode"),
        "preferred_command_kind": summary.get("command_control_preferred_command_kind"),
        "should_resume_retry_loop": as_bool(summary.get("should_resume_retry_loop")),
        "should_retry_smoke_directly": as_bool(summary.get("should_retry_smoke_directly")),
        "failure_is_terminal": as_bool(summary.get("failure_is_terminal")),
        "gate_escalation_allowed": as_bool(summary.get("gate_escalation_allowed")),
        "next_gate": {
            "command": summary.get("next_gate_command"),
            "status": summary.get("next_gate_status"),
            "dependency": summary.get("next_gate_dependency"),
            "summary": summary.get("next_gate_summary"),
        },
    },
    "solver_seed_file": summary.get("retry_loop_solver_seed_file"),
    "analysis_seed_file": summary.get("retry_loop_analysis_seed_file"),
    "artifacts": {
        "status_summary_path": str(summary_path),
        "status_report_path": summary.get("status_report"),
        "iteration_evidence_path": summary.get("iteration_evidence_path"),
        "diagnostics_manifest_path": summary.get("status_diagnostics_manifest"),
        "standard_gap_json_path": summary.get("published_smoke_standard_gap_json_path"),
        "structured_context_path": summary.get("source_failure_structured_context_path"),
        "source_failure_commands_path": summary.get("source_failure_commands_path"),
        "source_failure_artifact_manifest_path": summary.get("source_failure_artifact_manifest_path"),
        "source_failure_exact_seed_path": summary.get("source_failure_exact_seed_path"),
        "source_failure_invoked_command_path": summary.get("source_failure_invoked_command_path"),
        "source_failure_expected_output_path": summary.get("source_failure_expected_output_path"),
        "control_path": str(output_path),
        "published_control_path": summary.get("published_smoke_retry_loop_control_path"),
    },
    "gate_chain": build_gate_chain(summary, summary_path=summary_path),
}
if public_status == "PASS":
    payload["should_resume_retry_loop"] = False
    payload["should_retry_smoke_directly"] = False

output_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
}

write_launcher_run_tracking_artifacts() {
  if [[ -e "$LAUNCHER_RUN_HISTORY_INDEX" && ! -f "$LAUNCHER_RUN_HISTORY_INDEX" ]]; then
    remove_path_retry "$LAUNCHER_RUN_HISTORY_INDEX" || return 1
  fi

  python3 - \
    "$LAUNCHER_STATUS_SUMMARY" \
    "$LAUNCHER_STATUS_RUN_RECORD" \
    "$LAUNCHER_STATUS_RUN_COMPARISON" \
    "$LAUNCHER_RUN_HISTORY_INDEX" <<'PY'
from __future__ import annotations

import csv
import json
import sys
from pathlib import Path

summary_path = Path(sys.argv[1])
record_path = Path(sys.argv[2])
comparison_path = Path(sys.argv[3])
history_path = Path(sys.argv[4])

summary: dict[str, str] = {}
for raw_line in summary_path.read_text(encoding="utf-8").splitlines():
    if "=" not in raw_line:
        continue
    key, value = raw_line.split("=", 1)
    summary[key] = value


def as_int(value: str | None) -> int | None:
    if value is None or value == "":
        return None
    try:
        return int(value)
    except ValueError:
        return None


def as_bool(value: str | None) -> bool | None:
    if value is None or value == "":
        return None
    lowered = value.lower()
    if lowered in {"1", "true", "yes"}:
        return True
    if lowered in {"0", "false", "no"}:
        return False
    return None


def split_csv(raw: str | None) -> list[str]:
    if raw is None or raw == "":
        return []
    return [part for part in raw.split(",") if part]


def split_artifacts(value: str | None) -> list[str]:
    if value is None or value == "":
        return []
    return [part for part in value.split(" | ") if part]


def run_sort_key(run_id: str) -> tuple[int, int | str]:
    import re

    match = re.fullmatch(r"run\.(\d+)", run_id)
    if match is None:
        return (1, run_id)
    return (0, int(match.group(1)))


def normalize_history_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    canonical_rows: dict[str, dict[str, str]] = {}
    for row in rows:
        run_id = row.get("run_id", "")
        if run_id == "":
            continue
        canonical_rows[run_id] = row
    return [canonical_rows[run_id] for run_id in sorted(canonical_rows, key=run_sort_key)]


def build_gate_chain(summary: dict[str, str], *, summary_path: Path) -> list[dict[str, object]]:
    requirements: list[tuple[int, str, str, list[int]]] = [
        (2, "smoke", "./lca_smoke.sh", []),
        (3, "strong_gate", "./lca_strong_gate.sh", [2]),
        (4, "strong_gate_repeatability", "./lca_strong_gate.sh && ./lca_strong_gate.sh", [3]),
        (5, "boj3s_gate", "./lca_boj3s_gate.sh", [3]),
        (6, "boj3s_gate_repeatability", "./lca_boj3s_gate.sh && ./lca_boj3s_gate.sh", [5]),
    ]
    evidence_path = summary.get("published_smoke_summary_path") or str(summary_path)
    gate_chain: list[dict[str, object]] = []
    for ac, name, command, depends_on in requirements:
        gate_chain.append(
            {
                "ac": ac,
                "name": name,
                "command": command,
                "depends_on": depends_on,
                "status": summary.get(f"gate_chain_ac{ac}_status"),
                "summary": summary.get(f"gate_chain_ac{ac}_summary"),
                "evidence_path": evidence_path,
            }
        )
    return gate_chain


history_fields = [
    "run_id",
    "run_started_at_utc",
    "run_finished_at_utc",
    "run_elapsed_seconds",
    "public_status",
    "acceptance_signal_status",
    "iteration_support_status",
    "iteration_support_next_step",
    "result_family",
    "normalized_outcome",
    "normalized_exit_code",
    "raw_exit_code",
    "stage_label",
    "outcome_source",
    "source_failure_case",
    "source_failure_kind",
    "run_archive_root",
    "run_console_stderr_path",
    "run_dispatch_result_path",
    "status_summary_path",
    "status_report_path",
    "iteration_evidence_path",
    "diagnostics_manifest_path",
    "run_record_path",
    "run_comparison_path",
]
current_row = {
    "run_id": summary.get("run_id", ""),
    "run_started_at_utc": summary.get("run_started_at_utc", ""),
    "run_finished_at_utc": summary.get("run_finished_at_utc", ""),
    "run_elapsed_seconds": summary.get("run_elapsed_seconds", ""),
    "public_status": summary.get("public_status", ""),
    "acceptance_signal_status": summary.get("acceptance_signal_status", ""),
    "iteration_support_status": summary.get("iteration_support_status", ""),
    "iteration_support_next_step": summary.get("iteration_support_next_step", ""),
    "result_family": summary.get("result_family", ""),
    "normalized_outcome": summary.get("normalized_outcome", ""),
    "normalized_exit_code": summary.get("normalized_exit_code", ""),
    "raw_exit_code": summary.get("raw_exit_code", ""),
    "stage_label": summary.get("triage_stage_label") or summary.get("stage_label", ""),
    "outcome_source": summary.get("outcome_source", ""),
    "source_failure_case": summary.get("source_failure_case", ""),
    "source_failure_kind": summary.get("source_failure_kind", ""),
    "run_archive_root": summary.get("run_archive_root", ""),
    "run_console_stderr_path": summary.get("run_console_stderr_path", ""),
    "run_dispatch_result_path": summary.get("run_dispatch_result_path", ""),
    "status_summary_path": summary.get("run_archive_summary_path") or str(summary_path),
    "status_report_path": summary.get("run_archive_status_report_path") or summary.get("status_report_path", ""),
    "iteration_evidence_path": summary.get("run_archive_iteration_evidence_path") or summary.get("iteration_evidence_path", ""),
    "diagnostics_manifest_path": summary.get("run_archive_diagnostics_manifest_path") or summary.get("status_diagnostics_manifest", ""),
    "run_record_path": summary.get("run_archive_run_record_path") or summary.get("run_record_path", ""),
    "run_comparison_path": summary.get("run_archive_run_comparison_path") or summary.get("run_comparison_path", ""),
}

history_rows: list[dict[str, str]] = []
if history_path.is_file():
    with history_path.open(encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle, delimiter="\t")
        for row in reader:
            if row.get("run_id"):
                history_rows.append(row)
history_rows = normalize_history_rows(history_rows)

previous_row: dict[str, str] | None = None
if current_row["run_id"]:
    history_rows = [row for row in history_rows if row.get("run_id") != current_row["run_id"]]
history_rows.append(current_row)
history_rows = normalize_history_rows(history_rows)
current_index = len(history_rows) - 1
for index, row in enumerate(history_rows):
    if row.get("run_id") == current_row["run_id"]:
        current_index = index
        break
if current_index > 0:
    previous_row = history_rows[current_index - 1]

record_payload = {
    "schema": "lca_smoke_run_record_v1",
    "script": summary.get("script", "./lca_smoke.sh"),
    "required_standard": summary.get("required_standard", "lca_tree_stress_v5"),
    "run": {
        "id": current_row["run_id"],
        "started_at_utc": current_row["run_started_at_utc"],
        "finished_at_utc": current_row["run_finished_at_utc"],
        "elapsed_seconds": as_int(current_row["run_elapsed_seconds"]),
        "public_status": current_row["public_status"],
        "acceptance_signal_status": current_row["acceptance_signal_status"],
        "acceptance_signal_summary": summary.get("acceptance_signal_summary", ""),
        "iteration_support_status": current_row["iteration_support_status"],
        "iteration_support_next_step": current_row["iteration_support_next_step"],
        "iteration_support_summary": summary.get("iteration_support_summary", ""),
        "result_family": current_row["result_family"],
        "failure_partition": summary.get("failure_partition", ""),
        "failure_partition_label": summary.get("failure_partition_label", ""),
        "normalized_outcome": current_row["normalized_outcome"],
        "normalized_exit_code": as_int(current_row["normalized_exit_code"]),
        "raw_exit_code": as_int(current_row["raw_exit_code"]),
        "stage_label": current_row["stage_label"],
        "outcome_source": current_row["outcome_source"],
        "outcome_summary": summary.get("outcome_summary", ""),
        "source_failure_case": current_row["source_failure_case"],
        "source_failure_kind": current_row["source_failure_kind"],
    },
    "summary": {
        "public_status": current_row["public_status"],
        "acceptance_signal_status": current_row["acceptance_signal_status"],
        "acceptance_signal_summary": summary.get("acceptance_signal_summary", ""),
        "iteration_support_status": current_row["iteration_support_status"],
        "iteration_support_next_step": current_row["iteration_support_next_step"],
        "iteration_support_summary": summary.get("iteration_support_summary", ""),
        "result_family": current_row["result_family"],
        "failure_partition": summary.get("failure_partition", ""),
        "failure_partition_label": summary.get("failure_partition_label", ""),
        "normalized_outcome": current_row["normalized_outcome"],
        "normalized_exit_code": as_int(current_row["normalized_exit_code"]),
        "raw_exit_code": as_int(current_row["raw_exit_code"]),
        "outcome_source": current_row["outcome_source"],
        "outcome_summary": summary.get("outcome_summary", ""),
        "stage_label": current_row["stage_label"],
        "source_failure_kind": current_row["source_failure_kind"],
        "source_failure_origin": summary.get("source_failure_origin", ""),
        "source_failure_retryable": as_bool(summary.get("source_failure_retryable")),
    },
    "comparison": {
        "summary": summary.get("run_comparison_summary", ""),
        "changed_fields": split_csv(summary.get("run_comparison_changed_fields")),
        "previous_run_id": summary.get("previous_run_id", ""),
        "previous_run_archive_root": summary.get("previous_run_archive_root", ""),
        "previous_normalized_outcome": summary.get("previous_run_normalized_outcome", ""),
        "previous_stage_label": summary.get("previous_run_stage_label", ""),
        "previous_source_failure_case": summary.get("previous_run_source_failure_case", ""),
    },
    "launch": {
        "working_directory": summary.get("working_directory", ""),
        "original_working_directory": summary.get("original_launch_working_directory", ""),
        "branch_root": summary.get("branch_root", ""),
        "artifacts_root": summary.get("artifacts_root", ""),
        "smoke_output_root": summary.get("smoke_output_root", ""),
        "smoke_failure_root": summary.get("smoke_failure_root", ""),
        "status_root": summary.get("status_root", ""),
        "dispatch_timeout_seconds": summary.get("dispatch_timeout_s", ""),
        "invocation_command": summary.get("invocation_command", ""),
        "dispatch_command": summary.get("dispatch_command", ""),
    },
    "inputs": {
        "suite_config_path": summary.get("smoke_suite_config_path", ""),
        "suite_plan_path": summary.get("smoke_suite_plan_path", ""),
        "environment_validation_report_path": summary.get("smoke_environment_validation_report", ""),
        "environment_preflight_manifest_path": summary.get("smoke_environment_preflight_manifest_path", ""),
        "environment_setup_env_path": summary.get("smoke_environment_setup_env_path", ""),
        "environment_build_command_path": summary.get("smoke_environment_build_command_path", ""),
        "manifest_snapshot_path": summary.get("smoke_manifest_snapshot_path", ""),
        "source_failure_failed_case_row_path": summary.get("source_failure_failed_case_row_path", ""),
        "source_failure_exact_seed_path": summary.get("source_failure_exact_seed_path", ""),
        "source_failure_exact_input_path": summary.get("source_failure_exact_input_path", ""),
        "source_failure_expected_output_path": summary.get("source_failure_expected_output_path", ""),
    },
    "triage": {
        "scope": summary.get("triage_stage_scope", ""),
        "stage": summary.get("triage_stage", ""),
        "stage_label": summary.get("triage_stage_label", ""),
        "primary_summary": summary.get("triage_primary_summary", ""),
        "primary_report_path": summary.get("triage_primary_report", ""),
        "primary_manifest_path": summary.get("triage_primary_manifest", ""),
        "first_artifacts": split_artifacts(summary.get("triage_first_artifacts")),
        "retry_command": summary.get("triage_retry_command", ""),
        "retry_hint": summary.get("triage_retry_hint", ""),
    },
    "source_failure": {
        "summary": summary.get("source_failure_summary", ""),
        "case": summary.get("source_failure_case", ""),
        "seed": as_int(summary.get("source_failure_seed")),
        "stage": summary.get("source_failure_stage", ""),
        "kind": summary.get("source_failure_kind", ""),
        "origin": summary.get("source_failure_origin", ""),
        "retryable": as_bool(summary.get("source_failure_retryable")),
        "reporting_status": summary.get("source_failure_reporting_status", ""),
        "reporting_warning": summary.get("source_failure_reporting_warning", ""),
        "replay_command": summary.get("source_failure_replay_command", ""),
        "artifacts": {
            "failure_root_path": summary.get("source_failure_root_path", ""),
            "failure_case_dir_path": summary.get("source_failure_case_dir_path", ""),
            "commands_path": summary.get("source_failure_commands_path", ""),
            "artifact_manifest_path": summary.get("source_failure_artifact_manifest_path", ""),
            "rerun_command_path": summary.get("source_failure_rerun_command_path", ""),
            "exact_seed_path": summary.get("source_failure_exact_seed_path", ""),
            "exact_input_path": summary.get("source_failure_exact_input_path", ""),
            "exact_output_path": summary.get("source_failure_exact_output_path", ""),
            "expected_output_path": summary.get("source_failure_expected_output_path", ""),
            "invoked_command_path": summary.get("source_failure_invoked_command_path", ""),
            "helper_stdout_path": summary.get("source_failure_helper_stdout_path", ""),
            "helper_stderr_path": summary.get("source_failure_helper_stderr_path", ""),
            "helper_result_json_path": summary.get("source_failure_helper_result_json_path", ""),
            "checker_result_path": summary.get("source_failure_checker_result_path", ""),
            "checker_replay_stdout_path": summary.get("source_failure_checker_replay_stdout_path", ""),
            "checker_replay_stderr_path": summary.get("source_failure_checker_replay_stderr_path", ""),
            "mismatch_summary_path": summary.get("source_failure_mismatch_summary_path", ""),
            "retry_log_path": summary.get("source_failure_retry_log_path", ""),
            "runtime_env_path": summary.get("source_failure_runtime_env_path", ""),
            "runtime_env_exports_path": summary.get("source_failure_runtime_env_exports_path", ""),
            "preflight_manifest_path": summary.get("source_failure_preflight_manifest_path", ""),
            "setup_env_path": summary.get("source_failure_setup_env_path", ""),
            "build_command_path": summary.get("source_failure_build_command_path", ""),
            "build_stdout_path": summary.get("source_failure_build_stdout_path", ""),
            "build_stderr_path": summary.get("source_failure_build_stderr_path", ""),
            "structured_context_path": summary.get("source_failure_structured_context_path", ""),
            "manifest_snapshot_path": summary.get("source_failure_manifest_snapshot_path", ""),
            "failed_case_row_path": summary.get("source_failure_failed_case_row_path", ""),
            "suite_config_path": summary.get("source_failure_suite_config_path", ""),
            "suite_plan_path": summary.get("source_failure_suite_plan_path", ""),
            "checker_script_path": summary.get("source_failure_checker_script", ""),
            "seed_repro_script_path": summary.get("source_failure_seed_repro_script", ""),
            "preserved_input_replay_script_path": summary.get("source_failure_preserved_input_replay_script", ""),
            "active_solver_replay_script_path": summary.get("source_failure_active_solver_replay_script", ""),
        },
    },
    "artifacts": {
        "status_summary_path": str(summary_path),
        "status_report_path": current_row["status_report_path"],
        "iteration_evidence_path": current_row["iteration_evidence_path"],
        "status_artifact_manifest_path": summary.get("status_artifact_manifest", ""),
        "diagnostics_manifest_path": current_row["diagnostics_manifest_path"],
        "run_archive_root": current_row["run_archive_root"],
        "run_archive_manifest_path": summary.get("run_archive_manifest", ""),
        "run_archive_source_root_snapshot_path": summary.get("run_archive_source_root_snapshot_path", ""),
        "run_archive_source_failure_snapshot_manifest_path": summary.get("run_archive_source_failure_snapshot_manifest_path", ""),
        "run_console_stderr_path": current_row["run_console_stderr_path"],
        "run_dispatch_result_path": current_row["run_dispatch_result_path"],
        "published_smoke_summary_path": summary.get("published_smoke_summary_path", ""),
        "published_smoke_status_report_path": summary.get("published_smoke_status_report_path", ""),
        "published_smoke_failure_report_path": summary.get("published_smoke_failure_report_path", ""),
        "published_smoke_iteration_evidence_path": summary.get("published_smoke_iteration_evidence_path", ""),
        "published_smoke_retry_loop_control_path": summary.get("published_smoke_retry_loop_control_path", ""),
        "published_smoke_diagnostics_manifest_path": summary.get("published_smoke_diagnostics_manifest_path", ""),
        "published_smoke_standard_gap_json_path": summary.get("published_smoke_standard_gap_json_path", ""),
        "published_smoke_run_record_path": summary.get("published_smoke_run_record_path", ""),
        "published_smoke_run_comparison_path": summary.get("published_smoke_run_comparison_path", ""),
    },
    "gate_chain": build_gate_chain(summary, summary_path=summary_path),
}
record_path.parent.mkdir(parents=True, exist_ok=True)
record_path.write_text(json.dumps(record_payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

comparison_payload = {
    "schema": "lca_smoke_run_comparison_v1",
    "script": summary.get("script", "./lca_smoke.sh"),
    "required_standard": summary.get("required_standard", "lca_tree_stress_v5"),
    "has_previous_run": previous_row is not None,
    "summary": summary.get("run_comparison_summary", ""),
    "changed_fields": split_csv(summary.get("run_comparison_changed_fields")),
    "current_run": {
        "id": current_row["run_id"],
        "public_status": current_row["public_status"],
        "acceptance_signal_status": current_row["acceptance_signal_status"],
        "iteration_support_status": current_row["iteration_support_status"],
        "iteration_support_next_step": current_row["iteration_support_next_step"],
        "result_family": current_row["result_family"],
        "normalized_outcome": current_row["normalized_outcome"],
        "stage_label": current_row["stage_label"],
        "source_failure_case": current_row["source_failure_case"],
        "run_archive_root": current_row["run_archive_root"],
        "run_archive_source_root_snapshot_path": summary.get("run_archive_source_root_snapshot_path", ""),
        "run_archive_source_failure_snapshot_manifest_path": summary.get("run_archive_source_failure_snapshot_manifest_path", ""),
        "status_summary_path": current_row["status_summary_path"],
        "iteration_evidence_path": current_row["iteration_evidence_path"],
        "run_record_path": current_row["run_record_path"],
    },
    "previous_run": previous_row,
    "comparison_flags": {
        "public_status_changed": previous_row is not None and previous_row.get("public_status") != current_row["public_status"],
        "result_family_changed": previous_row is not None and previous_row.get("result_family") != current_row["result_family"],
        "normalized_outcome_changed": previous_row is not None and previous_row.get("normalized_outcome") != current_row["normalized_outcome"],
        "stage_label_changed": previous_row is not None and previous_row.get("stage_label") != current_row["stage_label"],
        "source_failure_case_changed": previous_row is not None and previous_row.get("source_failure_case") != current_row["source_failure_case"],
    },
    "history_index_path": str(history_path),
}
comparison_path.parent.mkdir(parents=True, exist_ok=True)
comparison_path.write_text(json.dumps(comparison_payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

history_path.parent.mkdir(parents=True, exist_ok=True)
with history_path.open("w", encoding="utf-8", newline="") as handle:
    writer = csv.DictWriter(handle, fieldnames=history_fields, delimiter="\t")
    writer.writeheader()
    for row in history_rows:
        writer.writerow({field: row.get(field, "") for field in history_fields})
PY
}

publish_launcher_smoke_summary_bundle() {
  if [[ -z "$SMOKE_OUTPUT_ROOT" ]]; then
    return 1
  fi
  mkdir -p "$SMOKE_OUTPUT_ROOT" || return 1
  cp "$LAUNCHER_STATUS_SUMMARY" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH" || return 1
  cp "$LAUNCHER_STATUS_REPORT" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH" || return 1
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" == "pass" ]]; then
    rm -f "$LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH" 2>/dev/null || true
  else
    cp "$LAUNCHER_STATUS_REPORT" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH" || return 1
  fi
  cp "$LAUNCHER_STATUS_ITERATION_EVIDENCE" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH" || return 1
  cp "$LAUNCHER_STATUS_RETRY_LOOP_CONTROL" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH" || return 1
  cp "$LAUNCHER_STATUS_RUN_RECORD" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH" || return 1
  cp "$LAUNCHER_STATUS_RUN_COMPARISON" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH" || return 1
  write_launcher_smoke_standard_gap_json "$LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH"
}

publish_launcher_smoke_diagnostics_manifest_mirror() {
  if [[ -z "$SMOKE_OUTPUT_ROOT" ]]; then
    return 1
  fi
  mkdir -p "$SMOKE_OUTPUT_ROOT" || return 1
  cp "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST" "$LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH" || return 1
}

write_launcher_status_bundle() {
  local status_parent=""
  local working_directory=""
  local shared_state_owned=0
  local outcome="${LAUNCHER_STATUS_OUTCOME:-harness_infrastructure_failure}"
  local public_status="${LAUNCHER_STATUS_PUBLIC_STATUS:-FAIL}"
  local result_family="${LAUNCHER_STATUS_RESULT_FAMILY:-unknown}"
  local normalized_rc="${LAUNCHER_STATUS_NORMALIZED_RC:-$SMOKE_EXIT_HARNESS_FAILURE}"
  local raw_rc="${LAUNCHER_STATUS_RAW_RC:-$normalized_rc}"
  local source_kind="${LAUNCHER_STATUS_SOURCE:-launcher}"
  local message="${LAUNCHER_STATUS_MESSAGE:-launcher status was not initialized}"
  local replay_case_descriptor=""
  local replay_artifact_descriptor=""
  local triage_scope="completed"
  local triage_stage="completed"
  local triage_primary_summary=""
  local triage_primary_report=""
  local triage_primary_manifest=""
  local triage_first_artifacts=""
  local triage_retry_command=""
  local triage_retry_hint=""
  local triage_stage_label="completed:completed"
  local failure_partition=""
  local failure_partition_label=""
  local acceptance_signal_status=""
  local acceptance_signal_summary=""
  local iteration_support_status=""
  local iteration_support_next_step=""
  local iteration_support_summary=""
  local command_control_mode=""
  local command_control_preferred_command_kind=""
  local should_resume_retry_loop=""
  local should_retry_smoke_directly=""
  local failure_is_terminal=""
  local gate_escalation_allowed=""
  local next_gate_status=""
  local next_gate_dependency=""
  local next_gate_summary=""

  resolve_launcher_status_root
  reset_launcher_previous_run_context
  if (( LAUNCHER_LOCK_HELD != 0 )); then
    shared_state_owned=1
    ensure_launcher_run_archive_root || return 1
    load_launcher_previous_run_context || return 1
  else
    LAUNCHER_RUN_ID=""
    LAUNCHER_RUN_ARCHIVE_ROOT=""
    LAUNCHER_RUN_EXPORT_ALIAS_ROOT=""
    LAUNCHER_RUN_CONSOLE_LOG=""
    LAUNCHER_RUN_STATUS_SUMMARY_PATH=""
    LAUNCHER_RUN_STATUS_REPORT_PATH=""
    LAUNCHER_RUN_STATUS_ITERATION_EVIDENCE_PATH=""
    LAUNCHER_RUN_STATUS_RETRY_LOOP_CONTROL_PATH=""
    LAUNCHER_RUN_STATUS_DIAGNOSTICS_PATH=""
    LAUNCHER_RUN_STATUS_ARTIFACT_MANIFEST_PATH=""
    LAUNCHER_RUN_STATUS_RUN_RECORD_PATH=""
    LAUNCHER_RUN_STATUS_RUN_COMPARISON_PATH=""
    LAUNCHER_RUN_PREFLIGHT_ROOT=""
    LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT=""
    LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH=""
    LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT=""
    LAUNCHER_RUN_ARTIFACT_MANIFEST=""
    if [[ -z "$LAUNCHER_RUN_STARTED_AT_UTC" ]]; then
      LAUNCHER_RUN_STARTED_AT_UTC="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    fi
    if (( LAUNCHER_RUN_STARTED_SECONDS < 0 )); then
      LAUNCHER_RUN_STARTED_SECONDS=$SECONDS
    fi
  fi
  status_parent="$(dirname "$LAUNCHER_STATUS_ROOT")"
  ensure_launcher_directory "$status_parent" "launcher status parent" || return 1
  if [[ -e "$LAUNCHER_STATUS_ROOT" ]]; then
    remove_path_retry "$LAUNCHER_STATUS_ROOT" || return 1
  fi
  ensure_launcher_directory "$LAUNCHER_STATUS_ROOT" "launcher status root" || return 1
  working_directory="$(pwd -P 2>/dev/null || pwd)"
  replay_case_descriptor="$(launcher_replay_case_descriptor)"
  replay_artifact_descriptor="$(launcher_replay_artifact_descriptor)"
  failure_partition="$(launcher_failure_partition_key)"
  failure_partition_label="$(launcher_failure_partition_label "$failure_partition")"
  refresh_launcher_status_diagnostics_paths
  if [[ "$outcome" != "pass" ]]; then
    triage_scope="$(launcher_triage_stage_scope)"
    triage_stage="$(launcher_triage_stage_name)"
    triage_stage_label="$triage_scope:$triage_stage"
    triage_primary_summary="$(launcher_triage_primary_summary)"
    triage_primary_report="$(launcher_triage_primary_report)"
    triage_primary_manifest="$(launcher_triage_primary_manifest)"
    triage_first_artifacts="$(launcher_triage_first_artifacts)"
    triage_retry_command="$(launcher_triage_retry_command)"
    triage_retry_hint="$(launcher_triage_retry_hint)"
  fi
  finalize_launcher_run_identity
  summarize_launcher_run_comparison \
    "$public_status" \
    "$result_family" \
    "$outcome" \
    "$triage_stage_label" \
    "$replay_case_descriptor"
  refresh_launcher_retry_loop_control
  acceptance_signal_status="$(launcher_acceptance_signal_status)"
  acceptance_signal_summary="$(launcher_acceptance_signal_summary)"
  iteration_support_status="$(launcher_iteration_support_status)"
  iteration_support_next_step="$(launcher_iteration_support_next_step)"
  iteration_support_summary="$(launcher_iteration_support_summary)"
  command_control_mode="$(launcher_command_control_mode)"
  command_control_preferred_command_kind="$(launcher_command_control_preferred_command_kind)"
  should_resume_retry_loop="$(launcher_should_resume_retry_loop)"
  should_retry_smoke_directly="$(launcher_should_retry_smoke_directly)"
  failure_is_terminal="$(launcher_failure_is_terminal)"
  gate_escalation_allowed="$(launcher_gate_escalation_allowed)"
  next_gate_status="$(launcher_next_gate_status)"
  next_gate_dependency="$(launcher_next_gate_dependency)"
  next_gate_summary="$(launcher_next_gate_summary)"

  {
    echo "script=./lca_smoke.sh"
    echo "required_standard=lca_tree_stress_v5"
    echo "standard_signal_role=smoke_wrapper_early_signal"
    echo "run_id=$LAUNCHER_RUN_ID"
    echo "run_started_at_utc=$LAUNCHER_RUN_STARTED_AT_UTC"
    echo "run_finished_at_utc=$LAUNCHER_RUN_FINISHED_AT_UTC"
    echo "run_elapsed_seconds=$LAUNCHER_RUN_ELAPSED_SECONDS"
    echo "public_status=$public_status"
    echo "result_family=$result_family"
    echo "failure_partition=$failure_partition"
    echo "failure_partition_label=$failure_partition_label"
    echo "normalized_exit_code=$normalized_rc"
    echo "raw_exit_code=$raw_rc"
    echo "normalized_outcome=$outcome"
    echo "outcome_source=$source_kind"
    echo "outcome_summary=$message"
    echo "acceptance_signal_status=$acceptance_signal_status"
    echo "acceptance_signal_summary=$acceptance_signal_summary"
    echo "iteration_support_status=$iteration_support_status"
    echo "iteration_support_next_step=$iteration_support_next_step"
    echo "iteration_support_summary=$iteration_support_summary"
    echo "command_control_mode=$command_control_mode"
    echo "command_control_preferred_command_kind=$command_control_preferred_command_kind"
    echo "should_resume_retry_loop=$should_resume_retry_loop"
    echo "should_retry_smoke_directly=$should_retry_smoke_directly"
    echo "failure_is_terminal=$failure_is_terminal"
    echo "working_directory=$working_directory"
    echo "original_launch_working_directory=$LAUNCHER_ORIGINAL_PWD"
    echo "branch_root=$BRANCH_ROOT"
    echo "artifacts_root=${ARTIFACTS_ROOT:-$BRANCH_ROOT/artifacts/lca_tree_stress_v5}"
    echo "smoke_output_root=$SMOKE_OUTPUT_ROOT"
    echo "smoke_failure_root=$SMOKE_FAILURE_ROOT"
    echo "launcher_failure_root=$LAUNCHER_FAILURE_ROOT"
    echo "status_root=$LAUNCHER_STATUS_ROOT"
    echo "status_summary_path=$LAUNCHER_STATUS_SUMMARY"
    echo "status_report=$LAUNCHER_STATUS_REPORT"
    echo "status_report_path=$LAUNCHER_STATUS_REPORT"
    echo "iteration_evidence_path=$LAUNCHER_STATUS_ITERATION_EVIDENCE"
    echo "status_artifact_manifest=$LAUNCHER_STATUS_ARTIFACT_MANIFEST"
    echo "status_diagnostics_manifest=$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST"
    echo "run_history_index_path=$LAUNCHER_RUN_HISTORY_INDEX"
    echo "run_record_path=$LAUNCHER_STATUS_RUN_RECORD"
    echo "run_comparison_path=$LAUNCHER_STATUS_RUN_COMPARISON"
    echo "run_dispatch_result_path=$LAUNCHER_DISPATCH_RESULT_PATH"
    echo "run_comparison_summary=$LAUNCHER_RUN_COMPARISON_SUMMARY"
    echo "run_comparison_changed_fields=$LAUNCHER_RUN_COMPARISON_CHANGED_FIELDS"
    echo "previous_run_id=$LAUNCHER_PREVIOUS_RUN_ID"
    echo "previous_run_archive_root=$LAUNCHER_PREVIOUS_RUN_ARCHIVE_ROOT"
    echo "previous_run_public_status=$LAUNCHER_PREVIOUS_RUN_PUBLIC_STATUS"
    echo "previous_run_result_family=$LAUNCHER_PREVIOUS_RUN_RESULT_FAMILY"
    echo "previous_run_normalized_outcome=$LAUNCHER_PREVIOUS_RUN_NORMALIZED_OUTCOME"
    echo "previous_run_stage_label=$LAUNCHER_PREVIOUS_RUN_STAGE_LABEL"
    echo "previous_run_source_failure_case=$LAUNCHER_PREVIOUS_RUN_SOURCE_FAILURE_CASE"
    echo "previous_run_status_summary_path=$LAUNCHER_PREVIOUS_RUN_STATUS_SUMMARY_PATH"
    echo "previous_run_iteration_evidence_path=$LAUNCHER_PREVIOUS_RUN_ITERATION_EVIDENCE_PATH"
    echo "run_history_root=$LAUNCHER_RUN_HISTORY_ROOT"
    echo "run_archive_root=$LAUNCHER_RUN_ARCHIVE_ROOT"
    echo "run_archive_source_root_snapshot_path=$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT"
    echo "run_archive_source_failure_snapshot_manifest_path=$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH"
    echo "run_archive_summary_path=$LAUNCHER_RUN_STATUS_SUMMARY_PATH"
    echo "run_archive_status_report_path=$LAUNCHER_RUN_STATUS_REPORT_PATH"
    echo "run_archive_iteration_evidence_path=$LAUNCHER_RUN_STATUS_ITERATION_EVIDENCE_PATH"
    echo "run_archive_diagnostics_manifest_path=$LAUNCHER_RUN_STATUS_DIAGNOSTICS_PATH"
    echo "run_archive_run_record_path=$LAUNCHER_RUN_STATUS_RUN_RECORD_PATH"
    echo "run_archive_run_comparison_path=$LAUNCHER_RUN_STATUS_RUN_COMPARISON_PATH"
    echo "run_archive_manifest=$LAUNCHER_RUN_ARTIFACT_MANIFEST"
    echo "run_console_stderr_path=$LAUNCHER_RUN_CONSOLE_LOG"
    echo "published_smoke_summary_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH"
    echo "published_smoke_status_report_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH"
    echo "published_smoke_failure_report_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH"
    echo "published_smoke_iteration_evidence_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH"
    echo "published_smoke_retry_loop_control_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH"
    echo "published_smoke_diagnostics_manifest_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH"
    echo "published_smoke_standard_gap_json_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH"
    echo "published_smoke_run_record_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH"
    echo "published_smoke_run_comparison_path=$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH"
    echo "retry_loop_control_path=$LAUNCHER_STATUS_RETRY_LOOP_CONTROL"
    echo "retry_loop_action=$LAUNCHER_RETRY_LOOP_ACTION"
    echo "retry_loop_preferred_command=$LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND"
    echo "retry_loop_launch_command=$LAUNCHER_RETRY_LOOP_LAUNCH_COMMAND"
    echo "retry_loop_direct_command=$LAUNCHER_RETRY_LOOP_DIRECT_COMMAND"
    echo "retry_loop_hint=$LAUNCHER_RETRY_LOOP_HINT"
    echo "retry_loop_log_path=$LAUNCHER_RETRY_LOOP_LOG_PATH"
    echo "retry_loop_solver_seed_file=$RETRY_LOOP_SOLVER_SEED_REL"
    echo "retry_loop_analysis_seed_file=$RETRY_LOOP_ANALYSIS_SEED_REL"
    echo "next_gate_command=$RETRY_LOOP_NEXT_GATE_COMMAND"
    echo "next_gate_status=$next_gate_status"
    echo "next_gate_dependency=$next_gate_dependency"
    echo "next_gate_summary=$next_gate_summary"
    echo "gate_escalation_allowed=$gate_escalation_allowed"
    echo "source_root=$LAUNCHER_STATUS_SOURCE_ROOT"
    echo "source_summary=$LAUNCHER_STATUS_SOURCE_SUMMARY"
    echo "source_report=$LAUNCHER_STATUS_SOURCE_REPORT"
    echo "smoke_suite_config_path=$LAUNCHER_STATUS_SUITE_CONFIG_PATH"
    echo "smoke_suite_plan_path=$LAUNCHER_STATUS_SUITE_PLAN_PATH"
    echo "smoke_environment_validation_report=$LAUNCHER_STATUS_ENV_VALIDATION_REPORT"
    echo "smoke_environment_preflight_manifest_path=$LAUNCHER_STATUS_ENV_MANIFEST_PATH"
    echo "smoke_environment_setup_env_path=$LAUNCHER_STATUS_ENV_SETUP_ENV_PATH"
    echo "smoke_environment_build_command_path=$LAUNCHER_STATUS_ENV_BUILD_COMMAND_PATH"
    echo "smoke_manifest_snapshot_path=$LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SNAPSHOT_PATH"
    echo "dispatch_timeout_s=$LAUNCHER_DISPATCH_TIMEOUT_S"
    echo "source_failure_summary=$LAUNCHER_REPLAY_SUMMARY"
    echo "source_failure_case=$replay_case_descriptor"
    echo "source_failure_seed=$LAUNCHER_REPLAY_SEED"
    echo "source_failure_stage=$LAUNCHER_SOURCE_FAILURE_STAGE"
    echo "source_failure_replay_command=$LAUNCHER_REPLAY_COMMAND"
    echo "source_failure_root_path=$LAUNCHER_REPLAY_FAILURE_ROOT"
    echo "source_failure_case_dir_path=$LAUNCHER_REPLAY_FAILURE_CASE_DIR"
    echo "source_failure_commands_path=$LAUNCHER_REPLAY_COMMANDS_PATH"
    echo "source_failure_artifact_manifest_path=$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH"
    echo "source_failure_rerun_command_path=$LAUNCHER_REPLAY_RERUN_COMMAND_PATH"
    echo "source_failure_exact_seed_path=$LAUNCHER_REPLAY_EXACT_SEED_PATH"
    echo "source_failure_exact_input_path=$LAUNCHER_REPLAY_EXACT_INPUT_PATH"
    echo "source_failure_exact_output_path=$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH"
    echo "source_failure_expected_output_path=$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH"
    echo "source_failure_invoked_command_path=$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH"
    echo "source_failure_artifacts=$replay_artifact_descriptor"
    echo "source_failure_kind=$LAUNCHER_SOURCE_FAILURE_KIND"
    echo "source_failure_origin=$LAUNCHER_SOURCE_FAILURE_ORIGIN"
    echo "source_failure_retryable=$LAUNCHER_SOURCE_FAILURE_RETRYABLE"
    echo "source_failure_reporting_status=$LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS"
    echo "source_failure_reporting_warning=$LAUNCHER_SOURCE_FAILURE_REPORTING_WARNING"
    echo "source_failure_helper_stdout_path=$LAUNCHER_SOURCE_HELPER_STDOUT"
    echo "source_failure_helper_stderr_path=$LAUNCHER_SOURCE_HELPER_STDERR"
    echo "source_failure_helper_result_json_path=$LAUNCHER_SOURCE_HELPER_RESULT_JSON"
    echo "source_failure_checker_result_path=$LAUNCHER_SOURCE_CHECKER_RESULT_PATH"
    echo "source_failure_checker_replay_stdout_path=$LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH"
    echo "source_failure_checker_replay_stderr_path=$LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH"
    echo "source_failure_mismatch_summary_path=$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH"
    echo "source_failure_retry_log_path=$LAUNCHER_SOURCE_RETRY_LOG_PATH"
    echo "source_failure_runtime_env_path=$LAUNCHER_SOURCE_RUNTIME_ENV_PATH"
    echo "source_failure_runtime_env_exports_path=$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH"
    echo "source_failure_preflight_manifest_path=$LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH"
    echo "source_failure_setup_env_path=$LAUNCHER_SOURCE_SETUP_ENV_PATH"
    echo "source_failure_build_command_path=$LAUNCHER_SOURCE_BUILD_COMMAND_PATH"
    echo "source_failure_build_stdout_path=$LAUNCHER_SOURCE_BUILD_STDOUT_PATH"
    echo "source_failure_build_stderr_path=$LAUNCHER_SOURCE_BUILD_STDERR_PATH"
    echo "source_failure_structured_context_path=$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH"
    echo "source_failure_manifest_snapshot_path=$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH"
    echo "source_failure_failed_case_row_path=$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH"
    echo "source_failure_suite_config_path=$LAUNCHER_SOURCE_SUITE_CONFIG_PATH"
    echo "source_failure_suite_plan_path=$LAUNCHER_SOURCE_SUITE_PLAN_PATH"
    echo "source_failure_checker_script=$LAUNCHER_SOURCE_CHECKER_SCRIPT"
    echo "source_failure_seed_repro_script=$LAUNCHER_SOURCE_SEED_REPRO_SCRIPT"
    echo "source_failure_preserved_input_replay_script=$LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT"
    echo "source_failure_active_solver_replay_script=$LAUNCHER_REPLAY_ACTIVE_SCRIPT"
    echo "invocation_command=$LAUNCHER_INVOCATION_COMMAND"
    echo "dispatch_command=$LAUNCHER_DISPATCH_COMMAND"
    if [[ "$outcome" == "pass" ]]; then
      echo "standard_gap_status=ready_for_gate_escalation"
      echo "standard_gap_summary=smoke passed; this working tree has fresh smoke evidence and can escalate to heavier gates"
    else
      echo "standard_gap_status=smoke_blocker_detected"
      echo "standard_gap_summary=$message"
    fi
    echo "gate_chain_ac2_status=$(launcher_gate_chain_status 2)"
    echo "gate_chain_ac2_summary=$(launcher_gate_chain_summary 2)"
    echo "gate_chain_ac3_status=$(launcher_gate_chain_status 3)"
    echo "gate_chain_ac3_summary=$(launcher_gate_chain_summary 3)"
    echo "gate_chain_ac4_status=$(launcher_gate_chain_status 4)"
    echo "gate_chain_ac4_summary=$(launcher_gate_chain_summary 4)"
    echo "gate_chain_ac5_status=$(launcher_gate_chain_status 5)"
    echo "gate_chain_ac5_summary=$(launcher_gate_chain_summary 5)"
    echo "gate_chain_ac6_status=$(launcher_gate_chain_status 6)"
    echo "gate_chain_ac6_summary=$(launcher_gate_chain_summary 6)"
    echo "triage_stage_scope=$triage_scope"
    echo "triage_stage=$triage_stage"
    echo "triage_stage_label=$triage_stage_label"
    if [[ "$outcome" != "pass" ]]; then
      echo "triage_primary_summary=$triage_primary_summary"
      echo "triage_primary_report=$triage_primary_report"
      echo "triage_primary_manifest=$triage_primary_manifest"
      echo "triage_first_artifacts=$triage_first_artifacts"
      echo "triage_retry_command=$triage_retry_command"
      echo "triage_retry_hint=$triage_retry_hint"
    fi
    if [[ -n "$LAUNCHER_FAILURE_STAGE" ]]; then
      echo "launcher_stage=$LAUNCHER_FAILURE_STAGE"
    fi
    if [[ -n "$LAUNCHER_LAST_CHECK_STATUS" ]]; then
      echo "last_check_kind=$LAUNCHER_LAST_CHECK_KIND"
      echo "last_check_label=$LAUNCHER_LAST_CHECK_LABEL"
      echo "last_check_status=$LAUNCHER_LAST_CHECK_STATUS"
      echo "last_check_detail=$LAUNCHER_LAST_CHECK_DETAIL"
      echo "last_check_artifact=$LAUNCHER_LAST_CHECK_ARTIFACT"
    fi
  } > "$LAUNCHER_STATUS_SUMMARY"

  write_launcher_iteration_evidence \
    "${triage_scope:-completed}" \
    "${triage_stage:-completed}" \
    "$triage_primary_summary" \
    "$triage_primary_report" \
    "$triage_primary_manifest" \
    "$triage_first_artifacts" \
    "$triage_retry_command" \
    "$triage_retry_hint"
  write_launcher_retry_loop_control_json "$LAUNCHER_STATUS_RETRY_LOOP_CONTROL"

  {
    echo "# lca_smoke Status Report"
    echo
    echo "- Run id: \`$LAUNCHER_RUN_ID\`"
    echo "- Run started at UTC: \`$LAUNCHER_RUN_STARTED_AT_UTC\`"
    echo "- Run finished at UTC: \`$LAUNCHER_RUN_FINISHED_AT_UTC\`"
    echo "- Run elapsed seconds: \`$LAUNCHER_RUN_ELAPSED_SECONDS\`"
    echo "- Public status: \`$public_status\`"
    echo "- Result family: \`$result_family\`"
    echo "- Failure partition: \`$failure_partition_label\`"
    echo "- Normalized outcome: \`$outcome\`"
    echo "- Normalized exit code: \`$normalized_rc\`"
    echo "- Raw exit code: \`$raw_rc\`"
    echo "- Outcome source: \`$source_kind\`"
    echo "- Required standard: \`lca_tree_stress_v5\`"
    echo "- Summary: \`$message\`"
    echo "- Working directory: \`$working_directory\`"
    echo "- Original launch working directory: \`$LAUNCHER_ORIGINAL_PWD\`"
    echo "- Branch root: \`$BRANCH_ROOT\`"
    echo "- Artifacts root: \`${ARTIFACTS_ROOT:-$BRANCH_ROOT/artifacts/lca_tree_stress_v5}\`"
    echo "- Smoke output root: \`$SMOKE_OUTPUT_ROOT\`"
    echo "- Smoke failure root: \`$SMOKE_FAILURE_ROOT\`"
    echo "- Launcher failure root: \`$LAUNCHER_FAILURE_ROOT\`"
    echo "- Status root: \`$LAUNCHER_STATUS_ROOT\`"
    echo "- Run history root: \`$LAUNCHER_RUN_HISTORY_ROOT\`"
    echo "- Run history index: \`$LAUNCHER_RUN_HISTORY_INDEX\`"
    echo "- Run archive root: \`$LAUNCHER_RUN_ARCHIVE_ROOT\`"
    echo "- Run-archive source snapshot: \`$LAUNCHER_RUN_SOURCE_ROOT_SNAPSHOT\`"
    echo "- Run-archive source snapshot manifest: \`$LAUNCHER_RUN_SOURCE_FAILURE_SNAPSHOT_MANIFEST_PATH\`"
    echo "- Run archive manifest: \`$LAUNCHER_RUN_ARTIFACT_MANIFEST\`"
    echo "- Launcher console transcript: \`$LAUNCHER_RUN_CONSOLE_LOG\`"
    echo "- Dispatch result snapshot: \`$LAUNCHER_DISPATCH_RESULT_PATH\`"
    echo "- Run record json: \`$LAUNCHER_STATUS_RUN_RECORD\`"
    echo "- Run comparison json: \`$LAUNCHER_STATUS_RUN_COMPARISON\`"
    if [[ -n "$LAUNCHER_STATUS_SOURCE_ROOT" ]]; then
      echo "- Source root: \`$LAUNCHER_STATUS_SOURCE_ROOT\`"
    fi
    if [[ -n "$LAUNCHER_STATUS_SOURCE_SUMMARY" ]]; then
      echo "- Source summary: \`$LAUNCHER_STATUS_SOURCE_SUMMARY\`"
    fi
    if [[ -n "$LAUNCHER_STATUS_SOURCE_REPORT" ]]; then
      echo "- Source report: \`$LAUNCHER_STATUS_SOURCE_REPORT\`"
    fi
    echo
    echo "## Acceptance Signal"
    echo
    echo "- Acceptance status: \`$acceptance_signal_status\`"
    echo "- Acceptance summary: \`$acceptance_signal_summary\`"
    echo
    echo "## Iteration Support"
    echo
    echo "- Iteration support: \`$iteration_support_status\`"
    echo "- Next step: \`$iteration_support_next_step\`"
    echo "- Iteration summary: \`$iteration_support_summary\`"
    echo "- Control action: \`$LAUNCHER_RETRY_LOOP_ACTION\`"
    echo "- Preferred next command: \`$LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND\`"
    echo "- Command control mode: \`$command_control_mode\`"
    echo "- Preferred command kind: \`$command_control_preferred_command_kind\`"
    echo "- Failure terminal: \`$( [[ "$failure_is_terminal" == "1" ]] && printf 'yes' || printf 'no' )\`"
    echo "- Gate escalation allowed: \`$( [[ "$gate_escalation_allowed" == "1" ]] && printf 'yes' || printf 'no' )\`"
    echo "- Next gate command: \`$RETRY_LOOP_NEXT_GATE_COMMAND\`"
    echo "- Next gate status: \`$next_gate_status\`"
    echo "- Next gate depends on: \`$next_gate_dependency\`"
    echo "- Next gate summary: \`$next_gate_summary\`"
    if [[ "$outcome" != "pass" ]]; then
      echo
      echo "## Failed Stage"
      echo
      echo "- Failed stage scope: \`$triage_scope\`"
      echo "- Failed stage: \`$triage_stage\`"
      echo "- Stage label: \`$triage_stage_label\`"
      echo "- Primary summary: \`$triage_primary_summary\`"
      echo "- Primary report: \`$triage_primary_report\`"
      echo "- Primary manifest: \`$triage_primary_manifest\`"
      echo "- Iteration evidence: \`$LAUNCHER_STATUS_ITERATION_EVIDENCE\`"
      echo "- Inspect first: \`$triage_first_artifacts\`"
    fi
    echo
    echo "## Standard Gap"
    echo
    if [[ "$outcome" == "pass" ]]; then
      echo "- Status: \`ready_for_gate_escalation\`"
      echo "- Explanation: \`smoke passed; this working tree has fresh smoke evidence and can escalate to heavier gates\`"
    else
      echo "- Status: \`smoke_blocker_detected\`"
      echo "- Explanation: \`$message\`"
      echo "- Triage focus: \`$triage_retry_hint\`"
    fi
    echo
    echo "## Gate Chain"
    echo
    write_launcher_gate_chain_report
    echo "- Smoke summary mirror: \`$LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH\`"
    echo "- Smoke report mirror: \`$LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH\`"
    if [[ "$outcome" != "pass" ]]; then
      echo "- Smoke failure-report mirror: \`$LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH\`"
    fi
    echo "- Smoke iteration-evidence mirror: \`$LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH\`"
    echo "- Smoke retry-loop control mirror: \`$LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH\`"
    echo "- Smoke diagnostics-manifest mirror: \`$LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH\`"
    echo "- Smoke standard-gap json: \`$LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH\`"
    echo "- Smoke run-record mirror: \`$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH\`"
    echo "- Smoke run-comparison mirror: \`$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH\`"
    echo
    echo "## Iteration Comparison"
    echo
    echo "- Summary: \`$LAUNCHER_RUN_COMPARISON_SUMMARY\`"
    echo "- Changed fields: \`${LAUNCHER_RUN_COMPARISON_CHANGED_FIELDS:-none}\`"
    echo "- Comparison artifact: \`$LAUNCHER_STATUS_RUN_COMPARISON\`"
    if [[ -n "$LAUNCHER_PREVIOUS_RUN_ID" ]]; then
      echo "- Previous run id: \`$LAUNCHER_PREVIOUS_RUN_ID\`"
      echo "- Previous run archive root: \`$LAUNCHER_PREVIOUS_RUN_ARCHIVE_ROOT\`"
      echo "- Previous normalized outcome: \`$LAUNCHER_PREVIOUS_RUN_NORMALIZED_OUTCOME\`"
      echo "- Previous stage label: \`$LAUNCHER_PREVIOUS_RUN_STAGE_LABEL\`"
      echo "- Previous failure case: \`$LAUNCHER_PREVIOUS_RUN_SOURCE_FAILURE_CASE\`"
      echo "- Previous status summary: \`$LAUNCHER_PREVIOUS_RUN_STATUS_SUMMARY_PATH\`"
      echo "- Previous iteration evidence: \`$LAUNCHER_PREVIOUS_RUN_ITERATION_EVIDENCE_PATH\`"
    fi
    if [[ -n "$LAUNCHER_REPLAY_SUMMARY" || -n "$replay_case_descriptor" || -n "$LAUNCHER_REPLAY_COMMAND" || -n "$replay_artifact_descriptor" ]]; then
      echo
      echo "## Replay Details"
      echo
      if [[ -n "$LAUNCHER_REPLAY_SUMMARY" ]]; then
        echo "- Concise replay summary: \`$LAUNCHER_REPLAY_SUMMARY\`"
      fi
      if [[ -n "$replay_case_descriptor" ]]; then
        echo "- Failing case: \`$replay_case_descriptor\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_SEED" ]]; then
        echo "- Seed: \`$LAUNCHER_REPLAY_SEED\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_COMMAND" ]]; then
        echo "- Preferred replay command: \`$LAUNCHER_REPLAY_COMMAND\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_RERUN_COMMAND_PATH" ]]; then
        echo "- Rerun command snapshot: \`$LAUNCHER_REPLAY_RERUN_COMMAND_PATH\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_FAILURE_ROOT" ]]; then
        echo "- Failure root: \`$LAUNCHER_REPLAY_FAILURE_ROOT\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_FAILURE_CASE_DIR" ]]; then
        echo "- Failure case dir: \`$LAUNCHER_REPLAY_FAILURE_CASE_DIR\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_EXACT_SEED_PATH" ]]; then
        echo "- Exact seed snapshot: \`$LAUNCHER_REPLAY_EXACT_SEED_PATH\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_EXACT_INPUT_PATH" ]]; then
        echo "- Exact input snapshot: \`$LAUNCHER_REPLAY_EXACT_INPUT_PATH\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH" ]]; then
        echo "- Exact output snapshot: \`$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH" ]]; then
        echo "- Expected output snapshot: \`$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH" ]]; then
        echo "- Invoked command snapshot: \`$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_COMMANDS_PATH" ]]; then
        echo "- Commands snapshot: \`$LAUNCHER_REPLAY_COMMANDS_PATH\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH" ]]; then
        echo "- Artifact manifest: \`$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH\`"
      fi
      if [[ -n "$LAUNCHER_REPLAY_ACTIVE_SCRIPT" ]]; then
        echo "- Active solver replay script: \`$LAUNCHER_REPLAY_ACTIVE_SCRIPT\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH" ]]; then
        echo "- Structured failure context: \`$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH" ]]; then
        echo "- Runtime env snapshot: \`$LAUNCHER_SOURCE_RUNTIME_ENV_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH" ]]; then
        echo "- Runtime env exports: \`$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH" ]]; then
        echo "- Manifest snapshot: \`$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_SUITE_CONFIG_PATH" ]]; then
        echo "- Suite config snapshot: \`$LAUNCHER_SOURCE_SUITE_CONFIG_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_SUITE_PLAN_PATH" ]]; then
        echo "- Suite plan snapshot: \`$LAUNCHER_SOURCE_SUITE_PLAN_PATH\`"
      fi
    fi
    echo
    echo "## Diagnostics"
    echo
    echo "- Iteration evidence: \`$LAUNCHER_STATUS_ITERATION_EVIDENCE\`"
    echo "- Retry-loop control: \`$LAUNCHER_STATUS_RETRY_LOOP_CONTROL\`"
    echo "- Diagnostics manifest: \`$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST\`"
    echo "- Run history index: \`$LAUNCHER_RUN_HISTORY_INDEX\`"
    echo "- Run record json: \`$LAUNCHER_STATUS_RUN_RECORD\`"
    echo "- Run comparison json: \`$LAUNCHER_STATUS_RUN_COMPARISON\`"
    echo "- Launcher console transcript: \`$LAUNCHER_RUN_CONSOLE_LOG\`"
    if [[ -n "$LAUNCHER_STATUS_SUITE_CONFIG_PATH" ]]; then
      echo "- Suite config: \`$LAUNCHER_STATUS_SUITE_CONFIG_PATH\`"
    fi
    if [[ -n "$LAUNCHER_STATUS_SUITE_PLAN_PATH" ]]; then
      echo "- Suite plan: \`$LAUNCHER_STATUS_SUITE_PLAN_PATH\`"
    fi
    if [[ -n "$LAUNCHER_STATUS_ENV_VALIDATION_REPORT" ]]; then
      echo "- Environment validation report: \`$LAUNCHER_STATUS_ENV_VALIDATION_REPORT\`"
    fi
    if [[ -n "$LAUNCHER_STATUS_ENV_MANIFEST_PATH" ]]; then
      echo "- Environment preflight manifest: \`$LAUNCHER_STATUS_ENV_MANIFEST_PATH\`"
    fi
    if [[ -n "$LAUNCHER_STATUS_ENV_SETUP_ENV_PATH" ]]; then
      echo "- Environment setup snapshot: \`$LAUNCHER_STATUS_ENV_SETUP_ENV_PATH\`"
    fi
    if [[ -n "$LAUNCHER_STATUS_ENV_BUILD_COMMAND_PATH" ]]; then
      echo "- Build command snapshot: \`$LAUNCHER_STATUS_ENV_BUILD_COMMAND_PATH\`"
    fi
    if [[ -n "$LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SNAPSHOT_PATH" ]]; then
      echo "- Manifest snapshot: \`$LAUNCHER_STATUS_ENV_SMOKE_MANIFEST_SNAPSHOT_PATH\`"
    fi
    if [[ -n "$LAUNCHER_DISPATCH_TIMEOUT_S" ]]; then
      echo "- Dispatch timeout: \`$LAUNCHER_DISPATCH_TIMEOUT_S\`"
    fi
    if [[ -n "$LAUNCHER_SOURCE_FAILURE_KIND" || -n "$LAUNCHER_SOURCE_HELPER_RESULT_JSON" || -n "$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH" || -n "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH" ]]; then
      if [[ -n "$LAUNCHER_SOURCE_FAILURE_KIND" ]]; then
        echo "- Source failure kind: \`$LAUNCHER_SOURCE_FAILURE_KIND\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_FAILURE_ORIGIN" ]]; then
        echo "- Source failure origin: \`$LAUNCHER_SOURCE_FAILURE_ORIGIN\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_FAILURE_STAGE" ]]; then
        echo "- Source failure stage: \`$LAUNCHER_SOURCE_FAILURE_STAGE\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_FAILURE_RETRYABLE" ]]; then
        echo "- Source failure retryable: \`$LAUNCHER_SOURCE_FAILURE_RETRYABLE\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS" ]]; then
        echo "- Source failure reporting status: \`$LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_FAILURE_REPORTING_WARNING" ]]; then
        echo "- Source failure reporting warning: \`$LAUNCHER_SOURCE_FAILURE_REPORTING_WARNING\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_HELPER_STDOUT" ]]; then
        echo "- Source helper stdout: \`$LAUNCHER_SOURCE_HELPER_STDOUT\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_HELPER_STDERR" ]]; then
        echo "- Source helper stderr: \`$LAUNCHER_SOURCE_HELPER_STDERR\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_HELPER_RESULT_JSON" ]]; then
        echo "- Source helper result json: \`$LAUNCHER_SOURCE_HELPER_RESULT_JSON\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_CHECKER_RESULT_PATH" ]]; then
        echo "- Source checker result: \`$LAUNCHER_SOURCE_CHECKER_RESULT_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH" ]]; then
        echo "- Source checker replay stdout: \`$LAUNCHER_SOURCE_CHECKER_REPLAY_STDOUT_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH" ]]; then
        echo "- Source checker replay stderr: \`$LAUNCHER_SOURCE_CHECKER_REPLAY_STDERR_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH" ]]; then
        echo "- Source mismatch summary: \`$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_RETRY_LOG_PATH" ]]; then
        echo "- Source retry log: \`$LAUNCHER_SOURCE_RETRY_LOG_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH" ]]; then
        echo "- Source runtime env: \`$LAUNCHER_SOURCE_RUNTIME_ENV_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH" ]]; then
        echo "- Source runtime env exports: \`$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH" ]]; then
        echo "- Source preflight manifest: \`$LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_SETUP_ENV_PATH" ]]; then
        echo "- Source setup env: \`$LAUNCHER_SOURCE_SETUP_ENV_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_BUILD_COMMAND_PATH" ]]; then
        echo "- Source build command: \`$LAUNCHER_SOURCE_BUILD_COMMAND_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_BUILD_STDOUT_PATH" ]]; then
        echo "- Source build stdout: \`$LAUNCHER_SOURCE_BUILD_STDOUT_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_BUILD_STDERR_PATH" ]]; then
        echo "- Source build stderr: \`$LAUNCHER_SOURCE_BUILD_STDERR_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH" ]]; then
        echo "- Source structured context: \`$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH" ]]; then
        echo "- Source failed-case row: \`$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_CHECKER_SCRIPT" ]]; then
        echo "- Source checker script: \`$LAUNCHER_SOURCE_CHECKER_SCRIPT\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_SEED_REPRO_SCRIPT" ]]; then
        echo "- Source seed repro script: \`$LAUNCHER_SOURCE_SEED_REPRO_SCRIPT\`"
      fi
      if [[ -n "$LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT" ]]; then
        echo "- Source preserved-input replay script: \`$LAUNCHER_SOURCE_PRESERVED_INPUT_REPLAY_SCRIPT\`"
      fi
      echo "- Next iteration anchor: start with \`$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST\`, \`${LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH:-$LAUNCHER_REPLAY_COMMANDS_PATH}\`, \`${LAUNCHER_REPLAY_COMMANDS_PATH:-$LAUNCHER_STATUS_SUITE_PLAN_PATH}\`, and \`${LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH:-$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH}\` before rerunning smoke."
    else
      echo "- Next iteration anchor: start with \`$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST\`, \`$LAUNCHER_STATUS_SUITE_CONFIG_PATH\`, \`$LAUNCHER_STATUS_SUITE_PLAN_PATH\`, and \`$LAUNCHER_STATUS_ENV_VALIDATION_REPORT\` before escalating to the next gate."
    fi
    if [[ "$outcome" != "pass" ]]; then
      echo
      echo "## Retry Next"
      echo
      echo "- Retry command: \`$triage_retry_command\`"
      echo "- Guidance: \`$triage_retry_hint\`"
      echo "- Retry-loop action: \`$LAUNCHER_RETRY_LOOP_ACTION\`"
      echo "- Preferred retry-loop command: \`$LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND\`"
      echo "- Launch-helper retry-loop command: \`$LAUNCHER_RETRY_LOOP_LAUNCH_COMMAND\`"
      echo "- Direct retry-loop command: \`$LAUNCHER_RETRY_LOOP_DIRECT_COMMAND\`"
      echo "- Retry-loop log path: \`$LAUNCHER_RETRY_LOOP_LOG_PATH\`"
      echo "- Retry-loop control json: \`$LAUNCHER_STATUS_RETRY_LOOP_CONTROL\`"
    fi
    echo
    echo "## Commands"
    echo
    echo "Invocation command:"
    echo
    echo "\`\`\`bash"
    echo "$LAUNCHER_INVOCATION_COMMAND"
    echo "\`\`\`"
    echo
    echo "Dispatch command:"
    echo
    echo "\`\`\`bash"
    echo "$LAUNCHER_DISPATCH_COMMAND"
    echo "\`\`\`"
  } > "$LAUNCHER_STATUS_REPORT"

  if (( shared_state_owned != 0 )); then
    write_launcher_run_tracking_artifacts || return 1
    publish_launcher_smoke_summary_bundle || return 1
    write_launcher_status_artifact_manifest
    archive_launcher_run_bundle || return 1
    publish_launcher_smoke_diagnostics_manifest_mirror || return 1
    write_launcher_status_diagnostics_manifest
    publish_launcher_smoke_diagnostics_manifest_mirror || return 1
    cp "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST" "$LAUNCHER_RUN_STATUS_DIAGNOSTICS_PATH" || return 1
  else
    write_launcher_status_artifact_manifest
    write_launcher_status_diagnostics_manifest
  fi
  LAUNCHER_STATUS_WRITTEN=1
}

report_launcher_status_context() {
  local replay_case_descriptor=""
  local replay_artifact_descriptor=""
  local triage_scope=""
  local triage_stage=""
  local triage_primary_report=""
  local triage_first_artifacts=""
  local triage_retry_command=""
  local triage_retry_hint=""
  local failure_partition_label=""

  if [[ -z "$LAUNCHER_STATUS_SUMMARY" ]]; then
    return
  fi
  replay_case_descriptor="$(launcher_replay_case_descriptor)"
  replay_artifact_descriptor="$(launcher_replay_artifact_descriptor)"
  failure_partition_label="$(launcher_failure_partition_label)"
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" != "pass" ]]; then
    triage_scope="$(launcher_triage_stage_scope)"
    triage_stage="$(launcher_triage_stage_name)"
    triage_primary_report="$(launcher_triage_primary_report)"
    triage_first_artifacts="$(launcher_triage_first_artifacts)"
    triage_retry_command="$(launcher_triage_retry_command)"
    triage_retry_hint="$(launcher_triage_retry_hint)"
  fi
  emit_launcher_context_line "[lca_smoke] public status: ${LAUNCHER_STATUS_PUBLIC_STATUS:-FAIL} family=${LAUNCHER_STATUS_RESULT_FAMILY:-unknown}"
  emit_launcher_context_line "[lca_smoke] acceptance signal: $(launcher_acceptance_signal_status)"
  emit_launcher_context_line "[lca_smoke] iteration support: $(launcher_iteration_support_status) next_step=$(launcher_iteration_support_next_step)"
  emit_launcher_context_line "[lca_smoke] normalized outcome: $LAUNCHER_STATUS_OUTCOME"
  emit_launcher_context_line "[lca_smoke] normalized exit code: $LAUNCHER_STATUS_NORMALIZED_RC raw_exit_code=$LAUNCHER_STATUS_RAW_RC source=$LAUNCHER_STATUS_SOURCE"
  emit_launcher_context_line "[lca_smoke] failure partition: $failure_partition_label public_exit=$LAUNCHER_STATUS_NORMALIZED_RC raw_exit=$LAUNCHER_STATUS_RAW_RC"
  emit_launcher_context_line "[lca_smoke] outcome summary: $LAUNCHER_STATUS_MESSAGE"
  emit_launcher_context_line "[lca_smoke] iteration summary: run_id=$LAUNCHER_RUN_ID elapsed_seconds=$LAUNCHER_RUN_ELAPSED_SECONDS comparison=$LAUNCHER_RUN_COMPARISON_SUMMARY"
  emit_launcher_context_line "[lca_smoke] gate chain: $(launcher_gate_chain_overview)"
  emit_launcher_context_line "[lca_smoke] command control: mode=$(launcher_command_control_mode) action=$LAUNCHER_RETRY_LOOP_ACTION preferred_kind=$(launcher_command_control_preferred_command_kind) failure_terminal=$(launcher_failure_is_terminal)"
  emit_launcher_context_line "[lca_smoke] next gate control: command=$RETRY_LOOP_NEXT_GATE_COMMAND status=$(launcher_next_gate_status) dependency=$(launcher_next_gate_dependency) gate_escalation_allowed=$(launcher_gate_escalation_allowed)"
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" != "pass" ]]; then
    emit_launcher_context_line "[lca_smoke] failed stage: $triage_stage scope=$triage_scope"
    emit_launcher_context_line "[lca_smoke] stage label: $triage_scope:$triage_stage"
    emit_launcher_context_line "[lca_smoke] primary report: $triage_primary_report"
    emit_launcher_context_line "[lca_smoke] inspect first: $triage_first_artifacts"
    emit_launcher_context_line "[lca_smoke] retry next: $triage_retry_command"
    emit_launcher_context_line "[lca_smoke] retry guidance: $triage_retry_hint"
    emit_launcher_context_line "[lca_smoke] retry loop action: $LAUNCHER_RETRY_LOOP_ACTION"
    emit_launcher_context_line "[lca_smoke] retry loop preferred: $LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND"
    emit_launcher_context_line "[lca_smoke] retry loop launch: $LAUNCHER_RETRY_LOOP_LAUNCH_COMMAND"
    emit_launcher_context_line "[lca_smoke] retry loop direct: $LAUNCHER_RETRY_LOOP_DIRECT_COMMAND"
  fi
  if [[ -n "$LAUNCHER_STATUS_SOURCE_ROOT" ]]; then
    emit_launcher_context_line "[lca_smoke] source root: $LAUNCHER_STATUS_SOURCE_ROOT"
  fi
  if [[ -n "$LAUNCHER_STATUS_SOURCE_SUMMARY" ]]; then
    emit_launcher_context_line "[lca_smoke] source summary: $LAUNCHER_STATUS_SOURCE_SUMMARY"
  fi
  if [[ -n "$LAUNCHER_STATUS_SOURCE_REPORT" ]]; then
    emit_launcher_context_line "[lca_smoke] source report: $LAUNCHER_STATUS_SOURCE_REPORT"
  fi
  if [[ -n "$LAUNCHER_REPLAY_SUMMARY" ]]; then
    emit_launcher_context_line "[lca_smoke] replay summary: $LAUNCHER_REPLAY_SUMMARY"
  fi
  if [[ -n "$replay_case_descriptor" ]]; then
    emit_launcher_context_line "[lca_smoke] replay case: $replay_case_descriptor"
  fi
  if [[ -n "$LAUNCHER_REPLAY_COMMAND" ]]; then
    emit_launcher_context_line "[lca_smoke] replay command: $LAUNCHER_REPLAY_COMMAND"
  fi
  if [[ -n "$replay_artifact_descriptor" ]]; then
    emit_launcher_context_line "[lca_smoke] replay artifacts: $replay_artifact_descriptor"
  fi
  if [[ -n "$LAUNCHER_REPLAY_COMMANDS_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] commands snapshot: $LAUNCHER_REPLAY_COMMANDS_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] source artifact manifest: $LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_EXACT_SEED_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] exact seed snapshot: $LAUNCHER_REPLAY_EXACT_SEED_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_EXACT_INPUT_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] exact input snapshot: $LAUNCHER_REPLAY_EXACT_INPUT_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] exact output snapshot: $LAUNCHER_REPLAY_EXACT_OUTPUT_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] expected output snapshot: $LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] invoked command snapshot: $LAUNCHER_REPLAY_INVOKED_COMMAND_PATH"
  fi
  if [[ -n "$LAUNCHER_REPLAY_ACTIVE_SCRIPT" ]]; then
    emit_launcher_context_line "[lca_smoke] active solver replay script: $LAUNCHER_REPLAY_ACTIVE_SCRIPT"
  fi
  if [[ -n "$LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] mismatch summary: $LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] structured context: $LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] source failed-case row: $LAUNCHER_SOURCE_FAILED_CASE_ROW_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_RETRY_LOG_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] retry log: $LAUNCHER_SOURCE_RETRY_LOG_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] source runtime env: $LAUNCHER_SOURCE_RUNTIME_ENV_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] source runtime env exports: $LAUNCHER_SOURCE_RUNTIME_ENV_EXPORTS_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] source manifest snapshot: $LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH"
  fi
  if [[ -n "$LAUNCHER_STATUS_SUITE_CONFIG_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] suite config: $LAUNCHER_STATUS_SUITE_CONFIG_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] source preflight manifest: $LAUNCHER_SOURCE_PREFLIGHT_MANIFEST_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_SETUP_ENV_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] source setup env: $LAUNCHER_SOURCE_SETUP_ENV_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_BUILD_COMMAND_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] source build command: $LAUNCHER_SOURCE_BUILD_COMMAND_PATH"
  fi
  if [[ -n "$LAUNCHER_SOURCE_BUILD_STDERR_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] source build stderr: $LAUNCHER_SOURCE_BUILD_STDERR_PATH"
  fi
  if [[ -n "$LAUNCHER_STATUS_SUITE_PLAN_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] suite plan: $LAUNCHER_STATUS_SUITE_PLAN_PATH"
  fi
  if [[ -n "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST" ]]; then
    emit_launcher_context_line "[lca_smoke] diagnostics manifest: $LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST"
  fi
  if [[ -n "$LAUNCHER_STATUS_ITERATION_EVIDENCE" ]]; then
    emit_launcher_context_line "[lca_smoke] iteration evidence: $LAUNCHER_STATUS_ITERATION_EVIDENCE"
  fi
  if [[ -n "$LAUNCHER_RUN_HISTORY_INDEX" ]]; then
    emit_launcher_context_line "[lca_smoke] run history index: $LAUNCHER_RUN_HISTORY_INDEX"
  fi
  if [[ -n "$LAUNCHER_STATUS_RUN_RECORD" ]]; then
    emit_launcher_context_line "[lca_smoke] run record: $LAUNCHER_STATUS_RUN_RECORD"
  fi
  if [[ -n "$LAUNCHER_STATUS_RUN_COMPARISON" ]]; then
    emit_launcher_context_line "[lca_smoke] run comparison: $LAUNCHER_STATUS_RUN_COMPARISON"
  fi
  if [[ -n "$LAUNCHER_RUN_COMPARISON_SUMMARY" ]]; then
    emit_launcher_context_line "[lca_smoke] iteration comparison: $LAUNCHER_RUN_COMPARISON_SUMMARY"
  fi
  if [[ -n "$LAUNCHER_PREVIOUS_RUN_ID" ]]; then
    emit_launcher_context_line "[lca_smoke] previous run: id=$LAUNCHER_PREVIOUS_RUN_ID outcome=$LAUNCHER_PREVIOUS_RUN_NORMALIZED_OUTCOME stage=$LAUNCHER_PREVIOUS_RUN_STAGE_LABEL archive=$LAUNCHER_PREVIOUS_RUN_ARCHIVE_ROOT"
  fi
  if [[ -n "$LAUNCHER_STATUS_RETRY_LOOP_CONTROL" ]]; then
    emit_launcher_context_line "[lca_smoke] retry loop control: $LAUNCHER_STATUS_RETRY_LOOP_CONTROL"
  fi
  if [[ -n "$LAUNCHER_RUN_ARCHIVE_ROOT" ]]; then
    emit_launcher_context_line "[lca_smoke] run archive root: $LAUNCHER_RUN_ARCHIVE_ROOT"
  fi
  if [[ -n "$LAUNCHER_RUN_CONSOLE_LOG" ]]; then
    emit_launcher_context_line "[lca_smoke] launcher console transcript: $LAUNCHER_RUN_CONSOLE_LOG"
  fi
  if [[ -n "$LAUNCHER_DISPATCH_RESULT_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] dispatch result: $LAUNCHER_DISPATCH_RESULT_PATH"
  fi
  if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] smoke summary mirror: $LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH"
  fi
  if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] smoke iteration evidence mirror: $LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH"
  fi
  if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] smoke retry loop control mirror: $LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH"
  fi
  if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] smoke standard gap json: $LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH"
  fi
  if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] smoke run-record mirror: $LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH"
  fi
  if [[ -n "$LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH" ]]; then
    emit_launcher_context_line "[lca_smoke] smoke run-comparison mirror: $LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH"
  fi
  emit_launcher_context_line "[lca_smoke] status summary: $LAUNCHER_STATUS_SUMMARY"
  emit_launcher_context_line "[lca_smoke] status report: $LAUNCHER_STATUS_REPORT"
}

record_launcher_dispatch_marker() {
  mkdir -p "$(dirname "$LAUNCHER_DISPATCH_MARKER")" || fail "failed to prepare launcher dispatch marker root: $LAUNCHER_DISPATCH_MARKER"
  rm -f "$LAUNCHER_DISPATCH_MARKER" 2>/dev/null || true
  : > "$LAUNCHER_DISPATCH_MARKER" || fail "failed to record launcher dispatch marker: $LAUNCHER_DISPATCH_MARKER"
  if ! LAUNCHER_DISPATCH_STARTED_NS="$(
    python3 - "$LAUNCHER_DISPATCH_MARKER" <<'PY'
from __future__ import annotations

import sys
from pathlib import Path

print(Path(sys.argv[1]).stat().st_mtime_ns)
PY
  )"; then
    fail "failed to record launcher dispatch marker timestamp: $LAUNCHER_DISPATCH_MARKER"
  fi
}

artifact_is_fresh_since_dispatch() {
  local artifact="$1"

  if [[ ! -e "$artifact" ]]; then
    return 1
  fi

  if [[ -n "${LAUNCHER_DISPATCH_STARTED_NS:-}" ]]; then
    python3 - "$LAUNCHER_DISPATCH_STARTED_NS" "$artifact" <<'PY' >/dev/null 2>&1
from __future__ import annotations

import sys
from pathlib import Path

dispatch_started_ns_raw = sys.argv[1]
artifact_path = Path(sys.argv[2])

if not dispatch_started_ns_raw.isdigit():
    raise SystemExit(1)

dispatch_started_ns = int(dispatch_started_ns_raw)
artifact_mtime_ns = artifact_path.stat().st_mtime_ns
raise SystemExit(0 if artifact_mtime_ns >= dispatch_started_ns else 1)
PY
    return
  fi

  if [[ -z "${LAUNCHER_DISPATCH_MARKER:-}" || ! -e "$LAUNCHER_DISPATCH_MARKER" ]]; then
    return 1
  fi
  python3 - "$LAUNCHER_DISPATCH_MARKER" "$artifact" <<'PY' >/dev/null 2>&1
from __future__ import annotations

import sys
from pathlib import Path

marker_path = Path(sys.argv[1])
artifact_path = Path(sys.argv[2])

marker_mtime_ns = marker_path.stat().st_mtime_ns
artifact_mtime_ns = artifact_path.stat().st_mtime_ns
raise SystemExit(0 if artifact_mtime_ns >= marker_mtime_ns else 1)
PY
}

validate_inner_wrapper_success_artifacts() {
  local issues=""
  local suite_config_path="$SMOKE_OUTPUT_ROOT/suite_config.txt"
  local suite_plan_path="$SMOKE_OUTPUT_ROOT/suite_plan.tsv"

  if [[ ! -d "$SMOKE_OUTPUT_ROOT" ]]; then
    issues="missing smoke output root at $SMOKE_OUTPUT_ROOT"
  elif ! artifact_is_fresh_since_dispatch "$SMOKE_OUTPUT_ROOT"; then
    issues="stale smoke output root at $SMOKE_OUTPUT_ROOT"
  fi
  if [[ ! -s "$suite_config_path" ]]; then
    issues="${issues:+$issues; }missing suite config at $suite_config_path"
  elif ! artifact_is_fresh_since_dispatch "$suite_config_path"; then
    issues="${issues:+$issues; }stale suite config at $suite_config_path"
  fi
  if [[ ! -s "$suite_plan_path" ]]; then
    issues="${issues:+$issues; }missing suite plan at $suite_plan_path"
  elif ! artifact_is_fresh_since_dispatch "$suite_plan_path"; then
    issues="${issues:+$issues; }stale suite plan at $suite_plan_path"
  fi

  if [[ -n "$issues" ]]; then
    printf '%s\n' "inner smoke wrapper returned success without publishing a fresh smoke bundle: $issues"
    return 1
  fi
}

inner_wrapper_failure_bundle_path_issue() {
  local label="$1"
  local path="$2"
  local expected_kind="${3:-path}"

  if [[ -z "$path" ]]; then
    printf 'missing %s path\n' "$label"
    return 0
  fi

  case "$expected_kind" in
    directory)
      if [[ ! -d "$path" ]]; then
        printf 'missing %s at %s\n' "$label" "$path"
        return 0
      fi
      ;;
    *)
      if [[ ! -e "$path" ]]; then
        printf 'missing %s at %s\n' "$label" "$path"
        return 0
      fi
      ;;
  esac

  if ! artifact_is_fresh_since_dispatch "$path"; then
    printf 'stale %s at %s\n' "$label" "$path"
    return 0
  fi
}

inner_wrapper_failure_bundle_issues() {
  local failure_root="$1"
  local source_summary="$2"
  local source_report="$3"
  local issues=""
  local detail=""
  local case_repro_required=0
  local minimal_bundle_ready=0
  local require_extended_metadata=0
  if [[ ! -d "$failure_root" ]]; then
    issues="missing preserved failure root at $failure_root"
  elif ! artifact_is_fresh_since_dispatch "$failure_root"; then
    issues="stale preserved failure root at $failure_root"
  fi
  if [[ ! -s "$source_summary" ]]; then
    issues="${issues:+$issues; }missing failure summary at $source_summary"
  elif ! artifact_is_fresh_since_dispatch "$source_summary"; then
    issues="${issues:+$issues; }stale failure summary at $source_summary"
  fi

  capture_launcher_source_failure_details "$source_summary"

  if [[ -n "$LAUNCHER_SOURCE_FAILURE_KIND" && -n "$LAUNCHER_SOURCE_FAILURE_ORIGIN" && -n "$LAUNCHER_SOURCE_FAILURE_RETRYABLE" ]]; then
    minimal_bundle_ready=1
  fi

  if [[ -n "$LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS" || \
        -n "$LAUNCHER_REPLAY_COMMANDS_PATH" || \
        -n "$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH" || \
        -n "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH" || \
        -n "$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH" || \
        -n "$LAUNCHER_SOURCE_SUITE_CONFIG_PATH" || \
        -n "$LAUNCHER_SOURCE_SUITE_PLAN_PATH" ]]; then
    require_extended_metadata=1
  fi

  if (( require_extended_metadata == 0 )); then
    if [[ ! -s "$source_report" ]]; then
      issues="${issues:+$issues; }missing failure report at $source_report"
    elif ! artifact_is_fresh_since_dispatch "$source_report"; then
      issues="${issues:+$issues; }stale failure report at $source_report"
    fi
  fi

  if (( require_extended_metadata != 0 )); then
    if [[ -z "$LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS" ]]; then
      issues="${issues:+$issues; }missing failure reporting status in $source_summary"
    elif [[ "$LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS" != "complete" && "$LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS" != "degraded" ]]; then
      issues="${issues:+$issues; }unknown failure reporting status '$LAUNCHER_SOURCE_FAILURE_REPORTING_STATUS' in $source_summary"
    fi

    detail="$(inner_wrapper_failure_bundle_path_issue "source commands snapshot" "$LAUNCHER_REPLAY_COMMANDS_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source artifact manifest" "$LAUNCHER_REPLAY_ARTIFACT_MANIFEST_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source structured context" "$LAUNCHER_SOURCE_STRUCTURED_CONTEXT_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source manifest snapshot" "$LAUNCHER_SOURCE_MANIFEST_SNAPSHOT_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source suite config" "$LAUNCHER_SOURCE_SUITE_CONFIG_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source suite plan" "$LAUNCHER_SOURCE_SUITE_PLAN_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
  fi

  if [[ -n "$LAUNCHER_REPLAY_CASE_TAG" || -n "$LAUNCHER_REPLAY_SEED" || -n "$LAUNCHER_REPLAY_ACTIVE_SCRIPT" || -n "$LAUNCHER_REPLAY_EXACT_INPUT_PATH" ]]; then
    case_repro_required=1
  fi
  if (( case_repro_required != 0 )); then
    detail="$(inner_wrapper_failure_bundle_path_issue "source failure case dir" "$LAUNCHER_REPLAY_FAILURE_CASE_DIR" "directory")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source rerun command snapshot" "$LAUNCHER_REPLAY_RERUN_COMMAND_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source exact seed snapshot" "$LAUNCHER_REPLAY_EXACT_SEED_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source exact input snapshot" "$LAUNCHER_REPLAY_EXACT_INPUT_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source exact output snapshot" "$LAUNCHER_REPLAY_EXACT_OUTPUT_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source expected output snapshot" "$LAUNCHER_REPLAY_EXPECTED_OUTPUT_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source invoked command snapshot" "$LAUNCHER_REPLAY_INVOKED_COMMAND_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
    if [[ -n "$LAUNCHER_REPLAY_ACTIVE_SCRIPT" ]]; then
      detail="$(inner_wrapper_failure_bundle_path_issue "source active solver replay script" "$LAUNCHER_REPLAY_ACTIVE_SCRIPT")"
      if [[ -n "$detail" ]]; then
        issues="${issues:+$issues; }$detail"
      fi
    fi
    detail="$(inner_wrapper_failure_bundle_path_issue "source runtime env snapshot" "$LAUNCHER_SOURCE_RUNTIME_ENV_PATH")"
    if [[ -n "$detail" ]]; then
      issues="${issues:+$issues; }$detail"
    fi
  fi

  if (( minimal_bundle_ready == 0 )) && (( require_extended_metadata == 0 )); then
    if [[ -z "$LAUNCHER_SOURCE_FAILURE_KIND" ]]; then
      issues="${issues:+$issues; }missing failure kind in $source_summary"
    fi
    if [[ -z "$LAUNCHER_SOURCE_FAILURE_ORIGIN" ]]; then
      issues="${issues:+$issues; }missing failure origin in $source_summary"
    fi
    if [[ -z "$LAUNCHER_SOURCE_FAILURE_RETRYABLE" ]]; then
      issues="${issues:+$issues; }missing failure retryable flag in $source_summary"
    fi
    issues="${issues:+$issues; }missing minimal fresh failure-bundle metadata in $source_summary"
  fi

  printf '%s\n' "$issues"
}

clear_stale_inner_wrapper_rerun_state() {
  local issues=""

  issues="$(clear_inner_wrapper_rerun_state_paths)"
  if [[ -n "$issues" ]]; then
    printf '%s\n' "inner smoke wrapper published a fresh smoke bundle but left stale rerun state behind: $issues"
    return 1
  fi
}

clear_inner_wrapper_rerun_state_paths() {
  local issues=""
  local stale=""
  local smoke_setup_root="$ARTIFACTS_ROOT/smoke_setup"
  local smoke_session_state_root="$TMP_PARENT/lca_smoke.session"
  local smoke_setup_tmpdir="$TMP_PARENT/lca_smoke.setup.tmp"
  local smoke_output_parent=""
  local smoke_backup_root=""

  smoke_output_parent="$(dirname "$SMOKE_OUTPUT_ROOT")"
  smoke_backup_root="${SMOKE_OUTPUT_ROOT}.previous"

  if [[ -e "$SMOKE_FAILURE_ROOT" ]] && ! remove_path_retry "$SMOKE_FAILURE_ROOT"; then
    issues="failed to clear stale inner failure root at $SMOKE_FAILURE_ROOT"
  fi
  if [[ -e "$smoke_setup_root" ]] && ! remove_path_retry "$smoke_setup_root"; then
    issues="${issues:+$issues; }failed to clear stale inner setup root at $smoke_setup_root"
  fi
  if [[ -e "$smoke_session_state_root" ]] && ! remove_path_retry "$smoke_session_state_root"; then
    issues="${issues:+$issues; }failed to clear stale inner session state at $smoke_session_state_root"
  fi
  if [[ -e "$smoke_setup_tmpdir" ]] && ! remove_path_retry "$smoke_setup_tmpdir"; then
    issues="${issues:+$issues; }failed to clear stale inner setup tmpdir at $smoke_setup_tmpdir"
  fi
  if [[ -e "$smoke_backup_root" ]] && ! remove_path_retry "$smoke_backup_root"; then
    issues="${issues:+$issues; }failed to clear stale inner backup output at $smoke_backup_root"
  fi
  if [[ -d "$TMP_PARENT" ]]; then
    shopt -s nullglob
    for stale in \
      "$TMP_PARENT"/lca_smoke_probe.* \
      "$TMP_PARENT"/lca_smoke.run.* \
      "$TMP_PARENT"/lca_smoke.tmp.* \
      "$TMP_PARENT"/$LAUNCHER_INNER_BUILD_TMP_GLOB \
      "$TMP_PARENT"/$LAUNCHER_INNER_BUILD_TMP_TMP_GLOB; do
      if ! remove_path_retry "$stale"; then
        issues="${issues:+$issues; }failed to clear stale inner tmp path at $stale"
      fi
    done
    shopt -u nullglob
  fi
  if [[ -d "$smoke_output_parent" ]]; then
    shopt -s nullglob
    for stale in "$smoke_output_parent"/$LAUNCHER_INNER_LEGACY_OUTPUT_GLOB; do
      if ! remove_path_retry "$stale"; then
        issues="${issues:+$issues; }failed to clear stale inner legacy output path at $stale"
      fi
    done
    shopt -u nullglob
  fi

  printf '%s\n' "$issues"
}

clear_stale_launcher_dispatch_state() {
  local cleanup_detail=""
  local cleanup_rc=0

  if [[ -z "${LAUNCHER_DISPATCH_STATE_PATH:-}" || ! -e "$LAUNCHER_DISPATCH_STATE_PATH" ]]; then
    return 0
  fi

  if ! cleanup_detail="$(
    python3 - "$LAUNCHER_DISPATCH_STATE_PATH" "$INNER_WRAPPER" "$LAUNCHER_DISPATCH_KILL_GRACE_S" "$$" <<'PY'
from __future__ import annotations

import os
import signal
import sys
import time
from pathlib import Path

state_path = Path(sys.argv[1])
expected_inner_wrapper = sys.argv[2]
kill_grace_s = float(sys.argv[3])
current_launcher_pid = int(sys.argv[4])

payload: dict[str, str] = {}
for raw_line in state_path.read_text(encoding="utf-8").splitlines():
    if "=" not in raw_line:
        continue
    key, value = raw_line.split("=", 1)
    payload[key.strip()] = value.strip()


def parse_pid(name: str) -> int:
    raw = payload.get(name, "")
    if not raw:
        return 0
    try:
        value = int(raw)
    except ValueError as exc:
        raise SystemExit(f"invalid {name} in {state_path}: {raw}") from exc
    if value < 0:
        raise SystemExit(f"invalid {name} in {state_path}: {raw}")
    return value


def pid_is_alive(pid: int) -> bool:
    if pid <= 0:
        return False
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def same_entrypoint(lhs: str, rhs: str) -> bool:
    if not lhs or not rhs:
        return False
    return os.path.realpath(lhs) == os.path.realpath(rhs)


manager_pid = parse_pid("manager_pid")
child_pid = parse_pid("child_pid")
child_pgid = parse_pid("child_pgid")
recorded_child_entrypoint = payload.get("child_entrypoint", "") or payload.get("child_command", "")
manager_is_current_launcher = manager_pid > 0 and manager_pid == current_launcher_pid

child_is_alive = pid_is_alive(child_pid)
if child_is_alive and recorded_child_entrypoint and not same_entrypoint(recorded_child_entrypoint, expected_inner_wrapper):
    state_path.unlink(missing_ok=True)
    print(
        f"removed stale dispatch state file {state_path} after its recorded child pid {child_pid} "
        f"stopped matching the inner wrapper entrypoint"
    )
    raise SystemExit(0)

if child_is_alive:
    if child_pgid > 0:
        try:
            os.killpg(child_pgid, signal.SIGTERM)
        except ProcessLookupError:
            pass
    else:
        try:
            os.kill(child_pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
    if pid_is_alive(child_pid):
        deadline = time.monotonic() + kill_grace_s
        while pid_is_alive(child_pid) and time.monotonic() < deadline:
            time.sleep(0.05)
        if pid_is_alive(child_pid):
            if child_pgid > 0:
                try:
                    os.killpg(child_pgid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            else:
                try:
                    os.kill(child_pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            deadline = time.monotonic() + max(kill_grace_s, 0.2)
            while pid_is_alive(child_pid) and time.monotonic() < deadline:
                time.sleep(0.05)
        if pid_is_alive(child_pid):
            print(
                f"failed to stop stale inner smoke wrapper pid {child_pid} "
                f"(pgid={child_pgid}) from {state_path}"
            )
            raise SystemExit(2)

state_path.unlink(missing_ok=True)
if child_is_alive:
    print(f"cleared stale inner smoke wrapper pid {child_pid} (pgid={child_pgid}) from {state_path}")
elif manager_is_current_launcher:
    print(f"removed stale self-owned launcher dispatch state file {state_path}")
elif pid_is_alive(manager_pid):
    # Once this launcher has acquired the run-control lock, a leftover manager
    # pid in the dispatch-state file is stale metadata, not an authoritative
    # signal that another smoke launch still owns this working tree.
    print(
        f"removed stale dispatch state file {state_path} after reacquiring the launcher lock "
        f"from live manager pid {manager_pid}"
    )
PY
  )"; then
    cleanup_rc=$?
    case "$cleanup_rc" in
      *)
        printf '%s\n' "${cleanup_detail:-failed to clear stale launcher dispatch state: $LAUNCHER_DISPATCH_STATE_PATH}"
        return 1
        ;;
    esac
  fi

  if [[ -n "$cleanup_detail" ]]; then
    emit_launcher_context_line "[lca_smoke] cleared stale dispatch state: $cleanup_detail"
  fi
}

clear_stale_inner_wrapper_dispatch_state() {
  local issues=""
  local dispatch_message=""

  if ! dispatch_message="$(clear_stale_launcher_dispatch_state)"; then
    printf '%s\n' "${dispatch_message:-failed to clear stale launcher dispatch state before dispatch}"
    return 1
  fi
  if [[ -n "$dispatch_message" ]]; then
    emit_launcher_context_line "[lca_smoke] stale dispatch cleanup: $dispatch_message"
  fi

  issues="$(clear_inner_wrapper_rerun_state_paths)"
  if [[ -n "$issues" ]]; then
    printf '%s\n' "failed to clear stale inner-wrapper rerun state before dispatch: $issues"
    return 1
  fi
}

validate_inner_wrapper_failure_bundle() {
  local failure_class="$1"
  local failure_root="$2"
  local source_summary="$3"
  local source_report="$4"
  local issues=""

  issues="$(inner_wrapper_failure_bundle_issues "$failure_root" "$source_summary" "$source_report")"
  if [[ -n "$issues" ]]; then
    printf '%s\n' "inner smoke wrapper returned a ${failure_class} result without publishing a complete fresh failure bundle: $issues"
    return 1
  fi
}

unexpected_inner_wrapper_message() {
  local raw_rc="$1"

  if (( LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL > 0 )); then
    printf 'launcher dispatch was interrupted by signal %d while the inner smoke wrapper was running; the launcher terminated the owned process group and preserved retry-safe state for the next invocation\n' "$LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL"
    return
  fi
  if (( raw_rc >= 128 )); then
    printf 'inner smoke wrapper terminated by signal %d before the launcher could validate the smoke bundle\n' "$(( raw_rc - 128 ))"
    return
  fi
  printf 'inner smoke wrapper returned unexpected exit code %s; treat the run as infrastructure-failed\n' "$raw_rc"
}

launcher_dispatch_timeout_message() {
  local message="launcher-enforced dispatch timeout after ${LAUNCHER_DISPATCH_TIMEOUT_S}s while waiting for inner smoke wrapper"
  local bundle_issues=""
  local source_summary="$SMOKE_FAILURE_ROOT/failure_summary.txt"
  local source_report="$SMOKE_FAILURE_ROOT/latest_failure_report.md"

  bundle_issues="$(inner_wrapper_failure_bundle_issues "$SMOKE_FAILURE_ROOT" "$source_summary" "$source_report")"
  if [[ -n "$bundle_issues" ]]; then
    if [[ -n "${LAUNCHER_REPLAY_SUMMARY:-}" ]]; then
      printf '%s; inner smoke wrapper timed out without publishing a complete fresh failure bundle: %s; preserved partial detail: %s\n' \
        "$message" \
        "$bundle_issues" \
        "$LAUNCHER_REPLAY_SUMMARY"
      return 0
    fi
    printf '%s; inner smoke wrapper timed out without publishing a complete fresh failure bundle: %s\n' \
      "$message" \
      "$bundle_issues"
    return 0
  fi

  if [[ -n "${LAUNCHER_REPLAY_SUMMARY:-}" ]]; then
    printf '%s; preserved inner-wrapper failure detail is advisory only: %s\n' "$message" "$LAUNCHER_REPLAY_SUMMARY"
    return 0
  fi
  printf '%s; no fresh inner-wrapper outcome was published before the launcher aborted the run\n' "$message"
}

dispatch_signal_name() {
  case "${1:-0}" in
    1)
      printf 'SIGHUP\n'
      ;;
    2)
      printf 'SIGINT\n'
      ;;
    15)
      printf 'SIGTERM\n'
      ;;
    *)
      printf 'signal %s\n' "${1:-0}"
      ;;
  esac
}

launcher_dispatch_interrupted_message() {
  local signal_name=""
  local message=""
  local bundle_issues=""
  local source_summary="$SMOKE_FAILURE_ROOT/failure_summary.txt"
  local source_report="$SMOKE_FAILURE_ROOT/latest_failure_report.md"

  signal_name="$(dispatch_signal_name "$LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL")"
  message="launcher received ${signal_name} while waiting for inner smoke wrapper"
  bundle_issues="$(inner_wrapper_failure_bundle_issues "$SMOKE_FAILURE_ROOT" "$source_summary" "$source_report")"
  if [[ -n "$bundle_issues" ]]; then
    if [[ -n "${LAUNCHER_REPLAY_SUMMARY:-}" ]]; then
      printf '%s; cleaned up the interrupted dispatch without publishing a complete fresh failure bundle: %s; preserved partial detail: %s\n' \
        "$message" \
        "$bundle_issues" \
        "$LAUNCHER_REPLAY_SUMMARY"
      return 0
    fi
    printf '%s; cleaned up the interrupted dispatch without publishing a complete fresh failure bundle: %s\n' \
      "$message" \
      "$bundle_issues"
    return 0
  fi

  if [[ -n "${LAUNCHER_REPLAY_SUMMARY:-}" ]]; then
    printf '%s; preserved inner-wrapper failure detail is advisory only: %s\n' "$message" "$LAUNCHER_REPLAY_SUMMARY"
    return 0
  fi
  printf '%s; no fresh inner-wrapper outcome was published before the launcher stopped the run\n' "$message"
}

launcher_normalized_failure_outcome() {
  local raw_rc="$1"

  case "${LAUNCHER_SOURCE_FAILURE_KIND:-}" in
    solver_acceptance_failure|solver_case_failure)
      printf 'reproducible_stress_gate_failure\n'
      return 0
      ;;
    solver_timeout|solver_runtime_failure|solver_signal_failure)
      printf 'reproducible_solver_failure\n'
      return 0
      ;;
    harness_usage_failure|harness_transient_failure)
      printf 'harness_infrastructure_failure\n'
      return 0
      ;;
  esac

  case "$raw_rc" in
    "$SMOKE_EXIT_SOLVER_FAILURE")
      printf 'reproducible_stress_gate_failure\n'
      return 0
      ;;
    "$SMOKE_EXIT_SOLVER_TIMEOUT"|"$SMOKE_EXIT_SOLVER_RUNTIME_FAILURE")
      printf 'reproducible_solver_failure\n'
      return 0
      ;;
    "$SMOKE_EXIT_USAGE"|"$SMOKE_EXIT_HARNESS_FAILURE")
      printf 'harness_infrastructure_failure\n'
      return 0
      ;;
  esac

  case "${LAUNCHER_SOURCE_FAILURE_ORIGIN:-}" in
    validator)
      printf 'reproducible_stress_gate_failure\n'
      ;;
    solver)
      printf 'reproducible_solver_failure\n'
      ;;
    *)
      printf 'harness_infrastructure_failure\n'
      ;;
  esac
}

launcher_normalized_failure_exit_code() {
  local raw_rc="$1"
  local outcome=""

  outcome="$(launcher_normalized_failure_outcome "$raw_rc")"
  case "$outcome" in
    reproducible_stress_gate_failure)
      printf '%s\n' "$SMOKE_EXIT_SOLVER_FAILURE"
      ;;
    reproducible_solver_failure)
      case "${LAUNCHER_SOURCE_FAILURE_KIND:-}" in
        solver_timeout)
          printf '%s\n' "$SMOKE_EXIT_SOLVER_TIMEOUT"
          return 0
          ;;
        solver_runtime_failure|solver_signal_failure)
          printf '%s\n' "$SMOKE_EXIT_SOLVER_RUNTIME_FAILURE"
          return 0
          ;;
      esac
      case "$raw_rc" in
        "$SMOKE_EXIT_SOLVER_TIMEOUT")
          printf '%s\n' "$SMOKE_EXIT_SOLVER_TIMEOUT"
          ;;
        *)
          printf '%s\n' "$SMOKE_EXIT_SOLVER_RUNTIME_FAILURE"
          ;;
      esac
      ;;
    *)
      printf '%s\n' "$SMOKE_EXIT_HARNESS_FAILURE"
      ;;
  esac
}

classify_inner_wrapper_exit() {
  local raw_rc="$1"
  local source_summary="$SMOKE_FAILURE_ROOT/failure_summary.txt"
  local source_report="$SMOKE_FAILURE_ROOT/latest_failure_report.md"
  local source_root=""
  local status_source_summary=""
  local status_source_report=""
  local validation_message=""
  local normalized_outcome=""
  local normalized_rc=0

  clear_launcher_source_failure_details
  if (( LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL != 0 )); then
    capture_launcher_source_failure_details "$source_summary"
    if [[ -d "$SMOKE_FAILURE_ROOT" ]]; then
      source_root="$SMOKE_FAILURE_ROOT"
    fi
    if [[ -s "$source_summary" ]]; then
      status_source_summary="$source_summary"
    fi
    if [[ -s "$source_report" ]]; then
      status_source_report="$source_report"
    fi
    set_launcher_status \
      "harness_infrastructure_failure" \
      "$SMOKE_EXIT_HARNESS_FAILURE" \
      "$raw_rc" \
      "launcher" \
      "$(launcher_dispatch_interrupted_message)" \
      "$source_root" \
      "$status_source_summary" \
      "$status_source_report"
    return
  fi
  if (( LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED == 1 )); then
    capture_launcher_source_failure_details "$source_summary"
    if [[ -d "$SMOKE_FAILURE_ROOT" ]]; then
      source_root="$SMOKE_FAILURE_ROOT"
    fi
    if [[ -s "$source_summary" ]]; then
      status_source_summary="$source_summary"
    fi
    if [[ -s "$source_report" ]]; then
      status_source_report="$source_report"
    fi
    set_launcher_status \
      "harness_infrastructure_failure" \
      "$SMOKE_EXIT_HARNESS_FAILURE" \
      "$raw_rc" \
      "launcher" \
      "$(launcher_dispatch_timeout_message)" \
      "$source_root" \
      "$status_source_summary" \
      "$status_source_report"
    return
  fi
  case "$raw_rc" in
    0)
      if validation_message="$(validate_inner_wrapper_failure_bundle "preserved" "$SMOKE_FAILURE_ROOT" "$source_summary" "$source_report")"; then
        capture_launcher_source_failure_details "$source_summary"
        normalized_outcome="$(launcher_normalized_failure_outcome "$SMOKE_EXIT_SOLVER_FAILURE")"
        normalized_rc="$(launcher_normalized_failure_exit_code "$SMOKE_EXIT_SOLVER_FAILURE")"
        set_launcher_status \
          "$normalized_outcome" \
          "$normalized_rc" \
          "$normalized_rc" \
          "inner_wrapper" \
          "$(launcher_source_failure_message "inner smoke wrapper exited zero but published a preserved failure bundle; honoring the preserved failure artifacts")" \
          "$SMOKE_FAILURE_ROOT" \
          "$source_summary" \
          "$source_report"
        return
      fi
      if ! validation_message="$(validate_inner_wrapper_success_artifacts)"; then
        set_launcher_status \
          "harness_infrastructure_failure" \
          "$SMOKE_EXIT_HARNESS_FAILURE" \
          "$raw_rc" \
          "inner_wrapper" \
          "$validation_message" \
          "$SMOKE_OUTPUT_ROOT"
        return
      fi
      if ! validation_message="$(clear_stale_inner_wrapper_rerun_state)"; then
        set_launcher_status \
          "harness_infrastructure_failure" \
          "$SMOKE_EXIT_HARNESS_FAILURE" \
          "$raw_rc" \
          "inner_wrapper" \
          "$validation_message" \
          "$SMOKE_OUTPUT_ROOT"
        return
      fi
      set_launcher_status \
        "pass" \
        0 \
        0 \
        "inner_wrapper" \
        "inner smoke suite passed all cases" \
        "$SMOKE_OUTPUT_ROOT"
      ;;
    "$SMOKE_EXIT_SOLVER_FAILURE"|"$SMOKE_EXIT_SOLVER_TIMEOUT"|"$SMOKE_EXIT_SOLVER_RUNTIME_FAILURE")
      if ! validation_message="$(validate_inner_wrapper_failure_bundle "solver-side" "$SMOKE_FAILURE_ROOT" "$source_summary" "$source_report")"; then
        set_launcher_status \
          "harness_infrastructure_failure" \
          "$SMOKE_EXIT_HARNESS_FAILURE" \
          "$raw_rc" \
          "inner_wrapper" \
          "$validation_message" \
          "$SMOKE_FAILURE_ROOT" \
          "$source_summary" \
          "$source_report"
        capture_launcher_source_failure_details "$source_summary"
        return
      fi
      capture_launcher_source_failure_details "$source_summary"
      normalized_outcome="$(launcher_normalized_failure_outcome "$raw_rc")"
      normalized_rc="$(launcher_normalized_failure_exit_code "$raw_rc")"
      set_launcher_status \
        "$normalized_outcome" \
        "$normalized_rc" \
        "$raw_rc" \
        "inner_wrapper" \
        "$(launcher_source_failure_message "inner smoke wrapper preserved a solver-side failure with replay artifacts")" \
        "$SMOKE_FAILURE_ROOT" \
        "$source_summary" \
        "$source_report"
      ;;
    "$SMOKE_EXIT_USAGE"|"$SMOKE_EXIT_HARNESS_FAILURE")
      if ! validation_message="$(validate_inner_wrapper_failure_bundle "harness-side" "$SMOKE_FAILURE_ROOT" "$source_summary" "$source_report")"; then
        set_launcher_status \
          "harness_infrastructure_failure" \
          "$SMOKE_EXIT_HARNESS_FAILURE" \
          "$raw_rc" \
          "inner_wrapper" \
          "$validation_message" \
          "$SMOKE_FAILURE_ROOT" \
          "$source_summary" \
          "$source_report"
        capture_launcher_source_failure_details "$source_summary"
        return
      fi
      capture_launcher_source_failure_details "$source_summary"
      set_launcher_status \
        "harness_infrastructure_failure" \
        "$SMOKE_EXIT_HARNESS_FAILURE" \
        "$raw_rc" \
        "inner_wrapper" \
        "$(launcher_source_failure_message "inner smoke wrapper reported a harness or infrastructure failure")" \
        "$SMOKE_FAILURE_ROOT" \
        "$source_summary" \
        "$source_report"
      ;;
    *)
      validation_message="$(unexpected_inner_wrapper_message "$raw_rc")"
      set_launcher_status \
        "harness_infrastructure_failure" \
        "$SMOKE_EXIT_HARNESS_FAILURE" \
        "$raw_rc" \
        "inner_wrapper" \
        "$validation_message" \
        "$SMOKE_FAILURE_ROOT" \
        "$source_summary" \
        "$source_report"
      capture_launcher_source_failure_details "$source_summary"
      ;;
  esac
}

capture_launcher_err() {
  local rc="${1:-1}"
  local line="${2:-}"
  local command_text="${3:-}"

  if [[ -z "$LAUNCHER_FAILURE_MESSAGE" ]]; then
    LAUNCHER_FAILURE_MESSAGE="unexpected launcher command failed before inner wrapper dispatch"
  fi
  if [[ "$LAUNCHER_FAILURE_RC" -eq 0 ]]; then
    LAUNCHER_FAILURE_RC="$rc"
  fi
  if [[ -z "$LAUNCHER_FAILURE_COMMAND" ]]; then
    LAUNCHER_FAILURE_COMMAND="$command_text"
  fi
  if [[ -z "$LAUNCHER_FAILURE_LINE" ]]; then
    LAUNCHER_FAILURE_LINE="$line"
  fi
}

write_launcher_failure_bundle() {
  local failure_parent=""
  local working_directory=""
  local message="${LAUNCHER_FAILURE_MESSAGE:-launcher exited non-zero before inner wrapper dispatch}"
  local failure_rc="${LAUNCHER_FAILURE_RC:-$SMOKE_EXIT_HARNESS_FAILURE}"
  local staged_last_check_artifact=""
  local stable_last_check_artifact=""
  local triage_first_artifacts=""
  local triage_retry_hint=""

  if (( LAUNCHER_FAILURE_BUNDLE_ACTIVE == 1 )); then
    return 1
  fi
  LAUNCHER_FAILURE_BUNDLE_ACTIVE=1
  resolve_launcher_failure_root

  failure_parent="$(dirname "$LAUNCHER_FAILURE_ROOT")"
  if ! ensure_launcher_directory "$failure_parent" "launcher failure parent"; then
    return 1
  fi
  if [[ -n "$LAUNCHER_LAST_CHECK_ARTIFACT" && -f "$LAUNCHER_LAST_CHECK_ARTIFACT" ]]; then
    staged_last_check_artifact="$(mktemp "$failure_parent/.launcher_last_check.XXXXXX")" || return 1
    if ! cp "$LAUNCHER_LAST_CHECK_ARTIFACT" "$staged_last_check_artifact"; then
      rm -f "$staged_last_check_artifact" 2>/dev/null || true
      return 1
    fi
  fi
  if [[ -e "$LAUNCHER_FAILURE_ROOT" ]]; then
    remove_path_retry "$LAUNCHER_FAILURE_ROOT" || return 1
  fi
  ensure_launcher_directory "$LAUNCHER_FAILURE_ROOT" "launcher failure root" || return 1
  if [[ -n "$staged_last_check_artifact" && -f "$staged_last_check_artifact" ]]; then
    stable_last_check_artifact="$LAUNCHER_FAILURE_ROOT/${LAUNCHER_LAST_CHECK_ARTIFACT##*/}"
    if ! cp "$staged_last_check_artifact" "$stable_last_check_artifact"; then
      rm -f "$staged_last_check_artifact" 2>/dev/null || true
      return 1
    fi
    LAUNCHER_LAST_CHECK_ARTIFACT="$stable_last_check_artifact"
    rm -f "$staged_last_check_artifact" 2>/dev/null || true
  fi

  printf '%s\n' "$LAUNCHER_INVOCATION_COMMAND" > "$LAUNCHER_FAILURE_INVOCATION_COMMAND_PATH"
  printf '%s\n' "$LAUNCHER_DISPATCH_COMMAND" > "$LAUNCHER_FAILURE_DISPATCH_COMMAND_PATH"
  printf '%s\n' "$message" > "$LAUNCHER_FAILURE_REASON_PATH"
  if [[ -n "$LAUNCHER_FAILURE_COMMAND" ]]; then
    printf '%s\n' "$LAUNCHER_FAILURE_COMMAND" > "$LAUNCHER_FAILURE_COMMAND_PATH"
  else
    rm -f "$LAUNCHER_FAILURE_COMMAND_PATH" 2>/dev/null || true
  fi
  {
    quote_command cd "$BRANCH_ROOT"
    echo "$LAUNCHER_INVOCATION_COMMAND"
    echo
    quote_command cd "$BRANCH_ROOT"
    echo "$LAUNCHER_DISPATCH_COMMAND"
  } > "$LAUNCHER_FAILURE_RERUN_COMMAND_PATH"
  working_directory="$(pwd -P)"
  write_launcher_environment_snapshot "$LAUNCHER_FAILURE_ENV_SNAPSHOT" || return 1
  sync_launcher_preflight_artifacts
  write_launcher_preflight_manifest
  if [[ -n "$LAUNCHER_LAST_CHECK_KIND" ]]; then
    printf '%s\t%s\t%s\t%s\t%s\n' \
      "$LAUNCHER_LAST_CHECK_KIND" \
      "$LAUNCHER_LAST_CHECK_LABEL" \
      "$LAUNCHER_LAST_CHECK_STATUS" \
      "$LAUNCHER_LAST_CHECK_DETAIL" \
      "$LAUNCHER_LAST_CHECK_ARTIFACT" >> "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
  fi
  triage_first_artifacts="$(join_launcher_paths \
    "$LAUNCHER_FAILURE_SUMMARY" \
    "$LAUNCHER_FAILURE_REPORT" \
    "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST" \
    "$LAUNCHER_FAILURE_REASON_PATH")"
  triage_retry_hint="fix the launcher/preflight failure at stage $LAUNCHER_FAILURE_STAGE, inspect $triage_first_artifacts, then rerun ./lca_smoke.sh"

  {
    echo "script=./lca_smoke.sh"
    echo "failure_stage=$LAUNCHER_FAILURE_STAGE"
    echo "exit_code=$failure_rc"
    echo "failure_kind=launcher_preflight_failure"
    echo "failure_origin=launcher"
    echo "failure_retryable=0"
    echo "failure_summary=$message"
    echo "message=$message"
    echo "working_directory=$working_directory"
    echo "original_launch_working_directory=$LAUNCHER_ORIGINAL_PWD"
    echo "branch_root=$BRANCH_ROOT"
    echo "artifacts_root=${ARTIFACTS_ROOT:-$BRANCH_ROOT/artifacts/lca_tree_stress_v5}"
    echo "failure_root=$LAUNCHER_FAILURE_ROOT"
    echo "run_archive_root=$LAUNCHER_RUN_ARCHIVE_ROOT"
    echo "run_archive_manifest=$LAUNCHER_RUN_ARTIFACT_MANIFEST"
    echo "run_console_stderr_path=$LAUNCHER_RUN_CONSOLE_LOG"
    echo "failure_reason_path=$LAUNCHER_FAILURE_REASON_PATH"
    echo "invocation_command_path=$LAUNCHER_FAILURE_INVOCATION_COMMAND_PATH"
    echo "dispatch_command_path=$LAUNCHER_FAILURE_DISPATCH_COMMAND_PATH"
    echo "rerun_command_path=$LAUNCHER_FAILURE_RERUN_COMMAND_PATH"
    echo "env_snapshot_path=$LAUNCHER_FAILURE_ENV_SNAPSHOT"
    echo "preflight_manifest_path=$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
    echo "artifact_manifest_path=$LAUNCHER_FAILURE_ARTIFACT_MANIFEST"
    if [[ -f "$LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION" ]]; then
      echo "smoke_manifest_selection_path=$LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION"
    fi
    if [[ -f "$LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR" ]]; then
      echo "smoke_manifest_check_stderr_path=$LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR"
    fi
    if [[ -n "$LAUNCHER_FAILURE_LINE" ]]; then
      echo "failing_line=$LAUNCHER_FAILURE_LINE"
    fi
    if [[ -f "$LAUNCHER_FAILURE_COMMAND_PATH" ]]; then
      echo "failing_command_path=$LAUNCHER_FAILURE_COMMAND_PATH"
    fi
    echo "last_check_kind=$LAUNCHER_LAST_CHECK_KIND"
    echo "last_check_label=$LAUNCHER_LAST_CHECK_LABEL"
    echo "last_check_status=$LAUNCHER_LAST_CHECK_STATUS"
    echo "last_check_detail=$LAUNCHER_LAST_CHECK_DETAIL"
    echo "last_check_artifact=$LAUNCHER_LAST_CHECK_ARTIFACT"
    echo "inner_wrapper=$INNER_WRAPPER"
    echo "build_wrapper=$BUILD_WRAPPER"
    echo "smoke_target_wrapper=$SMOKE_TARGET_WRAPPER"
    echo "artifact_resolver=$ARTIFACT_RESOLVER"
    echo "launcher_tmpdir=${LAUNCHER_TMPDIR:-}"
    echo "triage_stage_scope=launcher_pre_dispatch"
    echo "triage_stage=$LAUNCHER_FAILURE_STAGE"
    echo "triage_primary_summary=$LAUNCHER_FAILURE_SUMMARY"
    echo "triage_primary_report=$LAUNCHER_FAILURE_REPORT"
    echo "triage_primary_manifest=$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
    echo "triage_first_artifacts=$triage_first_artifacts"
    echo "triage_retry_command=./lca_smoke.sh"
    echo "triage_retry_hint=$triage_retry_hint"
  } > "$LAUNCHER_FAILURE_SUMMARY"

  {
    echo "# lca_smoke Launcher Failure Report"
    echo
    echo "- Stage: \`$LAUNCHER_FAILURE_STAGE\`"
    echo "- Exit code: \`$failure_rc\`"
    echo "- Failure kind: \`launcher_preflight_failure\`"
    echo "- Failure origin: \`launcher\`"
    echo "- Message: \`$message\`"
    echo "- Working directory: \`$working_directory\`"
    echo "- Original launch working directory: \`$LAUNCHER_ORIGINAL_PWD\`"
    echo "- Branch root: \`$BRANCH_ROOT\`"
    echo "- Artifacts root: \`${ARTIFACTS_ROOT:-$BRANCH_ROOT/artifacts/lca_tree_stress_v5}\`"
    echo "- Failure root: \`$LAUNCHER_FAILURE_ROOT\`"
    echo "- Run archive root: \`$LAUNCHER_RUN_ARCHIVE_ROOT\`"
    echo "- Run archive manifest: \`$LAUNCHER_RUN_ARTIFACT_MANIFEST\`"
    echo "- Launcher console transcript: \`$LAUNCHER_RUN_CONSOLE_LOG\`"
    echo "- Inner wrapper: \`$INNER_WRAPPER\`"
    echo "- Build wrapper: \`$BUILD_WRAPPER\`"
    echo "- Smoke target wrapper: \`$SMOKE_TARGET_WRAPPER\`"
    echo "- Artifact resolver: \`$ARTIFACT_RESOLVER\`"
    if [[ -n "$LAUNCHER_FAILURE_LINE" ]]; then
      echo "- Failing line: \`$LAUNCHER_FAILURE_LINE\`"
    fi
    echo
    echo "## Failed Stage"
    echo
    echo "- Failed stage scope: \`launcher_pre_dispatch\`"
    echo "- Failed stage: \`$LAUNCHER_FAILURE_STAGE\`"
    echo "- Primary summary: \`$LAUNCHER_FAILURE_SUMMARY\`"
    echo "- Primary report: \`$LAUNCHER_FAILURE_REPORT\`"
    echo "- Primary manifest: \`$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST\`"
    echo "- Inspect first: \`$triage_first_artifacts\`"
    echo
    echo "## Recorded Artifacts"
    echo
    echo "- Failure reason: \`$LAUNCHER_FAILURE_REASON_PATH\`"
    echo "- Invocation command: \`$LAUNCHER_FAILURE_INVOCATION_COMMAND_PATH\`"
    echo "- Dispatch command: \`$LAUNCHER_FAILURE_DISPATCH_COMMAND_PATH\`"
    echo "- Rerun command snapshot: \`$LAUNCHER_FAILURE_RERUN_COMMAND_PATH\`"
    echo "- Launcher env snapshot: \`$LAUNCHER_FAILURE_ENV_SNAPSHOT\`"
    echo "- Preflight manifest: \`$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST\`"
    echo "- Artifact manifest: \`$LAUNCHER_FAILURE_ARTIFACT_MANIFEST\`"
    echo "- Failure summary: \`$LAUNCHER_FAILURE_SUMMARY\`"
    if [[ -f "$LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION" ]]; then
      echo "- Smoke manifest selection: \`$LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION\`"
    fi
    if [[ -f "$LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR" ]]; then
      echo "- Smoke manifest check stderr: \`$LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR\`"
    fi
    if [[ -f "$LAUNCHER_FAILURE_COMMAND_PATH" ]]; then
      echo "- Failing command: \`$LAUNCHER_FAILURE_COMMAND_PATH\`"
    fi
    if [[ -n "$LAUNCHER_LAST_CHECK_STATUS" ]]; then
      echo
      echo "## Last Recorded Check"
      echo
      echo "- Kind: \`$LAUNCHER_LAST_CHECK_KIND\`"
      echo "- Label: \`$LAUNCHER_LAST_CHECK_LABEL\`"
      echo "- Status: \`$LAUNCHER_LAST_CHECK_STATUS\`"
      echo "- Detail: \`$LAUNCHER_LAST_CHECK_DETAIL\`"
      if [[ -n "$LAUNCHER_LAST_CHECK_ARTIFACT" ]]; then
        echo "- Artifact: \`$LAUNCHER_LAST_CHECK_ARTIFACT\`"
      fi
    fi
    echo
    echo "## Commands"
    echo
    echo "Invocation command:"
    echo
    echo "\`\`\`bash"
    echo "$LAUNCHER_INVOCATION_COMMAND"
    echo "\`\`\`"
    echo
    echo "Intended inner-wrapper dispatch command:"
    echo
    echo "\`\`\`bash"
    echo "$LAUNCHER_DISPATCH_COMMAND"
    echo "\`\`\`"
    echo
    echo "## Retry Next"
    echo
    echo "- Retry command: \`./lca_smoke.sh\`"
    echo "- Guidance: \`$triage_retry_hint\`"
  } > "$LAUNCHER_FAILURE_REPORT"
  write_launcher_artifact_manifest
}

report_launcher_failure_context() {
  local message="${LAUNCHER_FAILURE_MESSAGE:-launcher exited non-zero before inner wrapper dispatch}"
  local failure_rc="${LAUNCHER_FAILURE_RC:-$SMOKE_EXIT_HARNESS_FAILURE}"
  local triage_first_artifacts=""

  triage_first_artifacts="$(join_launcher_paths \
    "$LAUNCHER_FAILURE_SUMMARY" \
    "$LAUNCHER_FAILURE_REPORT" \
    "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST" \
    "$LAUNCHER_FAILURE_REASON_PATH")"

  emit_launcher_context_line "[lca_smoke] launcher failed before inner wrapper dispatch"
  emit_launcher_context_line "[lca_smoke] stage=$LAUNCHER_FAILURE_STAGE exit_code=$failure_rc message=$message"
  emit_launcher_context_line "[lca_smoke] failed stage: $LAUNCHER_FAILURE_STAGE scope=launcher_pre_dispatch"
  if [[ -n "$LAUNCHER_FAILURE_ROOT" ]]; then
    emit_launcher_context_line "[lca_smoke] failure reason snapshot: $LAUNCHER_FAILURE_REASON_PATH"
    emit_launcher_context_line "[lca_smoke] launcher failure root: $LAUNCHER_FAILURE_ROOT"
    emit_launcher_context_line "[lca_smoke] invocation command: $LAUNCHER_INVOCATION_COMMAND"
    emit_launcher_context_line "[lca_smoke] intended dispatch command: $LAUNCHER_DISPATCH_COMMAND"
    emit_launcher_context_line "[lca_smoke] invocation command snapshot: $LAUNCHER_FAILURE_INVOCATION_COMMAND_PATH"
    emit_launcher_context_line "[lca_smoke] dispatch command snapshot: $LAUNCHER_FAILURE_DISPATCH_COMMAND_PATH"
    emit_launcher_context_line "[lca_smoke] rerun command snapshot: $LAUNCHER_FAILURE_RERUN_COMMAND_PATH"
    emit_launcher_context_line "[lca_smoke] launcher env snapshot: $LAUNCHER_FAILURE_ENV_SNAPSHOT"
    emit_launcher_context_line "[lca_smoke] preflight manifest: $LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
    emit_launcher_context_line "[lca_smoke] artifact manifest: $LAUNCHER_FAILURE_ARTIFACT_MANIFEST"
    if [[ -n "$LAUNCHER_RUN_ARCHIVE_ROOT" ]]; then
      emit_launcher_context_line "[lca_smoke] run archive root: $LAUNCHER_RUN_ARCHIVE_ROOT"
    fi
    if [[ -n "$LAUNCHER_RUN_CONSOLE_LOG" ]]; then
      emit_launcher_context_line "[lca_smoke] launcher console transcript: $LAUNCHER_RUN_CONSOLE_LOG"
    fi
    if [[ -f "$LAUNCHER_FAILURE_COMMAND_PATH" ]]; then
      emit_launcher_context_line "[lca_smoke] failing command snapshot: $LAUNCHER_FAILURE_COMMAND_PATH"
    fi
    if [[ -n "$LAUNCHER_LAST_CHECK_STATUS" ]]; then
      emit_launcher_context_line "[lca_smoke] last recorded check: kind=$LAUNCHER_LAST_CHECK_KIND label=$LAUNCHER_LAST_CHECK_LABEL status=$LAUNCHER_LAST_CHECK_STATUS detail=$LAUNCHER_LAST_CHECK_DETAIL artifact=${LAUNCHER_LAST_CHECK_ARTIFACT:-"-"}"
    fi
    emit_launcher_context_line "[lca_smoke] inspect first: $triage_first_artifacts"
    emit_launcher_context_line "[lca_smoke] retry next: ./lca_smoke.sh"
    emit_launcher_context_line "[lca_smoke] failure summary: $LAUNCHER_FAILURE_SUMMARY"
    emit_launcher_context_line "[lca_smoke] failure report: $LAUNCHER_FAILURE_REPORT"
  fi
}

bootstrap_clean_env() {
  local -a clean_env_args
  local -a launcher_args=("$@")
  local preserved_name=""

  if [[ "${launcher_args[0]:-}" == "$LCA_SMOKE_LAUNCHER_REEXEC_ARG" ]]; then
    if [[ "${!LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG:-0}" == "1" ]]; then
      return
    fi
    fail "launcher clean-env reexec marker is missing the clean-env guard"
  fi

  unset "$LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG" "$LCA_SMOKE_INNER_CLEAN_ENV_FLAG"
  clean_env_args=(
    /usr/bin/env -i
    "HOME=$BRANCH_ROOT"
    "PATH=$LCA_SMOKE_CLEAN_PATH"
    "TERM=dumb"
    "LC_ALL=C"
    "LANG=C"
    "TZ=UTC"
    "TMPDIR=/tmp"
    "TMP=/tmp"
    "TEMP=/tmp"
    "PYTHONDONTWRITEBYTECODE=1"
    "PYTHONIOENCODING=UTF-8"
    "PYTHONUTF8=1"
    "PYTHONNOUSERSITE=1"
    "PYTHONHASHSEED=0"
    "$LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG=1"
    "$LCA_SMOKE_INNER_CLEAN_ENV_FLAG=1"
  )
  for preserved_name in \
    LCA_SMOKE_EXPORT_SNAPSHOT_ROOT \
    LCA_SMOKE_DEBUG_MANIFEST \
    LCA_SMOKE_BUILD_TIMEOUT_S \
    LCA_SMOKE_LAUNCHER_TIMEOUT_S \
    LCA_SMOKE_LAUNCHER_LOCK_TIMEOUT_S \
    LCA_SMOKE_LAUNCHER_ORIGINAL_COMMAND \
    LCA_SMOKE_LAUNCHER_ORIGINAL_PWD; do
    if [[ -n "${!preserved_name:-}" ]]; then
      clean_env_args+=("$preserved_name=${!preserved_name}")
    fi
  done
  if ((${#launcher_args[@]} > 0)); then
    exec "${clean_env_args[@]}" /usr/bin/env bash "$SELF_PATH" "$LCA_SMOKE_LAUNCHER_REEXEC_ARG" "${launcher_args[@]}"
  fi
  exec "${clean_env_args[@]}" /usr/bin/env bash "$SELF_PATH" "$LCA_SMOKE_LAUNCHER_REEXEC_ARG"
}

resolve_bash_bin() {
  if [[ -n "$BASH_BIN" && -x "$BASH_BIN" ]]; then
    set_launcher_last_check "command" "bash" "ok" "$BASH_BIN"
    return
  fi
  BASH_BIN="$(command -v bash 2>/dev/null || true)"
  if [[ -z "$BASH_BIN" || ! -x "$BASH_BIN" ]]; then
    set_launcher_last_check "command" "bash" "not_executable" "${BASH_BIN:-"-"}"
    fail "unable to locate an executable bash interpreter"
  fi
  set_launcher_last_check "command" "bash" "ok" "$BASH_BIN"
}

resolve_branch_local_roots() {
  local resolver_stderr=""

  resolve_launcher_failure_root
  mkdir -p "$LAUNCHER_FAILURE_ROOT" || fail "failed to prepare launcher failure root: $LAUNCHER_FAILURE_ROOT"
  resolver_stderr="$LAUNCHER_FAILURE_ROOT/artifact_resolver.stderr.txt"
  if ! BRANCH_ARTIFACTS_ROOT="$(PYTHONDONTWRITEBYTECODE=1 PYTHONNOUSERSITE=1 python3 -B "$ARTIFACT_RESOLVER" --artifacts-root 2>"$resolver_stderr")"; then
    set_launcher_last_check "artifact_resolver" "artifact resolver" "broken" "$ARTIFACT_RESOLVER" "$resolver_stderr"
    fail "broken artifact resolver: $ARTIFACT_RESOLVER"
  fi
  if [[ -z "$BRANCH_ARTIFACTS_ROOT" ]]; then
    set_launcher_last_check "artifact_resolver" "artifact resolver" "empty" "$ARTIFACT_RESOLVER" "$resolver_stderr"
    fail "artifact resolver returned an empty branch artifacts root"
  fi
  BRANCH_ARTIFACTS_ROOT="$(normalize_existing_path "$BRANCH_ARTIFACTS_ROOT" "branch artifacts root")"

  case "$BRANCH_ARTIFACTS_ROOT" in
    "$BRANCH_ROOT"/artifacts)
      ;;
    *)
      set_launcher_last_check "artifact_resolver" "artifact resolver" "escaped_repo_artifacts_root" "$BRANCH_ARTIFACTS_ROOT" "$resolver_stderr"
      fail "artifact resolver escaped repo-relative artifacts root: $BRANCH_ARTIFACTS_ROOT"
      ;;
  esac

  set_launcher_last_check "artifact_resolver" "artifact resolver" "ok" "$BRANCH_ARTIFACTS_ROOT" "$resolver_stderr"
  ARTIFACTS_ROOT="$BRANCH_ARTIFACTS_ROOT/lca_tree_stress_v5"
  SMOKE_OUTPUT_ROOT="$ARTIFACTS_ROOT/smoke"
  SMOKE_FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
  TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
  LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
  LAUNCHER_LOCKDIR="$LOCK_ROOT/lca_smoke.launcher"
  LAUNCHER_LOCK_PID_FILE="$LAUNCHER_LOCKDIR/pid"
  LAUNCHER_DISPATCH_STATE_PATH="$LOCK_ROOT/lca_smoke.dispatch.state"
  LAUNCHER_TMPDIR="$TMP_PARENT/lca_smoke.launcher.tmp"
  LAUNCHER_PREFLIGHT_ROOT="$LAUNCHER_TMPDIR/preflight"
  LAUNCHER_HOME="$LAUNCHER_TMPDIR/home"
  LAUNCHER_XDG_CONFIG_HOME="$LAUNCHER_TMPDIR/xdg_config"
  LAUNCHER_XDG_CACHE_HOME="$LAUNCHER_TMPDIR/xdg_cache"
  LAUNCHER_XDG_STATE_HOME="$LAUNCHER_TMPDIR/xdg_state"
  LAUNCHER_PYCACHE_ROOT="$LAUNCHER_TMPDIR/pycache"
  LAUNCHER_PREFLIGHT_MANIFEST_PATH="$LAUNCHER_PREFLIGHT_ROOT/preflight_manifest.tsv"
  LAUNCHER_PREFLIGHT_ENV_SNAPSHOT_PATH="$LAUNCHER_PREFLIGHT_ROOT/launcher_env.txt"
  LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION="$LAUNCHER_PREFLIGHT_ROOT/smoke_manifest_selection.txt"
  LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR="$LAUNCHER_PREFLIGHT_ROOT/smoke_manifest_check.stderr.txt"
  LAUNCHER_DISPATCH_MARKER="$LAUNCHER_RUN_PREFLIGHT_ROOT/dispatch.started"
  LAUNCHER_DISPATCH_RESULT_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/dispatch_result.txt"
  resolve_launcher_status_root
}

normalize_launcher_supported_overrides() {
  local normalized_snapshot_root=""

  if [[ -n "${LCA_SMOKE_DEBUG_MANIFEST:-}" ]]; then
    export LCA_SMOKE_DEBUG_MANIFEST="$SMOKE_CASES_SOURCE"
    set_launcher_last_check \
      "path" \
      "debug smoke manifest override" \
      "ok" \
      "$LCA_SMOKE_DEBUG_MANIFEST"
  fi

  if [[ -z "${LCA_SMOKE_EXPORT_SNAPSHOT_ROOT:-}" ]]; then
    return
  fi

  normalized_snapshot_root="$(normalize_branch_artifact_path "$LCA_SMOKE_EXPORT_SNAPSHOT_ROOT" "external smoke snapshot root")"
  case "$normalized_snapshot_root" in
    "$ARTIFACTS_ROOT"|"$ARTIFACTS_ROOT"/*)
      ;;
    *)
      set_launcher_last_check \
        "path" \
        "external smoke snapshot root" \
        "escaped_lca_tree_stress_root" \
        "$normalized_snapshot_root"
      fail "external smoke snapshot root escaped branch-local lca_tree_stress_v5 artifacts: $normalized_snapshot_root"
      ;;
  esac
  export LCA_SMOKE_EXPORT_SNAPSHOT_ROOT="$normalized_snapshot_root"
  set_launcher_last_check \
    "path" \
    "external smoke snapshot root" \
    "ok" \
    "$normalized_snapshot_root"
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

prepare_launcher_environment() {
  local state_path=""

  ensure_launcher_directory "$ARTIFACTS_ROOT" "launcher artifacts root" || fail "failed to prepare launcher artifacts root: $ARTIFACTS_ROOT"
  ensure_launcher_directory "$TMP_PARENT" "launcher tmp parent" || fail "failed to prepare launcher tmp parent: $TMP_PARENT"
  ensure_launcher_directory "$LOCK_ROOT" "launcher lock root" || fail "failed to prepare launcher lock root: $LOCK_ROOT"
  ensure_under_artifacts "$TMP_PARENT"
  ensure_under_artifacts "$LOCK_ROOT"
  ensure_under_artifacts "$LAUNCHER_TMPDIR"
  ensure_under_artifacts "$LAUNCHER_PREFLIGHT_ROOT"
  ensure_launcher_run_archive_root || fail "failed to initialize launcher run archive under $LAUNCHER_RUN_HISTORY_ROOT"

  LAUNCHER_TMPDIR_PARENT=""
  if [[ -e "$LAUNCHER_TMPDIR" ]]; then
    remove_path_retry "$LAUNCHER_TMPDIR" || fail "failed to clear stale launcher tmpdir: $LAUNCHER_TMPDIR"
  fi
  ensure_launcher_directory "$LAUNCHER_TMPDIR" "launcher tmpdir" || fail "failed to prepare launcher tmpdir: $LAUNCHER_TMPDIR"
  ensure_under_artifacts "$LAUNCHER_TMPDIR"
  LAUNCHER_PREFLIGHT_ROOT="$LAUNCHER_TMPDIR/preflight"
  LAUNCHER_HOME="$LAUNCHER_TMPDIR/home"
  LAUNCHER_XDG_CONFIG_HOME="$LAUNCHER_TMPDIR/xdg_config"
  LAUNCHER_XDG_CACHE_HOME="$LAUNCHER_TMPDIR/xdg_cache"
  LAUNCHER_XDG_STATE_HOME="$LAUNCHER_TMPDIR/xdg_state"
  LAUNCHER_PYCACHE_ROOT="$LAUNCHER_TMPDIR/pycache"
  LAUNCHER_PREFLIGHT_MANIFEST_PATH="$LAUNCHER_PREFLIGHT_ROOT/preflight_manifest.tsv"
  LAUNCHER_PREFLIGHT_ENV_SNAPSHOT_PATH="$LAUNCHER_PREFLIGHT_ROOT/launcher_env.txt"
  LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION="$LAUNCHER_PREFLIGHT_ROOT/smoke_manifest_selection.txt"
  LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR="$LAUNCHER_PREFLIGHT_ROOT/smoke_manifest_check.stderr.txt"
  LAUNCHER_DISPATCH_MARKER="$LAUNCHER_RUN_PREFLIGHT_ROOT/dispatch.started"
  LAUNCHER_DISPATCH_RESULT_PATH="$LAUNCHER_RUN_ARCHIVE_ROOT/dispatch_result.txt"

  for state_path in \
    "$LAUNCHER_HOME" \
    "$LAUNCHER_XDG_CONFIG_HOME" \
    "$LAUNCHER_XDG_CACHE_HOME" \
    "$LAUNCHER_XDG_STATE_HOME" \
    "$LAUNCHER_PYCACHE_ROOT"; do
    mkdir -p "$state_path"
    ensure_under_artifacts "$state_path"
  done
  mkdir -p "$LAUNCHER_PREFLIGHT_ROOT"
  ensure_under_artifacts "$LAUNCHER_PREFLIGHT_ROOT"

  export PATH="$LCA_SMOKE_CLEAN_PATH"
  export TERM=dumb
  export LC_ALL=C
  export LANG=C
  export TZ=UTC
  export PYTHONDONTWRITEBYTECODE=1
  export PYTHONIOENCODING=UTF-8
  export PYTHONUTF8=1
  export PYTHONNOUSERSITE=1
  export PYTHONHASHSEED=0
  export BRANCH_ARTIFACT_TMP_ROOT="$LAUNCHER_TMPDIR"
  export TMPDIR="$LAUNCHER_TMPDIR"
  export TMP="$LAUNCHER_TMPDIR"
  export TEMP="$LAUNCHER_TMPDIR"
  export HOME="$LAUNCHER_HOME"
  export XDG_CONFIG_HOME="$LAUNCHER_XDG_CONFIG_HOME"
  export XDG_CACHE_HOME="$LAUNCHER_XDG_CACHE_HOME"
  export XDG_STATE_HOME="$LAUNCHER_XDG_STATE_HOME"
  export PYTHONPYCACHEPREFIX="$LAUNCHER_PYCACHE_ROOT"
}

handle_launcher_signal() {
  local signal_name="$1"
  local signal_number="$2"

  if [[ -n "${LAUNCHER_ACTIVE_DISPATCH_MONITOR_PID:-}" ]]; then
    LAUNCHER_FAILURE_MESSAGE="launcher received ${signal_name} while waiting for inner smoke wrapper"
    if [[ "$LAUNCHER_FAILURE_RC" -eq 0 ]]; then
      LAUNCHER_FAILURE_RC="$SMOKE_EXIT_HARNESS_FAILURE"
    fi
    LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL="$signal_number"
    set_launcher_failure_stage "dispatch"
    set_launcher_last_check \
      "dispatch" \
      "outer smoke wrapper" \
      "interrupted" \
      "$signal_name" \
      "$LAUNCHER_DISPATCH_RESULT_PATH"
    kill -s "$signal_name" "$LAUNCHER_ACTIVE_DISPATCH_MONITOR_PID" 2>/dev/null || true
    return 0
  fi

  LAUNCHER_FAILURE_MESSAGE="launcher received ${signal_name} before inner wrapper dispatch completed"
  if [[ "$LAUNCHER_FAILURE_RC" -eq 0 ]]; then
    LAUNCHER_FAILURE_RC="$SMOKE_EXIT_HARNESS_FAILURE"
  fi
  exit "$SMOKE_EXIT_HARNESS_FAILURE"
}

wait_for_launcher_dispatch_result_after_signal() {
  local manager_pid="$1"
  local attempt=0

  if [[ -z "$manager_pid" || "$manager_pid" == "0" ]]; then
    return 1
  fi

  for (( attempt = 1; attempt <= 40; attempt++ )); do
    if [[ -s "$LAUNCHER_DISPATCH_RESULT_PATH" ]]; then
      return 0
    fi
    if ! kill -0 "$manager_pid" 2>/dev/null; then
      [[ -s "$LAUNCHER_DISPATCH_RESULT_PATH" ]]
      return $?
    fi
    sleep 0.05
  done

  [[ -s "$LAUNCHER_DISPATCH_RESULT_PATH" ]]
}

stop_active_launcher_dispatch_monitor() {
  local monitor_pid="${LAUNCHER_ACTIVE_DISPATCH_MONITOR_PID:-}"

  if [[ -z "$monitor_pid" || "$monitor_pid" == "0" ]]; then
    return 0
  fi
  if kill -0 "$monitor_pid" 2>/dev/null; then
    kill -TERM "$monitor_pid" 2>/dev/null || true
    wait_for_launcher_dispatch_result_after_signal "$monitor_pid" || true
    if kill -0 "$monitor_pid" 2>/dev/null; then
      kill -KILL "$monitor_pid" 2>/dev/null || true
    fi
  fi
  wait "$monitor_pid" 2>/dev/null || true
  LAUNCHER_ACTIVE_DISPATCH_MONITOR_PID=""
}

resolve_launcher_dispatch_timeout() {
  local raw_timeout="${LCA_SMOKE_LAUNCHER_TIMEOUT_S:-$LAUNCHER_DISPATCH_TIMEOUT_S_DEFAULT}"

  if ! LAUNCHER_DISPATCH_TIMEOUT_S="$(parse_positive_decimal_setting "$raw_timeout" "LCA_SMOKE_LAUNCHER_TIMEOUT_S")"; then
    set_launcher_last_check \
      "dispatch_timeout" \
      "launcher dispatch timeout" \
      "invalid" \
      "$raw_timeout"
    fail "invalid launcher dispatch timeout override: ${raw_timeout}"
  fi
  LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED=0
  LAUNCHER_DISPATCH_RAW_RC=0
  LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL=0
  set_launcher_last_check \
    "dispatch_timeout" \
    "launcher dispatch timeout" \
    "ok" \
    "$LAUNCHER_DISPATCH_TIMEOUT_S"
}

resolve_launcher_lock_timeout() {
  local raw_timeout="${LCA_SMOKE_LAUNCHER_LOCK_TIMEOUT_S:-$LAUNCHER_LOCK_WAIT_TIMEOUT_S_DEFAULT}"

  if ! LAUNCHER_LOCK_WAIT_TIMEOUT_S="$(parse_positive_decimal_setting "$raw_timeout" "LCA_SMOKE_LAUNCHER_LOCK_TIMEOUT_S")"; then
    set_launcher_last_check \
      "lock_timeout" \
      "launcher lock timeout" \
      "invalid" \
      "$raw_timeout"
    fail "invalid launcher lock timeout override: ${raw_timeout}"
  fi
  set_launcher_last_check \
    "lock_timeout" \
    "launcher lock timeout" \
    "ok" \
    "$LAUNCHER_LOCK_WAIT_TIMEOUT_S"
}

launcher_lock_wait_attempt_budget() {
  python3 - "$LAUNCHER_LOCK_WAIT_TIMEOUT_S" "$LAUNCHER_LOCK_RETRY_SLEEP_S" <<'PY'
from __future__ import annotations

import math
import sys

timeout_s = float(sys.argv[1])
sleep_s = float(sys.argv[2])
print(max(1, int(math.ceil(timeout_s / sleep_s))))
PY
}

release_launcher_lock() {
  local rc=0

  if (( LAUNCHER_LOCK_HELD )) && [[ -e "$LAUNCHER_LOCKDIR" ]]; then
    if ! remove_path_retry "$LAUNCHER_LOCKDIR"; then
      echo "[lca_smoke] warning: failed to release launcher run-control lock: $LAUNCHER_LOCKDIR" >&2
      rc=1
    fi
  fi
  LAUNCHER_LOCK_HELD=0
  return "$rc"
}

acquire_launcher_lock() {
  local holder=""
  local wait_budget=1
  local wait_count=0

  if [[ -e "$LOCK_ROOT" && ! -d "$LOCK_ROOT" ]]; then
    remove_path_retry "$LOCK_ROOT" || fail "failed to clear invalid launcher lock root: $LOCK_ROOT"
  fi
  mkdir -p "$LOCK_ROOT" || fail "failed to prepare launcher lock root: $LOCK_ROOT"
  ensure_under_artifacts "$LOCK_ROOT"
  ensure_under_artifacts "$LAUNCHER_LOCKDIR"
  if ! wait_budget="$(launcher_lock_wait_attempt_budget)"; then
    fail "failed to compute launcher lock wait budget from timeout ${LAUNCHER_LOCK_WAIT_TIMEOUT_S}s"
  fi

  while true; do
    if mkdir "$LAUNCHER_LOCKDIR" 2>/dev/null; then
      printf '%s\n' "$$" > "$LAUNCHER_LOCK_PID_FILE"
      LAUNCHER_LOCK_HELD=1
      set_launcher_last_check \
        "lock" \
        "launcher run-control lock" \
        "ok" \
        "$LAUNCHER_LOCKDIR" \
        "$LAUNCHER_LOCK_PID_FILE"
      return 0
    fi

    if [[ ! -f "$LAUNCHER_LOCK_PID_FILE" ]]; then
      sleep "$LAUNCHER_LOCK_RETRY_SLEEP_S"
      if [[ ! -f "$LAUNCHER_LOCK_PID_FILE" ]]; then
        remove_path_retry "$LAUNCHER_LOCKDIR" || fail "failed to clear stale launcher run-control lock: $LAUNCHER_LOCKDIR"
      fi
      continue
    fi

    read -r holder < "$LAUNCHER_LOCK_PID_FILE" || holder=""
    if [[ -z "$holder" ]]; then
      sleep "$LAUNCHER_LOCK_RETRY_SLEEP_S"
      if [[ -f "$LAUNCHER_LOCK_PID_FILE" ]]; then
        continue
      fi
      remove_path_retry "$LAUNCHER_LOCKDIR" || fail "failed to clear empty launcher run-control lock: $LAUNCHER_LOCKDIR"
      continue
    fi

    if kill -0 "$holder" 2>/dev/null; then
      if (( wait_count >= wait_budget )); then
        set_launcher_last_check \
          "lock" \
          "launcher run-control lock" \
          "busy" \
          "$holder" \
          "$LAUNCHER_LOCKDIR"
        fail "another lca_smoke.sh launcher run is active (pid $holder); waited ${LAUNCHER_LOCK_WAIT_TIMEOUT_S}s for the launcher run-control lock"
      fi
      sleep "$LAUNCHER_LOCK_RETRY_SLEEP_S"
      wait_count=$(( wait_count + 1 ))
      continue
    fi

    remove_path_retry "$LAUNCHER_LOCKDIR" || fail "failed to clear stale launcher run-control lock: $LAUNCHER_LOCKDIR"
  done
}

load_launcher_dispatch_result() {
  local result_path="$1"
  local line=""
  local raw_rc=""
  local timed_out="0"
  local interrupted_signal="0"

  LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED=0
  LAUNCHER_DISPATCH_RAW_RC=0
  LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL=0

  while IFS= read -r line || [[ -n "$line" ]]; do
    case "$line" in
      raw_exit_code=*)
        raw_rc="${line#*=}"
        ;;
      timed_out=*)
        timed_out="${line#*=}"
        ;;
      interrupted_signal=*)
        interrupted_signal="${line#*=}"
        ;;
    esac
  done < "$result_path"

  case "$raw_rc" in
    ''|*[!0-9]*)
      return 1
      ;;
  esac
  case "$timed_out" in
    0|1)
      ;;
    *)
      return 1
      ;;
  esac
  case "$interrupted_signal" in
    ''|*[!0-9]*)
      return 1
      ;;
  esac

  LAUNCHER_DISPATCH_RAW_RC="$raw_rc"
  LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED="$timed_out"
  LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL="$interrupted_signal"
}

copy_launcher_preflight_artifact() {
  local source_path="$1"
  local target_path="$2"

  if [[ -f "$source_path" ]]; then
    cp "$source_path" "$target_path"
  fi
}

sync_launcher_preflight_artifacts() {
  copy_launcher_preflight_artifact \
    "$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION" \
    "$LAUNCHER_FAILURE_SMOKE_MANIFEST_SELECTION"
  copy_launcher_preflight_artifact \
    "$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR" \
    "$LAUNCHER_FAILURE_SMOKE_MANIFEST_STDERR"
}

check_smoke_manifest_selection() {
  mkdir -p "$LAUNCHER_PREFLIGHT_ROOT" || fail "failed to prepare launcher preflight root: $LAUNCHER_PREFLIGHT_ROOT"
  rm -f "$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION" "$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR" 2>/dev/null || true
  if python3 - "$SMOKE_CASES_SOURCE" "$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION" "$SMOKE_MANIFEST_INPUT_POLICY" <<'PY' \
    >/dev/null 2>"$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR"
from __future__ import annotations

import csv
import hashlib
import sys
from pathlib import Path

manifest_path = Path(sys.argv[1])
selection_path = Path(sys.argv[2])
input_policy = sys.argv[3]
rows: list[tuple[str, str, str, str, str, str, str]] = []

with manifest_path.open("r", encoding="utf-8", newline="") as handle:
    reader = csv.reader(handle, delimiter="\t")
    for line_no, raw_row in enumerate(reader, start=1):
        row = [field[:-1] if field.endswith("\r") else field for field in raw_row]
        if not any(row):
            continue
        if line_no == 1 and row[:7] == [
            "stage",
            "mode",
            "n",
            "seed",
            "shuffle_labels",
            "shuffle_queries",
            "timeout_s",
        ]:
            continue
        if len(row) != 7:
            raise SystemExit(f"smoke manifest row {line_no} must have 7 tab-separated columns")
        stage, mode, n_raw, seed_raw, shuffle_labels, shuffle_queries, timeout_raw = row
        if not stage or not mode:
            raise SystemExit(f"smoke manifest row {line_no} must provide non-empty stage/mode columns")
        if not n_raw.isdecimal():
            raise SystemExit(f"smoke manifest row {line_no} has invalid n: {n_raw}")
        if int(n_raw) <= 0:
            raise SystemExit(f"smoke manifest row {line_no} must use n > 0 (got: {n_raw})")
        if not seed_raw.isdecimal():
            raise SystemExit(f"smoke manifest row {line_no} has invalid seed: {seed_raw}")
        if shuffle_labels not in {"0", "1"}:
            raise SystemExit(
                f"smoke manifest row {line_no} has invalid shuffle_labels flag: {shuffle_labels}"
            )
        if shuffle_queries not in {"0", "1"}:
            raise SystemExit(
                f"smoke manifest row {line_no} has invalid shuffle_queries flag: {shuffle_queries}"
            )
        try:
            timeout_value = float(timeout_raw)
        except ValueError as exc:
            raise SystemExit(
                f"smoke manifest row {line_no} timeout_s must be a positive decimal (got: {timeout_raw})"
            ) from exc
        if timeout_value <= 0.0:
            raise SystemExit(
                f"smoke manifest row {line_no} timeout_s must be > 0 (got: {timeout_raw})"
            )
        rows.append((stage, mode, n_raw, seed_raw, shuffle_labels, shuffle_queries, timeout_raw))

if not rows:
    raise SystemExit(f"smoke case manifest produced zero executable rows: {manifest_path}")

digest = hashlib.sha256()
for row in rows:
    digest.update(("\t".join(row) + "\n").encode("utf-8"))

summary_lines = [
    f"manifest_path={manifest_path}",
    f"case_count={len(rows)}",
    "selection_policy=manifest_row_order",
    f"input_policy={input_policy}",
    "seed_policy=manifest_seed",
    f"normalized_manifest_sha256={digest.hexdigest()}",
]
for index, row in enumerate(rows, start=1):
    summary_lines.append(f"case{index:02d}=" + "\t".join(row))

selection_path.write_text("\n".join(summary_lines) + "\n", encoding="utf-8")
PY
  then
    set_launcher_last_check \
      "smoke_manifest" \
      "smoke case manifest" \
      "ok" \
      "$SMOKE_CASES_SOURCE" \
      "$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION"
    return
  fi

  set_launcher_last_check \
    "smoke_manifest" \
    "smoke case manifest" \
    "invalid" \
    "$SMOKE_CASES_SOURCE" \
    "$LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR"
  fail "invalid smoke case manifest: $SMOKE_CASES_SOURCE"
}

clear_stale_launcher_failure_bundle() {
  if [[ -z "${LAUNCHER_FAILURE_ROOT:-}" ]]; then
    return
  fi
  if [[ -e "$LAUNCHER_FAILURE_ROOT" ]]; then
    remove_path_retry "$LAUNCHER_FAILURE_ROOT" || fail "failed to clear stale launcher failure root: $LAUNCHER_FAILURE_ROOT"
  fi
}

clear_stale_launcher_status_bundle() {
  if [[ -z "${LAUNCHER_STATUS_ROOT:-}" ]]; then
    return
  fi
  if [[ -e "$LAUNCHER_STATUS_ROOT" ]]; then
    remove_path_retry "$LAUNCHER_STATUS_ROOT" || fail "failed to clear stale launcher status root: $LAUNCHER_STATUS_ROOT"
  fi
}

clear_stale_launcher_smoke_output_root() {
  if [[ -z "${SMOKE_OUTPUT_ROOT:-}" ]]; then
    return
  fi
  if [[ -e "$SMOKE_OUTPUT_ROOT" ]]; then
    remove_path_retry "$SMOKE_OUTPUT_ROOT" || fail "failed to clear stale smoke output root: $SMOKE_OUTPUT_ROOT"
  fi
}

cleanup_launcher() {
  local rc="${1:-$?}"

  trap - EXIT ERR HUP INT TERM
  set +e
  stop_active_launcher_dispatch_monitor || true
  if (( rc != 0 )) && (( LAUNCHER_SKIP_FAILURE_BUNDLE == 0 )); then
    if [[ "$LAUNCHER_FAILURE_RC" -eq 0 ]]; then
      LAUNCHER_FAILURE_RC="$rc"
    fi
    if [[ -z "$LAUNCHER_FAILURE_MESSAGE" ]]; then
      LAUNCHER_FAILURE_MESSAGE="launcher exited non-zero before inner wrapper dispatch"
    fi
    write_launcher_failure_bundle || true
    set_launcher_status \
      "harness_infrastructure_failure" \
      "$SMOKE_EXIT_HARNESS_FAILURE" \
      "$LAUNCHER_FAILURE_RC" \
      "launcher" \
      "$LAUNCHER_FAILURE_MESSAGE" \
      "$LAUNCHER_FAILURE_ROOT" \
      "$LAUNCHER_FAILURE_SUMMARY" \
      "$LAUNCHER_FAILURE_REPORT"
    write_launcher_status_bundle || true
    report_launcher_failure_context || true
    report_launcher_status_context || true
    rc="$LAUNCHER_STATUS_NORMALIZED_RC"
  fi
  if (( LAUNCHER_LOCK_HELD != 0 )); then
    if [[ -n "${LAUNCHER_TMPDIR:-}" && -e "$LAUNCHER_TMPDIR" ]]; then
      remove_path_retry "$LAUNCHER_TMPDIR" || true
    fi
    if [[ -n "${LAUNCHER_TMPDIR_PARENT:-}" && -e "$LAUNCHER_TMPDIR_PARENT" ]]; then
      remove_path_retry "$LAUNCHER_TMPDIR_PARENT" || true
    fi
    release_launcher_lock || true
  fi
  if [[ -n "${LOCK_ROOT:-}" ]]; then
    rmdir "$LOCK_ROOT" 2>/dev/null || true
  fi
  if [[ -n "${TMP_PARENT:-}" ]]; then
    rmdir "$TMP_PARENT" 2>/dev/null || true
  fi
  exit "$rc"
}

finalize_launcher_dispatch_result() {
  local normalized_rc="${LAUNCHER_STATUS_NORMALIZED_RC:-$SMOKE_EXIT_HARNESS_FAILURE}"
  local status_bundle_rc=0
  local status_report_rc=0

  LAUNCHER_SKIP_FAILURE_BUNDLE=1
  trap - ERR HUP INT TERM
  set +e
  write_launcher_status_bundle
  status_bundle_rc=$?
  if (( status_bundle_rc != 0 )); then
    echo "[lca_smoke] warning: failed to publish launcher status bundle after dispatch normalization; preserving normalized exit code $normalized_rc" >&2
  fi
  report_launcher_status_context
  status_report_rc=$?
  if (( status_report_rc != 0 )); then
    echo "[lca_smoke] warning: failed to print launcher status context after dispatch normalization; preserving normalized exit code $normalized_rc" >&2
  fi
  exit "$normalized_rc"
}

run_inner_wrapper_dispatch() {
  local manager_rc=0

  mkdir -p "$LAUNCHER_PREFLIGHT_ROOT" || fail "failed to prepare launcher preflight root for dispatch monitor: $LAUNCHER_PREFLIGHT_ROOT"
  mkdir -p "$(dirname "$LAUNCHER_DISPATCH_RESULT_PATH")" || fail "failed to prepare launcher dispatch result root: $LAUNCHER_DISPATCH_RESULT_PATH"
  mkdir -p "$(dirname "$LAUNCHER_DISPATCH_STATE_PATH")" || fail "failed to prepare launcher dispatch state root: $LAUNCHER_DISPATCH_STATE_PATH"
  rm -f "$LAUNCHER_DISPATCH_RESULT_PATH" 2>/dev/null || true
  rm -f "$LAUNCHER_DISPATCH_STATE_PATH" 2>/dev/null || true
  python3 - "$LAUNCHER_DISPATCH_RESULT_PATH" "$LAUNCHER_DISPATCH_STATE_PATH" "$LAUNCHER_DISPATCH_TIMEOUT_S" "$LAUNCHER_DISPATCH_KILL_GRACE_S" "$BASH_BIN" "$INNER_WRAPPER" "$@" <<'PY' &
from __future__ import annotations

import os
import signal
import subprocess
import sys
from pathlib import Path

result_path = Path(sys.argv[1])
state_path = Path(sys.argv[2])
timeout_s = float(sys.argv[3])
kill_grace_s = float(sys.argv[4])
command = sys.argv[5:]

timed_out = False
raw_exit_code = 0
interrupted_signal = 0
result_written = False
proc: subprocess.Popen[str] | None = None


def current_child_pgid() -> int:
    if proc is None:
        return 0
    try:
        return os.getpgid(proc.pid)
    except ProcessLookupError:
        return 0


def write_state() -> None:
    if proc is None:
        return
    tmp_path = state_path.with_name(state_path.name + ".tmp")
    child_entrypoint = command[1] if len(command) > 1 else (command[0] if command else "")
    tmp_path.write_text(
        f"manager_pid={os.getpid()}\n"
        f"child_pid={proc.pid}\n"
        f"child_pgid={current_child_pgid()}\n"
        f"child_command={command[-1] if command else ''}\n"
        f"child_entrypoint={child_entrypoint}\n",
        encoding="utf-8",
    )
    os.replace(tmp_path, state_path)


def terminate_child_process_group(initial_signal: int = int(signal.SIGTERM)) -> int:
    if proc is None:
        return raw_exit_code
    child_pgid = current_child_pgid()
    if child_pgid > 0:
        try:
            os.killpg(child_pgid, initial_signal)
        except ProcessLookupError:
            pass
    try:
        exit_code = proc.wait(timeout=kill_grace_s)
    except subprocess.TimeoutExpired:
        if child_pgid > 0:
            try:
                os.killpg(child_pgid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        exit_code = proc.wait()
    return exit_code

def write_result() -> None:
    global result_written
    tmp_path = result_path.with_name(result_path.name + ".tmp")
    tmp_path.write_text(
        f"raw_exit_code={raw_exit_code}\n"
        f"timed_out={1 if timed_out else 0}\n"
        f"interrupted_signal={interrupted_signal}\n",
        encoding="utf-8",
    )
    os.replace(tmp_path, result_path)
    result_written = True


def clear_state() -> None:
    state_path.unlink(missing_ok=True)

def handle_signal(signum: int, _frame: object) -> None:
    global raw_exit_code, interrupted_signal
    interrupted_signal = signum
    raw_exit_code = terminate_child_process_group(signum)
    if raw_exit_code < 0:
        raw_exit_code = 128 + (-raw_exit_code)
    if raw_exit_code == 0:
        raw_exit_code = 128 + signum
    write_result()
    clear_state()
    raise SystemExit(128 + signum)

for signum in (signal.SIGHUP, signal.SIGINT, signal.SIGTERM):
    signal.signal(signum, handle_signal)

try:
    proc = subprocess.Popen(command, start_new_session=True)
    write_state()
    raw_exit_code = proc.wait(timeout=timeout_s)
except KeyboardInterrupt:
    interrupted_signal = int(signal.SIGINT)
    raw_exit_code = terminate_child_process_group(int(signal.SIGINT))
except subprocess.TimeoutExpired:
    timed_out = True
    raw_exit_code = terminate_child_process_group()
finally:
    if proc is not None and proc.poll() is None:
        raw_exit_code = terminate_child_process_group()
    if raw_exit_code < 0:
        raw_exit_code = 128 + (-raw_exit_code)
    if timed_out:
        raw_exit_code = 124
    if not result_written:
        write_result()
    clear_state()
PY
  LAUNCHER_ACTIVE_DISPATCH_MONITOR_PID=$!
  if wait "$LAUNCHER_ACTIVE_DISPATCH_MONITOR_PID"; then
    :
  else
    manager_rc=$?
    if (( LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL != 0 )); then
      wait_for_launcher_dispatch_result_after_signal "$LAUNCHER_ACTIVE_DISPATCH_MONITOR_PID" || true
    fi
  fi

  if (( manager_rc != 0 )) && [[ ! -s "$LAUNCHER_DISPATCH_RESULT_PATH" ]]; then
    set_launcher_failure_stage "dispatch_monitor"
    set_launcher_last_check \
      "dispatch_monitor" \
      "inner wrapper dispatch monitor" \
      "broken" \
      "$manager_rc" \
      "$LAUNCHER_DISPATCH_RESULT_PATH"
    fail "inner wrapper dispatch monitor failed with exit code $manager_rc"
  fi
  if [[ ! -s "$LAUNCHER_DISPATCH_RESULT_PATH" ]]; then
    set_launcher_failure_stage "dispatch_result_capture"
    set_launcher_last_check \
      "dispatch_monitor" \
      "inner wrapper dispatch monitor" \
      "missing_result" \
      "$LAUNCHER_DISPATCH_RESULT_PATH"
    fail "inner wrapper dispatch monitor did not record a dispatch result"
  fi
  if ! load_launcher_dispatch_result "$LAUNCHER_DISPATCH_RESULT_PATH"; then
    set_launcher_failure_stage "dispatch_result_capture"
    set_launcher_last_check \
      "dispatch_monitor" \
      "inner wrapper dispatch monitor" \
      "invalid_result" \
      "$LAUNCHER_DISPATCH_RESULT_PATH" \
      "$LAUNCHER_DISPATCH_RESULT_PATH"
    fail "inner wrapper dispatch monitor wrote an invalid dispatch result"
  fi
  LAUNCHER_ACTIVE_DISPATCH_MONITOR_PID=""
  return "$LAUNCHER_DISPATCH_RAW_RC"
}

main() {
  local inner_wrapper_rc=0
  local -a launcher_args=("$@")

  if ((${#launcher_args[@]} > 0)); then
    capture_original_launcher_context "${launcher_args[@]}"
    bootstrap_clean_env "${launcher_args[@]}"
  else
    capture_original_launcher_context
    bootstrap_clean_env
  fi
  if [[ "${launcher_args[0]:-}" == "$LCA_SMOKE_LAUNCHER_REEXEC_ARG" ]]; then
    launcher_args=("${launcher_args[@]:1}")
  fi
  sanitize_shell_state
  trap 'cleanup_launcher "$?"' EXIT
  trap 'capture_launcher_err "$?" "$LINENO" "$BASH_COMMAND"' ERR
  trap 'handle_launcher_signal SIGHUP 1' HUP
  trap 'handle_launcher_signal SIGINT 2' INT
  trap 'handle_launcher_signal SIGTERM 15' TERM
  umask 022
  if ((${#launcher_args[@]} > 0)); then
    record_launcher_invocation "${launcher_args[@]}"
  else
    record_launcher_invocation
  fi
  set_launcher_failure_stage "working_directory_normalization"
  enter_branch_root
  set_launcher_failure_stage "preflight"

  require_command bash
  require_command python3
  require_command mkdir
  require_command mktemp
  require_command dirname
  require_command chmod
  require_command cp
  require_command mv
  require_command rm
  require_command rmdir
  require_command kill
  require_command tail
  require_command sleep
  require_command grep
  require_command sort
  require_command date
  require_command ln
  require_build_compiler
  prepare_launcher_artifact_namespace
  validate_launcher_repo_root_layout
  normalize_launcher_prerequisite_paths
  require_executable "$INNER_WRAPPER" "outer smoke wrapper"
  require_file "$RELEASE_ENV" "release env wrapper"
  require_file "$ARTIFACT_RESOLVER" "artifact resolver"
  require_file "$RUN_CASE_HELPER" "branch-local case helper"
  require_file "$CHECKER_HELPER" "branch-local validator"
  require_file "$BUILD_HELPER" "build helper"
  require_file "$RESUME_HELPER" "resume helper"
  require_file "$SOURCE" "solver source"
  require_file "$SMOKE_CASES_SOURCE" "smoke case manifest"
  require_executable "$BUILD_WRAPPER" "build wrapper"
  require_executable "$SMOKE_TARGET_WRAPPER" "smoke target wrapper"

  set_launcher_failure_stage "dispatch_resolution"
  resolve_bash_bin
  if ((${#launcher_args[@]} > 0)); then
    LAUNCHER_DISPATCH_COMMAND="$(quote_command "$BASH_BIN" "$INNER_WRAPPER" "${launcher_args[@]}")"
  else
    LAUNCHER_DISPATCH_COMMAND="$(quote_command "$BASH_BIN" "$INNER_WRAPPER")"
  fi
  set_launcher_failure_stage "launcher_environment_setup"
  resolve_branch_local_roots
  normalize_launcher_supported_overrides
  set_launcher_failure_stage "launcher_lock_timeout_resolution"
  resolve_launcher_lock_timeout
  set_launcher_failure_stage "launcher_lock_acquisition"
  acquire_launcher_lock
  set_launcher_failure_stage "launcher_environment_setup"
  prepare_launcher_environment
  set_launcher_failure_stage "shell_entrypoint_validation"
  check_shell_syntax "$INNER_WRAPPER" "outer smoke wrapper syntax"
  check_shell_syntax "$RELEASE_ENV" "release env wrapper syntax"
  check_shell_syntax "$BUILD_WRAPPER" "build wrapper syntax"
  check_shell_syntax "$SMOKE_TARGET_WRAPPER" "smoke target wrapper syntax"
  set_launcher_failure_stage "smoke_manifest_validation"
  check_smoke_manifest_selection
  set_launcher_failure_stage "python_entrypoint_validation"
  check_python_entrypoint "$ARTIFACT_RESOLVER" "artifact resolver imports"
  check_python_entrypoint "$BUILD_HELPER" "build helper imports"
  check_python_entrypoint "$RUN_CASE_HELPER" "run case helper imports"
  check_python_entrypoint "$CHECKER_HELPER" "validator helper imports"
  check_python_entrypoint "$RESUME_HELPER" "resume helper imports"

  set_launcher_failure_stage "dispatch_timeout_resolution"
  resolve_launcher_dispatch_timeout
  set_launcher_failure_stage "preflight_snapshot_publication"
  write_launcher_preflight_artifacts
  set_launcher_failure_stage "stale_status_cleanup"
  clear_stale_launcher_status_bundle
  set_launcher_failure_stage "stale_smoke_root_cleanup"
  clear_stale_launcher_smoke_output_root
  set_launcher_failure_stage "stale_failure_cleanup"
  clear_stale_launcher_failure_bundle
  set_launcher_failure_stage "stale_inner_rerun_cleanup"
  if ! LAUNCHER_FAILURE_MESSAGE="$(clear_stale_inner_wrapper_dispatch_state)"; then
    fail "$LAUNCHER_FAILURE_MESSAGE"
  fi
  set_launcher_failure_stage "dispatch_preparation"
  record_launcher_dispatch_marker
  set_launcher_failure_stage "dispatch"
  cd "$BRANCH_ROOT"
  # Keep inner-wrapper failures on the handled normalization path instead of
  # tripping launcher-side ERR/errexit handling before status publication.
  if ((${#launcher_args[@]} > 0)); then
    if run_inner_wrapper_dispatch "${launcher_args[@]}"; then
      inner_wrapper_rc=0
    else
      inner_wrapper_rc=$?
    fi
  else
    if run_inner_wrapper_dispatch; then
      inner_wrapper_rc=0
    else
      inner_wrapper_rc=$?
    fi
  fi
  if (( LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL != 0 )); then
    set_launcher_last_check \
      "dispatch" \
      "outer smoke wrapper" \
      "interrupted" \
      "$(dispatch_signal_name "$LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL")" \
      "$LAUNCHER_DISPATCH_RESULT_PATH"
  elif (( LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED == 1 )); then
    set_launcher_last_check \
      "dispatch" \
      "outer smoke wrapper" \
      "timeout" \
      "$LAUNCHER_DISPATCH_TIMEOUT_S" \
      "$LAUNCHER_DISPATCH_RESULT_PATH"
  else
    set_launcher_last_check \
      "dispatch" \
      "outer smoke wrapper" \
      "returned" \
      "$inner_wrapper_rc" \
      "$LAUNCHER_DISPATCH_RESULT_PATH"
  fi
  if (( LAUNCHER_DISPATCH_INTERRUPTED_SIGNAL != 0 )) || (( LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED == 1 )); then
    set_launcher_failure_stage "dispatch"
  else
    set_launcher_failure_stage "status_normalization"
  fi
  classify_inner_wrapper_exit "$inner_wrapper_rc"
  finalize_launcher_dispatch_result
}

main "$@"
