#!/usr/bin/env bash
set -u
set -o pipefail
PY=python
RUNNER=/mnt/data/run_progress19_case.py
REL=/mnt/data/boj28350_progress22_release
OUTROOT=/mnt/data/progress22_complete_runs
COMMON_ENV=(
  --env ENABLE_REUSE_APPLY_OPT=1
  --env ENABLE_PRESERVED_SPLIT_OPT=1
  --env ENABLE_WATCH_SCAN_OPT=1
  --env ENABLE_RETAIN_COMPACTION_OPT=1
  --env ENABLE_KEPT_VECTOR_OPT=1
  --env ENABLE_STABLE_COMPACTION_OPT=1
  --env ENABLE_BLOCK_COPY_COMPACTION_OPT=1
  --env ENABLE_COPY_PLAN_BUILD_OPT=1
  --env ENABLE_RUN_DISCOVERY_FUSION_OPT=1
  --env ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT=1
  --env ENABLE_TSCAN_CORE_OPT=1
  --env ENABLE_TSCAN_BRANCH_STATE_OPT=1
  --env ENABLE_STATE_LOAD_MATERIALIZATION_OPT=1
)
run_case(){
  tag="$1"; mode="$2"; n="$3"; timeout="$4"
  echo "[START] $(date -Is) $tag mode=$mode n=$n timeout=$timeout"
  $PY "$RUNNER" --solver "$REL" --run-tag "$tag" --mode "$mode" --n "$n" \
    --profile-mode PROFILE_BASE --delta-mode both_on --outdir "$OUTROOT/$tag" --timeout-sec "$timeout" \
    "${COMMON_ENV[@]}"
  rc=$?
  echo "[DONE] $(date -Is) $tag runner_rc=$rc"
}
run_case after_both_on_dense_1024_release_repeat comb_rect_dense 1024 1800
run_case after_both_on_dense_4096_release comb_rect_dense 4096 450
run_case after_both_on_multi_4096_release multi_comb_rect 4096 450
