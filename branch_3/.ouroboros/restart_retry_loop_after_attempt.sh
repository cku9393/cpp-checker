#!/bin/zsh
set -euo pipefail
export PYTHONDONTWRITEBYTECODE=1

if [[ $# -lt 4 || $# -gt 5 ]]; then
  echo "usage: $0 <old_loop_pid> <workflow_pid> <attempt_number> <attempt_log> [seed_file]" >&2
  exit 2
fi

old_loop_pid="$1"
workflow_pid="$2"
attempt_number="$3"
attempt_log="$4"
seed_file="${5:-.ouroboros/seed_branch3_progress40_research_loop.yaml}"

branch_root="/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3"
loop_script="$branch_root/.ouroboros/run_until_pass_progress40.sh"
watch_log="$branch_root/artifacts/lca_tree_stress_v5/retry_loop/restart_after_attempt_${attempt_number}.log"

timestamp() {
  date '+%Y-%m-%d %H:%M:%S %Z'
}

echo "[$(timestamp)] watcher start: old_loop_pid=$old_loop_pid workflow_pid=$workflow_pid attempt=$attempt_number" >> "$watch_log"

while kill -0 "$workflow_pid" 2>/dev/null; do
  sleep 2
done

echo "[$(timestamp)] workflow pid $workflow_pid exited" >> "$watch_log"

for _ in {1..120}; do
  if ! kill -0 "$old_loop_pid" 2>/dev/null; then
    echo "[$(timestamp)] old loop already exited; nothing to restart" >> "$watch_log"
    exit 0
  fi
  if [[ -f "$attempt_log" ]] && grep -q "attempt $attempt_number succeeded" "$attempt_log"; then
    echo "[$(timestamp)] attempt $attempt_number succeeded; no loop restart needed" >> "$watch_log"
    exit 0
  fi
  if [[ -f "$attempt_log" ]] && grep -q "attempt $attempt_number failed with exit code" "$attempt_log"; then
    echo "[$(timestamp)] detected failure marker for attempt $attempt_number" >> "$watch_log"
    break
  fi
  sleep 1
done

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
nohup zsh "$loop_script" "$seed_file" >> "$watch_log" 2>&1 &
new_loop_pid=$!
echo "[$(timestamp)] started updated retry loop pid=$new_loop_pid using seed=$seed_file" >> "$watch_log"
