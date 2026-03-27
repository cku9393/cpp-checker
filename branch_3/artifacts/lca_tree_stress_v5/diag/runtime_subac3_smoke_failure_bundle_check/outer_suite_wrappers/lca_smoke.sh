#!/usr/bin/env bash
set -euo pipefail

LCA_SMOKE_CLEAN_ENV_FLAG="LCA_SMOKE_CLEAN_ENV_READY"
LCA_SMOKE_CLEAN_PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"

if [[ "${!LCA_SMOKE_CLEAN_ENV_FLAG:-0}" != "1" ]]; then
  exec /usr/bin/env -i \
    HOME="${HOME:-}" \
    PATH="$LCA_SMOKE_CLEAN_PATH" \
    TERM="${TERM:-dumb}" \
    "$LCA_SMOKE_CLEAN_ENV_FLAG=1" \
    /usr/bin/env bash "$0" "$@"
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"
cd "$BRANCH_ROOT"
SUITE_ROOT="$(cd "$BRANCH_ROOT/.." && pwd -P)"
BRANCH_ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts"
export PYTHONDONTWRITEBYTECODE=1
SOURCE="$BRANCH_ROOT/boj28350_resume/boj28350_branch_3_solver.cpp"
BINARY="$BRANCH_ROOT/artifacts/boj28350_resume/build/solve"
BUILD_ROOT="${BINARY%/*}"
BUILD_OUTPUT_TMP_GLOB=".${BINARY##*/}.*.tmp"
SOLVER="$BINARY"
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
BUILD_WRAPPER="$BRANCH_ROOT/build.sh"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
RUN_CASE_HELPER="$BRANCH_ROOT/branch_run_case.py"
CHECKER_HELPER="$BRANCH_ROOT/branch_validator.py"
RUN_CASE_RESULT_NAME="run_case_result.json"
SMOKE_CASES="$BRANCH_ROOT/boj28350_resume/smoke_cases.tsv"
ARTIFACTS_ROOT="$BRANCH_ARTIFACTS_ROOT/lca_tree_stress_v5"
TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
RUN_STAGE_ROOT="$TMP_PARENT"
FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
SETUP_ROOT="$ARTIFACTS_ROOT/smoke_setup"
SETUP_TMPDIR="$TMP_PARENT/lca_smoke.setup.tmp"
SETUP_MANIFEST="$SETUP_ROOT/preflight_manifest.tsv"
SETUP_ENV_SNAPSHOT="$SETUP_ROOT/setup_env.txt"
SETUP_BUILD_STDOUT="$SETUP_ROOT/build.stdout.txt"
SETUP_BUILD_STDERR="$SETUP_ROOT/build.stderr.txt"
SETUP_BUILD_COMMAND="$SETUP_ROOT/build.command.txt"
SESSION_STATE_ROOT="$TMP_PARENT/lca_smoke.session"
SESSION_HOME="$SESSION_STATE_ROOT/home"
SESSION_XDG_CONFIG_HOME="$SESSION_STATE_ROOT/xdg_config"
SESSION_XDG_CACHE_HOME="$SESSION_STATE_ROOT/xdg_cache"
SESSION_XDG_STATE_HOME="$SESSION_STATE_ROOT/xdg_state"
SESSION_PYCACHE_ROOT="$SESSION_STATE_ROOT/pycache"
LEGACY_TMP_GLOB="lca_smoke.*"
PROBE_TMP_GLOB="lca_smoke_probe.*"
BUILD_TMP_GLOB="boj28350_branch_3_solver-*.o"
BUILD_TMP_TMP_GLOB="boj28350_branch_3_solver-*.o.tmp"
LEGACY_OUT_GLOB=".lca_smoke_in_progress.*"
RUN_WORK_GLOB="lca_smoke.run.*"
RUN_WORK_TEMPLATE="lca_smoke.run.XXXXXX"
RUN_TMP_GLOB="lca_smoke.tmp.*"
RUN_TMP_TEMPLATE="lca_smoke.tmp.XXXXXX"
LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
LOCKDIR="$LOCK_ROOT/lca_smoke"
LOCK_PID_FILE="$LOCKDIR/pid"
OUTROOT=""
OUTPARENT=""
BACKUP_ROOT=""
WORKDIR=""
RUN_TMPDIR=""
LOCK_HELD=0
SMOKE_EXIT_SOLVER_FAILURE=1
SMOKE_EXIT_USAGE=2
SMOKE_EXIT_SOLVER_TIMEOUT=124
SMOKE_EXIT_SOLVER_RUNTIME_FAILURE=125
SMOKE_EXIT_HARNESS_FAILURE=70
SMOKE_CASE_RETRY_LIMIT=1
SMOKE_RETRY_SLEEP_S="0.05"
SMOKE_ITERATION_POLICY="manifest_row_order"
SMOKE_SEED_POLICY="manifest_seed"
SMOKE_TIMEOUT_POLICY="manifest_fixed"
SMOKE_RETRY_POLICY="harness_transient_only"
SMOKE_PLAN_COUNT=0
SMOKE_PLAN_STAGE=()
SMOKE_PLAN_MODE=()
SMOKE_PLAN_N=()
SMOKE_PLAN_SEED=()
SMOKE_PLAN_SHUFFLE_LABELS=()
SMOKE_PLAN_SHUFFLE_QUERIES=()
SMOKE_PLAN_TIMEOUT=()
SMOKE_PLAN_TAG=()
SMOKE_RETRY_LOG=""
CURRENT_CASE_INDEX=""
CURRENT_CASE_ATTEMPT=""
CURRENT_CASE_STAGE=""
CURRENT_CASE_MODE=""
CURRENT_CASE_N=""
CURRENT_CASE_SEED=""
CURRENT_CASE_SHUFFLE_LABELS=""
CURRENT_CASE_SHUFFLE_QUERIES=""
CURRENT_CASE_TIMEOUT=""
CURRENT_CASE_TAG=""
CURRENT_CASE_MANIFEST_ROW=""
CURRENT_CASE_STDOUT=""
CURRENT_CASE_STDERR=""
CURRENT_CASE_EXEC_COMMAND=""
CURRENT_CASE_REPRO_COMMAND=""
CURRENT_CASE_PRESERVED_INPUT_COMMAND=""
CURRENT_CASE_REPRO_DIR=""
CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR=""
CURRENT_CASE_SOLVER_SNAPSHOT=""
CURRENT_CASE_CHECKER_COMMAND=""
CURRENT_CASE_RESULT_JSON=""
CURRENT_FAILURE_RC=0
CURRENT_FAILURE_HELPER_RC=0
CURRENT_FAILURE_KIND=""
CURRENT_FAILURE_ORIGIN=""
CURRENT_FAILURE_RETRYABLE=0
CURRENT_FAILURE_SUMMARY=""
CURRENT_FAILURE_SOLVER_EXIT=""
CURRENT_FAILURE_SIGNAL=""

sanitize_shell_state() {
  unset CDPATH BASH_ENV ENV GLOBIGNORE
  unalias -a 2>/dev/null || true
  set +f
  shopt -u dotglob extglob failglob nocaseglob nullglob
}

fail() {
  echo "[lca_smoke] $*" >&2
  exit "$SMOKE_EXIT_HARNESS_FAILURE"
}

usage() {
  echo "usage: ./outer_suite_wrappers/lca_smoke.sh" >&2
  echo "[lca_smoke] smoke output is fixed to branch-local artifacts" >&2
  echo "[lca_smoke] exit codes: 0=pass 1=acceptance 124=timeout 125=solver runtime 70=harness 2=usage" >&2
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
  if [[ ! -e "$path" ]]; then
    fail "missing executable ${label}: $path"
  fi
  if [[ ! -f "$path" ]]; then
    fail "executable ${label} is not a regular file: $path"
  fi
  if [[ ! -x "$path" ]]; then
    fail "missing executable ${label}: $path"
  fi
}

parse_nonnegative_integer_setting() {
  local raw="$1"
  local label="$2"
  case "$raw" in
    ''|*[!0-9]*)
      fail "$label must be a non-negative integer (got: $raw)"
      ;;
  esac
  printf '%s\n' "$raw"
}

parse_nonnegative_decimal_setting() {
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
    print(f"[lca_smoke] {label} must be a non-negative decimal (got: {raw})", file=sys.stderr)
    raise SystemExit(1)

if value < 0.0:
    print(f"[lca_smoke] {label} must be >= 0 (got: {raw})", file=sys.stderr)
    raise SystemExit(1)

print(raw)
PY
}

validate_positive_timeout_setting() {
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
    print(f"[lca_smoke] {label} must be a positive decimal (got: {raw})", file=sys.stderr)
    raise SystemExit(1)

if value <= 0.0:
    print(f"[lca_smoke] {label} must be > 0 (got: {raw})", file=sys.stderr)
    raise SystemExit(1)
PY
}

normalize_manifest_field() {
  local raw="${1:-}"
  raw="${raw%$'\r'}"
  printf '%s\n' "$raw"
}

sanitize_case_tag_token() {
  local raw=""
  raw="$(normalize_manifest_field "$1")"
  raw="${raw//./p}"
  raw="${raw//[^A-Za-z0-9._-]/_}"
  while [[ "$raw" == *__* ]]; do
    raw="${raw//__/_}"
  done
  raw="${raw##_}"
  raw="${raw%%_}"
  if [[ -z "$raw" ]]; then
    raw="x"
  fi
  printf '%s\n' "$raw"
}

build_case_tag() {
  local case_index="$1"
  local stage="$2"
  local mode="$3"
  local n="$4"
  local seed="$5"
  local shuffle_labels="$6"
  local shuffle_queries="$7"
  local timeout_s="$8"
  printf \
    'case%02d_%s_%s_n%s_s%s_L%s_Q%s_t%s\n' \
    "$case_index" \
    "$(sanitize_case_tag_token "$stage")" \
    "$(sanitize_case_tag_token "$mode")" \
    "$(sanitize_case_tag_token "$n")" \
    "$(sanitize_case_tag_token "$seed")" \
    "$(sanitize_case_tag_token "$shuffle_labels")" \
    "$(sanitize_case_tag_token "$shuffle_queries")" \
    "$(sanitize_case_tag_token "$timeout_s")"
}

configure_deterministic_smoke_controls() {
  SMOKE_CASE_RETRY_LIMIT="$(
    parse_nonnegative_integer_setting \
      "${LCA_SMOKE_CASE_RETRY_LIMIT:-$SMOKE_CASE_RETRY_LIMIT}" \
      "LCA_SMOKE_CASE_RETRY_LIMIT"
  )"
  SMOKE_RETRY_SLEEP_S="$(
    parse_nonnegative_decimal_setting \
      "${LCA_SMOKE_RETRY_SLEEP_S:-$SMOKE_RETRY_SLEEP_S}" \
      "LCA_SMOKE_RETRY_SLEEP_S"
  )"
}

reset_smoke_plan() {
  SMOKE_PLAN_COUNT=0
  SMOKE_PLAN_STAGE=()
  SMOKE_PLAN_MODE=()
  SMOKE_PLAN_N=()
  SMOKE_PLAN_SEED=()
  SMOKE_PLAN_SHUFFLE_LABELS=()
  SMOKE_PLAN_SHUFFLE_QUERIES=()
  SMOKE_PLAN_TIMEOUT=()
  SMOKE_PLAN_TAG=()
}

validate_smoke_manifest_row() {
  local line_no="$1"
  local stage="$2"
  local mode="$3"
  local n="$4"
  local seed="$5"
  local shuffle_labels="$6"
  local shuffle_queries="$7"
  local timeout_s="$8"

  if [[ -z "$stage" || -z "$mode" ]]; then
    fail "smoke manifest row $line_no must provide non-empty stage/mode columns"
  fi
  case "$n" in
    ''|*[!0-9]*)
      fail "smoke manifest row $line_no has invalid n: $n"
      ;;
  esac
  if (( n <= 0 )); then
    fail "smoke manifest row $line_no must use n > 0 (got: $n)"
  fi
  case "$seed" in
    ''|*[!0-9]*)
      fail "smoke manifest row $line_no has invalid seed: $seed"
      ;;
  esac
  case "$shuffle_labels" in
    0|1)
      ;;
    *)
      fail "smoke manifest row $line_no has invalid shuffle_labels flag: $shuffle_labels"
      ;;
  esac
  case "$shuffle_queries" in
    0|1)
      ;;
    *)
      fail "smoke manifest row $line_no has invalid shuffle_queries flag: $shuffle_queries"
      ;;
  esac
  validate_positive_timeout_setting "$timeout_s" "smoke manifest row $line_no timeout_s"
}

load_smoke_plan() {
  local raw_stage=""
  local raw_mode=""
  local raw_n=""
  local raw_seed=""
  local raw_shuffle_labels=""
  local raw_shuffle_queries=""
  local raw_timeout_s=""
  local stage=""
  local mode=""
  local n=""
  local seed=""
  local shuffle_labels=""
  local shuffle_queries=""
  local timeout_s=""
  local case_index=0
  local case_tag=""
  local line_no=0
  local existing_tag=""
  local existing_index=0

  reset_smoke_plan
  while IFS=$'\t' read -r raw_stage raw_mode raw_n raw_seed raw_shuffle_labels raw_shuffle_queries raw_timeout_s || \
    [[ -n "$raw_stage$raw_mode$raw_n$raw_seed$raw_shuffle_labels$raw_shuffle_queries$raw_timeout_s" ]]; do
    line_no=$(( line_no + 1 ))
    stage="$(normalize_manifest_field "$raw_stage")"
    mode="$(normalize_manifest_field "$raw_mode")"
    n="$(normalize_manifest_field "$raw_n")"
    seed="$(normalize_manifest_field "$raw_seed")"
    shuffle_labels="$(normalize_manifest_field "$raw_shuffle_labels")"
    shuffle_queries="$(normalize_manifest_field "$raw_shuffle_queries")"
    timeout_s="$(normalize_manifest_field "$raw_timeout_s")"

    if [[ -z "$stage$mode$n$seed$shuffle_labels$shuffle_queries$timeout_s" ]]; then
      continue
    fi
    if (( line_no == 1 )) && [[ "$stage" == "stage" && "$mode" == "mode" ]]; then
      continue
    fi

    validate_smoke_manifest_row "$line_no" "$stage" "$mode" "$n" "$seed" "$shuffle_labels" "$shuffle_queries" "$timeout_s"
    case_index=$(( SMOKE_PLAN_COUNT + 1 ))
    case_tag="$(build_case_tag "$case_index" "$stage" "$mode" "$n" "$seed" "$shuffle_labels" "$shuffle_queries" "$timeout_s")"
    for (( existing_index = 0; existing_index < SMOKE_PLAN_COUNT; ++existing_index )); do
      existing_tag="${SMOKE_PLAN_TAG[$existing_index]}"
      if [[ "$existing_tag" == "$case_tag" ]]; then
        fail "smoke manifest row $line_no collided with an existing deterministic case tag: $case_tag"
      fi
    done

    SMOKE_PLAN_STAGE+=("$stage")
    SMOKE_PLAN_MODE+=("$mode")
    SMOKE_PLAN_N+=("$n")
    SMOKE_PLAN_SEED+=("$seed")
    SMOKE_PLAN_SHUFFLE_LABELS+=("$shuffle_labels")
    SMOKE_PLAN_SHUFFLE_QUERIES+=("$shuffle_queries")
    SMOKE_PLAN_TIMEOUT+=("$timeout_s")
    SMOKE_PLAN_TAG+=("$case_tag")
    SMOKE_PLAN_COUNT=$case_index
  done < "$SMOKE_CASES"

  if (( SMOKE_PLAN_COUNT == 0 )); then
    fail "smoke case manifest produced zero executable rows: $SMOKE_CASES"
  fi
}

write_smoke_suite_metadata() {
  local index=0
  {
    echo "manifest_path=$SMOKE_CASES"
    echo "case_count=$SMOKE_PLAN_COUNT"
    echo "iteration_policy=$SMOKE_ITERATION_POLICY"
    echo "seed_policy=$SMOKE_SEED_POLICY"
    echo "timeout_policy=$SMOKE_TIMEOUT_POLICY"
    echo "retry_policy=$SMOKE_RETRY_POLICY"
    echo "case_retry_limit=$SMOKE_CASE_RETRY_LIMIT"
    echo "retry_sleep_s=$SMOKE_RETRY_SLEEP_S"
  } > "$WORKDIR/suite_config.txt"

  {
    printf 'case_index\tcase_tag\tstage\tmode\tn\tseed\tshuffle_labels\tshuffle_queries\ttimeout_s\n'
    for (( index = 0; index < SMOKE_PLAN_COUNT; ++index )); do
      printf \
        '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
        "$(( index + 1 ))" \
        "${SMOKE_PLAN_TAG[$index]}" \
        "${SMOKE_PLAN_STAGE[$index]}" \
        "${SMOKE_PLAN_MODE[$index]}" \
        "${SMOKE_PLAN_N[$index]}" \
        "${SMOKE_PLAN_SEED[$index]}" \
        "${SMOKE_PLAN_SHUFFLE_LABELS[$index]}" \
        "${SMOKE_PLAN_SHUFFLE_QUERIES[$index]}" \
        "${SMOKE_PLAN_TIMEOUT[$index]}"
    done
  } > "$WORKDIR/suite_plan.tsv"
}

write_environment_validation_bundle() {
  local report_path="$WORKDIR/environment_validation.txt"
  local validation_dir="$WORKDIR/environment_validation"

  require_file "$SETUP_MANIFEST" "setup preflight manifest"
  require_file "$SETUP_ENV_SNAPSHOT" "setup environment snapshot"
  require_file "$SETUP_BUILD_COMMAND" "setup build command"

  mkdir -p "$validation_dir"
  cp "$SETUP_MANIFEST" "$validation_dir/preflight_manifest.tsv"
  cp "$SETUP_ENV_SNAPSHOT" "$validation_dir/setup_env.txt"
  cp "$SETUP_BUILD_COMMAND" "$validation_dir/build.command.txt"

  {
    echo "reset_strategy=clean_env_exec_and_branch_local_session_roots"
    echo "clean_env_flag=${!LCA_SMOKE_CLEAN_ENV_FLAG:-0}"
    echo "shell_state_sanitized=1"
    echo "cwd=$PWD"
    echo "branch_root=$BRANCH_ROOT"
    echo "suite_root=$SUITE_ROOT"
    echo "artifacts_root=$ARTIFACTS_ROOT"
    echo "smoke_output_root=$OUTROOT"
    echo "failure_root=$FAILURE_ROOT"
    echo "setup_root=$SETUP_ROOT"
    echo "setup_tmpdir=$SETUP_TMPDIR"
    echo "tmp_parent=$TMP_PARENT"
    echo "runtime_stage_root=$RUN_STAGE_ROOT"
    echo "runtime_tmpdir_policy=mktemp_under:${RUN_STAGE_ROOT}/${RUN_TMP_TEMPLATE}"
    echo "session_state_root=$SESSION_STATE_ROOT"
    echo "home=$HOME"
    echo "xdg_config_home=$XDG_CONFIG_HOME"
    echo "xdg_cache_home=$XDG_CACHE_HOME"
    echo "xdg_state_home=$XDG_STATE_HOME"
    echo "python_pycachedir=$PYTHONPYCACHEPREFIX"
    echo "path=$PATH"
    echo "lc_all=$LC_ALL"
    echo "lang=$LANG"
    echo "tz=$TZ"
    echo "pythondontwritebytecode=${PYTHONDONTWRITEBYTECODE:-}"
    echo "pythonhashseed=$PYTHONHASHSEED"
    echo "pythonnousersite=$PYTHONNOUSERSITE"
    echo "pythonutf8=$PYTHONUTF8"
    echo "cleanup_targets=$TMP_PARENT;$SETUP_ROOT;$SESSION_STATE_ROOT;$FAILURE_ROOT;$BACKUP_ROOT;$BUILD_ROOT"
    echo "cleanup_globs=$LEGACY_TMP_GLOB;$PROBE_TMP_GLOB;$RUN_WORK_GLOB;$RUN_TMP_GLOB;$BUILD_OUTPUT_TMP_GLOB;$BUILD_TMP_GLOB;$BUILD_TMP_TMP_GLOB;$LEGACY_OUT_GLOB"
    echo "setup_manifest_snapshot=environment_validation/preflight_manifest.tsv"
    echo "setup_env_snapshot=environment_validation/setup_env.txt"
    echo "build_command_snapshot=environment_validation/build.command.txt"
  } > "$report_path"
}

prepare_retry_log() {
  SMOKE_RETRY_LOG="$RUN_TMPDIR/lca_smoke_retry_log.tsv"
  printf 'case_index\tcase_tag\tattempt\texit_code\thelpper_exit_code\tfailure_kind\tfailure_origin\tretryable\ttimeout_s\tsummary\n' > "$SMOKE_RETRY_LOG"
}

record_retry_attempt() {
  if [[ -z "$SMOKE_RETRY_LOG" ]]; then
    return
  fi
  printf \
    '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$CURRENT_CASE_INDEX" \
    "$CURRENT_CASE_TAG" \
    "$CURRENT_CASE_ATTEMPT" \
    "$CURRENT_FAILURE_RC" \
    "$CURRENT_FAILURE_HELPER_RC" \
    "$CURRENT_FAILURE_KIND" \
    "$CURRENT_FAILURE_ORIGIN" \
    "$CURRENT_FAILURE_RETRYABLE" \
    "$CURRENT_CASE_TIMEOUT" \
    "$CURRENT_FAILURE_SUMMARY" \
    >> "$SMOKE_RETRY_LOG"
}

copy_retry_log_to_failure_root() {
  if [[ -n "$SMOKE_RETRY_LOG" && -f "$SMOKE_RETRY_LOG" ]]; then
    cp "$SMOKE_RETRY_LOG" "$FAILURE_ROOT/retry_log.tsv"
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

ensure_under_branch_artifacts() {
  local path="$1"
  case "$path" in
    "$BRANCH_ARTIFACTS_ROOT"|"$BRANCH_ARTIFACTS_ROOT"/*)
      ;;
    *)
      fail "path escaped branch-local artifacts root: $path"
      ;;
  esac
}

enter_function_errexit() {
  local __state_var="$1"
  if [[ $- == *e* ]]; then
    printf -v "$__state_var" '1'
    return
  fi
  printf -v "$__state_var" '0'
  set -e
}

restore_function_errexit() {
  local prior_state="${1:-1}"
  if [[ "$prior_state" == "0" ]]; then
    set +e
  fi
}

clear_stale_build_output_temps() {
  local stale=""
  ensure_under_branch_artifacts "$BUILD_ROOT"
  if [[ -d "$BUILD_ROOT" ]]; then
    shopt -s nullglob
    for stale in "$BUILD_ROOT"/$BUILD_OUTPUT_TMP_GLOB; do
      remove_path_retry "$stale" || return 1
    done
    shopt -u nullglob
  fi
}

resolve_output_roots() {
  OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_smoke)"
  if [[ -z "$OUTROOT" ]]; then
    fail "artifact resolver returned an empty smoke output path"
  fi
  OUTPARENT="$(dirname "$OUTROOT")"
  BACKUP_ROOT="${OUTROOT}.previous"

  ensure_under_artifacts "$OUTROOT"
  ensure_under_artifacts "$OUTPARENT"
  ensure_under_artifacts "$BACKUP_ROOT"
  ensure_under_branch_artifacts "$BUILD_ROOT"
  ensure_under_artifacts "$FAILURE_ROOT"
  ensure_under_artifacts "$SETUP_ROOT"
  ensure_under_artifacts "$SETUP_TMPDIR"
  ensure_under_artifacts "$SESSION_STATE_ROOT"
  ensure_under_artifacts "$SESSION_HOME"
  ensure_under_artifacts "$SESSION_XDG_CONFIG_HOME"
  ensure_under_artifacts "$SESSION_XDG_CACHE_HOME"
  ensure_under_artifacts "$SESSION_XDG_STATE_HOME"
  ensure_under_artifacts "$SESSION_PYCACHE_ROOT"
}

configure_runtime_environment() {
  umask 022
  export LC_ALL=C
  export LANG=C
  export PATH="$LCA_SMOKE_CLEAN_PATH"
  export HOME="$SESSION_HOME"
  export XDG_CONFIG_HOME="$SESSION_XDG_CONFIG_HOME"
  export XDG_CACHE_HOME="$SESSION_XDG_CACHE_HOME"
  export XDG_STATE_HOME="$SESSION_XDG_STATE_HOME"
  export PYTHONIOENCODING=UTF-8
  export PYTHONUTF8=1
  export PYTHONNOUSERSITE=1
  export PYTHONPYCACHEPREFIX="$SESSION_PYCACHE_ROOT"
  export PYTHONHASHSEED=0
  export TZ=UTC
  export BRANCH_ROOT
  export SUITE_ROOT
  export LCA_SMOKE_ARTIFACT_ROOT="$ARTIFACTS_ROOT"
  export LCA_SMOKE_OUTROOT="$OUTROOT"
  export LCA_SMOKE_STAGE_ROOT="$RUN_STAGE_ROOT"
  export LCA_SMOKE_SETUP_ROOT="$SETUP_ROOT"

  ensure_under_artifacts "$LCA_SMOKE_ARTIFACT_ROOT"
  ensure_under_artifacts "$LCA_SMOKE_OUTROOT"
  ensure_under_artifacts "$LCA_SMOKE_STAGE_ROOT"
  ensure_under_artifacts "$LCA_SMOKE_SETUP_ROOT"
  ensure_under_artifacts "$HOME"
  ensure_under_artifacts "$XDG_CONFIG_HOME"
  ensure_under_artifacts "$XDG_CACHE_HOME"
  ensure_under_artifacts "$XDG_STATE_HOME"
  ensure_under_artifacts "$PYTHONPYCACHEPREFIX"
}

prepare_session_environment_state() {
  local state_path=""
  if [[ -e "$SESSION_STATE_ROOT" ]]; then
    remove_path_retry "$SESSION_STATE_ROOT" || fail "failed to clear stale smoke session state: $SESSION_STATE_ROOT"
  fi
  for state_path in \
    "$SESSION_HOME" \
    "$SESSION_XDG_CONFIG_HOME" \
    "$SESSION_XDG_CACHE_HOME" \
    "$SESSION_XDG_STATE_HOME" \
    "$SESSION_PYCACHE_ROOT"; do
    mkdir -p "$state_path"
    ensure_under_artifacts "$state_path"
  done
}

assert_session_environment() {
  local env_name=""
  local expected=""

  for env_name in HOME XDG_CONFIG_HOME XDG_CACHE_HOME XDG_STATE_HOME PYTHONPYCACHEPREFIX; do
    if [[ -z "${!env_name:-}" ]]; then
      fail "required deterministic environment variable is unset: $env_name"
    fi
  done

  for expected in \
    "$HOME" \
    "$XDG_CONFIG_HOME" \
    "$XDG_CACHE_HOME" \
    "$XDG_STATE_HOME" \
    "$PYTHONPYCACHEPREFIX"; do
    ensure_under_artifacts "$expected"
  done

  if [[ "$HOME" != "$SESSION_HOME" ]]; then
    fail "HOME drifted from smoke session home"
  fi
  if [[ "$XDG_CONFIG_HOME" != "$SESSION_XDG_CONFIG_HOME" ]]; then
    fail "XDG_CONFIG_HOME drifted from smoke session config root"
  fi
  if [[ "$XDG_CACHE_HOME" != "$SESSION_XDG_CACHE_HOME" ]]; then
    fail "XDG_CACHE_HOME drifted from smoke session cache root"
  fi
  if [[ "$XDG_STATE_HOME" != "$SESSION_XDG_STATE_HOME" ]]; then
    fail "XDG_STATE_HOME drifted from smoke session state root"
  fi
  if [[ "$PYTHONPYCACHEPREFIX" != "$SESSION_PYCACHE_ROOT" ]]; then
    fail "PYTHONPYCACHEPREFIX drifted from smoke session pycache root"
  fi
}

configure_setup_tmpdir() {
  if [[ -e "$SETUP_TMPDIR" ]]; then
    remove_path_retry "$SETUP_TMPDIR" || fail "failed to clear stale setup tmpdir: $SETUP_TMPDIR"
  fi
  mkdir -p "$SETUP_TMPDIR"
  export BRANCH_ARTIFACT_TMP_ROOT="$SETUP_TMPDIR"
  export TMPDIR="$SETUP_TMPDIR"
  export TMP="$SETUP_TMPDIR"
  export TEMP="$SETUP_TMPDIR"
}

configure_runtime_tmpdir() {
  RUN_TMPDIR="$(mktemp -d "$RUN_STAGE_ROOT/$RUN_TMP_TEMPLATE")"
  if [[ -z "$RUN_TMPDIR" ]]; then
    fail "mktemp returned an empty smoke runtime tmpdir"
  fi
  ensure_under_artifacts "$RUN_TMPDIR"
  export BRANCH_ARTIFACT_TMP_ROOT="$RUN_TMPDIR"
  export TMPDIR="$RUN_TMPDIR"
  export TMP="$RUN_TMPDIR"
  export TEMP="$RUN_TMPDIR"
}

assert_branch_tmpdir_environment() {
  local expected_tmpdir="$1"
  local phase_label="$2"
  local env_name=""
  if [[ "$PWD" != "$BRANCH_ROOT" ]]; then
    fail "runtime cwd drifted outside branch root: $PWD"
  fi
  if [[ -z "$expected_tmpdir" || ! -d "$expected_tmpdir" ]]; then
    fail "$phase_label tmpdir is missing: ${expected_tmpdir:-<unset>}"
  fi
  ensure_under_artifacts "$expected_tmpdir"
  for env_name in BRANCH_ARTIFACT_TMP_ROOT TMPDIR TMP TEMP; do
    if [[ -z "${!env_name:-}" ]]; then
      fail "required $phase_label variable is unset: $env_name"
    fi
    if [[ "${!env_name}" != "$expected_tmpdir" ]]; then
      fail "$phase_label variable $env_name must resolve to $expected_tmpdir (got: ${!env_name})"
    fi
    ensure_under_artifacts "${!env_name}"
  done
}

assert_setup_environment() {
  assert_branch_tmpdir_environment "$SETUP_TMPDIR" "setup/build"
  assert_session_environment
  if [[ "${LCA_SMOKE_SETUP_ROOT:-}" != "$SETUP_ROOT" ]]; then
    fail "LCA_SMOKE_SETUP_ROOT drifted from setup root"
  fi
}

assert_runtime_environment() {
  assert_branch_tmpdir_environment "$RUN_TMPDIR" "runtime"
  assert_session_environment
  if [[ "${LCA_SMOKE_OUTROOT:-}" != "$OUTROOT" ]]; then
    fail "LCA_SMOKE_OUTROOT drifted from resolved output path"
  fi
  if [[ "${LCA_SMOKE_STAGE_ROOT:-}" != "$RUN_STAGE_ROOT" ]]; then
    fail "LCA_SMOKE_STAGE_ROOT drifted from staging root"
  fi
}

restore_previous_output() {
  if [[ -n "$BACKUP_ROOT" && -e "$BACKUP_ROOT" && ! -e "$OUTROOT" ]]; then
    mv "$BACKUP_ROOT" "$OUTROOT"
  fi
}

clear_stale_state() {
  local stale
  restore_previous_output
  clear_stale_build_output_temps || fail "failed to clear stale smoke build temp outputs under $BUILD_ROOT"
  if [[ -d "$TMP_PARENT" ]]; then
    shopt -s nullglob
    for stale in "$TMP_PARENT"/$LEGACY_TMP_GLOB "$TMP_PARENT"/$BUILD_TMP_GLOB "$TMP_PARENT"/$BUILD_TMP_TMP_GLOB; do
      remove_path_retry "$stale" || fail "failed to clear stale temp path: $stale"
    done
    for stale in "$TMP_PARENT"/$PROBE_TMP_GLOB; do
      remove_path_retry "$stale" || fail "failed to clear stale smoke probe path: $stale"
    done
    for stale in "$TMP_PARENT"/$RUN_WORK_GLOB "$TMP_PARENT"/$RUN_TMP_GLOB; do
      remove_path_retry "$stale" || fail "failed to clear stale smoke runtime path: $stale"
    done
    shopt -u nullglob
  fi
  if [[ -e "$SESSION_STATE_ROOT" ]]; then
    remove_path_retry "$SESSION_STATE_ROOT" || fail "failed to clear stale smoke session state root: $SESSION_STATE_ROOT"
  fi
  if [[ -d "$OUTPARENT" ]]; then
    shopt -s nullglob
    for stale in "$OUTPARENT"/$LEGACY_OUT_GLOB; do
      remove_path_retry "$stale" || fail "failed to clear stale legacy output path: $stale"
    done
    shopt -u nullglob
  fi
  if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
    remove_path_retry "$BACKUP_ROOT" || fail "failed to clear stale backup path: $BACKUP_ROOT"
  fi
}

clear_stale_failure_state() {
  if [[ -e "$FAILURE_ROOT" ]]; then
    remove_path_retry "$FAILURE_ROOT" || fail "failed to clear stale failure root: $FAILURE_ROOT"
  fi
}

prepare_setup_root() {
  if [[ -e "$SETUP_ROOT" ]]; then
    remove_path_retry "$SETUP_ROOT" || fail "failed to clear stale setup root: $SETUP_ROOT"
  fi
  mkdir -p "$SETUP_ROOT"
  printf 'kind\tname\tstatus\n' > "$SETUP_MANIFEST"
}

release_lock() {
  local rc=0
  if (( LOCK_HELD )) && [[ -d "$LOCKDIR" ]]; then
    if ! remove_path_retry "$LOCKDIR"; then
      echo "[lca_smoke] warning: failed to release smoke lock: $LOCKDIR" >&2
      rc=1
    fi
  fi
  LOCK_HELD=0
  rmdir "$LOCK_ROOT" 2>/dev/null || true
  return "$rc"
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
        remove_path_retry "$LOCKDIR" || fail "failed to clear stale smoke lock: $LOCKDIR"
      fi
      continue
    fi

    read -r holder < "$LOCK_PID_FILE" || holder=""
    if [[ -z "$holder" ]]; then
      sleep 0.05
      if [[ -f "$LOCK_PID_FILE" ]]; then
        continue
      fi
      remove_path_retry "$LOCKDIR" || fail "failed to clear empty smoke lock: $LOCKDIR"
      continue
    fi

    if kill -0 "$holder" 2>/dev/null; then
      fail "another lca_smoke.sh run is active (pid $holder)"
    fi

    remove_path_retry "$LOCKDIR" || fail "failed to clear stale smoke lock: $LOCKDIR"
  done
}

cleanup() {
  local rc="${1:-$?}"
  local stale=""
  trap - EXIT
  set +e
  if (( rc == 0 )); then
    if [[ -e "$SETUP_ROOT" ]]; then
      remove_path_retry "$SETUP_ROOT" || true
    fi
    if [[ -e "$SESSION_STATE_ROOT" ]]; then
      remove_path_retry "$SESSION_STATE_ROOT" || true
    fi
  fi
  if [[ -n "${WORKDIR:-}" && -e "$WORKDIR" ]]; then
    remove_path_retry "$WORKDIR" || true
  fi
  if [[ -n "${RUN_TMPDIR:-}" && -e "$RUN_TMPDIR" ]]; then
    remove_path_retry "$RUN_TMPDIR" || true
  fi
  if [[ -n "${SETUP_TMPDIR:-}" && -e "$SETUP_TMPDIR" ]]; then
    remove_path_retry "$SETUP_TMPDIR" || true
  fi
  clear_stale_build_output_temps || true
  if [[ -d "$TMP_PARENT" ]]; then
    shopt -s nullglob
    for stale in "$TMP_PARENT"/$LEGACY_TMP_GLOB "$TMP_PARENT"/$PROBE_TMP_GLOB "$TMP_PARENT"/$RUN_WORK_GLOB "$TMP_PARENT"/$RUN_TMP_GLOB "$TMP_PARENT"/$BUILD_TMP_GLOB "$TMP_PARENT"/$BUILD_TMP_TMP_GLOB; do
      remove_path_retry "$stale" || true
    done
    shopt -u nullglob
  fi
  if (( rc != 0 )); then
    restore_previous_output
  fi
  if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
    remove_path_retry "$BACKUP_ROOT" || true
  fi
  release_lock || true
  rmdir "$TMP_PARENT" 2>/dev/null || true
  exit "$rc"
}

quote_command() {
  local quoted=""
  local word=""
  for word in "$@"; do
    printf -v quoted '%s%q ' "$quoted" "$word"
  done
  printf '%s\n' "${quoted% }"
}

quote_word() {
  local quoted=""
  printf -v quoted '%q' "$1"
  printf '%s\n' "$quoted"
}

record_setup_check() {
  printf '%s\t%s\t%s\n' "$1" "$2" "$3" >> "$SETUP_MANIFEST"
}

record_setup_environment_snapshot() {
  {
    echo "pwd=$PWD"
    echo "branch_root=$BRANCH_ROOT"
    echo "suite_root=$SUITE_ROOT"
    echo "artifacts_root=$ARTIFACTS_ROOT"
    echo "output_root=$OUTROOT"
    echo "failure_root=$FAILURE_ROOT"
    echo "setup_root=$SETUP_ROOT"
    echo "setup_tmpdir=$SETUP_TMPDIR"
    echo "build_root=$BUILD_ROOT"
    echo "session_state_root=$SESSION_STATE_ROOT"
    echo "home=$HOME"
    echo "xdg_config_home=$XDG_CONFIG_HOME"
    echo "xdg_cache_home=$XDG_CACHE_HOME"
    echo "xdg_state_home=$XDG_STATE_HOME"
    echo "python_pycachedir=$PYTHONPYCACHEPREFIX"
    echo "solver_source=$SOURCE"
    echo "build_wrapper=$BUILD_WRAPPER"
    echo "build_output=$BINARY"
    echo "path=$PATH"
  } > "$SETUP_ENV_SNAPSHOT"
}

check_required_command_recorded() {
  local name="$1"
  local resolved=""
  if resolved="$(command -v "$name" 2>/dev/null)"; then
    record_setup_check "command" "$name" "$resolved"
    return 0
  fi
  record_setup_check "missing_command" "$name" "-"
  return 1
}

check_required_file_recorded() {
  local path="$1"
  local label="$2"
  if [[ -f "$path" ]]; then
    record_setup_check "file" "$label" "$path"
    return 0
  fi
  record_setup_check "missing_file" "$label" "$path"
  return 1
}

check_required_executable_recorded() {
  local path="$1"
  local label="$2"
  if [[ -f "$path" && -x "$path" ]]; then
    record_setup_check "executable" "$label" "$path"
    return 0
  fi
  record_setup_check "missing_executable" "$label" "$path"
  return 1
}

check_python_entrypoint_recorded() {
  local path="$1"
  local label="$2"
  if python3 - "$path" <<'PY' >/dev/null 2>&1
from __future__ import annotations

import runpy
import sys

runpy.run_path(sys.argv[1], run_name="__lca_smoke_preflight__")
PY
  then
    record_setup_check "python_entrypoint" "$label" "$path"
    return 0
  fi
  record_setup_check "broken_python_entrypoint" "$label" "$path"
  return 1
}

check_build_compiler_recorded() {
  local candidate=""
  local resolved=""
  for candidate in g++ clang++ c++; do
    if resolved="$(command -v "$candidate" 2>/dev/null)"; then
      record_setup_check "compiler" "$candidate" "$resolved"
      return 0
    fi
  done
  record_setup_check "missing_compiler" "g++|clang++|c++" "-"
  return 1
}

write_setup_failure_summary() {
  local phase="$1"
  local exit_code="$2"
  local message="$3"
  local setup_bundle="$FAILURE_ROOT/setup_build"
  local failure_summary="$FAILURE_ROOT/failure_summary.txt"
  local failure_report="$FAILURE_ROOT/latest_failure_report.md"

  remove_path_retry "$FAILURE_ROOT" || fail "failed to clear previous setup failure root: $FAILURE_ROOT"
  mkdir -p "$setup_bundle"

  if [[ -f "$SETUP_MANIFEST" ]]; then
    cp "$SETUP_MANIFEST" "$setup_bundle/preflight_manifest.tsv"
  fi
  if [[ -f "$SETUP_ENV_SNAPSHOT" ]]; then
    cp "$SETUP_ENV_SNAPSHOT" "$setup_bundle/setup_env.txt"
  fi
  if [[ -f "$SETUP_BUILD_COMMAND" ]]; then
    cp "$SETUP_BUILD_COMMAND" "$setup_bundle/build.command.txt"
  fi
  if [[ -f "$SETUP_BUILD_STDOUT" ]]; then
    cp "$SETUP_BUILD_STDOUT" "$setup_bundle/build.stdout.txt"
  fi
  if [[ -f "$SETUP_BUILD_STDERR" ]]; then
    cp "$SETUP_BUILD_STDERR" "$setup_bundle/build.stderr.txt"
  fi

  {
    echo "script=./outer_suite_wrappers/lca_smoke.sh"
    echo "failure_stage=$phase"
    echo "exit_code=$exit_code"
    echo "message=$message"
    echo "setup_root=$SETUP_ROOT"
    echo "setup_tmpdir=$SETUP_TMPDIR"
    echo "output_root=$OUTROOT"
    echo "failure_root=$FAILURE_ROOT"
    echo "build_wrapper=$BUILD_WRAPPER"
    echo "build_output=$BINARY"
    echo "preflight_manifest=$setup_bundle/preflight_manifest.tsv"
    echo "setup_env=$setup_bundle/setup_env.txt"
    echo "build_command=$setup_bundle/build.command.txt"
    echo "build_stdout=$setup_bundle/build.stdout.txt"
    echo "build_stderr=$setup_bundle/build.stderr.txt"
  } > "$failure_summary"

  {
    echo "# lca_smoke Setup/Build Failure Report"
    echo
    echo "- Stage: \`$phase\`"
    echo "- Exit code: \`$exit_code\`"
    echo "- Message: \`$message\`"
    echo "- Setup root: \`$SETUP_ROOT\`"
    echo "- Setup tmpdir: \`$SETUP_TMPDIR\`"
    echo "- Smoke output root: \`$OUTROOT\`"
    echo "- Failure root: \`$FAILURE_ROOT\`"
    echo "- Build wrapper: \`$BUILD_WRAPPER\`"
    echo "- Build output: \`$BINARY\`"
    echo
    echo "## Recorded Artifacts"
    echo
    echo "- Preflight manifest: \`$setup_bundle/preflight_manifest.tsv\`"
    echo "- Setup env snapshot: \`$setup_bundle/setup_env.txt\`"
    echo "- Build command: \`$setup_bundle/build.command.txt\`"
    echo "- Build stdout: \`$setup_bundle/build.stdout.txt\`"
    echo "- Build stderr: \`$setup_bundle/build.stderr.txt\`"
    if [[ -s "$setup_bundle/build.stderr.txt" ]]; then
      echo
      echo "## build stderr tail"
      echo
      echo "\`\`\`text"
      tail -n 40 "$setup_bundle/build.stderr.txt"
      echo "\`\`\`"
    fi
  } > "$failure_report"
}

report_setup_failure_context() {
  local phase="$1"
  local exit_code="$2"
  local message="$3"
  echo "[lca_smoke] setup/build failed before stress start" >&2
  echo "[lca_smoke] stage=$phase exit_code=$exit_code message=$message" >&2
  echo "[lca_smoke] setup root: $SETUP_ROOT" >&2
  echo "[lca_smoke] setup tmpdir: $SETUP_TMPDIR" >&2
  echo "[lca_smoke] preflight manifest: $SETUP_MANIFEST" >&2
  echo "[lca_smoke] build stdout: $SETUP_BUILD_STDOUT" >&2
  echo "[lca_smoke] build stderr: $SETUP_BUILD_STDERR" >&2
  echo "[lca_smoke] failure summary: $FAILURE_ROOT/failure_summary.txt" >&2
  echo "[lca_smoke] failure report: $FAILURE_ROOT/latest_failure_report.md" >&2
}

run_setup_preflight() {
  local preflight_rc=0

  record_setup_environment_snapshot
  assert_setup_environment
  record_setup_check "path" "script_dir" "$SCRIPT_DIR"
  record_setup_check "path" "branch_root" "$BRANCH_ROOT"
  record_setup_check "path" "artifacts_root" "$ARTIFACTS_ROOT"
  record_setup_check "path" "smoke_output_root" "$OUTROOT"
  record_setup_check "path" "failure_root" "$FAILURE_ROOT"
  record_setup_check "path" "setup_root" "$SETUP_ROOT"
  record_setup_check "path" "setup_tmpdir" "$SETUP_TMPDIR"
  record_setup_check "path" "session_state_root" "$SESSION_STATE_ROOT"
  record_setup_check "path" "build_root" "$BUILD_ROOT"

  check_required_command_recorded bash || preflight_rc=2
  check_required_command_recorded python3 || preflight_rc=2
  check_required_command_recorded mktemp || preflight_rc=2
  check_required_command_recorded dirname || preflight_rc=2
  check_required_command_recorded chmod || preflight_rc=2
  check_required_command_recorded cp || preflight_rc=2
  check_required_command_recorded mv || preflight_rc=2
  check_required_command_recorded rm || preflight_rc=2
  check_required_command_recorded tail || preflight_rc=2
  check_required_command_recorded sleep || preflight_rc=2
  check_required_command_recorded grep || preflight_rc=2
  check_build_compiler_recorded || preflight_rc=2

  check_required_file_recorded "$SOURCE" "solver source" || preflight_rc=2
  check_required_file_recorded "$ARTIFACT_RESOLVER" "artifact resolver" || preflight_rc=2
  check_required_file_recorded "$RELEASE_ENV" "release env wrapper" || preflight_rc=2
  check_required_file_recorded "$RUN_CASE_HELPER" "branch-local case helper" || preflight_rc=2
  check_required_file_recorded "$CHECKER_HELPER" "branch-local validator" || preflight_rc=2
  check_required_file_recorded "$SMOKE_CASES" "smoke case manifest" || preflight_rc=2
  check_required_file_recorded "$BRANCH_ROOT/build.py" "build helper" || preflight_rc=2
  check_required_file_recorded "$BRANCH_ROOT/boj28350_resume.py" "resume helper" || preflight_rc=2
  check_required_executable_recorded "$BUILD_WRAPPER" "build wrapper" || preflight_rc=2
  check_python_entrypoint_recorded "$BRANCH_ROOT/build.py" "build helper imports" || preflight_rc=2
  check_python_entrypoint_recorded "$RUN_CASE_HELPER" "run case helper imports" || preflight_rc=2
  check_python_entrypoint_recorded "$CHECKER_HELPER" "validator helper imports" || preflight_rc=2

  if (( preflight_rc != 0 )); then
    write_setup_failure_summary "preflight" "$preflight_rc" "required setup/build dependency is missing"
    report_setup_failure_context "preflight" "$preflight_rc" "required setup/build dependency is missing"
  fi
  return "$preflight_rc"
}

write_failure_runtime_env() {
  local runtime_env_txt="$FAILURE_ROOT/runtime_env.txt"
  local runtime_env_exports="$FAILURE_ROOT/runtime_env_exports.sh"
  python3 - "$runtime_env_txt" "$runtime_env_exports" <<'PY'
from __future__ import annotations

import os
import shlex
import sys
from pathlib import Path

runtime_env_txt = Path(sys.argv[1])
runtime_env_exports = Path(sys.argv[2])

base_names = {
    "HOME",
    "PATH",
    "TERM",
    "LC_ALL",
    "LANG",
    "TZ",
    "XDG_CONFIG_HOME",
    "XDG_CACHE_HOME",
    "XDG_STATE_HOME",
    "LOCAL_SKIP_SELF_TEST",
    "BRANCH_ROOT",
    "SUITE_ROOT",
    "BRANCH_ARTIFACT_TMP_ROOT",
    "TMPDIR",
    "TMP",
    "TEMP",
    "LCA_SMOKE_ARTIFACT_ROOT",
    "LCA_SMOKE_OUTROOT",
    "LCA_SMOKE_STAGE_ROOT",
    "PYTHONPYCACHEPREFIX",
}
prefixes = ("ENABLE_", "PROFILE_", "PYTHON")

selected: dict[str, str] = {}
for key in sorted(os.environ):
    if key in base_names or key.startswith(prefixes):
        selected[key] = os.environ[key]

runtime_env_txt.write_text(
    "".join(f"{key}={value}\n" for key, value in selected.items()),
    encoding="utf-8",
)

export_lines = ["#!/usr/bin/env bash", "set -euo pipefail", ""]
for key, value in selected.items():
    export_lines.append(f"export {key}={shlex.quote(value)}")
runtime_env_exports.write_text("\n".join(export_lines) + "\n", encoding="utf-8")
runtime_env_exports.chmod(0o755)
PY
}

reset_failure_classification() {
  CURRENT_FAILURE_KIND=""
  CURRENT_FAILURE_ORIGIN=""
  CURRENT_FAILURE_RETRYABLE=0
  CURRENT_FAILURE_SUMMARY=""
  CURRENT_FAILURE_SOLVER_EXIT=""
  CURRENT_FAILURE_SIGNAL=""
}

set_failure_classification() {
  CURRENT_FAILURE_KIND="$1"
  CURRENT_FAILURE_ORIGIN="$2"
  CURRENT_FAILURE_RETRYABLE="$3"
  CURRENT_FAILURE_SUMMARY="$4"
}

load_case_failure_result() {
  local result_path="$1"
  local parsed_assignments=""
  local parsed_status=""
  local parsed_category=""
  local parsed_exit_code=""
  local parsed_retryable=""
  local parsed_message=""
  local parsed_solver_exit=""
  local parsed_solver_signal=""
  local raw_exit_code=""

  if [[ ! -f "$result_path" ]]; then
    return 1
  fi

  if ! parsed_assignments="$(
    python3 - "$result_path" <<'PY'
from __future__ import annotations

import json
import shlex
import sys
from pathlib import Path

result_path = Path(sys.argv[1])
try:
    data = json.loads(result_path.read_text(encoding="utf-8"))
except Exception as exc:  # pragma: no cover - shell-side fallback
    print(f"ERROR:{exc}")
    raise SystemExit(1)

status = str(data.get("status") or "").strip() or "unclassified_case_failure"
category = str(data.get("category") or "").strip() or "unknown"
exit_code = data.get("exit_code")
if not isinstance(exit_code, int):
    print("ERROR:missing integer exit_code")
    raise SystemExit(1)

message = str(data.get("message") or "").replace("\n", " ").strip()
if not message:
    message = "helper emitted an empty failure message"

solver_exit = data.get("solver_exit_code")
solver_signal = data.get("solver_signal")
retryable = 1 if status == "harness_transient_failure" else 0

fields = (
    ("parsed_status", status),
    ("parsed_category", category),
    ("parsed_exit_code", exit_code),
    ("parsed_retryable", retryable),
    ("parsed_message", message),
    ("parsed_solver_exit", "" if solver_exit is None else solver_exit),
    ("parsed_solver_signal", "" if solver_signal is None else solver_signal),
)
for key, value in fields:
    print(f"{key}={shlex.quote(str(value))}")
PY
  )"; then
    return 1
  fi

  if [[ -z "$parsed_assignments" ]]; then
    return 1
  fi
  eval "$parsed_assignments"

  if [[ -z "$parsed_status" || -z "$parsed_category" || -z "$parsed_exit_code" || -z "$parsed_retryable" ]]; then
    return 1
  fi

  CURRENT_FAILURE_KIND="$parsed_status"
  CURRENT_FAILURE_ORIGIN="$parsed_category"
  raw_exit_code="$parsed_exit_code"
  CURRENT_FAILURE_RETRYABLE="$parsed_retryable"
  CURRENT_FAILURE_SUMMARY="$parsed_message"
  CURRENT_FAILURE_SOLVER_EXIT="$parsed_solver_exit"
  CURRENT_FAILURE_SIGNAL="$parsed_solver_signal"

  case "$CURRENT_FAILURE_KIND" in
    pass)
      CURRENT_FAILURE_RC=0
      ;;
    solver_timeout)
      CURRENT_FAILURE_RC="$SMOKE_EXIT_SOLVER_TIMEOUT"
      ;;
    solver_runtime_failure|solver_signal_failure)
      CURRENT_FAILURE_RC="$SMOKE_EXIT_SOLVER_RUNTIME_FAILURE"
      ;;
    solver_acceptance_failure|solver_case_failure)
      CURRENT_FAILURE_RC="$SMOKE_EXIT_SOLVER_FAILURE"
      ;;
    harness_usage_failure)
      CURRENT_FAILURE_RC="$SMOKE_EXIT_USAGE"
      ;;
    harness_transient_failure)
      CURRENT_FAILURE_RC="$SMOKE_EXIT_HARNESS_FAILURE"
      ;;
    *)
      if [[ "$CURRENT_FAILURE_ORIGIN" == "harness" ]]; then
        CURRENT_FAILURE_RC="$SMOKE_EXIT_HARNESS_FAILURE"
      elif [[ "$CURRENT_FAILURE_ORIGIN" == "solver" ]]; then
        if (( raw_exit_code == SMOKE_EXIT_SOLVER_TIMEOUT )); then
          CURRENT_FAILURE_RC="$SMOKE_EXIT_SOLVER_TIMEOUT"
        elif (( raw_exit_code == SMOKE_EXIT_SOLVER_FAILURE )); then
          CURRENT_FAILURE_RC="$SMOKE_EXIT_SOLVER_FAILURE"
        else
          CURRENT_FAILURE_RC="$SMOKE_EXIT_SOLVER_RUNTIME_FAILURE"
        fi
      else
        CURRENT_FAILURE_RC="$raw_exit_code"
      fi
      ;;
  esac
  return 0
}

classify_case_failure() {
  local rc="$1"

  reset_failure_classification
  CURRENT_FAILURE_HELPER_RC="$rc"
  CURRENT_FAILURE_RC="$rc"

  if [[ -n "$CURRENT_CASE_RESULT_JSON" ]]; then
    if load_case_failure_result "$CURRENT_CASE_RESULT_JSON"; then
      return
    fi
    CURRENT_FAILURE_RC="$SMOKE_EXIT_HARNESS_FAILURE"
    set_failure_classification \
      "harness_transient_failure" \
      "harness" \
      1 \
      "helper did not leave a readable ${RUN_CASE_RESULT_NAME} for the preserved case"
    return
  fi

  case "$rc" in
    "$SMOKE_EXIT_SOLVER_TIMEOUT")
      set_failure_classification "solver_timeout" "solver" 0 "helper reported a solver timeout"
      return
      ;;
    "$SMOKE_EXIT_USAGE")
      set_failure_classification "harness_usage_failure" "harness" 0 "helper rejected the branch-local invocation or artifact routing"
      return
      ;;
    "$SMOKE_EXIT_HARNESS_FAILURE")
      set_failure_classification "harness_transient_failure" "harness" 1 "helper hit an unexpected harness/runtime failure"
      return
      ;;
    "$SMOKE_EXIT_SOLVER_RUNTIME_FAILURE")
      set_failure_classification "solver_runtime_failure" "solver" 0 "helper reported a non-timeout solver runtime failure"
      return
      ;;
  esac

  if [[ -f "$CURRENT_CASE_STDERR" ]]; then
    if grep -Fq "[run_case] harness failure:" "$CURRENT_CASE_STDERR"; then
      set_failure_classification "harness_transient_failure" "harness" 1 "helper reported an unexpected harness/runtime failure"
      return
    fi
    if grep -Fq "[run_case] validator failed:" "$CURRENT_CASE_STDERR"; then
      set_failure_classification "solver_acceptance_failure" "solver" 0 "validator rejected the solver output for the preserved case"
      return
    fi
    if grep -Fq "[run_case] solver timed out after" "$CURRENT_CASE_STDERR"; then
      set_failure_classification "solver_timeout" "solver" 0 "helper reported a solver timeout"
      return
    fi
    if grep -Fq "[run_case] solver terminated by signal" "$CURRENT_CASE_STDERR"; then
      set_failure_classification "solver_signal_failure" "solver" 0 "solver terminated abnormally during preserved case execution"
      return
    fi
    if grep -Fq "[run_case] solver exited with code" "$CURRENT_CASE_STDERR"; then
      if (( rc >= 128 )); then
        set_failure_classification "solver_signal_failure" "solver" 0 "solver terminated abnormally during preserved case execution"
      else
        set_failure_classification "solver_runtime_failure" "solver" 0 "solver exited non-zero during preserved case execution"
      fi
      return
    fi
  fi

  if (( rc >= 128 )); then
    set_failure_classification "solver_signal_failure" "solver" 0 "solver terminated abnormally during preserved case execution"
    return
  fi

  if (( rc == SMOKE_EXIT_SOLVER_FAILURE )); then
    set_failure_classification "solver_case_failure" "solver" 0 "helper returned the generic solver-failure code"
    return
  fi

  set_failure_classification "solver_runtime_failure" "solver" 0 "helper preserved a non-timeout solver runtime failure"
}

write_failure_artifact_manifest() {
  local failure_case_dir="$1"
  local manifest_path="$FAILURE_ROOT/artifact_manifest.tsv"
  python3 - "$failure_case_dir" "$manifest_path" "$FAILURE_ROOT" "$CURRENT_CASE_SOLVER_SNAPSHOT" <<'PY'
from __future__ import annotations

import hashlib
import sys
from pathlib import Path

failure_case_dir = Path(sys.argv[1])
manifest_path = Path(sys.argv[2])
failure_root = Path(sys.argv[3])
solver_snapshot = Path(sys.argv[4])

entries = [
    ("failed_case_row", failure_root / "failed_case_row.tsv"),
    ("smoke_manifest_snapshot", failure_root / "smoke_cases_manifest.tsv"),
    ("suite_config", failure_root / "suite_config.txt"),
    ("suite_plan", failure_root / "suite_plan.tsv"),
    ("retry_log", failure_root / "retry_log.tsv"),
    ("runtime_env", failure_root / "runtime_env.txt"),
    ("runtime_env_exports", failure_root / "runtime_env_exports.sh"),
    ("commands", failure_root / "commands.txt"),
    ("exact_seed", failure_root / "seed.txt"),
    ("exact_input", failure_root / "input.txt"),
    ("exact_output", failure_root / "solver_output.txt"),
    ("checker_result", failure_root / "checker_result.txt"),
    ("checker_replay_stdout", failure_root / "checker_replay.stdout.txt"),
    ("checker_replay_stderr", failure_root / "checker_replay.stderr.txt"),
    ("mismatch_summary", failure_root / "mismatch_summary.txt"),
    ("rerun_command", failure_root / "rerun_command.txt"),
    ("checker_replay_script", failure_root / "recheck_preserved_output.sh"),
    ("seed_repro_script", failure_root / "repro_from_seed.sh"),
    ("preserved_input_replay_script", failure_root / "replay_preserved_input.sh"),
    ("solver_snapshot", solver_snapshot),
    ("case_input", failure_case_dir / "in.txt"),
    ("case_meta", failure_case_dir / "meta.json"),
    ("case_hidden_parent", failure_case_dir / "hidden_parent.txt"),
    ("case_output", failure_case_dir / "out.txt"),
    ("case_time", failure_case_dir / "time.txt"),
    ("case_solver_stderr", failure_case_dir / "solver_stderr.txt"),
    ("case_helper_result_json", failure_case_dir / "run_case_result.json"),
    ("case_helper_stdout", failure_case_dir / "run_case.stdout.txt"),
    ("case_helper_stderr", failure_case_dir / "run_case.stderr.txt"),
  ]

rows = ["label\tpath\texists\tbytes\tsha256"]
for label, path in entries:
    exists = path.exists()
    size = path.stat().st_size if exists and path.is_file() else -1
    digest = "-"
    if exists and path.is_file():
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
    rows.append(
        "\t".join(
            [
                label,
                str(path),
                "1" if exists else "0",
                str(size),
                digest,
            ]
        )
    )

manifest_path.write_text("\n".join(rows) + "\n", encoding="utf-8")
PY
}

write_failure_checker_bundle() {
  local failure_case_dir="$1"
  local checker_result_txt="$FAILURE_ROOT/checker_result.txt"
  local checker_replay_stdout="$FAILURE_ROOT/checker_replay.stdout.txt"
  local checker_replay_stderr="$FAILURE_ROOT/checker_replay.stderr.txt"
  local mismatch_summary_txt="$FAILURE_ROOT/mismatch_summary.txt"
  local checker_script="$FAILURE_ROOT/recheck_preserved_output.sh"
  local checker_assignments=""
  local recorded_status="missing_helper_result"
  local recorded_category="unknown"
  local recorded_exit_code=""
  local recorded_validator_ok=""
  local checker_result_kind="not_recorded"
  local recorded_message="helper did not leave a readable ${RUN_CASE_RESULT_NAME}"
  local rechecked_status="not_run"
  local rechecked_exit_code=""
  local rechecked_message=""
  local primary_message=""
  local prior_errexit=1

  if [[ -f "$CURRENT_CASE_RESULT_JSON" ]]; then
    if checker_assignments="$(
      python3 - "$CURRENT_CASE_RESULT_JSON" <<'PY'
from __future__ import annotations

import json
import shlex
import sys
from pathlib import Path

path = Path(sys.argv[1])
try:
    data = json.loads(path.read_text(encoding="utf-8"))
except Exception:
    raise SystemExit(1)

status = str(data.get("status") or "").strip() or "missing_status"
category = str(data.get("category") or "").strip() or "unknown"
exit_code = data.get("exit_code")
validator_ok = data.get("validator_ok")
message = str(data.get("message") or "").replace("\n", " ").strip()
if not message:
    message = "helper emitted an empty failure message"

if status == "pass":
    checker_result_kind = "pass"
elif status == "solver_acceptance_failure":
    checker_result_kind = "fail"
elif status in {
    "solver_timeout",
    "solver_runtime_failure",
    "solver_signal_failure",
    "solver_case_failure",
    "harness_usage_failure",
    "harness_transient_failure",
}:
    checker_result_kind = "not_run"
else:
    checker_result_kind = "unknown"

fields = (
    ("recorded_status", status),
    ("recorded_category", category),
    ("recorded_exit_code", "" if exit_code is None else exit_code),
    ("recorded_validator_ok", "" if validator_ok is None else int(bool(validator_ok))),
    ("checker_result_kind", checker_result_kind),
    ("recorded_message", message),
)
for key, value in fields:
    print(f"{key}={shlex.quote(str(value))}")
PY
    )"; then
      if [[ -n "$checker_assignments" ]]; then
        eval "$checker_assignments"
      fi
    fi
  fi

  enter_function_errexit prior_errexit
  : > "$checker_replay_stdout"
  : > "$checker_replay_stderr"
  if [[ -f "$failure_case_dir/in.txt" && -f "$failure_case_dir/out.txt" ]]; then
    set +e
    python3 "$CHECKER_HELPER" "$failure_case_dir/in.txt" "$failure_case_dir/out.txt" >"$checker_replay_stdout" 2>"$checker_replay_stderr"
    rechecked_exit_code=$?
    set -e
    if (( rechecked_exit_code == 0 )); then
      rechecked_status="pass"
    else
      rechecked_status="fail"
    fi
  else
    rechecked_status="unavailable"
    rechecked_exit_code=""
    rechecked_message="preserved input/output missing; checker replay skipped"
  fi
  restore_function_errexit "$prior_errexit"

  if [[ -s "$checker_replay_stdout" ]]; then
    IFS= read -r rechecked_message < "$checker_replay_stdout" || true
  elif [[ -s "$checker_replay_stderr" ]]; then
    IFS= read -r rechecked_message < "$checker_replay_stderr" || true
  fi
  if [[ -z "$rechecked_message" ]]; then
    if [[ -n "$rechecked_exit_code" ]]; then
      rechecked_message="checker replay exited with code $rechecked_exit_code"
    else
      rechecked_message="$recorded_message"
    fi
  fi
  primary_message="$rechecked_message"
  if [[ -z "$primary_message" ]]; then
    primary_message="$recorded_message"
  fi

  python3 - "$mismatch_summary_txt" "$primary_message" \
    "$CURRENT_CASE_TAG" "$CURRENT_CASE_STAGE" "$CURRENT_CASE_MODE" "$CURRENT_CASE_N" \
    "$CURRENT_CASE_SEED" "$CURRENT_CASE_SHUFFLE_LABELS" "$CURRENT_CASE_SHUFFLE_QUERIES" \
    "$CURRENT_CASE_TIMEOUT" "$checker_result_kind" "$recorded_status" "$recorded_message" \
    "$rechecked_status" "$rechecked_exit_code" "$rechecked_message" \
    "$failure_case_dir/in.txt" "$failure_case_dir/out.txt" \
    "$checker_replay_stdout" "$checker_replay_stderr" <<'PY'
from __future__ import annotations

import re
import sys
from pathlib import Path

(
    summary_path,
    primary_message,
    case_tag,
    stage,
    mode,
    n,
    seed,
    shuffle_labels,
    shuffle_queries,
    timeout_s,
    checker_result_kind,
    recorded_status,
    recorded_message,
    rechecked_status,
    rechecked_exit_code,
    rechecked_message,
    input_path,
    output_path,
    replay_stdout,
    replay_stderr,
) = sys.argv[1:]

lines = [
    f"case_tag={case_tag}",
    f"stage={stage}",
    f"mode={mode}",
    f"n={n}",
    f"seed={seed}",
    f"shuffle_labels={shuffle_labels}",
    f"shuffle_queries={shuffle_queries}",
    f"timeout_s={timeout_s}",
    f"checker_result={checker_result_kind}",
    f"recorded_status={recorded_status}",
    f"recorded_message={recorded_message}",
    f"rechecked_status={rechecked_status}",
    f"rechecked_exit_code={rechecked_exit_code}",
    f"rechecked_message={rechecked_message}",
    f"input_path={input_path}",
    f"output_path={output_path}",
    f"checker_replay_stdout={replay_stdout}",
    f"checker_replay_stderr={replay_stderr}",
]

match = re.match(
    r"^query #(?P<query_index>\d+) mismatch: lca\((?P<u>\d+), (?P<v>\d+)\)=(?P<got>\d+), expected (?P<expected>\d+)$",
    primary_message,
)
if match is None:
    lines.append("mismatch_kind=unparsed")
    lines.append(f"mismatch_message={primary_message}")
else:
    lines.append("mismatch_kind=query_lca_mismatch")
    lines.extend(
        [
            f"query_index={match.group('query_index')}",
            f"query_u={match.group('u')}",
            f"query_v={match.group('v')}",
            f"got_lca={match.group('got')}",
            f"expected_lca={match.group('expected')}",
        ]
    )

Path(summary_path).write_text("\n".join(lines) + "\n", encoding="utf-8")
PY

  {
    echo "case_tag=$CURRENT_CASE_TAG"
    echo "stage=$CURRENT_CASE_STAGE"
    echo "mode=$CURRENT_CASE_MODE"
    echo "n=$CURRENT_CASE_N"
    echo "seed=$CURRENT_CASE_SEED"
    echo "shuffle_labels=$CURRENT_CASE_SHUFFLE_LABELS"
    echo "shuffle_queries=$CURRENT_CASE_SHUFFLE_QUERIES"
    echo "timeout_s=$CURRENT_CASE_TIMEOUT"
    echo "helper_result_json=$CURRENT_CASE_RESULT_JSON"
    echo "recorded_status=$recorded_status"
    echo "recorded_category=$recorded_category"
    echo "recorded_exit_code=$recorded_exit_code"
    echo "recorded_validator_ok=$recorded_validator_ok"
    echo "checker_result=$checker_result_kind"
    echo "message=$recorded_message"
    echo "rechecked_status=$rechecked_status"
    echo "rechecked_exit_code=$rechecked_exit_code"
    echo "rechecked_message=$rechecked_message"
    echo "checker_command=$CURRENT_CASE_CHECKER_COMMAND"
    echo "checker_script=$checker_script"
    echo "checker_replay_stdout=$checker_replay_stdout"
    echo "checker_replay_stderr=$checker_replay_stderr"
    echo "mismatch_summary=$mismatch_summary_txt"
    echo "input_path=$failure_case_dir/in.txt"
    echo "output_path=$failure_case_dir/out.txt"
  } > "$checker_result_txt"

  cat > "$checker_script" <<EOF
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="\$(cd -- "\$(dirname -- "\${BASH_SOURCE[0]}")" && pwd -P)"
source "\$SCRIPT_DIR/runtime_env_exports.sh"
exec $CURRENT_CASE_CHECKER_COMMAND
EOF
  chmod +x "$checker_script"
}

write_failure_repro_exports() {
  local failure_case_dir="$1"
  local seed_txt="$FAILURE_ROOT/seed.txt"
  local input_txt="$FAILURE_ROOT/input.txt"
  local output_txt="$FAILURE_ROOT/solver_output.txt"
  local rerun_command_txt="$FAILURE_ROOT/rerun_command.txt"

  printf '%s\n' "$CURRENT_CASE_SEED" > "$seed_txt"

  if [[ -f "$failure_case_dir/in.txt" ]]; then
    cp "$failure_case_dir/in.txt" "$input_txt"
  else
    {
      echo "# preserved input missing"
      echo "source_path=$failure_case_dir/in.txt"
      echo "seed=$CURRENT_CASE_SEED"
      echo "case_tag=$CURRENT_CASE_TAG"
    } > "$input_txt"
  fi

  if [[ -f "$failure_case_dir/out.txt" ]]; then
    cp "$failure_case_dir/out.txt" "$output_txt"
  else
    {
      echo "# preserved solver output missing"
      echo "source_path=$failure_case_dir/out.txt"
      echo "seed=$CURRENT_CASE_SEED"
      echo "case_tag=$CURRENT_CASE_TAG"
    } > "$output_txt"
  fi

  {
    echo "preferred_preserved_input_replay=bash $(quote_word "$FAILURE_ROOT/replay_preserved_input.sh")"
    echo "preferred_seed_repro=bash $(quote_word "$FAILURE_ROOT/repro_from_seed.sh")"
    echo "preferred_checker_replay=bash $(quote_word "$FAILURE_ROOT/recheck_preserved_output.sh")"
    echo "raw_preserved_input_replay=$CURRENT_CASE_PRESERVED_INPUT_COMMAND"
    echo "raw_seed_repro=$CURRENT_CASE_REPRO_COMMAND"
    echo "raw_checker_replay=$CURRENT_CASE_CHECKER_COMMAND"
  } > "$rerun_command_txt"
}

write_failure_debug_bundle() {
  local failure_case_dir="$1"
  local commands_txt="$FAILURE_ROOT/commands.txt"
  local seed_txt="$FAILURE_ROOT/seed.txt"
  local input_txt="$FAILURE_ROOT/input.txt"
  local output_txt="$FAILURE_ROOT/solver_output.txt"
  local checker_result_txt="$FAILURE_ROOT/checker_result.txt"
  local checker_replay_stdout="$FAILURE_ROOT/checker_replay.stdout.txt"
  local checker_replay_stderr="$FAILURE_ROOT/checker_replay.stderr.txt"
  local mismatch_summary_txt="$FAILURE_ROOT/mismatch_summary.txt"
  local rerun_command_txt="$FAILURE_ROOT/rerun_command.txt"
  local checker_script="$FAILURE_ROOT/recheck_preserved_output.sh"
  local seed_repro_script="$FAILURE_ROOT/repro_from_seed.sh"
  local preserved_input_script="$FAILURE_ROOT/replay_preserved_input.sh"
  local env_exports_path="$FAILURE_ROOT/runtime_env_exports.sh"

  printf '%s\n' "$CURRENT_CASE_MANIFEST_ROW" > "$FAILURE_ROOT/failed_case_row.tsv"
  cp "$SMOKE_CASES" "$FAILURE_ROOT/smoke_cases_manifest.tsv"
  write_failure_runtime_env
  write_failure_checker_bundle "$failure_case_dir"
  write_failure_repro_exports "$failure_case_dir"
  copy_retry_log_to_failure_root

  {
    echo "executed_command=$CURRENT_CASE_EXEC_COMMAND"
    echo "checker_command=$CURRENT_CASE_CHECKER_COMMAND"
    echo "exact_seed=$seed_txt"
    echo "exact_input=$input_txt"
    echo "exact_output=$output_txt"
    echo "checker_result=$checker_result_txt"
    echo "checker_replay_stdout=$checker_replay_stdout"
    echo "checker_replay_stderr=$checker_replay_stderr"
    echo "mismatch_summary=$mismatch_summary_txt"
    echo "rerun_command=$rerun_command_txt"
    echo "checker_script=$checker_script"
    echo "seed_repro_command=$CURRENT_CASE_REPRO_COMMAND"
    echo "preserved_input_command=$CURRENT_CASE_PRESERVED_INPUT_COMMAND"
    echo "seed_repro_script=$seed_repro_script"
    echo "preserved_input_replay_script=$preserved_input_script"
    echo "runtime_env_exports=$env_exports_path"
    echo "solver_snapshot=$CURRENT_CASE_SOLVER_SNAPSHOT"
    echo "seed_repro_dir=$CURRENT_CASE_REPRO_DIR"
    echo "preserved_input_replay_dir=$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR"
    echo "retry_log=$FAILURE_ROOT/retry_log.tsv"
    echo "suite_config=$FAILURE_ROOT/suite_config.txt"
    echo "suite_plan=$FAILURE_ROOT/suite_plan.tsv"
  } > "$commands_txt"

  cat > "$seed_repro_script" <<EOF
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="\$(cd -- "\$(dirname -- "\${BASH_SOURCE[0]}")" && pwd -P)"
source "\$SCRIPT_DIR/runtime_env_exports.sh"
exec $CURRENT_CASE_REPRO_COMMAND
EOF
  chmod +x "$seed_repro_script"

  cat > "$preserved_input_script" <<EOF
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="\$(cd -- "\$(dirname -- "\${BASH_SOURCE[0]}")" && pwd -P)"
source "\$SCRIPT_DIR/runtime_env_exports.sh"
if [[ -n "\${BRANCH_ARTIFACT_TMP_ROOT:-}" ]]; then
  mkdir -p "\$BRANCH_ARTIFACT_TMP_ROOT"
fi
REPLAY_DIR="\${1:-$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR}"
rm -rf "\$REPLAY_DIR"
mkdir -p "\$REPLAY_DIR"
env \
  DENSE_SHADOW_CASE_MODE=$CURRENT_CASE_MODE \
  DENSE_SHADOW_CASE_N=$CURRENT_CASE_N \
  DENSE_SHADOW_CASE_SEED=$CURRENT_CASE_SEED \
  DENSE_PROFILE_OUTDIR="\$REPLAY_DIR" \
  DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1 \
  "$CURRENT_CASE_SOLVER_SNAPSHOT" \
  < "$failure_case_dir/in.txt" \
  > "\$REPLAY_DIR/out.txt" \
  2> "\$REPLAY_DIR/solver_stderr.txt"
echo "[lca_smoke] preserved-input replay artifacts: \$REPLAY_DIR"
EOF
  chmod +x "$preserved_input_script"

  write_failure_artifact_manifest "$failure_case_dir"
}

write_failure_summary() {
  local failure_case_dir="$1"
  local failure_summary="$FAILURE_ROOT/failure_summary.txt"
  local failure_report="$FAILURE_ROOT/latest_failure_report.md"
  local artifact_manifest="$FAILURE_ROOT/artifact_manifest.tsv"
  local exact_seed_txt="$FAILURE_ROOT/seed.txt"
  local exact_input_txt="$FAILURE_ROOT/input.txt"
  local exact_output_txt="$FAILURE_ROOT/solver_output.txt"
  local checker_result_txt="$FAILURE_ROOT/checker_result.txt"
  local checker_replay_stdout="$FAILURE_ROOT/checker_replay.stdout.txt"
  local checker_replay_stderr="$FAILURE_ROOT/checker_replay.stderr.txt"
  local mismatch_summary_txt="$FAILURE_ROOT/mismatch_summary.txt"
  local rerun_command_txt="$FAILURE_ROOT/rerun_command.txt"
  local checker_script="$FAILURE_ROOT/recheck_preserved_output.sh"
  local env_snapshot="$FAILURE_ROOT/runtime_env.txt"
  local env_exports="$FAILURE_ROOT/runtime_env_exports.sh"
  local seed_repro_script="$FAILURE_ROOT/repro_from_seed.sh"
  local preserved_input_script="$FAILURE_ROOT/replay_preserved_input.sh"
  local manifest_snapshot="$FAILURE_ROOT/smoke_cases_manifest.tsv"
  local failed_case_row="$FAILURE_ROOT/failed_case_row.tsv"
  local retry_log="$FAILURE_ROOT/retry_log.tsv"
  mkdir -p "$FAILURE_ROOT"
  write_failure_debug_bundle "$failure_case_dir"
  {
    echo "script=./outer_suite_wrappers/lca_smoke.sh"
    echo "exit_code=$CURRENT_FAILURE_RC"
    echo "helper_exit_code=$CURRENT_FAILURE_HELPER_RC"
    echo "failure_kind=$CURRENT_FAILURE_KIND"
    echo "failure_origin=$CURRENT_FAILURE_ORIGIN"
    echo "failure_retryable=$CURRENT_FAILURE_RETRYABLE"
    echo "failure_summary=$CURRENT_FAILURE_SUMMARY"
    echo "solver_exit_code=${CURRENT_FAILURE_SOLVER_EXIT:-}"
    echo "solver_signal=${CURRENT_FAILURE_SIGNAL:-}"
    echo "failed_stage=$CURRENT_CASE_STAGE"
    echo "failed_case_index=$CURRENT_CASE_INDEX"
    echo "failed_attempt=$CURRENT_CASE_ATTEMPT"
    echo "failed_mode=$CURRENT_CASE_MODE"
    echo "failed_n=$CURRENT_CASE_N"
    echo "failed_seed=$CURRENT_CASE_SEED"
    echo "failed_shuffle_labels=$CURRENT_CASE_SHUFFLE_LABELS"
    echo "failed_shuffle_queries=$CURRENT_CASE_SHUFFLE_QUERIES"
    echo "failed_timeout_s=$CURRENT_CASE_TIMEOUT"
    echo "failed_case_tag=$CURRENT_CASE_TAG"
    echo "manifest_row=$CURRENT_CASE_MANIFEST_ROW"
    echo "executed_command=$CURRENT_CASE_EXEC_COMMAND"
    echo "repro_command=$CURRENT_CASE_REPRO_COMMAND"
    echo "preserved_input_command=$CURRENT_CASE_PRESERVED_INPUT_COMMAND"
    echo "failure_root=$FAILURE_ROOT"
    echo "failure_case_dir=$failure_case_dir"
    echo "helper_stdout=$CURRENT_CASE_STDOUT"
    echo "helper_stderr=$CURRENT_CASE_STDERR"
    echo "helper_result_json=$CURRENT_CASE_RESULT_JSON"
    echo "exact_seed_path=$exact_seed_txt"
    echo "exact_input_path=$exact_input_txt"
    echo "exact_output_path=$exact_output_txt"
    echo "checker_result_path=$checker_result_txt"
    echo "checker_replay_stdout_path=$checker_replay_stdout"
    echo "checker_replay_stderr_path=$checker_replay_stderr"
    echo "mismatch_summary_path=$mismatch_summary_txt"
    echo "rerun_command_path=$rerun_command_txt"
    echo "checker_command=$CURRENT_CASE_CHECKER_COMMAND"
    echo "checker_script=$checker_script"
    echo "solver_snapshot=$CURRENT_CASE_SOLVER_SNAPSHOT"
    echo "seed_repro_dir=$CURRENT_CASE_REPRO_DIR"
    echo "preserved_input_replay_dir=$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR"
    echo "failed_case_row_path=$failed_case_row"
    echo "manifest_snapshot_path=$manifest_snapshot"
    echo "runtime_env_path=$env_snapshot"
    echo "runtime_env_exports_path=$env_exports"
    echo "artifact_manifest_path=$artifact_manifest"
    echo "retry_log_path=$retry_log"
    echo "suite_config_path=$FAILURE_ROOT/suite_config.txt"
    echo "suite_plan_path=$FAILURE_ROOT/suite_plan.tsv"
    echo "seed_repro_script=$seed_repro_script"
    echo "preserved_input_replay_script=$preserved_input_script"
    echo "input_path=$failure_case_dir/in.txt"
    echo "meta_path=$failure_case_dir/meta.json"
    echo "hidden_parent_path=$failure_case_dir/hidden_parent.txt"
    echo "output_path=$failure_case_dir/out.txt"
    echo "time_path=$failure_case_dir/time.txt"
    echo "solver_stderr_path=$failure_case_dir/solver_stderr.txt"
    echo "smoke_output_root=$OUTROOT"
    echo "smoke_manifest=$SMOKE_CASES"
  } > "$failure_summary"

  {
    echo "# lca_smoke Failure Report"
    echo
    echo "- Exit code: \`$CURRENT_FAILURE_RC\`"
    echo "- Helper exit code: \`$CURRENT_FAILURE_HELPER_RC\`"
    echo "- Failure kind: \`$CURRENT_FAILURE_KIND\`"
    echo "- Failure origin: \`$CURRENT_FAILURE_ORIGIN\`"
    echo "- Retryable harness issue: \`$CURRENT_FAILURE_RETRYABLE\`"
    echo "- Failure summary: \`$CURRENT_FAILURE_SUMMARY\`"
    if [[ -n "$CURRENT_FAILURE_SOLVER_EXIT" ]]; then
      echo "- Solver exit code: \`$CURRENT_FAILURE_SOLVER_EXIT\`"
    fi
    if [[ -n "$CURRENT_FAILURE_SIGNAL" ]]; then
      echo "- Solver signal: \`$CURRENT_FAILURE_SIGNAL\`"
    fi
    echo "- Case tag: \`$CURRENT_CASE_TAG\`"
    echo "- Case index: \`$CURRENT_CASE_INDEX\`"
    echo "- Attempt: \`$CURRENT_CASE_ATTEMPT\`"
    echo "- Stage: \`$CURRENT_CASE_STAGE\`"
    echo "- Mode: \`$CURRENT_CASE_MODE\`"
    echo "- n: \`$CURRENT_CASE_N\`"
    echo "- Seed: \`$CURRENT_CASE_SEED\`"
    echo "- Shuffle labels: \`$CURRENT_CASE_SHUFFLE_LABELS\`"
    echo "- Shuffle queries: \`$CURRENT_CASE_SHUFFLE_QUERIES\`"
    echo "- Timeout (s): \`$CURRENT_CASE_TIMEOUT\`"
    echo "- Manifest row: \`$CURRENT_CASE_MANIFEST_ROW\`"
    echo "- Failure root: \`$FAILURE_ROOT\`"
    echo "- Failure case dir: \`$failure_case_dir\`"
    echo "- Smoke output root: \`$OUTROOT\`"
    echo "- Smoke manifest: \`$SMOKE_CASES\`"
    echo "- Helper stdout: \`$CURRENT_CASE_STDOUT\`"
    echo "- Helper stderr: \`$CURRENT_CASE_STDERR\`"
    echo "- Helper result json: \`$CURRENT_CASE_RESULT_JSON\`"
    echo "- Exact seed snapshot: \`$exact_seed_txt\`"
    echo "- Exact input snapshot: \`$exact_input_txt\`"
    echo "- Exact solver output snapshot: \`$exact_output_txt\`"
    echo "- Checker result: \`$checker_result_txt\`"
    echo "- Checker replay stdout: \`$checker_replay_stdout\`"
    echo "- Checker replay stderr: \`$checker_replay_stderr\`"
    echo "- Mismatch summary: \`$mismatch_summary_txt\`"
    echo "- Rerun command snapshot: \`$rerun_command_txt\`"
    echo "- Checker replay script: \`$checker_script\`"
    echo "- Frozen solver snapshot: \`$CURRENT_CASE_SOLVER_SNAPSHOT\`"
    echo "- Failed row snapshot: \`$failed_case_row\`"
    echo "- Smoke manifest snapshot: \`$manifest_snapshot\`"
    echo "- Runtime env snapshot: \`$env_snapshot\`"
    echo "- Runtime env exports: \`$env_exports\`"
    echo "- Artifact manifest: \`$artifact_manifest\`"
    echo "- Retry log: \`$retry_log\`"
    echo "- Suite config: \`$FAILURE_ROOT/suite_config.txt\`"
    echo "- Suite plan: \`$FAILURE_ROOT/suite_plan.tsv\`"
    echo "- Seed repro script: \`$seed_repro_script\`"
    echo "- Preserved-input replay script: \`$preserved_input_script\`"
    echo
    echo "## Commands"
    echo
    echo "Executed command:"
    echo
    echo "\`\`\`bash"
    echo "$CURRENT_CASE_EXEC_COMMAND"
    echo "\`\`\`"
    echo
    echo "Preferred checker replay invocation:"
    echo
    echo "\`\`\`bash"
    echo "bash $(quote_word "$checker_script")"
    echo "\`\`\`"
    echo
    echo "Raw checker replay command body recorded for low-level debugging:"
    echo
    echo "\`\`\`bash"
    echo "$CURRENT_CASE_CHECKER_COMMAND"
    echo "\`\`\`"
    echo
    echo "Preferred seed repro invocation:"
    echo
    echo "\`\`\`bash"
    echo "bash $(quote_word "$seed_repro_script")"
    echo "\`\`\`"
    echo
    echo "Raw seed repro command body recorded for low-level debugging:"
    echo
    echo "\`\`\`bash"
    echo "$CURRENT_CASE_REPRO_COMMAND"
    echo "\`\`\`"
    echo
    echo "Preferred preserved-input replay invocation:"
    echo
    echo "\`\`\`bash"
    echo "bash $(quote_word "$preserved_input_script")"
    echo "\`\`\`"
    echo
    echo "Raw preserved-input replay command body recorded for low-level debugging:"
    echo
    echo "\`\`\`bash"
    echo "$CURRENT_CASE_PRESERVED_INPUT_COMMAND"
    echo "\`\`\`"
    echo
    echo "## Artifact Paths"
    echo
    echo "- input: \`$failure_case_dir/in.txt\`"
    echo "- meta: \`$failure_case_dir/meta.json\`"
    echo "- hidden parent: \`$failure_case_dir/hidden_parent.txt\`"
    echo "- solver output: \`$failure_case_dir/out.txt\`"
    echo "- timing: \`$failure_case_dir/time.txt\`"
    echo "- solver stderr: \`$failure_case_dir/solver_stderr.txt\`"
    echo "- helper result json: \`$CURRENT_CASE_RESULT_JSON\`"
    echo "- checker result: \`$checker_result_txt\`"
    echo "- checker replay stdout: \`$checker_replay_stdout\`"
    echo "- checker replay stderr: \`$checker_replay_stderr\`"
    echo "- mismatch summary: \`$mismatch_summary_txt\`"
    echo "- helper stdout: \`$CURRENT_CASE_STDOUT\`"
    echo "- helper stderr: \`$CURRENT_CASE_STDERR\`"
    echo "- artifact manifest: \`$artifact_manifest\`"
    echo
    echo "## Preserved Debug Bundle"
    echo
    echo "- \`solver_snapshot\` freezes the exact failing binary."
    echo "- \`runtime_env_exports.sh\` restores the branch-local release env that the failure used."
    echo "- \`seed.txt\`, \`input.txt\`, and \`solver_output.txt\` expose the exact preserved seed/input/output without digging into the case subdirectory."
    echo "- \`checker_result.txt\` preserves the helper-recorded checker outcome and the exact preserved-output validator command."
    echo "- \`checker_replay.stdout.txt\` and \`checker_replay.stderr.txt\` persist the direct recheck of the preserved \`in.txt\` and \`out.txt\`."
    echo "- \`mismatch_summary.txt\` extracts the concrete validator mismatch, including parsed query/expected/got fields when available."
    echo "- \`rerun_command.txt\` collects the exact rerun commands for preserved-input replay, seed repro, and checker replay."
    echo "- \`recheck_preserved_output.sh\` reruns the branch-local checker directly on the preserved \`in.txt\` and \`out.txt\`."
    echo "- \`repro_from_seed.sh\` regenerates the same seed into \`$CURRENT_CASE_REPRO_DIR\` without overwriting the preserved failure tree."
    echo "- \`replay_preserved_input.sh\` reruns the frozen solver directly on the preserved \`in.txt\` without regenerating the case."
    echo "- \`artifact_manifest.tsv\` records existence, size, and SHA-256 for every preserved debug artifact."
    if [[ -s "$checker_result_txt" ]]; then
      echo
      echo "## Checker Result Snapshot"
      echo
      echo "\`\`\`text"
      cat "$checker_result_txt"
      echo "\`\`\`"
    fi
    if [[ -s "$mismatch_summary_txt" ]]; then
      echo
      echo "## Mismatch Summary"
      echo
      echo "\`\`\`text"
      cat "$mismatch_summary_txt"
      echo "\`\`\`"
    fi
    if [[ -s "$failure_case_dir/time.txt" ]]; then
      echo
      echo "## Timing Artifact"
      echo
      echo "\`\`\`text"
      cat "$failure_case_dir/time.txt"
      echo "\`\`\`"
    fi
    if [[ -s "$failure_case_dir/solver_stderr.txt" ]]; then
      echo
      echo "## Solver stderr tail"
      echo
      echo "\`\`\`text"
      tail -n 20 "$failure_case_dir/solver_stderr.txt"
      echo "\`\`\`"
    fi
    if [[ -s "$CURRENT_CASE_STDERR" ]]; then
      echo
      echo "## Helper stderr tail"
      echo
      echo "\`\`\`text"
      tail -n 20 "$CURRENT_CASE_STDERR"
      echo "\`\`\`"
    fi
    if [[ -s "$CURRENT_CASE_STDOUT" ]]; then
      echo
      echo "## Helper stdout tail"
      echo
      echo "\`\`\`text"
      tail -n 20 "$CURRENT_CASE_STDOUT"
      echo "\`\`\`"
    fi
  } > "$failure_report"
}

populate_failure_repro_commands() {
  local failure_case_dir="$1"
  local solver_for_repro="${2:-$SOLVER}"
  local -a repro_cmd
  local -a replay_cmd
  local input_q=""
  local out_q=""
  local stderr_q=""
  local replay_prefix=""

  CURRENT_CASE_SOLVER_SNAPSHOT="$solver_for_repro"
  CURRENT_CASE_REPRO_DIR="$FAILURE_ROOT/repro_from_seed/$CURRENT_CASE_TAG"
  CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR="$FAILURE_ROOT/replay_from_input/$CURRENT_CASE_TAG"
  repro_cmd=(
    python3
    "$RUN_CASE_HELPER"
    "$CURRENT_CASE_MODE"
    "$CURRENT_CASE_N"
    "$CURRENT_CASE_SEED"
    "$CURRENT_CASE_SHUFFLE_LABELS"
    "$CURRENT_CASE_SHUFFLE_QUERIES"
    "$solver_for_repro"
    "$CURRENT_CASE_REPRO_DIR"
    --timeout
    "$CURRENT_CASE_TIMEOUT"
    --env
    "DENSE_SHADOW_CASE_MODE=$CURRENT_CASE_MODE"
    --env
    "DENSE_SHADOW_CASE_N=$CURRENT_CASE_N"
    --env
    "DENSE_SHADOW_CASE_SEED=$CURRENT_CASE_SEED"
    --env
    "DENSE_PROFILE_OUTDIR=$CURRENT_CASE_REPRO_DIR"
    --env
    DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
  )
  CURRENT_CASE_REPRO_COMMAND="$(quote_command "${repro_cmd[@]}")"
  replay_cmd=(
    env
    "DENSE_SHADOW_CASE_MODE=$CURRENT_CASE_MODE"
    "DENSE_SHADOW_CASE_N=$CURRENT_CASE_N"
    "DENSE_SHADOW_CASE_SEED=$CURRENT_CASE_SEED"
    "DENSE_PROFILE_OUTDIR=$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR"
    DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
    "$solver_for_repro"
  )
  replay_prefix="$(quote_command "${replay_cmd[@]}")"
  input_q="$(quote_word "$failure_case_dir/in.txt")"
  out_q="$(quote_word "$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR/out.txt")"
  stderr_q="$(quote_word "$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR/solver_stderr.txt")"
  CURRENT_CASE_PRESERVED_INPUT_COMMAND="mkdir -p $(quote_word "$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR") && ${replay_prefix} < ${input_q} > ${out_q} 2> ${stderr_q}"
  CURRENT_CASE_CHECKER_COMMAND="$(quote_command python3 "$CHECKER_HELPER" "$failure_case_dir/in.txt" "$failure_case_dir/out.txt")"
}

preserve_failure_artifacts() {
  local solver_for_repro="$SOLVER"
  local failure_case_dir=""

  if [[ -z "${WORKDIR:-}" || ! -d "$WORKDIR" ]]; then
    return 1
  fi
  if ! remove_path_retry "$FAILURE_ROOT"; then
    return 1
  fi
  if ! mkdir -p "$(dirname "$FAILURE_ROOT")"; then
    return 1
  fi
  if ! mv "$WORKDIR" "$FAILURE_ROOT"; then
    return 1
  fi
  WORKDIR=""
  failure_case_dir="$FAILURE_ROOT/$CURRENT_CASE_TAG"
  CURRENT_CASE_RESULT_JSON="$FAILURE_ROOT/$CURRENT_CASE_TAG/$RUN_CASE_RESULT_NAME"
  CURRENT_CASE_STDOUT="$FAILURE_ROOT/$CURRENT_CASE_TAG/run_case.stdout.txt"
  CURRENT_CASE_STDERR="$FAILURE_ROOT/$CURRENT_CASE_TAG/run_case.stderr.txt"
  CURRENT_CASE_SOLVER_SNAPSHOT="$FAILURE_ROOT/solver_snapshot"
  if [[ -x "$SOLVER" ]]; then
    if cp "$SOLVER" "$CURRENT_CASE_SOLVER_SNAPSHOT"; then
      chmod +x "$CURRENT_CASE_SOLVER_SNAPSHOT" || true
      solver_for_repro="$CURRENT_CASE_SOLVER_SNAPSHOT"
    else
      CURRENT_CASE_SOLVER_SNAPSHOT="$SOLVER"
    fi
  else
    CURRENT_CASE_SOLVER_SNAPSHOT="$SOLVER"
  fi
  populate_failure_repro_commands "$failure_case_dir" "$solver_for_repro"
}

report_failure_context() {
  local failure_case_dir="${1:-$FAILURE_ROOT/$CURRENT_CASE_TAG}"
  echo "[lca_smoke] smoke case failed with exit code $CURRENT_FAILURE_RC" >&2
  echo "[lca_smoke] helper exit code: $CURRENT_FAILURE_HELPER_RC" >&2
  echo "[lca_smoke] failure kind: $CURRENT_FAILURE_KIND origin=$CURRENT_FAILURE_ORIGIN retryable=$CURRENT_FAILURE_RETRYABLE summary=$CURRENT_FAILURE_SUMMARY" >&2
  echo "[lca_smoke] case index/attempt: index=$CURRENT_CASE_INDEX attempt=$CURRENT_CASE_ATTEMPT" >&2
  if [[ -n "$CURRENT_FAILURE_SOLVER_EXIT" ]]; then
    echo "[lca_smoke] solver exit code: $CURRENT_FAILURE_SOLVER_EXIT" >&2
  fi
  if [[ -n "$CURRENT_FAILURE_SIGNAL" ]]; then
    echo "[lca_smoke] solver signal: $CURRENT_FAILURE_SIGNAL" >&2
  fi
  echo "[lca_smoke] failed case: tag=$CURRENT_CASE_TAG stage=$CURRENT_CASE_STAGE mode=$CURRENT_CASE_MODE n=$CURRENT_CASE_N seed=$CURRENT_CASE_SEED shuffle_labels=$CURRENT_CASE_SHUFFLE_LABELS shuffle_queries=$CURRENT_CASE_SHUFFLE_QUERIES timeout_s=$CURRENT_CASE_TIMEOUT" >&2
  echo "[lca_smoke] manifest row: $CURRENT_CASE_MANIFEST_ROW" >&2
  echo "[lca_smoke] executed command: $CURRENT_CASE_EXEC_COMMAND" >&2
  echo "[lca_smoke] repro command: $CURRENT_CASE_REPRO_COMMAND" >&2
  echo "[lca_smoke] preserved-input replay command: $CURRENT_CASE_PRESERVED_INPUT_COMMAND" >&2
  echo "[lca_smoke] preserved failure root: $FAILURE_ROOT" >&2
  echo "[lca_smoke] preserved case dir: $failure_case_dir" >&2
  echo "[lca_smoke] helper stdout: $CURRENT_CASE_STDOUT" >&2
  echo "[lca_smoke] helper stderr: $CURRENT_CASE_STDERR" >&2
  echo "[lca_smoke] helper result json: $CURRENT_CASE_RESULT_JSON" >&2
  echo "[lca_smoke] exact seed snapshot: $FAILURE_ROOT/seed.txt" >&2
  echo "[lca_smoke] exact input snapshot: $FAILURE_ROOT/input.txt" >&2
  echo "[lca_smoke] exact output snapshot: $FAILURE_ROOT/solver_output.txt" >&2
  echo "[lca_smoke] checker result: $FAILURE_ROOT/checker_result.txt" >&2
  echo "[lca_smoke] checker replay stdout: $FAILURE_ROOT/checker_replay.stdout.txt" >&2
  echo "[lca_smoke] checker replay stderr: $FAILURE_ROOT/checker_replay.stderr.txt" >&2
  echo "[lca_smoke] mismatch summary: $FAILURE_ROOT/mismatch_summary.txt" >&2
  echo "[lca_smoke] rerun command snapshot: $FAILURE_ROOT/rerun_command.txt" >&2
  echo "[lca_smoke] checker command: $CURRENT_CASE_CHECKER_COMMAND" >&2
  echo "[lca_smoke] checker replay script: $FAILURE_ROOT/recheck_preserved_output.sh" >&2
  echo "[lca_smoke] solver snapshot: $CURRENT_CASE_SOLVER_SNAPSHOT" >&2
  echo "[lca_smoke] artifact manifest: $FAILURE_ROOT/artifact_manifest.tsv" >&2
  echo "[lca_smoke] seed repro script: $FAILURE_ROOT/repro_from_seed.sh" >&2
  echo "[lca_smoke] preserved-input replay script: $FAILURE_ROOT/replay_preserved_input.sh" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/in.txt" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/meta.json" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/hidden_parent.txt" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/out.txt" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/time.txt" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/solver_stderr.txt" >&2
  echo "[lca_smoke] stable smoke output root: $OUTROOT" >&2
  echo "[lca_smoke] smoke manifest: $SMOKE_CASES" >&2
  echo "[lca_smoke] failure summary: $FAILURE_ROOT/failure_summary.txt" >&2
  echo "[lca_smoke] failure report: $FAILURE_ROOT/latest_failure_report.md" >&2
  if [[ -s "$CURRENT_CASE_STDERR" ]]; then
    echo "[lca_smoke] helper stderr tail:" >&2
    tail -n 20 "$CURRENT_CASE_STDERR" >&2
  elif [[ -s "$CURRENT_CASE_STDOUT" ]]; then
    echo "[lca_smoke] helper stdout tail:" >&2
    tail -n 20 "$CURRENT_CASE_STDOUT" >&2
  fi
}

handle_signal() {
  local rc="$1"
  trap - HUP INT TERM
  exit "$rc"
}

build_solver_if_needed() {
  local build_rc=0
  local prior_errexit=1

  enter_function_errexit prior_errexit
  clear_stale_build_output_temps || fail "failed to clear stale smoke build temp outputs under $BUILD_ROOT"

  # Smoke should not inherit a previously built binary; rebuild from a clean
  # artifact path so repeated invocations start from a known solver state.
  if [[ -e "$BINARY" ]]; then
    remove_path_retry "$BINARY" || fail "failed to clear stale smoke build binary: $BINARY"
  fi

  printf '%s\n' "$(quote_command "$BUILD_WRAPPER")" > "$SETUP_BUILD_COMMAND"
  : > "$SETUP_BUILD_STDOUT"
  : > "$SETUP_BUILD_STDERR"

  if "$BUILD_WRAPPER" >"$SETUP_BUILD_STDOUT" 2>"$SETUP_BUILD_STDERR"; then
    :
  else
    build_rc=$?
    echo "[lca_smoke] build wrapper failed with exit code $build_rc" >&2
    write_setup_failure_summary "build" "$build_rc" "build wrapper failed before smoke cases started"
    report_setup_failure_context "build" "$build_rc" "build wrapper failed before smoke cases started"
    restore_function_errexit "$prior_errexit"
    return "$SMOKE_EXIT_HARNESS_FAILURE"
  fi
  require_executable "$BINARY" "solver binary"
  restore_function_errexit "$prior_errexit"
}

load_release_environment() {
  source "$RELEASE_ENV"
  assert_runtime_environment
}

run_smoke_case_once() {
  local case_index="$1"
  local attempt_index="$2"
  local stage="$3"
  local mode="$4"
  local n="$5"
  local seed="$6"
  local shuffle_labels="$7"
  local shuffle_queries="$8"
  local case_tag="$9"
  local timeout_s="${10}"
  local case_dir="$WORKDIR/$case_tag"
  local rc=0
  local prior_errexit=1
  local -a cmd

  enter_function_errexit prior_errexit
  if [[ -e "$case_dir" ]]; then
    remove_path_retry "$case_dir" || fail "failed to clear stale case directory before attempt: $case_dir"
  fi
  mkdir -p "$case_dir"
  CURRENT_CASE_INDEX="$case_index"
  CURRENT_CASE_ATTEMPT="$attempt_index"
  CURRENT_CASE_STAGE="$stage"
  CURRENT_CASE_MODE="$mode"
  CURRENT_CASE_N="$n"
  CURRENT_CASE_SEED="$seed"
  CURRENT_CASE_SHUFFLE_LABELS="$shuffle_labels"
  CURRENT_CASE_SHUFFLE_QUERIES="$shuffle_queries"
  CURRENT_CASE_TIMEOUT="$timeout_s"
  CURRENT_CASE_TAG="$case_tag"
  CURRENT_CASE_MANIFEST_ROW="$(printf '%s\t%s\t%s\t%s\t%s\t%s\t%s' "$stage" "$mode" "$n" "$seed" "$shuffle_labels" "$shuffle_queries" "$timeout_s")"
  CURRENT_CASE_STDOUT="$case_dir/run_case.stdout.txt"
  CURRENT_CASE_STDERR="$case_dir/run_case.stderr.txt"
  CURRENT_CASE_RESULT_JSON="$case_dir/$RUN_CASE_RESULT_NAME"
  CURRENT_CASE_REPRO_COMMAND=""
  CURRENT_CASE_PRESERVED_INPUT_COMMAND=""
  CURRENT_CASE_REPRO_DIR=""
  CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR=""
  CURRENT_CASE_SOLVER_SNAPSHOT="$SOLVER"
  CURRENT_CASE_CHECKER_COMMAND=""
  reset_failure_classification
  cmd=(
    python3
    "$RUN_CASE_HELPER"
    "$mode"
    "$n"
    "$seed"
    "$shuffle_labels"
    "$shuffle_queries"
    "$SOLVER"
    "$case_dir"
    --timeout
    "$timeout_s"
    --env
    "DENSE_SHADOW_CASE_MODE=$mode"
    --env
    "DENSE_SHADOW_CASE_N=$n"
    --env
    "DENSE_SHADOW_CASE_SEED=$seed"
    --env
    "DENSE_PROFILE_OUTDIR=$case_dir"
    --env
    DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
  )
  CURRENT_CASE_EXEC_COMMAND="$(quote_command "${cmd[@]}")"

  if "${cmd[@]}" >"$CURRENT_CASE_STDOUT" 2>"$CURRENT_CASE_STDERR"; then
    restore_function_errexit "$prior_errexit"
    return 0
  fi

  rc=$?
  CURRENT_FAILURE_RC="$rc"
  classify_case_failure "$CURRENT_FAILURE_RC"
  populate_failure_repro_commands "$case_dir" "$SOLVER"
  restore_function_errexit "$prior_errexit"
  return "$CURRENT_FAILURE_RC"
}

run_smoke_case_with_retry() {
  local case_index="$1"
  local stage="$2"
  local mode="$3"
  local n="$4"
  local seed="$5"
  local shuffle_labels="$6"
  local shuffle_queries="$7"
  local case_tag="$8"
  local timeout_s="$9"
  local max_attempts=$(( SMOKE_CASE_RETRY_LIMIT + 1 ))
  local attempt_index=0
  local case_dir="$WORKDIR/$case_tag"
  local failure_case_dir="$case_dir"
  local rc=0
  local prior_errexit=1

  enter_function_errexit prior_errexit
  for (( attempt_index = 1; attempt_index <= max_attempts; ++attempt_index )); do
    set +e
    run_smoke_case_once \
      "$case_index" \
      "$attempt_index" \
      "$stage" \
      "$mode" \
      "$n" \
      "$seed" \
      "$shuffle_labels" \
      "$shuffle_queries" \
      "$case_tag" \
      "$timeout_s"
    rc=$?
    set -e
    if (( rc == 0 )); then
      restore_function_errexit "$prior_errexit"
      return 0
    fi

    record_retry_attempt
    if (( CURRENT_FAILURE_RETRYABLE == 0 || attempt_index >= max_attempts )); then
      if preserve_failure_artifacts; then
        failure_case_dir="$FAILURE_ROOT/$case_tag"
      else
        echo "[lca_smoke] warning: failed to promote preserved failure bundle; keeping staging tree at $case_dir" >&2
        WORKDIR=""
      fi
      if ! write_failure_summary "$failure_case_dir"; then
        echo "[lca_smoke] warning: failed to write failure summary/report; classified case exit remains $CURRENT_FAILURE_RC" >&2
      fi
      report_failure_context "$failure_case_dir"
      restore_function_errexit "$prior_errexit"
      return "$CURRENT_FAILURE_RC"
    fi

    echo "[lca_smoke] retrying harness-transient failure: case=$case_tag index=$case_index attempt=$(( attempt_index + 1 ))/$max_attempts seed=$seed timeout_s=$timeout_s" >&2
    if [[ "$SMOKE_RETRY_SLEEP_S" != "0" && "$SMOKE_RETRY_SLEEP_S" != "0.0" && "$SMOKE_RETRY_SLEEP_S" != "0.00" ]]; then
      sleep "$SMOKE_RETRY_SLEEP_S"
    fi
  done

  restore_function_errexit "$prior_errexit"
  return "$CURRENT_FAILURE_RC"
}

run_smoke_suite() {
  local case_index=0
  local rc=0
  local prior_errexit=1

  enter_function_errexit prior_errexit
  for (( case_index = 0; case_index < SMOKE_PLAN_COUNT; ++case_index )); do
    set +e
    run_smoke_case_with_retry \
      "$(( case_index + 1 ))" \
      "${SMOKE_PLAN_STAGE[$case_index]}" \
      "${SMOKE_PLAN_MODE[$case_index]}" \
      "${SMOKE_PLAN_N[$case_index]}" \
      "${SMOKE_PLAN_SEED[$case_index]}" \
      "${SMOKE_PLAN_SHUFFLE_LABELS[$case_index]}" \
      "${SMOKE_PLAN_SHUFFLE_QUERIES[$case_index]}" \
      "${SMOKE_PLAN_TAG[$case_index]}" \
      "${SMOKE_PLAN_TIMEOUT[$case_index]}"
    rc=$?
    set -e
    if (( rc == 0 )); then
      continue
    fi
    restore_function_errexit "$prior_errexit"
    return "$rc"
  done
  restore_function_errexit "$prior_errexit"
}

publish_output() {
  local outleaf="${OUTROOT##*/}"

  if [[ ! -d "$WORKDIR" ]]; then
    fail "staging output directory disappeared before publish: $WORKDIR"
  fi

  mkdir -p "$OUTPARENT"
  if [[ -e "$OUTROOT" ]]; then
    remove_path_retry "$BACKUP_ROOT" || fail "failed to clear backup path before publish: $BACKUP_ROOT"
    mv "$OUTROOT" "$BACKUP_ROOT"
  fi
  mv "$WORKDIR" "$OUTPARENT/$outleaf"
  WORKDIR=""
  remove_path_retry "$BACKUP_ROOT" || fail "failed to clear backup path after publish: $BACKUP_ROOT"
}

trap 'cleanup "$?"' EXIT
trap 'handle_signal 129' HUP
trap 'handle_signal 130' INT
trap 'handle_signal 143' TERM

run_main() {
  local smoke_rc=0

  sanitize_shell_state
  mkdir -p "$ARTIFACTS_ROOT"
  resolve_output_roots
  configure_runtime_environment
  configure_deterministic_smoke_controls
  acquire_lock

  if [[ -e "$OUTROOT" && ! -d "$OUTROOT" ]]; then
    fail "output path exists but is not a directory: $OUTROOT"
  fi
  if [[ -e "$BACKUP_ROOT" && ! -d "$BACKUP_ROOT" ]]; then
    fail "backup path exists but is not a directory: $BACKUP_ROOT"
  fi

  mkdir -p "$OUTPARENT" "$TMP_PARENT" "$RUN_STAGE_ROOT" "$BUILD_ROOT"
  clear_stale_state
  clear_stale_failure_state
  prepare_setup_root
  prepare_session_environment_state
  configure_setup_tmpdir

  set +e
  run_setup_preflight
  smoke_rc=$?
  set -e
  if (( smoke_rc != 0 )); then
    return "$smoke_rc"
  fi

  set +e
  build_solver_if_needed
  smoke_rc=$?
  set -e
  if (( smoke_rc != 0 )); then
    return "$smoke_rc"
  fi

  WORKDIR="$(mktemp -d "$RUN_STAGE_ROOT/$RUN_WORK_TEMPLATE")"
  if [[ -z "$WORKDIR" ]]; then
    fail "mktemp returned an empty smoke staging directory"
  fi
  ensure_under_artifacts "$WORKDIR"
  configure_runtime_tmpdir
  prepare_retry_log
  load_smoke_plan
  write_smoke_suite_metadata
  load_release_environment
  write_environment_validation_bundle

  set +e
  run_smoke_suite
  smoke_rc=$?
  set -e
  if (( smoke_rc != 0 )); then
    return "$smoke_rc"
  fi
  publish_output
  clear_stale_failure_state
}

main() {
  if (( $# != 0 )); then
    usage
  fi

  require_command bash
  require_command python3
  require_command mktemp
  require_command dirname
  require_command chmod
  require_command cp
  require_command mv
  require_command rm
  require_command tail
  require_command sleep
  require_command grep
  require_file "$SOURCE" "solver source"
  require_file "$ARTIFACT_RESOLVER" "artifact resolver"
  require_file "$RELEASE_ENV" "release env wrapper"
  require_file "$RUN_CASE_HELPER" "branch-local case helper"
  require_file "$CHECKER_HELPER" "branch-local validator"
  require_file "$SMOKE_CASES" "smoke case manifest"
  require_executable "$BUILD_WRAPPER" "build wrapper"

  run_main
}

main "$@"
