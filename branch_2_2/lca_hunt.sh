#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BRANCH="$ROOT/branch_2_2"
SOLVER="$BRANCH/round45_resume/solve"
OUTDIR_INPUT="${1:-artifacts/lca_tree_stress_v5/hunt}"
SIZES="${2:-12000,24000,48000,99999}"
SEEDS="${3:-1,2,3}"
TIMEOUT="${4:-8.0}"

if [[ "$OUTDIR_INPUT" != /* ]]; then
  OUTDIR="$BRANCH/$OUTDIR_INPUT"
else
  OUTDIR="$OUTDIR_INPUT"
fi

if [[ ! -x "$SOLVER" ]]; then
  "$BRANCH/build.sh" --mode plain
fi

export DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
exec "$ROOT/hunt.sh" "$SOLVER" "$OUTDIR" "$SIZES" "$SEEDS" "$TIMEOUT"
