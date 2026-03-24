#!/usr/bin/env bash
set -euo pipefail
ROOT=/mnt/data
RUNNER="$ROOT/run_progress23_case.py"
SOLVER_LOCAL="$ROOT/p28_local"
SOLVER_RELEASE="$ROOT/p28_release"
OUTROOT="$ROOT/p28_runs"
mkdir -p "$OUTROOT"
common_env=(
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
  PROFILE_PROGRESS_STRIDE=16
)
run(){
  local solver="$1" tag="$2" mode="$3" n="$4" profile="$5" delta="$6" timeout="$7" prebind="$8"
  local outdir="$OUTROOT/$tag"
  rm -rf "$outdir"
  mkdir -p "$outdir"
  cmd=(python "$RUNNER" --solver "$solver" --run-tag "$tag" --mode "$mode" --n "$n" --profile-mode "$profile" --delta-mode "$delta" --timeout-sec "$timeout" --outdir "$outdir")
  for kv in "${common_env[@]}"; do cmd+=(--env "$kv"); done
  cmd+=(--env "ENABLE_POINTER_REBIND_OPT=$prebind")
  echo "[resume] start $tag"
  "${cmd[@]}"
  echo "[resume] done  $tag"
}
run "$SOLVER_LOCAL" before_both_on_dense_512_base comb_rect_dense 512 PROFILE_BASE both_on 400 0
run "$SOLVER_LOCAL" before_connector_only_dense_512_sampled comb_rect_dense 512 PROFILE_SAMPLED connector_only 400 0
run "$SOLVER_LOCAL" before_both_on_dense_512_sampled comb_rect_dense 512 PROFILE_SAMPLED both_on 400 0
run "$SOLVER_LOCAL" before_both_on_multi_512_sampled multi_comb_rect 512 PROFILE_SAMPLED both_on 240 0
run "$SOLVER_LOCAL" after_both_on_dense_512_base comb_rect_dense 512 PROFILE_BASE both_on 400 1
run "$SOLVER_LOCAL" after_both_on_dense_512_sampled comb_rect_dense 512 PROFILE_SAMPLED both_on 400 1
run "$SOLVER_LOCAL" after_both_on_multi_512_sampled multi_comb_rect 512 PROFILE_SAMPLED both_on 240 1
run "$SOLVER_RELEASE" both_on_dense_1024_release comb_rect_dense 1024 PROFILE_BASE both_on 1800 1
run "$SOLVER_RELEASE" both_on_multi_1024_release multi_comb_rect 1024 PROFILE_BASE both_on 240 1
run "$SOLVER_RELEASE" both_on_dense_4096_release comb_rect_dense 4096 PROFILE_BASE both_on 500 1
run "$SOLVER_RELEASE" both_on_multi_4096_release multi_comb_rect 4096 PROFILE_BASE both_on 500 1
