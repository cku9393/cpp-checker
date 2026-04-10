#!/bin/zsh
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${(%):-%N}")" && pwd -P)"
branch_root="$(cd -- "$script_dir/.." && pwd -P)"
cd "$branch_root"
export PYTHONDONTWRITEBYTECODE=1

branch_root="$PWD"
artifact_resolver="$branch_root/artifact_paths.py"
seed_file="${1:-.ouroboros/seed_branch3_progress40_research_loop.yaml}"
analysis_seed_file="${2:-.ouroboros/seed_branch3_failure_analysis.yaml}"
start_attempt_raw="${3:-${RETRY_LOOP_START_ATTEMPT:-}}"
report_root="artifacts/lca_tree_stress_v5/retry_loop"
retry_tmp_parent="$branch_root/artifacts/lca_tree_stress_v5/.tmp"
prepare_state_helper=".ouroboros/prepare_retry_attempt_state.py"
runtime_snapshot_helper=".ouroboros/snapshot_retry_runtime.py"
retry_input_snapshot_helper=".ouroboros/snapshot_retry_inputs.py"
quota_watch_helper=".ouroboros/monitor_codex_quota.py"
retry_outcome_helper=".ouroboros/classify_retry_loop_outcome.py"
retry_runtime_env_root=""
soft_stop_file=""
attempt=1
max_analysis_rounds="${MAX_ANALYSIS_ROUNDS:-3}"
next_probe_timeout_seconds="${NEXT_PROBE_TIMEOUT_SECONDS:-180}"
quota_watch_poll_seconds="${QUOTA_WATCH_POLL_SECONDS:-1}"
soft_stop_poll_seconds="${SOFT_STOP_POLL_SECONDS:-1}"
runtime_snapshot_interval_seconds="${RUNTIME_SNAPSHOT_INTERVAL_SECONDS:-30}"
quota_primary_remaining_threshold="${QUOTA_PRIMARY_REMAINING_THRESHOLD_PERCENT:-1}"
quota_secondary_remaining_threshold="${QUOTA_SECONDARY_REMAINING_THRESHOLD_PERCENT:-1}"
quota_pause_retry_seconds="${QUOTA_PAUSE_RETRY_SECONDS:-300}"
quota_pause_poll_seconds="${QUOTA_PAUSE_POLL_SECONDS:-60}"
codex_sessions_root="${CODEX_SESSIONS_ROOT:-$HOME/.codex/sessions}"
soft_stop_exit_code=91
output_locality_escape_exit_code=92
output_locality_guard_error_exit_code=93

fail() {
  echo "[run_until_pass_progress40] $*" >&2
  exit 1
}

ensure_artifact_path() {
  local raw_path="$1"
  local resolved=""
  if ! resolved="$(python3 "$artifact_resolver" --ensure "$raw_path")"; then
    fail "path must stay under branch-local artifacts: $raw_path"
  fi
  printf '%s\n' "$resolved"
}

remove_artifact_path() {
  local target_path="$1"
  if [[ -d "$target_path" && ! -L "$target_path" ]]; then
    rm -rf "$target_path" 2>/dev/null || true
    return
  fi
  rm -f "$target_path" 2>/dev/null || rm -rf "$target_path" 2>/dev/null || true
}

prepare_artifact_file_target() {
  local target_path="$1"
  local parent_dir="${target_path:h}"
  if [[ -e "$parent_dir" && ! -d "$parent_dir" ]]; then
    remove_artifact_path "$parent_dir"
  fi
  mkdir -p "$parent_dir"
  if [[ -e "$target_path" && -d "$target_path" ]]; then
    remove_artifact_path "$target_path"
  elif [[ -L "$target_path" ]]; then
    remove_artifact_path "$target_path"
  fi
}

copy_artifact_file() {
  local source_path="$1"
  local target_path="$2"
  prepare_artifact_file_target "$target_path"
  cp "$source_path" "$target_path"
}

sanitize_artifact_label() {
  local raw_label="${1:-run}"
  local sanitized=""
  sanitized="$(printf '%s' "$raw_label" | LC_ALL=C tr -cs 'A-Za-z0-9._-' '_')"
  sanitized="${sanitized##_}"
  sanitized="${sanitized%%_}"
  if [[ -z "$sanitized" ]]; then
    sanitized="run"
  fi
  printf '%s\n' "$sanitized"
}

verify_workflow_output_locality() {
  local status_prefix="$1"
  local workflow_log="$2"
  local non_artifact_baseline="$3"
  local non_artifact_current="$4"
  local non_artifact_report="$5"
  local latest_current=""
  local latest_report=""
  local label=""
  local verify_exit_code=0
  local had_errexit=0

  label="$(sanitize_artifact_label "$status_prefix")"
  latest_current="$(ensure_artifact_path "$report_root/latest_${label}_non_artifact_tree_current.json")"
  latest_report="$(ensure_artifact_path "$report_root/latest_${label}_non_artifact_tree_report.txt")"

  if [[ -o errexit ]]; then
    had_errexit=1
  fi

  set +e
  python3 "$artifact_resolver" \
    --verify-non-artifact-tree \
    "$non_artifact_baseline" \
    "$non_artifact_current" \
    "$non_artifact_report" >> "$workflow_log" 2>&1
  verify_exit_code=$?
  if (( had_errexit == 1 )); then
    set -e
  fi

  copy_artifact_file "$non_artifact_current" "$latest_current" 2>/dev/null || true
  copy_artifact_file "$non_artifact_report" "$latest_report" 2>/dev/null || true

  if (( verify_exit_code == 0 )); then
    return 0
  fi

  if (( verify_exit_code == 3 )); then
    printf '[%s] attempt %d %s generated non-artifact output outside branch-local artifacts; see %s\n' \
      "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$attempt" "$status_prefix" "$latest_report" | tee -a "$workflow_log"
    return "$output_locality_escape_exit_code"
  fi

  printf '[%s] attempt %d output locality guard failed while verifying %s; see %s\n' \
    "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$attempt" "$status_prefix" "$latest_report" | tee -a "$workflow_log"
  return "$output_locality_guard_error_exit_code"
}

next_attempt_number() {
  python3 - "$1" <<'PY'
import re
import sys
from pathlib import Path

report_root = Path(sys.argv[1])
pattern = re.compile(r"attempt_(\d+)_")
max_attempt = 0
if report_root.exists():
    for child in report_root.iterdir():
        match = pattern.match(child.name)
        if match:
            max_attempt = max(max_attempt, int(match.group(1)))
print(max_attempt + 1)
PY
}

cleanup_retry_runtime_environment() {
  if [[ -n "${retry_runtime_env_root:-}" && -e "$retry_runtime_env_root" ]]; then
    rm -rf "$retry_runtime_env_root"
  fi
  retry_runtime_env_root=""
  unset BRANCH_ARTIFACT_TMP_ROOT TMPDIR TMP TEMP PYTHONPYCACHEPREFIX
}

configure_retry_runtime_environment() {
  local retry_tmpdir=""
  local retry_pycache_root=""

  mkdir -p "$retry_tmp_parent"
  retry_runtime_env_root="$(mktemp -d "$retry_tmp_parent/retry_loop.runtime.env.XXXXXX")"
  if [[ -z "$retry_runtime_env_root" ]]; then
    fail "mktemp returned an empty retry-loop runtime root"
  fi
  retry_runtime_env_root="$(ensure_artifact_path "$retry_runtime_env_root")"
  retry_tmpdir="$(ensure_artifact_path "$retry_runtime_env_root/tmp")"
  retry_pycache_root="$(ensure_artifact_path "$retry_runtime_env_root/pycache")"

  mkdir -p \
    "$retry_tmpdir" \
    "$retry_pycache_root"

  # Preserve the user's HOME-backed Codex auth/session roots while routing
  # retry-loop scratch files and bytecode under branch-local artifacts.
  export BRANCH_ARTIFACT_TMP_ROOT="$retry_tmpdir"
  export TMPDIR="$retry_tmpdir"
  export TMP="$retry_tmpdir"
  export TEMP="$retry_tmpdir"
  export PYTHONPYCACHEPREFIX="$retry_pycache_root"
}

report_root="$(ensure_artifact_path "$report_root")"
retry_tmp_parent="$(ensure_artifact_path "$retry_tmp_parent")"
soft_stop_file="$report_root/soft_stop_request.json"
if [[ -n "$start_attempt_raw" ]]; then
  if ! [[ "$start_attempt_raw" == <-> ]] || (( start_attempt_raw < 1 )); then
    fail "RETRY_LOOP_START_ATTEMPT/start attempt must be a positive integer: $start_attempt_raw"
  fi
  attempt="$start_attempt_raw"
else
  attempt="$(next_attempt_number "$report_root")"
fi

trap cleanup_retry_runtime_environment EXIT

mkdir -p "$report_root"

reset_attempt_dir() {
  if [[ -e "$attempt_dir" ]]; then
    rm -rf "$attempt_dir"
  fi
  mkdir -p "$attempt_dir"
}

publish_runtime_snapshot() {
  local status_label="$1"
  local current_log="${2:-$attempt_log}"
  local write_pause_state="${3:-0}"
  local -a cmd
  cmd=(
    python3 "$runtime_snapshot_helper"
    --branch-root "$PWD" \
    --attempt-dir "$attempt_dir" \
    --report-root "$report_root" \
    --attempt-log "$attempt_log" \
    --current-log "$current_log" \
    --seed-file "$seed_file" \
    --analysis-seed-file "$analysis_seed_file" \
    --status-label "$status_label" \
    --loop-pid "$$" \
    --soft-stop-file "$soft_stop_file" \
  )
  if [[ -n "${workflow_pid:-}" ]]; then
    cmd+=(--workflow-pid "$workflow_pid")
  fi
  if [[ -n "${quota_watchdog_pid:-}" ]]; then
    cmd+=(--quota-watchdog-pid "$quota_watchdog_pid")
  fi
  if [[ "$write_pause_state" == "1" ]]; then
    cmd+=(--write-pause-state)
  fi
  "${cmd[@]}" >/dev/null 2>&1 || true
}

stop_quota_watchdog() {
  if [[ -n "${quota_watchdog_pid:-}" ]] && kill -0 "$quota_watchdog_pid" 2>/dev/null; then
    kill "$quota_watchdog_pid" 2>/dev/null || true
    wait "$quota_watchdog_pid" 2>/dev/null || true
  fi
  quota_watchdog_pid=""
}

run_quota_guarded_workflow() {
  local workflow_seed="$1"
  local workflow_log="$2"
  local status_prefix="$3"
  local workflow_pid_local=""
  local snapshot_elapsed=0
  local had_errexit=0
  local locality_label=""
  local locality_root=""
  local non_artifact_baseline=""
  local non_artifact_current=""
  local non_artifact_report=""

  if [[ -o errexit ]]; then
    had_errexit=1
  fi

  if [[ -f "$soft_stop_file" ]]; then
    return "$soft_stop_exit_code"
  fi

  locality_label="$(sanitize_artifact_label "$status_prefix")"
  locality_root="$(ensure_artifact_path "$attempt_dir/.workflow_output_locality/$locality_label")"
  non_artifact_baseline="$(ensure_artifact_path "$locality_root/non_artifact_tree_baseline.json")"
  non_artifact_current="$(ensure_artifact_path "$locality_root/non_artifact_tree_current.json")"
  non_artifact_report="$(ensure_artifact_path "$locality_root/non_artifact_tree_report.txt")"
  mkdir -p "$locality_root"

  if ! python3 "$artifact_resolver" --snapshot-non-artifact-tree "$non_artifact_baseline" >> "$workflow_log" 2>&1; then
    printf '[%s] attempt %d output locality guard could not capture a baseline for %s; aborting workflow launch\n' \
      "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$attempt" "$status_prefix" | tee -a "$workflow_log"
    return "$output_locality_guard_error_exit_code"
  fi

  set +e
  ouroboros run workflow "$workflow_seed" --runtime codex >> "$workflow_log" 2>&1 &
  workflow_pid_local=$!
  if (( had_errexit == 1 )); then
    set -e
  fi

  workflow_pid="$workflow_pid_local"
  quota_watchdog_pid=""
  publish_runtime_snapshot "${status_prefix}_started" "$workflow_log" 0

  python3 "$quota_watch_helper" \
    --branch-root "$PWD" \
    --attempt-dir "$attempt_dir" \
    --report-root "$report_root" \
    --soft-stop-file "$soft_stop_file" \
    --codex-sessions-root "$codex_sessions_root" \
    --poll-seconds "$quota_watch_poll_seconds" \
    --primary-remaining-threshold "$quota_primary_remaining_threshold" \
    --secondary-remaining-threshold "$quota_secondary_remaining_threshold" \
    >> "$workflow_log" 2>&1 &
  quota_watchdog_pid=$!
  publish_runtime_snapshot "${status_prefix}_watching" "$workflow_log" 0

  while kill -0 "$workflow_pid_local" 2>/dev/null; do
    if (( snapshot_elapsed == 0 )); then
      publish_runtime_snapshot "${status_prefix}_running" "$workflow_log" 0
    fi
    if [[ -f "$soft_stop_file" ]]; then
      printf '[%s] attempt %d soft stop requested; terminating workflow pid %s for %s\n' \
        "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$attempt" "$workflow_pid_local" "$status_prefix" | tee -a "$workflow_log"
      kill -TERM "$workflow_pid_local" 2>/dev/null || true
      for _ in {1..30}; do
        if ! kill -0 "$workflow_pid_local" 2>/dev/null; then
          break
        fi
        sleep 1
      done
      if kill -0 "$workflow_pid_local" 2>/dev/null; then
        kill -KILL "$workflow_pid_local" 2>/dev/null || true
      fi
      break
    fi
    sleep "$soft_stop_poll_seconds"
    snapshot_elapsed=$((snapshot_elapsed + soft_stop_poll_seconds))
    if (( snapshot_elapsed >= runtime_snapshot_interval_seconds )); then
      snapshot_elapsed=0
    fi
  done

  set +e
  wait "$workflow_pid_local"
  local workflow_exit_code=$?
  if (( had_errexit == 1 )); then
    set -e
  fi

  stop_quota_watchdog
  publish_runtime_snapshot "${status_prefix}_finished" "$workflow_log" 0
  workflow_pid=""

  set +e
  verify_workflow_output_locality \
    "$status_prefix" \
    "$workflow_log" \
    "$non_artifact_baseline" \
    "$non_artifact_current" \
    "$non_artifact_report"
  local output_locality_exit_code=$?
  if (( had_errexit == 1 )); then
    set -e
  fi
  if (( output_locality_exit_code != 0 )); then
    return "$output_locality_exit_code"
  fi

  if [[ -f "$soft_stop_file" ]]; then
    return "$soft_stop_exit_code"
  fi
  return "$workflow_exit_code"
}

compute_quota_pause_wait_seconds() {
  python3 - "$report_root/latest_quota_watch_status.json" "$soft_stop_file" "$quota_pause_retry_seconds" <<'PY'
import json
import sys
import time
from pathlib import Path

latest_status = Path(sys.argv[1])
soft_stop = Path(sys.argv[2])
fallback = max(60, int(sys.argv[3]))

payload = None
for candidate in (latest_status, soft_stop):
    if not candidate.exists():
        continue
    try:
        payload = json.loads(candidate.read_text(encoding="utf-8"))
        break
    except Exception:
        continue

wait_seconds = fallback
if payload:
    now = time.time()
    triggered_limits = payload.get("triggered_limits") or []
    candidates: list[int] = []

    primary_reset = (payload.get("primary") or {}).get("resets_at") or payload.get("primary_resets_at")
    secondary_reset = (payload.get("secondary") or {}).get("resets_at") or payload.get("secondary_resets_at")

    if "primary_5h" in triggered_limits and primary_reset:
        candidates.append(max(0, int(float(primary_reset) - now)) + 60)
    if "secondary_1w" in triggered_limits and secondary_reset:
        candidates.append(max(0, int(float(secondary_reset) - now)) + 60)

    if candidates:
        wait_seconds = max(60, min(candidates))

print(wait_seconds)
PY
}

handle_soft_stop_and_wait() {
  local current_log="${1:-$attempt_log}"
  local wait_seconds=""
  local remaining_seconds=0
  local sleep_chunk=0

  publish_runtime_snapshot "quota_pause" "$current_log" 1
  copy_artifact_file "$attempt_log" "$report_root/latest_workflow.log" 2>/dev/null || true
  wait_seconds="$(compute_quota_pause_wait_seconds)"
  printf '[%s] attempt %d paused due to soft stop request; keeping retry loop alive and waiting %ss for quota recovery before the next attempt\n' \
    "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$attempt" "$wait_seconds" | tee -a "$current_log" "$attempt_log"
  rm -f "$soft_stop_file"
  remaining_seconds="$wait_seconds"
  while (( remaining_seconds > 0 )); do
    sleep_chunk="$quota_pause_poll_seconds"
    if (( sleep_chunk > remaining_seconds )); then
      sleep_chunk="$remaining_seconds"
    fi
    sleep "$sleep_chunk"
    remaining_seconds=$((remaining_seconds - sleep_chunk))
    if (( remaining_seconds > 0 )) && (( remaining_seconds % 600 == 0 || remaining_seconds < quota_pause_poll_seconds )); then
      printf '[%s] quota recovery wait in progress; approximately %ss remaining before the next retry attempt\n' \
        "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$remaining_seconds" | tee -a "$current_log" "$attempt_log"
    fi
  done
  printf '[%s] quota recovery wait finished; preparing the next retry attempt\n' \
    "$(date '+%Y-%m-%d %H:%M:%S %Z')" | tee -a "$current_log" "$attempt_log"
}

validate_analysis_seed() {
  python3 - "$analysis_seed_file" <<'PY'
import sys
from pathlib import Path

import yaml

seed_path = Path(sys.argv[1])
payload = yaml.safe_load(seed_path.read_text())
if not isinstance(payload, dict):
    raise SystemExit("analysis seed preflight failed: seed is not a mapping")
if not payload.get("ontology_schema"):
    raise SystemExit("analysis seed preflight failed: missing ontology_schema")
if not payload.get("acceptance_criteria"):
    raise SystemExit("analysis seed preflight failed: missing acceptance_criteria")
print(f"analysis seed preflight ok: {seed_path}")
PY
}

retryable_failure_field() {
  local workflow_log="$1"
  local field_name="$2"
  python3 "$retry_outcome_helper" \
    --workflow-log "$workflow_log" \
    --attempt-number "$attempt" \
    --field "$field_name"
}

while true; do
  cleanup_retry_runtime_environment
  timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
  attempt_stamp="$(date '+%Y%m%d_%H%M%S')"
  attempt_dir="$report_root/attempt_$(printf '%03d' "$attempt")_${attempt_stamp}"
  attempt_log="$attempt_dir/workflow.log"
  reset_attempt_dir
  workflow_pid=""
  quota_watchdog_pid=""
  soft_stop_restart_requested=0
  stale_soft_stop_request_cleared=0

  if [[ -f "$soft_stop_file" ]]; then
    stale_soft_stop_request_cleared=1
  fi

  if ! python3 "$prepare_state_helper" \
    --branch-root "$PWD" \
    --attempt-dir "$attempt_dir" \
    --report-root "$report_root" >> "$attempt_log" 2>&1; then
    timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d stale-state preflight failed; aborting retry loop before workflow start\n' \
      "$timestamp" "$attempt" | tee -a "$attempt_log"
    exit 4
  fi
  configure_retry_runtime_environment
  if (( stale_soft_stop_request_cleared == 1 )); then
    printf '[%s] attempt %d cleared a stale soft stop request from a prior quota pause before starting the new workflow\n' \
      "$timestamp" "$attempt" | tee -a "$attempt_log"
  fi
  printf '[%s] attempt %d start: %s\n' "$timestamp" "$attempt" "$seed_file" | tee -a "$attempt_log"
  if ! python3 "$retry_input_snapshot_helper" \
    --branch-root "$PWD" \
    --attempt-dir "$attempt_dir" \
    --report-root "$report_root" \
    --seed-file "$seed_file" \
    --analysis-seed-file "$analysis_seed_file" \
    --attempt-number "$attempt" >> "$attempt_log" 2>&1; then
    timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d retry-input snapshot failed; aborting retry loop before workflow start\n' \
      "$timestamp" "$attempt" | tee -a "$attempt_log"
    exit 5
  fi

  python3 .ouroboros/git_repo_health.py \
    --branch-root "$PWD" \
    --attempt-dir "$attempt_dir" \
    --report-root "$report_root" \
    --phase pre_attempt >> "$attempt_log" 2>&1 || true

  set +e
  run_quota_guarded_workflow "$seed_file" "$attempt_log" "solver_attempt"
  exit_code=$?
  set -e

  if (( exit_code == soft_stop_exit_code )); then
    handle_soft_stop_and_wait "$attempt_log"
    attempt=$((attempt + 1))
    continue
  fi
  if (( exit_code == output_locality_escape_exit_code )); then
    timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d output locality guard detected generated output outside branch-local artifacts; aborting retry loop\n' \
      "$timestamp" "$attempt" | tee -a "$attempt_log"
    copy_artifact_file "$attempt_log" "$report_root/latest_workflow.log" 2>/dev/null || true
    exit 6
  fi
  if (( exit_code == output_locality_guard_error_exit_code )); then
    timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d output locality guard failed before a retry decision could be trusted; aborting retry loop\n' \
      "$timestamp" "$attempt" | tee -a "$attempt_log"
    copy_artifact_file "$attempt_log" "$report_root/latest_workflow.log" 2>/dev/null || true
    exit 7
  fi

  set +e
  python3 .ouroboros/post_attempt_guard.py \
    --branch-root "$PWD" \
    --workflow-log "$attempt_log" \
    --attempt-dir "$attempt_dir" \
    --report-root "$report_root" >> "$attempt_log" 2>&1
  guard_exit_code=$?
  set -e
  if (( exit_code == 0 && guard_exit_code != 0 )); then
    timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d guard rejected a nominal PASS; converting to retryable failure\n' \
      "$timestamp" "$attempt" | tee -a "$attempt_log"
    exit_code=90
  fi

  retryable_failure_reason=""
  if retryable_failure_flag="$(retryable_failure_field "$attempt_log" retryable_intermediate_failure 2>/dev/null)" \
    && [[ "$retryable_failure_flag" == "true" ]]; then
    retryable_failure_reason="$(retryable_failure_field "$attempt_log" reason 2>/dev/null || true)"
  fi

  if (( exit_code == 0 )); then
    if [[ -n "$retryable_failure_reason" ]]; then
      timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
      printf '[%s] attempt %d nominal workflow success still contained retryable intermediate acceptance failure (%s); converting to refinement cycle\n' \
        "$timestamp" "$attempt" "$retryable_failure_reason" | tee -a "$attempt_log"
      exit_code=90
    fi
  fi

  if (( exit_code == 0 )); then
    timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d succeeded\n' "$timestamp" "$attempt" | tee -a "$attempt_log"
    copy_artifact_file "$attempt_log" "$report_root/latest_workflow.log"
    break
  fi

  if [[ -n "$retryable_failure_reason" ]]; then
    timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d recorded a retryable intermediate acceptance failure (%s); starting analysis/refinement cycle\n' \
      "$timestamp" "$attempt" "$retryable_failure_reason" | tee -a "$attempt_log"
  fi

  python3 .ouroboros/git_repo_health.py \
    --branch-root "$PWD" \
    --attempt-dir "$attempt_dir" \
    --report-root "$report_root" \
    --phase post_failure >> "$attempt_log" 2>&1 || true

  if ! python3 .ouroboros/capture_failure_context.py \
    --attempt "$attempt" \
    --seed-file "$seed_file" \
    --workflow-log "$attempt_log" \
    --report-root "$report_root" \
    --exit-code "$exit_code"; then
    printf '[%s] attempt %d triage capture failed\n' "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$attempt" | tee -a "$attempt_log"
  fi

  analysis_baseline="$(python3 - <<'PY'
from datetime import datetime
print(datetime.now().timestamp())
PY
)"
  if ! validate_analysis_seed >> "$attempt_log" 2>&1; then
    printf '[%s] attempt %d analysis seed preflight failed; aborting retry loop before analysis rounds\n' \
      "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$attempt" | tee -a "$attempt_log"
    exit 2
  fi
  analysis_round=1
  analysis_verified=0
  while true; do
    analysis_timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    analysis_log="$attempt_dir/analysis_workflow_round_$(printf '%02d' "$analysis_round").log"
    printf '[%s] attempt %d analysis round %d start: %s\n' \
      "$analysis_timestamp" "$attempt" "$analysis_round" "$analysis_seed_file" | tee -a "$analysis_log"

    set +e
    run_quota_guarded_workflow "$analysis_seed_file" "$analysis_log" "analysis_round_${analysis_round}"
    analysis_exit_code=$?
    set -e

    if (( analysis_exit_code == soft_stop_exit_code )); then
      handle_soft_stop_and_wait "$analysis_log"
      soft_stop_restart_requested=1
      break
    fi
    if (( analysis_exit_code == output_locality_escape_exit_code )); then
      analysis_timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
      printf '[%s] attempt %d analysis round %d generated non-artifact output outside branch-local artifacts; aborting retry loop\n' \
        "$analysis_timestamp" "$attempt" "$analysis_round" | tee -a "$analysis_log" "$attempt_log"
      exit 6
    fi
    if (( analysis_exit_code == output_locality_guard_error_exit_code )); then
      analysis_timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
      printf '[%s] attempt %d analysis round %d output locality guard failed before the next retry could be trusted; aborting retry loop\n' \
        "$analysis_timestamp" "$attempt" "$analysis_round" | tee -a "$analysis_log" "$attempt_log"
      exit 7
    fi

    analysis_refresh_verified=0
    if python3 .ouroboros/capture_failure_context.py \
      --attempt "$attempt" \
      --seed-file "$seed_file" \
      --workflow-log "$attempt_log" \
      --report-root "$report_root" \
      --exit-code "$exit_code" >> "$analysis_log" 2>&1 \
      && python3 .ouroboros/refresh_analysis_state.py \
      --attempt "$attempt" \
      --attempt-dir "$attempt_dir" \
      --report-root "$report_root" \
      --analysis-log "$analysis_log" \
      --analysis-round "$analysis_round" \
      --state-file ".ouroboros/failure_analysis_state.json" \
      --iteration-file ".ouroboros/failure_analysis_iteration.md" >> "$analysis_log" 2>&1 \
      && python3 .ouroboros/verify_analysis_refresh.py \
      --baseline-epoch "$analysis_baseline" \
      --analysis-log "$analysis_log" \
      --target ".ouroboros/capture_failure_context.py" \
      --target ".ouroboros/failure_analysis_playbook.md" \
      --target ".ouroboros/failure_analysis_iteration.md" \
      --target ".ouroboros/failure_analysis_state.json" \
      --target ".ouroboros/refresh_analysis_state.py" \
      --target ".ouroboros/verify_analysis_refresh.py" \
      --latest-failure-report "$attempt_dir/failure_report.json" \
      --latest-failure-breakdown "$attempt_dir/failure_breakdown.json" \
      --require-current-state ".ouroboros/failure_analysis_state.json" \
      --require-json-key ".ouroboros/failure_analysis_state.json:current_for_latest_failure" \
      --require-json-key ".ouroboros/failure_analysis_state.json:current_failure_attempt" \
      --require-json-key ".ouroboros/failure_analysis_state.json:current_failure_signature" \
      --require-json-key ".ouroboros/failure_analysis_state.json:pinned_primary_axis" \
      --require-json-key ".ouroboros/failure_analysis_state.json:next_probe_command" \
      --require-json-key ".ouroboros/failure_analysis_state.json:why_this_axis" >> "$analysis_log" 2>&1; then
      analysis_refresh_verified=1
    fi

    if (( analysis_refresh_verified == 1 )); then
      analysis_timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
      analysis_state_lines=("${(@f)$(python3 - <<'PY'
import json
from pathlib import Path
state = json.loads(Path('.ouroboros/failure_analysis_state.json').read_text())
print(state.get('pinned_primary_axis') or 'unknown')
print(state.get('pinned_secondary_axis') or 'none')
print(state.get('next_probe_command') or 'none')
print(state.get('why_this_axis') or 'not recorded')
print(state.get('current_failure_attempt') or 'unknown')
print(state.get('current_failure_signature') or 'unknown')
PY
)}")
      primary_axis="${analysis_state_lines[1]:-unknown}"
      secondary_axis="${analysis_state_lines[2]:-none}"
      next_probe_command="${analysis_state_lines[3]:-none}"
      why_this_axis="${analysis_state_lines[4]:-not recorded}"
      current_failure_attempt="${analysis_state_lines[5]:-unknown}"
      current_failure_signature="${analysis_state_lines[6]:-unknown}"
      if (( analysis_exit_code == 0 )); then
        printf '[%s] attempt %d analysis round %d refreshed analysis assets; solver retry may continue\n' \
          "$analysis_timestamp" "$attempt" "$analysis_round" | tee -a "$analysis_log"
      else
        printf '[%s] attempt %d analysis round %d workflow exited with code %d, but refreshed analysis assets verified; solver retry may continue\n' \
          "$analysis_timestamp" "$attempt" "$analysis_round" "$analysis_exit_code" | tee -a "$analysis_log"
      fi
      prepare_artifact_file_target "$attempt_dir/latest_analysis_session.md"
      cat > "$attempt_dir/latest_analysis_session.md" <<EOF
# Analysis Session Summary

- Timestamp: \`$analysis_timestamp\`
- Failed solver attempt: \`$attempt\`
- Analysis seed: \`$analysis_seed_file\`
- Analysis round: \`$analysis_round\`
- Analysis log: \`$analysis_log\`
- Analysis workflow exit code: \`$analysis_exit_code\`
- Verification: \`refreshed analysis assets linked to latest failure\`
- Current for latest failure: \`yes\`
- Current failure attempt: \`$current_failure_attempt\`
- Current failure signature: \`$current_failure_signature\`
- Primary axis: \`$primary_axis\`
- Secondary axis: \`$secondary_axis\`
- Next probe command: \`$next_probe_command\`
- Why this axis: \`$why_this_axis\`

Analysis targets considered refreshed after baseline:
- \`.ouroboros/capture_failure_context.py\`
- \`.ouroboros/failure_analysis_playbook.md\`
- \`.ouroboros/failure_analysis_iteration.md\`
- \`.ouroboros/failure_analysis_state.json\`
- \`.ouroboros/verify_analysis_refresh.py\`

The retry loop verified that \`.ouroboros/failure_analysis_state.json\` is marked
current for the latest captured failure before allowing another solver retry.

Next solver retry must read:
- \`artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md\`
- \`artifacts/lca_tree_stress_v5/retry_loop/latest_failure_breakdown.md\`
- \`artifacts/lca_tree_stress_v5/retry_loop/latest_analysis_session.md\`
- \`.ouroboros/failure_analysis_iteration.md\`
- \`.ouroboros/failure_analysis_state.json\`

The next solver retry must stay anchored to the primary/secondary axis above and
must not broaden into an unrelated rewrite unless new evidence disproves them.
EOF
      copy_artifact_file "$attempt_dir/latest_analysis_session.md" "$report_root/latest_analysis_session.md"
      analysis_verified=1
      break
    fi

    if (( analysis_round >= max_analysis_rounds )); then
      analysis_timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
      printf '[%s] attempt %d analysis round %d failed to refresh mandatory analysis assets; aborting retry loop instead of starting a blind solver retry\n' \
        "$analysis_timestamp" "$attempt" "$analysis_round" | tee -a "$analysis_log" "$attempt_log"
      prepare_artifact_file_target "$attempt_dir/latest_analysis_session.md"
      cat > "$attempt_dir/latest_analysis_session.md" <<EOF
# Analysis Session Summary

- Timestamp: \`$analysis_timestamp\`
- Failed solver attempt: \`$attempt\`
- Analysis seed: \`$analysis_seed_file\`
- Analysis round: \`$analysis_round\`
- Analysis log: \`$analysis_log\`
- Verification: \`failed to refresh mandatory analysis assets\`

The retry loop stopped here to avoid starting another solver retry without a verified analysis-tooling refresh.
EOF
      copy_artifact_file "$attempt_dir/latest_analysis_session.md" "$report_root/latest_analysis_session.md"
      break
    fi

    analysis_timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d analysis round %d did not produce a verified analysis refresh; retrying analysis in 10 seconds\n' \
      "$analysis_timestamp" "$attempt" "$analysis_round" | tee -a "$analysis_log"
    analysis_round=$((analysis_round + 1))
    sleep 10
  done

  if (( soft_stop_restart_requested == 1 )); then
    attempt=$((attempt + 1))
    continue
  fi

  if (( analysis_verified == 0 )); then
    exit 2
  fi

  probe_timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
  printf '[%s] attempt %d running next probe before solver retry\n' "$probe_timestamp" "$attempt" | tee -a "$attempt_log"
  if ! python3 .ouroboros/run_next_probe.py \
    --state-file ".ouroboros/failure_analysis_state.json" \
    --attempt-dir "$attempt_dir" \
    --report-root "$report_root" \
    --branch-root "$PWD" \
    --timeout-seconds "$next_probe_timeout_seconds" >> "$attempt_log" 2>&1; then
    probe_timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d next probe runner failed; aborting retry loop instead of starting a solver retry without probe evidence\n' \
      "$probe_timestamp" "$attempt" | tee -a "$attempt_log"
    exit 3
  fi

  timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
  printf '[%s] attempt %d failed with exit code %d; failure decomposition recorded; retrying in 10 seconds\n' \
    "$timestamp" "$attempt" "$exit_code" | tee -a "$attempt_log"
  attempt=$((attempt + 1))
  sleep 10
done
