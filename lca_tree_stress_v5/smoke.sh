#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOLVER="${1:-$ROOT/lca_tree_stress_v5/solve}"
OUTROOT="${2:-lca_tree_stress_v5/artifacts/smoke}"

if [[ "$OUTROOT" != /* ]]; then
  OUTROOT="$ROOT/$OUTROOT"
fi

if [[ ! -x "$SOLVER" ]]; then
  "$ROOT/lca_tree_stress_v5/build.sh"
fi

"$ROOT/run_case.sh" comb_dense 256 1 1 1 "$SOLVER" "$OUTROOT/smoke_comb_dense_256_s1" \
  --env DENSE_SHADOW_CASE_MODE=comb_dense \
  --env DENSE_SHADOW_CASE_N=256 \
  --env DENSE_SHADOW_CASE_SEED=1 \
  --env DENSE_PROFILE_OUTDIR="$OUTROOT/smoke_comb_dense_256_s1" \
  --env DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
"$ROOT/run_case.sh" comb_dense 1024 1 1 1 "$SOLVER" "$OUTROOT/smoke_comb_dense_1024_s1" \
  --env DENSE_SHADOW_CASE_MODE=comb_dense \
  --env DENSE_SHADOW_CASE_N=1024 \
  --env DENSE_SHADOW_CASE_SEED=1 \
  --env DENSE_PROFILE_OUTDIR="$OUTROOT/smoke_comb_dense_1024_s1" \
  --env DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
