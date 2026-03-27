#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
source "$SCRIPT_DIR/runtime_env_exports.sh"
if [[ -n "${BRANCH_ARTIFACT_TMP_ROOT:-}" ]]; then
  mkdir -p "$BRANCH_ARTIFACT_TMP_ROOT"
fi
REPLAY_DIR="${1:-/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/subac3_smoke_failure_bundle_check/artifacts/lca_tree_stress_v5/smoke_latest_failure/replay_from_input/case01_smoke_fake_mode_n8_s42_L1_Q0_t0p5}"
rm -rf "$REPLAY_DIR"
mkdir -p "$REPLAY_DIR"
env   DENSE_SHADOW_CASE_MODE=fake_mode   DENSE_SHADOW_CASE_N=8   DENSE_SHADOW_CASE_SEED=42   DENSE_PROFILE_OUTDIR="$REPLAY_DIR"   DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1   "/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/subac3_smoke_failure_bundle_check/artifacts/lca_tree_stress_v5/smoke_latest_failure/solver_snapshot"   < "/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3/artifacts/lca_tree_stress_v5/diag/subac3_smoke_failure_bundle_check/artifacts/lca_tree_stress_v5/smoke_latest_failure/case01_smoke_fake_mode_n8_s42_L1_Q0_t0p5/in.txt"   > "$REPLAY_DIR/out.txt"   2> "$REPLAY_DIR/solver_stderr.txt"
echo "[lca_smoke] preserved-input replay artifacts: $REPLAY_DIR"
