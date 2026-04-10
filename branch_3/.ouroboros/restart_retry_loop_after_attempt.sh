#!/bin/zsh
set -euo pipefail
export PYTHONDONTWRITEBYTECODE=1

if [[ $# -lt 4 || $# -gt 6 ]]; then
  echo "usage: $0 <old_loop_pid> <workflow_pid> <attempt_number> <attempt_log> [seed_file] [analysis_seed_file]" >&2
  exit 2
fi

old_loop_pid="$1"
workflow_pid="$2"
attempt_number="$3"
attempt_log="$4"
seed_file="${5:-.ouroboros/seed_branch3_progress40_research_loop.yaml}"
analysis_seed_file="${6:-.ouroboros/seed_branch3_failure_analysis.yaml}"
next_attempt_number=$(( attempt_number + 1 ))

script_dir="$(cd -- "$(dirname -- "${(%):-%N}")" && pwd -P)"
branch_root="$(cd -- "$script_dir/.." && pwd -P)"
artifact_resolver="$branch_root/artifact_paths.py"
loop_script="$branch_root/.ouroboros/run_until_pass_progress40.sh"
outcome_helper="$branch_root/.ouroboros/classify_retry_loop_outcome.py"
analysis_verify_helper="$branch_root/.ouroboros/verify_analysis_refresh.py"
analysis_state_file="$branch_root/.ouroboros/failure_analysis_state.json"
analysis_iteration_file="$branch_root/.ouroboros/failure_analysis_iteration.md"
watch_log="$branch_root/artifacts/lca_tree_stress_v5/retry_loop/restart_after_attempt_${attempt_number}.log"

ensure_artifact_path() {
  local raw_path="$1"
  local resolved=""
  if ! resolved="$(python3 "$artifact_resolver" --ensure "$raw_path")"; then
    echo "[restart_retry_loop_after_attempt] path must stay under branch-local artifacts: $raw_path" >&2
    exit 1
  fi
  printf '%s\n' "$resolved"
}

watch_log="$(ensure_artifact_path "$watch_log")"
attempt_log="$(ensure_artifact_path "$attempt_log")"

mkdir -p "$(dirname "$watch_log")"
if [[ -e "$watch_log" && ! -f "$watch_log" ]]; then
  rm -rf "$watch_log"
fi
: > "$watch_log"

timestamp() {
  date '+%Y-%m-%d %H:%M:%S %Z'
}

attempt_log_field() {
  local field_name="$1"
  python3 "$outcome_helper" \
    --workflow-log "$attempt_log" \
    --attempt-number "$attempt_number" \
    --field "$field_name" 2>/dev/null
}

verify_latest_failure_analysis_session() {
  local latest_failure_report="$branch_root/artifacts/lca_tree_stress_v5/retry_loop/latest_failure_report.md"
  local latest_failure_breakdown="$branch_root/artifacts/lca_tree_stress_v5/retry_loop/latest_failure_breakdown.md"
  local latest_analysis_session="$branch_root/artifacts/lca_tree_stress_v5/retry_loop/latest_analysis_session.md"
  local verify_output=""

  if [[ ! -e "$latest_failure_report" && ! -e "$latest_failure_breakdown" ]]; then
    return 0
  fi

  if ! verify_output="$(
    python3 "$analysis_verify_helper" \
      --baseline-epoch 0 \
      --analysis-log "$watch_log" \
      --target "$analysis_state_file" \
      --target "$analysis_iteration_file" \
      --latest-failure-report "$latest_failure_report" \
      --latest-failure-breakdown "$latest_failure_breakdown" \
      --require-current-state "$analysis_state_file" \
      --require-analysis-session "$latest_analysis_session" \
      --require-json-key "$analysis_state_file:current_for_latest_failure" \
      --require-json-key "$analysis_state_file:current_failure_signature" \
      --require-json-key "$analysis_state_file:pinned_primary_axis" \
      --require-json-key "$analysis_state_file:next_probe_command" \
      --require-json-key "$analysis_state_file:why_this_axis" 2>&1
  )"; then
    printf '%s\n' "$verify_output" >> "$watch_log"
    echo "[$(timestamp)] retry restart blocked: latest analysis session is missing, stale, or not tied to the newest failed attempt" >> "$watch_log"
    return 1
  fi

  printf '%s\n' "$verify_output" >> "$watch_log"
}

echo "[$(timestamp)] watcher start: old_loop_pid=$old_loop_pid workflow_pid=$workflow_pid attempt=$attempt_number" >> "$watch_log"

while kill -0 "$workflow_pid" 2>/dev/null; do
  sleep 2
done

echo "[$(timestamp)] workflow pid $workflow_pid exited" >> "$watch_log"

should_restart=0
for _ in {1..120}; do
  if ! kill -0 "$old_loop_pid" 2>/dev/null; then
    echo "[$(timestamp)] old loop already exited; nothing to restart" >> "$watch_log"
    exit 0
  fi
  if [[ -f "$attempt_log" ]] && [[ "$(attempt_log_field success_marker_present)" == "true" ]]; then
    echo "[$(timestamp)] attempt $attempt_number succeeded; no loop restart needed" >> "$watch_log"
    exit 0
  fi
  if [[ -f "$attempt_log" ]] && [[ "$(attempt_log_field retryable_intermediate_failure)" == "true" ]]; then
    retry_reason="$(attempt_log_field reason)"
    echo "[$(timestamp)] detected retryable intermediate acceptance failure for attempt $attempt_number (reason=$retry_reason)" >> "$watch_log"
    should_restart=1
    break
  fi
  sleep 1
done

if [[ "$should_restart" != "1" ]]; then
  echo "[$(timestamp)] no retryable intermediate failure detected for attempt $attempt_number; leaving old loop in place" >> "$watch_log"
  exit 0
fi

if ! verify_latest_failure_analysis_session; then
  exit 1
fi

if ! kill -0 "$old_loop_pid" 2>/dev/null; then
  echo "[$(timestamp)] old loop exited before restart point" >> "$watch_log"
  exit 0
fi

echo "[$(timestamp)] stopping old loop pid $old_loop_pid" >> "$watch_log"
kill "$old_loop_pid" 2>/dev/null || true

for _ in {1..20}; do
  if ! kill -0 "$old_loop_pid" 2>/dev/null; then
    break
  fi
  sleep 1
done

if kill -0 "$old_loop_pid" 2>/dev/null; then
  echo "[$(timestamp)] old loop pid $old_loop_pid still alive after grace period" >> "$watch_log"
  exit 1
fi

cd "$branch_root"
nohup env RETRY_LOOP_START_ATTEMPT="$next_attempt_number" \
  zsh "$loop_script" "$seed_file" "$analysis_seed_file" >> "$watch_log" 2>&1 &
new_loop_pid=$!
echo "[$(timestamp)] started updated retry loop pid=$new_loop_pid using seed=$seed_file analysis_seed=$analysis_seed_file next_attempt=$next_attempt_number" >> "$watch_log"
