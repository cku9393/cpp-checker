#!/bin/zsh
set -euo pipefail

cd "/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3"
export PYTHONDONTWRITEBYTECODE=1

seed_file="${1:-.ouroboros/seed_branch3_progress40_research_loop.yaml}"
analysis_seed_file="${2:-.ouroboros/seed_branch3_failure_analysis.yaml}"
report_root="artifacts/lca_tree_stress_v5/retry_loop"
prepare_state_helper=".ouroboros/prepare_retry_attempt_state.py"
runtime_snapshot_helper=".ouroboros/snapshot_retry_runtime.py"
quota_watch_helper=".ouroboros/monitor_codex_quota.py"
soft_stop_file=".ouroboros/soft_stop_request.json"
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

mkdir -p "$report_root"

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

  if [[ -o errexit ]]; then
    had_errexit=1
  fi

  if [[ -f "$soft_stop_file" ]]; then
    return "$soft_stop_exit_code"
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
  cp "$attempt_log" "$report_root/latest_workflow.log" 2>/dev/null || true
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

while true; do
  timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
  attempt_stamp="$(date '+%Y%m%d_%H%M%S')"
  attempt_dir="$report_root/attempt_$(printf '%03d' "$attempt")_${attempt_stamp}"
  attempt_log="$attempt_dir/workflow.log"
  mkdir -p "$attempt_dir"
  workflow_pid=""
  quota_watchdog_pid=""
  soft_stop_restart_requested=0

  if [[ -f "$soft_stop_file" ]]; then
    cp "$soft_stop_file" "$attempt_dir/stale_soft_stop_request.json" 2>/dev/null || true
    rm -f "$soft_stop_file"
    printf '[%s] attempt %d cleared a stale soft stop request from a prior quota pause before starting the new workflow\n' \
      "$timestamp" "$attempt" | tee -a "$attempt_log"
  fi

  printf '[%s] attempt %d start: %s\n' "$timestamp" "$attempt" "$seed_file" | tee -a "$attempt_log"
  if ! python3 "$prepare_state_helper" \
    --branch-root "$PWD" \
    --attempt-dir "$attempt_dir" \
    --report-root "$report_root" >> "$attempt_log" 2>&1; then
    timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d stale-state preflight failed; aborting retry loop before workflow start\n' \
      "$timestamp" "$attempt" | tee -a "$attempt_log"
    exit 4
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

  set +e
  python3 .ouroboros/post_attempt_guard.py \
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

  if (( exit_code == 0 )); then
    timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
    printf '[%s] attempt %d succeeded\n' "$timestamp" "$attempt" | tee -a "$attempt_log"
    cp "$attempt_log" "$report_root/latest_workflow.log"
    break
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

    if (( analysis_exit_code == 0 )) && python3 .ouroboros/refresh_analysis_state.py \
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
      --require-json-key ".ouroboros/failure_analysis_state.json:why_this_axis"; then
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
      printf '[%s] attempt %d analysis round %d refreshed analysis assets; solver retry may continue\n' \
        "$analysis_timestamp" "$attempt" "$analysis_round" | tee -a "$analysis_log"
      cat > "$attempt_dir/latest_analysis_session.md" <<EOF
# Analysis Session Summary

- Timestamp: \`$analysis_timestamp\`
- Failed solver attempt: \`$attempt\`
- Analysis seed: \`$analysis_seed_file\`
- Analysis round: \`$analysis_round\`
- Analysis log: \`$analysis_log\`
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
      cp "$attempt_dir/latest_analysis_session.md" "$report_root/latest_analysis_session.md"
      analysis_verified=1
      break
    fi

    if (( analysis_round >= max_analysis_rounds )); then
      analysis_timestamp="$(date '+%Y-%m-%d %H:%M:%S %Z')"
      printf '[%s] attempt %d analysis round %d failed to refresh mandatory analysis assets; aborting retry loop instead of starting a blind solver retry\n' \
        "$analysis_timestamp" "$attempt" "$analysis_round" | tee -a "$analysis_log" "$attempt_log"
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
      cp "$attempt_dir/latest_analysis_session.md" "$report_root/latest_analysis_session.md"
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
