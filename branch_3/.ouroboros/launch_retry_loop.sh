#!/bin/zsh
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${(%):-%N}")" && pwd -P)"
branch_root="$(cd -- "$script_dir/.." && pwd -P)"
artifact_resolver="$branch_root/artifact_paths.py"
retry_log_root="$branch_root/artifacts/lca_tree_stress_v5/retry_loop"
retry_tmp_parent="$branch_root/artifacts/lca_tree_stress_v5/.tmp"
analysis_verify_helper="$branch_root/.ouroboros/verify_analysis_refresh.py"
auto_remediation_helper="$branch_root/.ouroboros/auto_remediate_retry_abort.py"
analysis_state_file="$branch_root/.ouroboros/failure_analysis_state.json"
analysis_iteration_file="$branch_root/.ouroboros/failure_analysis_iteration.md"
analysis_playbook_file="$branch_root/.ouroboros/failure_analysis_playbook.md"
analysis_capture_helper="$branch_root/.ouroboros/capture_failure_context.py"
analysis_pre_attempt_helper="$branch_root/.ouroboros/prepare_retry_attempt_state.py"
analysis_refresh_helper="$branch_root/.ouroboros/refresh_analysis_state.py"
branch_name="${branch_root##*/}"
launcher_runtime_root=""

cd "$branch_root"

fail() {
  echo "[launch_retry_loop] $*" >&2
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

cleanup_launcher_runtime_environment() {
  if [[ -n "${launcher_runtime_root:-}" && -e "$launcher_runtime_root" ]]; then
    rm -rf "$launcher_runtime_root"
  fi
}

verify_latest_failure_analysis_session() {
  local latest_failure_report="$retry_log_root/latest_failure_report.md"
  local latest_failure_breakdown="$retry_log_root/latest_failure_breakdown.md"
  local latest_analysis_session="$retry_log_root/latest_analysis_session.md"
  local verify_output=""

  if [[ ! -e "$latest_failure_report" && ! -e "$latest_failure_breakdown" ]]; then
    return 0
  fi

  if ! verify_output="$(
    python3 "$analysis_verify_helper" \
      --baseline-epoch 0 \
      --analysis-log "$log_file" \
      --target-from-current-state \
      --target "$analysis_state_file" \
      --target "$analysis_iteration_file" \
      --target "$analysis_playbook_file" \
      --target "$analysis_capture_helper" \
      --target "$analysis_pre_attempt_helper" \
      --target "$analysis_refresh_helper" \
      --latest-failure-report "$latest_failure_report" \
      --latest-failure-breakdown "$latest_failure_breakdown" \
      --require-current-state "$analysis_state_file" \
      --require-analysis-session "$latest_analysis_session" \
      --require-json-key "${analysis_state_file}:current_for_latest_failure" \
      --require-json-key "${analysis_state_file}:current_failure_signature" \
      --require-json-key "${analysis_state_file}:pinned_primary_axis" \
      --require-json-key "${analysis_state_file}:next_probe_command" \
      --require-json-key "${analysis_state_file}:why_this_axis" 2>&1
  )"; then
    printf '%s\n' "$verify_output" >> "$log_file"
    echo "$verify_output" >&2
    fail "retry start blocked until at least one refreshed workflow-recognized branch-local analysis asset is present: latest analysis session/state is missing, stale, or not tied to the newest failed attempt; refresh a supporting .ouroboros note/helper such as failure_analysis_iteration.md or failure_analysis_playbook.md and record it in failure_analysis_state.json refresh_evidence.freshness_record before retrying"
  fi

  printf '%s\n' "$verify_output" >> "$log_file"
}

configure_launcher_runtime_environment() {
  local launcher_tmpdir=""
  local launcher_pycache_root=""

  mkdir -p "$retry_tmp_parent"
  launcher_runtime_root="$(mktemp -d "$retry_tmp_parent/retry_loop.launcher.env.XXXXXX")"
  if [[ -z "$launcher_runtime_root" ]]; then
    fail "mktemp returned an empty retry-loop launcher runtime root"
  fi
  launcher_runtime_root="$(ensure_artifact_path "$launcher_runtime_root")"
  launcher_tmpdir="$(ensure_artifact_path "$launcher_runtime_root/tmp")"
  launcher_pycache_root="$(ensure_artifact_path "$launcher_runtime_root/pycache")"

  mkdir -p \
    "$launcher_tmpdir" \
    "$launcher_pycache_root"

  # Keep the user's HOME-backed Codex auth/session state intact while routing
  # scratch files and bytecode under branch-local artifacts.
  export BRANCH_ARTIFACT_TMP_ROOT="$launcher_tmpdir"
  export TMPDIR="$launcher_tmpdir"
  export TMP="$launcher_tmpdir"
  export TEMP="$launcher_tmpdir"
  export PYTHONPYCACHEPREFIX="$launcher_pycache_root"
}

retry_log_root="$(ensure_artifact_path "$retry_log_root")"
retry_tmp_parent="$(ensure_artifact_path "$retry_tmp_parent")"

trap cleanup_launcher_runtime_environment EXIT

if [[ $# -gt 3 ]]; then
  echo "usage: $0 [artifact_subpath_or_log_name] [seed_file] [analysis_seed_file]" >&2
  exit 2
fi

raw_log_file="${1:-manual_launch_$(date '+%Y%m%d_%H%M%S').log}"
seed_file="${2:-.ouroboros/seed_branch3_progress40_research_loop.yaml}"
analysis_seed_file="${3:-.ouroboros/seed_branch3_failure_analysis.yaml}"
if [[ "$raw_log_file" = /* ]]; then
  candidate_log_file="$raw_log_file"
elif [[ "$raw_log_file" == "$branch_name/artifacts/"* ]]; then
  candidate_log_file="$branch_root/${raw_log_file#"$branch_name/"}"
elif [[ "$raw_log_file" == artifacts/* ]]; then
  candidate_log_file="$branch_root/$raw_log_file"
else
  candidate_log_file="$retry_log_root/$raw_log_file"
fi

if ! log_file="$(python3 "$artifact_resolver" --ensure "$candidate_log_file")"; then
  fail "log path must stay under branch-local artifacts: $raw_log_file"
fi

mkdir -p "$(dirname "$log_file")"
if [[ -e "$log_file" && ! -f "$log_file" ]]; then
  rm -rf "$log_file"
fi
: > "$log_file"
verify_latest_failure_analysis_session
configure_launcher_runtime_environment

exec >> "$log_file" 2>&1

while true; do
  verify_latest_failure_analysis_session
  set +e
  caffeinate -ims zsh ".ouroboros/run_until_pass_progress40.sh" "$seed_file" "$analysis_seed_file"
  loop_exit_code=$?
  set -e

  if (( loop_exit_code == 0 )); then
    exit 0
  fi

  if python3 "$auto_remediation_helper" \
    --branch-root "$branch_root" \
    --report-root "$retry_log_root" \
    --launch-log "$log_file" \
    --loop-exit-code "$loop_exit_code"; then
    printf '[%s] auto-remediation handled loop exit code %d; relaunching retry loop\n' \
      "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$loop_exit_code"
    sleep 10
    continue
  fi

  printf '[%s] auto-remediation could not safely handle loop exit code %d; leaving retry loop stopped\n' \
    "$(date '+%Y-%m-%d %H:%M:%S %Z')" "$loop_exit_code"
  exit "$loop_exit_code"
done
