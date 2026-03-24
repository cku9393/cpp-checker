#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/data/progress30_runs
SOLVER_LOCAL=/mnt/data/p30_local
SOLVER_RELEASE=/mnt/data/p30_release
RUNNER=/mnt/data/run_progress30_case_transactional.py
COMMON_ENV=(
  ENABLE_REUSE_APPLY_OPT=1
  ENABLE_PRESERVED_SPLIT_OPT=1
  ENABLE_WATCH_SCAN_OPT=1
  ENABLE_RETAIN_COMPACTION_OPT=1
  ENABLE_KEPT_VECTOR_OPT=1
  ENABLE_STABLE_COMPACTION_OPT=1
  ENABLE_BLOCK_COPY_COMPACTION_OPT=1
  ENABLE_COPY_PLAN_BUILD_OPT=1
  ENABLE_RUN_DISCOVERY_FUSION_OPT=1
  ENABLE_FUSED_DISCOVERY_CLASSIFY_OPT=1
  ENABLE_TSCAN_CORE_OPT=1
  ENABLE_TSCAN_BRANCH_STATE_OPT=1
  ENABLE_STATE_LOAD_MATERIALIZATION_OPT=1
  ENABLE_PREV_STATE_CARRY_REUSE_OPT=1
  ENABLE_CARRY_REUSE_FASTPATH_OPT=1
  ENABLE_CARRY_HIT_APPLY_OPT=1
  ENABLE_PREV_STATE_WRITEBACK_OPT=1
  ENABLE_TARGET_RESOLVE_PINNING_OPT=1
  ENABLE_POINTER_REBIND_OPT=1
  ENABLE_DIRECT_PREBIND_OPT=1
  ENABLE_REBIND_COMMIT_OPT=1
)
run_case(){
  local solver=$1 tag=$2 mode=$3 n=$4 profile=$5 delta=$6 timeout=$7 outdir=$8
  shift 8
  python "$RUNNER" --solver "$solver" --run-tag "$tag" --mode "$mode" --n "$n" --profile-mode "$profile" --delta-mode "$delta" --outdir "$outdir" --timeout-sec "$timeout" "$@"
}
# Fill in missing cases one by one; verify result.json after each before continuing.
