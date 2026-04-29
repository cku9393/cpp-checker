#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH="$(cd "$SCRIPT_DIR/.." && pwd -P)"
ROOT="$(cd "$BRANCH/.." && pwd -P)"
TOOLING="$ROOT/lca_tree_stress_v5/tooling"
SOLVER="$BRANCH/round45_resume/solve"
OUTDIR_INPUT="${1:-artifacts/lca_tree_stress_v5/boj3s_gate}"
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
exec "$TOOLING/gate_boj3s.sh" "$SOLVER" "$OUTDIR" "$LIMIT_SCALE"
