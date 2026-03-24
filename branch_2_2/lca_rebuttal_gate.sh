#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BRANCH="$ROOT/branch_2_2"
SOLVER="$BRANCH/round45_resume/solve"
OUTDIR_INPUT="${1:-artifacts/lca_tree_stress_v5/rebuttal_gate}"
LIMIT_SCALE="${2:-1.0}"

if [[ "$OUTDIR_INPUT" != /* ]]; then
  OUTDIR="$BRANCH/$OUTDIR_INPUT"
else
  OUTDIR="$OUTDIR_INPUT"
fi

if [[ ! -x "$SOLVER" ]]; then
  "$BRANCH/build.sh" --mode plain
fi

export DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
exec "$ROOT/gate.sh" "$SOLVER" suite_presets/rebuttal_gate.json "$OUTDIR" "$LIMIT_SCALE"
