#!/usr/bin/env bash
set -euo pipefail
run_case() {
  local seed="$1"
  local rounds="$2"
  local label="$3"
  local dump_dir="dumps/rewrite_r_seq_arcphase/${label}"
  local log_path="verify/rewrite_r_seq_arcphase/logs/${label}.log"
  mkdir -p "$dump_dir"
  ./build/rewrite_r_harness \
    --backend ogdf \
    --mode rewrite-r-seq \
    --seed "$seed" \
    --rounds "$rounds" \
    --dump-dir "$dump_dir" \
    > "$log_path" 2>&1
}
run_case 1 100 s1_r100
run_case 1 1000 s1_r1000_single
for s in $(seq 1 10); do
  run_case "$s" 1000 "s${s}_r1000_loop"
done
for s in $(seq 1 20); do
  run_case "$s" 5000 "s${s}_r5000"
done
