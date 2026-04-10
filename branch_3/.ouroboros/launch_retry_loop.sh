#!/bin/zsh
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${(%):-%N}")" && pwd -P)"
branch_root="$(cd -- "$script_dir/.." && pwd -P)"
artifact_resolver="$branch_root/artifact_paths.py"
retry_log_root="$branch_root/artifacts/lca_tree_stress_v5/retry_loop"
retry_tmp_parent="$branch_root/artifacts/lca_tree_stress_v5/.tmp"
analysis_verify_helper="$branch_root/.ouroboros/verify_analysis_refresh.py"
analysis_state_file="$branch_root/.ouroboros/failure_analysis_state.json"
analysis_iteration_file="$branch_root/.ouroboros/failure_analysis_iteration.md"
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
      --target "$analysis_state_file" \
      --target "$analysis_iteration_file" \
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
    fail "latest analysis session is missing, stale, or not tied to the newest failed attempt"
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

caffeinate -ims zsh ".ouroboros/run_until_pass_progress40.sh" "$seed_file" "$analysis_seed_file"
