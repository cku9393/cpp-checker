#!/usr/bin/env bash
set -Eeuo pipefail

SMOKE_EXIT_SOLVER_FAILURE=1
SMOKE_EXIT_USAGE=2
SMOKE_EXIT_SOLVER_TIMEOUT=124
SMOKE_EXIT_SOLVER_RUNTIME_FAILURE=125
SMOKE_EXIT_HARNESS_FAILURE=70
LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG="LCA_SMOKE_LAUNCHER_CLEAN_ENV_READY"
LCA_SMOKE_INNER_CLEAN_ENV_FLAG="LCA_SMOKE_CLEAN_ENV_READY"
LCA_SMOKE_LAUNCHER_REEXEC_ARG="--__lca_smoke_launcher_clean_env_reexec"
LCA_SMOKE_CLEAN_PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"
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
LAUNCHER_TMPDIR=""
LAUNCHER_TMPDIR_PARENT=""
LAUNCHER_PREFLIGHT_ROOT=""
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
SMOKE_OUTPUT_ROOT=""
SMOKE_FAILURE_ROOT=""
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
LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT=""
LAUNCHER_RUN_ARTIFACT_MANIFEST=""
LAUNCHER_RUN_ID=""
LAUNCHER_RUN_STARTED_AT_UTC=""
LAUNCHER_RUN_FINISHED_AT_UTC=""
LAUNCHER_RUN_STARTED_SECONDS=0
LAUNCHER_RUN_ELAPSED_SECONDS=0
LAUNCHER_RUN_COMPARISON_SUMMARY=""
LAUNCHER_RUN_COMPARISON_CHANGED_FIELDS=""
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
LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH=""
LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH=""
LAUNCHER_REPLAY_SUMMARY=""
LAUNCHER_REPLAY_CASE_TAG=""
LAUNCHER_REPLAY_STAGE=""
LAUNCHER_REPLAY_MODE=""
LAUNCHER_REPLAY_N=""
LAUNCHER_REPLAY_SEED=""
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
LAUNCHER_RETRY_LOOP_ACTION=""
LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND=""
LAUNCHER_RETRY_LOOP_LAUNCH_COMMAND=""
LAUNCHER_RETRY_LOOP_DIRECT_COMMAND=""
LAUNCHER_RETRY_LOOP_HINT=""
LAUNCHER_RETRY_LOOP_LOG_PATH=""
LAUNCHER_STATUS_WRITTEN=0
LAUNCHER_SKIP_FAILURE_BUNDLE=0
LAUNCHER_DISPATCH_TIMEOUT_S_DEFAULT="600"
LAUNCHER_DISPATCH_KILL_GRACE_S="0.2"
LAUNCHER_DISPATCH_TIMEOUT_S=""
LAUNCHER_DISPATCH_RESULT_PATH=""
LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED=0
LAUNCHER_DISPATCH_RAW_RC=0
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

  if [[ -e "$path" && ! -d "$path" ]]; then
    remove_path_retry "$path" || return 1
  fi
  mkdir -p "$path" || return 1
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
  LAUNCHER_STATUS_PUBLISHED_SMOKE_SUMMARY_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/summary.txt}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_REPORT_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/status_report.md}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_FAILURE_REPORT_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/failure_report.md}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_ITERATION_EVIDENCE_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/iteration_evidence.txt}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_RETRY_LOOP_CONTROL_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/retry_loop_control.json}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_DIAGNOSTICS_MANIFEST_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/diagnostics_manifest.tsv}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_STANDARD_GAP_JSON_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/standard_gap.json}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_RECORD_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/run_record.json}"
  LAUNCHER_STATUS_PUBLISHED_SMOKE_RUN_COMPARISON_PATH="${SMOKE_OUTPUT_ROOT:+$SMOKE_OUTPUT_ROOT/run_comparison.json}"
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
  LAUNCHER_RUN_ARCHIVE_ROOT="$(mktemp -d "$LAUNCHER_RUN_HISTORY_ROOT/run.XXXXXX")" || return 1
  case "$LAUNCHER_RUN_ARCHIVE_ROOT" in
    "$effective_artifacts_root"|"$effective_artifacts_root"/*)
      ;;
    *)
      fail "launcher run archive root escaped branch-local artifacts root: $LAUNCHER_RUN_ARCHIVE_ROOT"
      ;;
  esac
  LAUNCHER_RUN_EXPORT_ALIAS_ROOT="$LAUNCHER_RUN_EXPORT_ROOT/run-${LAUNCHER_RUN_ARCHIVE_ROOT##*.}"
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
  LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT="$LAUNCHER_RUN_ARCHIVE_ROOT/launcher_failure_root_snapshot"
  LAUNCHER_RUN_ARTIFACT_MANIFEST="$LAUNCHER_RUN_ARCHIVE_ROOT/artifact_manifest.tsv"
  LAUNCHER_RUN_ID="${LAUNCHER_RUN_ARCHIVE_ROOT##*/}"
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

write_launcher_run_artifact_manifest() {
  {
    printf 'artifact\tpath\tprovenance\n'
    printf 'console_stderr\t%s\tlauncher_console_transcript\n' "$LAUNCHER_RUN_CONSOLE_LOG"
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
    if [[ -e "$LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT" ]]; then
      printf 'launcher_failure_root_snapshot\t%s\tcopy_of_%s\n' "$LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT" "$LAUNCHER_FAILURE_ROOT"
    fi
  } > "$LAUNCHER_RUN_ARTIFACT_MANIFEST"
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

clear_launcher_source_failure_details() {
  LAUNCHER_REPLAY_SUMMARY=""
  LAUNCHER_REPLAY_CASE_TAG=""
  LAUNCHER_REPLAY_STAGE=""
  LAUNCHER_REPLAY_MODE=""
  LAUNCHER_REPLAY_N=""
  LAUNCHER_REPLAY_SEED=""
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

  clear_launcher_source_failure_details
  if [[ -n "$source_summary" && -f "$source_summary" ]]; then
    while IFS= read -r line || [[ -n "$line" ]]; do
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
  LAUNCHER_RETRY_LOOP_ACTION="resume_progress40_retry_loop"
  LAUNCHER_RETRY_LOOP_PREFERRED_COMMAND="$LAUNCHER_RETRY_LOOP_LAUNCH_COMMAND"
  LAUNCHER_RETRY_LOOP_HINT="after inspecting the smoke failure handoff, relaunch the branch-local retry loop so the next solver iteration starts with fresh same-worktree artifacts"
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

last = rows[-1]
if last.get("run_id") == current_run_id:
    if len(rows) < 2:
        raise SystemExit(0)
    last = rows[-2]
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
print("\t".join(last.get(field, "") for field in fields))
PY
  )"
  if [[ -z "$previous_row" ]]; then
    return 0
  fi
  IFS=$'\t' read -r \
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
  if (( LAUNCHER_RUN_STARTED_SECONDS > 0 )); then
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
    echo "normalized_outcome=${LAUNCHER_STATUS_OUTCOME:-harness_infrastructure_failure}"
    echo "normalized_exit_code=${LAUNCHER_STATUS_NORMALIZED_RC:-$SMOKE_EXIT_HARNESS_FAILURE}"
    echo "raw_exit_code=${LAUNCHER_STATUS_RAW_RC:-${LAUNCHER_STATUS_NORMALIZED_RC:-$SMOKE_EXIT_HARNESS_FAILURE}}"
    echo "outcome_source=${LAUNCHER_STATUS_SOURCE:-launcher}"
    echo "run_history_index_path=$LAUNCHER_RUN_HISTORY_INDEX"
    echo "run_record_path=$LAUNCHER_STATUS_RUN_RECORD"
    echo "run_comparison_path=$LAUNCHER_STATUS_RUN_COMPARISON"
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
    printf 'launcher_pre_dispatch\n'
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
    "${LAUNCHER_SOURCE_MISMATCH_SUMMARY_PATH:-}" \
    "${LAUNCHER_SOURCE_RETRY_LOG_PATH:-}" \
    "${LAUNCHER_SOURCE_HELPER_STDERR:-}" \
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
  append_launcher_status_diagnostic_entry "run_archive_launcher_failure_snapshot" "$LAUNCHER_RUN_FAILURE_ROOT_SNAPSHOT" "copy of the launcher failure bundle preserved under the per-run archive"
}

append_launcher_manifest_command_status() {
  local name="$1"
  local status="missing"
  local detail="-"

  if detail="$(command -v "$name" 2>/dev/null)"; then
    status="ok"
  else
    detail="-"
  fi

  printf 'command\t%s\t%s\t%s\t-\n' "$name" "$status" "$detail" >> "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
}

append_launcher_manifest_compiler_status() {
  local candidate=""
  local resolved=""

  for candidate in clang++ g++ c++; do
    if resolved="$(command -v "$candidate" 2>/dev/null)"; then
      printf 'compiler\t%s\tok\t%s\t-\n' "$candidate" "$resolved" >> "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
      return 0
    fi
  done

  printf 'compiler\t%s\tmissing\t-\t-\n' "clang++|g++|c++" >> "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
}

append_launcher_manifest_path_status() {
  local kind="$1"
  local label="$2"
  local path="$3"
  local status="missing"

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

  printf '%s\t%s\t%s\t%s\t-\n' "$kind" "$label" "$status" "$path" >> "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
}

write_launcher_preflight_manifest() {
  printf 'kind\tlabel\tstatus\tdetail\tartifact\n' > "$LAUNCHER_FAILURE_PREFLIGHT_MANIFEST"
  append_launcher_manifest_command_status bash
  append_launcher_manifest_command_status python3
  append_launcher_manifest_command_status mkdir
  append_launcher_manifest_command_status mktemp
  append_launcher_manifest_command_status dirname
  append_launcher_manifest_command_status chmod
  append_launcher_manifest_command_status cp
  append_launcher_manifest_command_status mv
  append_launcher_manifest_command_status rm
  append_launcher_manifest_command_status rmdir
  append_launcher_manifest_command_status kill
  append_launcher_manifest_command_status tail
  append_launcher_manifest_command_status sleep
  append_launcher_manifest_command_status grep
  append_launcher_manifest_command_status sort
  append_launcher_manifest_compiler_status
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
    "normalized_outcome": summary.get("normalized_outcome"),
    "normalized_exit_code": as_int(summary.get("normalized_exit_code")),
    "raw_exit_code": as_int(summary.get("raw_exit_code")),
    "outcome_summary": summary.get("outcome_summary"),
    "standard_gap_summary": summary.get("standard_gap_summary"),
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
            "rerun_command_path": summary.get("source_failure_rerun_command_path"),
            "expected_output_path": summary.get("source_failure_expected_output_path"),
            "invoked_command_path": summary.get("source_failure_invoked_command_path"),
            "mismatch_summary_path": summary.get("source_failure_mismatch_summary_path"),
            "retry_log_path": summary.get("source_failure_retry_log_path"),
            "runtime_env_path": summary.get("source_failure_runtime_env_path"),
            "preflight_manifest_path": summary.get("source_failure_preflight_manifest_path"),
            "setup_env_path": summary.get("source_failure_setup_env_path"),
            "build_command_path": summary.get("source_failure_build_command_path"),
            "build_stdout_path": summary.get("source_failure_build_stdout_path"),
            "build_stderr_path": summary.get("source_failure_build_stderr_path"),
            "structured_context_path": summary.get("source_failure_structured_context_path"),
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

public_status = summary.get("public_status", "FAIL")
payload = {
    "script": summary.get("script", "./lca_smoke.sh"),
    "public_status": public_status,
    "normalized_outcome": summary.get("normalized_outcome"),
    "outcome_summary": summary.get("outcome_summary"),
    "retry_loop_action": summary.get("retry_loop_action"),
    "preferred_command": summary.get("retry_loop_preferred_command"),
    "launch_command": summary.get("retry_loop_launch_command"),
    "direct_command": summary.get("retry_loop_direct_command"),
    "hint": summary.get("retry_loop_hint"),
    "log_path": summary.get("retry_loop_log_path"),
    "should_resume_retry_loop": summary.get("retry_loop_action") == "resume_progress40_retry_loop",
    "smoke_retry_command": summary.get("triage_retry_command"),
    "smoke_retry_hint": summary.get("triage_retry_hint"),
    "next_gate_command": summary.get("next_gate_command"),
    "solver_seed_file": summary.get("retry_loop_solver_seed_file"),
    "analysis_seed_file": summary.get("retry_loop_analysis_seed_file"),
    "artifacts": {
        "status_summary_path": str(summary_path),
        "status_report_path": summary.get("status_report"),
        "iteration_evidence_path": summary.get("iteration_evidence_path"),
        "diagnostics_manifest_path": summary.get("status_diagnostics_manifest"),
        "standard_gap_json_path": summary.get("published_smoke_standard_gap_json_path"),
        "structured_context_path": summary.get("source_failure_structured_context_path"),
        "control_path": str(output_path),
        "published_control_path": summary.get("published_smoke_retry_loop_control_path"),
    },
}
if public_status == "PASS":
    payload["should_resume_retry_loop"] = False

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


def split_csv(raw: str | None) -> list[str]:
    if raw is None or raw == "":
        return []
    return [part for part in raw.split(",") if part]


history_fields = [
    "run_id",
    "run_started_at_utc",
    "run_finished_at_utc",
    "run_elapsed_seconds",
    "public_status",
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

previous_row: dict[str, str] | None = None
if history_rows:
    if history_rows[-1].get("run_id") == current_row["run_id"]:
        previous_row = history_rows[-2] if len(history_rows) > 1 else None
        history_rows[-1] = current_row
    else:
        previous_row = history_rows[-1]
        history_rows.append(current_row)
else:
    history_rows.append(current_row)

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
        "result_family": current_row["result_family"],
        "normalized_outcome": current_row["normalized_outcome"],
        "normalized_exit_code": as_int(current_row["normalized_exit_code"]),
        "raw_exit_code": as_int(current_row["raw_exit_code"]),
        "stage_label": current_row["stage_label"],
        "outcome_source": current_row["outcome_source"],
        "source_failure_case": current_row["source_failure_case"],
        "source_failure_kind": current_row["source_failure_kind"],
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
    "artifacts": {
        "status_summary_path": str(summary_path),
        "status_report_path": current_row["status_report_path"],
        "iteration_evidence_path": current_row["iteration_evidence_path"],
        "diagnostics_manifest_path": current_row["diagnostics_manifest_path"],
        "run_archive_root": current_row["run_archive_root"],
        "run_console_stderr_path": current_row["run_console_stderr_path"],
        "published_smoke_summary_path": summary.get("published_smoke_summary_path", ""),
        "published_smoke_status_report_path": summary.get("published_smoke_status_report_path", ""),
        "published_smoke_iteration_evidence_path": summary.get("published_smoke_iteration_evidence_path", ""),
        "published_smoke_diagnostics_manifest_path": summary.get("published_smoke_diagnostics_manifest_path", ""),
        "published_smoke_run_record_path": summary.get("published_smoke_run_record_path", ""),
        "published_smoke_run_comparison_path": summary.get("published_smoke_run_comparison_path", ""),
    },
}
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
        "result_family": current_row["result_family"],
        "normalized_outcome": current_row["normalized_outcome"],
        "stage_label": current_row["stage_label"],
        "source_failure_case": current_row["source_failure_case"],
        "run_archive_root": current_row["run_archive_root"],
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
  local outcome="${LAUNCHER_STATUS_OUTCOME:-harness_infrastructure_failure}"
  local public_status="${LAUNCHER_STATUS_PUBLIC_STATUS:-FAIL}"
  local result_family="${LAUNCHER_STATUS_RESULT_FAMILY:-unknown}"
  local normalized_rc="${LAUNCHER_STATUS_NORMALIZED_RC:-$SMOKE_EXIT_HARNESS_FAILURE}"
  local raw_rc="${LAUNCHER_STATUS_RAW_RC:-$normalized_rc}"
  local source_kind="${LAUNCHER_STATUS_SOURCE:-launcher}"
  local message="${LAUNCHER_STATUS_MESSAGE:-launcher status was not initialized}"
  local replay_case_descriptor=""
  local replay_artifact_descriptor=""
  local triage_scope=""
  local triage_stage=""
  local triage_primary_summary=""
  local triage_primary_report=""
  local triage_primary_manifest=""
  local triage_first_artifacts=""
  local triage_retry_command=""
  local triage_retry_hint=""
  local triage_stage_label="completed:completed"

  resolve_launcher_status_root
  ensure_launcher_run_archive_root || return 1
  load_launcher_previous_run_context || return 1
  status_parent="$(dirname "$LAUNCHER_STATUS_ROOT")"
  ensure_launcher_directory "$status_parent" "launcher status parent" || return 1
  if [[ -e "$LAUNCHER_STATUS_ROOT" ]]; then
    remove_path_retry "$LAUNCHER_STATUS_ROOT" || return 1
  fi
  ensure_launcher_directory "$LAUNCHER_STATUS_ROOT" "launcher status root" || return 1
  working_directory="$(pwd -P 2>/dev/null || pwd)"
  replay_case_descriptor="$(launcher_replay_case_descriptor)"
  replay_artifact_descriptor="$(launcher_replay_artifact_descriptor)"
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
    echo "normalized_exit_code=$normalized_rc"
    echo "raw_exit_code=$raw_rc"
    echo "normalized_outcome=$outcome"
    echo "outcome_source=$source_kind"
    echo "outcome_summary=$message"
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
    echo "source_failure_rerun_command_path=$LAUNCHER_REPLAY_RERUN_COMMAND_PATH"
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
    if [[ "$outcome" != "pass" ]]; then
      echo "triage_stage_scope=$triage_scope"
      echo "triage_stage=$triage_stage"
      echo "triage_stage_label=$triage_stage_label"
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
    echo "- Run archive manifest: \`$LAUNCHER_RUN_ARTIFACT_MANIFEST\`"
    echo "- Launcher console transcript: \`$LAUNCHER_RUN_CONSOLE_LOG\`"
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

  write_launcher_run_tracking_artifacts || return 1
  publish_launcher_smoke_summary_bundle || return 1
  write_launcher_status_artifact_manifest
  archive_launcher_run_bundle || return 1
  publish_launcher_smoke_diagnostics_manifest_mirror || return 1
  write_launcher_status_diagnostics_manifest
  publish_launcher_smoke_diagnostics_manifest_mirror || return 1
  cp "$LAUNCHER_STATUS_DIAGNOSTICS_MANIFEST" "$LAUNCHER_RUN_STATUS_DIAGNOSTICS_PATH" || return 1
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

  if [[ -z "$LAUNCHER_STATUS_SUMMARY" ]]; then
    return
  fi
  replay_case_descriptor="$(launcher_replay_case_descriptor)"
  replay_artifact_descriptor="$(launcher_replay_artifact_descriptor)"
  if [[ "${LAUNCHER_STATUS_OUTCOME:-}" != "pass" ]]; then
    triage_scope="$(launcher_triage_stage_scope)"
    triage_stage="$(launcher_triage_stage_name)"
    triage_primary_report="$(launcher_triage_primary_report)"
    triage_first_artifacts="$(launcher_triage_first_artifacts)"
    triage_retry_command="$(launcher_triage_retry_command)"
    triage_retry_hint="$(launcher_triage_retry_hint)"
  fi
  emit_launcher_context_line "[lca_smoke] public status: ${LAUNCHER_STATUS_PUBLIC_STATUS:-FAIL} family=${LAUNCHER_STATUS_RESULT_FAMILY:-unknown}"
  emit_launcher_context_line "[lca_smoke] normalized outcome: $LAUNCHER_STATUS_OUTCOME"
  emit_launcher_context_line "[lca_smoke] normalized exit code: $LAUNCHER_STATUS_NORMALIZED_RC raw_exit_code=$LAUNCHER_STATUS_RAW_RC source=$LAUNCHER_STATUS_SOURCE"
  emit_launcher_context_line "[lca_smoke] outcome summary: $LAUNCHER_STATUS_MESSAGE"
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
  mkdir -p "$LAUNCHER_PREFLIGHT_ROOT" || fail "failed to prepare launcher preflight root for dispatch marker: $LAUNCHER_PREFLIGHT_ROOT"
  rm -f "$LAUNCHER_DISPATCH_MARKER" 2>/dev/null || true
  : > "$LAUNCHER_DISPATCH_MARKER" || fail "failed to record launcher dispatch marker: $LAUNCHER_DISPATCH_MARKER"
}

artifact_is_fresh_since_dispatch() {
  local artifact="$1"

  if [[ -z "${LAUNCHER_DISPATCH_MARKER:-}" || ! -e "$LAUNCHER_DISPATCH_MARKER" ]]; then
    return 1
  fi
  if [[ ! -e "$artifact" ]]; then
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

clear_stale_inner_wrapper_rerun_state() {
  local issues=""
  local stale=""
  local smoke_setup_root="$ARTIFACTS_ROOT/smoke_setup"
  local smoke_session_state_root="$TMP_PARENT/lca_smoke.session"
  local smoke_setup_tmpdir="$TMP_PARENT/lca_smoke.setup.tmp"

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
  if [[ -d "$TMP_PARENT" ]]; then
    shopt -s nullglob
    for stale in "$TMP_PARENT"/lca_smoke_probe.* "$TMP_PARENT"/lca_smoke.run.* "$TMP_PARENT"/lca_smoke.tmp.*; do
      if ! remove_path_retry "$stale"; then
        issues="${issues:+$issues; }failed to clear stale inner tmp path at $stale"
      fi
    done
    shopt -u nullglob
  fi

  if [[ -n "$issues" ]]; then
    printf '%s\n' "inner smoke wrapper published a fresh smoke bundle but left stale rerun state behind: $issues"
    return 1
  fi
}

validate_inner_wrapper_failure_bundle() {
  local failure_class="$1"
  local failure_root="$2"
  local source_summary="$3"
  local source_report="$4"
  local issues=""

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
  if [[ ! -s "$source_report" ]]; then
    issues="${issues:+$issues; }missing failure report at $source_report"
  elif ! artifact_is_fresh_since_dispatch "$source_report"; then
    issues="${issues:+$issues; }stale failure report at $source_report"
  fi

  if [[ -n "$issues" ]]; then
    printf '%s\n' "inner smoke wrapper returned a ${failure_class} result without publishing a complete fresh failure bundle: $issues"
    return 1
  fi
}

unexpected_inner_wrapper_message() {
  local raw_rc="$1"

  if (( raw_rc >= 128 )); then
    printf 'inner smoke wrapper terminated by signal %d before the launcher could validate the smoke bundle\n' "$(( raw_rc - 128 ))"
    return
  fi
  printf 'inner smoke wrapper returned unexpected exit code %s; treat the run as infrastructure-failed\n' "$raw_rc"
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
  local validation_message=""
  local normalized_outcome=""
  local normalized_rc=0

  clear_launcher_source_failure_details
  case "$raw_rc" in
    0)
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
  {
    echo "PWD=$working_directory"
    echo "ORIGINAL_LAUNCH_PWD=$LAUNCHER_ORIGINAL_PWD"
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
    echo "$LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG=${!LCA_SMOKE_LAUNCHER_CLEAN_ENV_FLAG:-0}"
    echo "$LCA_SMOKE_INNER_CLEAN_ENV_FLAG=${!LCA_SMOKE_INNER_CLEAN_ENV_FLAG:-0}"
    echo "launcher_tmpdir=${LAUNCHER_TMPDIR:-}"
    echo "launcher_home=${LAUNCHER_HOME:-}"
    echo "launcher_preflight_root=${LAUNCHER_PREFLIGHT_ROOT:-}"
    echo
    env | LC_ALL=C sort
  } > "$LAUNCHER_FAILURE_ENV_SNAPSHOT"
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
  LAUNCHER_TMPDIR="$TMP_PARENT/lca_smoke.launcher.tmp"
  LAUNCHER_PREFLIGHT_ROOT="$LAUNCHER_TMPDIR/preflight"
  LAUNCHER_HOME="$LAUNCHER_TMPDIR/home"
  LAUNCHER_XDG_CONFIG_HOME="$LAUNCHER_TMPDIR/xdg_config"
  LAUNCHER_XDG_CACHE_HOME="$LAUNCHER_TMPDIR/xdg_cache"
  LAUNCHER_XDG_STATE_HOME="$LAUNCHER_TMPDIR/xdg_state"
  LAUNCHER_PYCACHE_ROOT="$LAUNCHER_TMPDIR/pycache"
  LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION="$LAUNCHER_PREFLIGHT_ROOT/smoke_manifest_selection.txt"
  LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR="$LAUNCHER_PREFLIGHT_ROOT/smoke_manifest_check.stderr.txt"
  LAUNCHER_DISPATCH_MARKER="$LAUNCHER_PREFLIGHT_ROOT/dispatch.started"
  LAUNCHER_DISPATCH_RESULT_PATH="$LAUNCHER_PREFLIGHT_ROOT/dispatch_result.txt"
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
  LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_SELECTION="$LAUNCHER_PREFLIGHT_ROOT/smoke_manifest_selection.txt"
  LAUNCHER_PREFLIGHT_SMOKE_MANIFEST_STDERR="$LAUNCHER_PREFLIGHT_ROOT/smoke_manifest_check.stderr.txt"
  LAUNCHER_DISPATCH_MARKER="$LAUNCHER_PREFLIGHT_ROOT/dispatch.started"
  LAUNCHER_DISPATCH_RESULT_PATH="$LAUNCHER_PREFLIGHT_ROOT/dispatch_result.txt"

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
  set_launcher_last_check \
    "dispatch_timeout" \
    "launcher dispatch timeout" \
    "ok" \
    "$LAUNCHER_DISPATCH_TIMEOUT_S"
}

load_launcher_dispatch_result() {
  local result_path="$1"
  local line=""
  local raw_rc=""
  local timed_out="0"

  LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED=0
  LAUNCHER_DISPATCH_RAW_RC=0

  while IFS= read -r line || [[ -n "$line" ]]; do
    case "$line" in
      raw_exit_code=*)
        raw_rc="${line#*=}"
        ;;
      timed_out=*)
        timed_out="${line#*=}"
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

  LAUNCHER_DISPATCH_RAW_RC="$raw_rc"
  LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED="$timed_out"
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

cleanup_launcher() {
  local rc="${1:-$?}"

  trap - EXIT ERR
  set +e
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
  if [[ -n "${LAUNCHER_TMPDIR:-}" && -e "$LAUNCHER_TMPDIR" ]]; then
    remove_path_retry "$LAUNCHER_TMPDIR" || true
  fi
  if [[ -n "${LAUNCHER_TMPDIR_PARENT:-}" && -e "$LAUNCHER_TMPDIR_PARENT" ]]; then
    remove_path_retry "$LAUNCHER_TMPDIR_PARENT" || true
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
  trap - ERR
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
  rm -f "$LAUNCHER_DISPATCH_RESULT_PATH" 2>/dev/null || true
  if python3 - "$LAUNCHER_DISPATCH_RESULT_PATH" "$LAUNCHER_DISPATCH_TIMEOUT_S" "$LAUNCHER_DISPATCH_KILL_GRACE_S" "$BASH_BIN" "$INNER_WRAPPER" "$@" <<'PY'
from __future__ import annotations

import os
import signal
import subprocess
import sys
from pathlib import Path

result_path = Path(sys.argv[1])
timeout_s = float(sys.argv[2])
kill_grace_s = float(sys.argv[3])
command = sys.argv[4:]

timed_out = False
raw_exit_code = 0
proc = subprocess.Popen(command, start_new_session=True)

try:
    raw_exit_code = proc.wait(timeout=timeout_s)
except subprocess.TimeoutExpired:
    timed_out = True
    try:
        os.killpg(proc.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        raw_exit_code = proc.wait(timeout=kill_grace_s)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        raw_exit_code = proc.wait()

if raw_exit_code < 0:
    raw_exit_code = 128 + (-raw_exit_code)
if timed_out:
    raw_exit_code = 124

result_path.write_text(
    f"raw_exit_code={raw_exit_code}\n"
    f"timed_out={1 if timed_out else 0}\n",
    encoding="utf-8",
)
PY
  then
    :
  else
    manager_rc=$?
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
  require_build_compiler
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
  prepare_launcher_environment
  set_launcher_failure_stage "stale_status_cleanup"
  clear_stale_launcher_status_bundle
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
  set_launcher_failure_stage "stale_failure_cleanup"
  clear_stale_launcher_failure_bundle
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
  if (( LAUNCHER_DISPATCH_TIMEOUT_TRIGGERED == 1 )); then
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
  set_launcher_failure_stage "status_normalization"
  classify_inner_wrapper_exit "$inner_wrapper_rc"
  finalize_launcher_dispatch_result
}

main "$@"
