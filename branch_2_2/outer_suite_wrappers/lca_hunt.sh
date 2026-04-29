#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH="$(cd "$SCRIPT_DIR/.." && pwd -P)"
ROOT="$(cd "$BRANCH/.." && pwd -P)"
TOOLING="$ROOT/lca_tree_stress_v5/tooling"
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
exec "$TOOLING/hunt.sh" "$SOLVER" "$OUTDIR" "$SIZES" "$SEEDS" "$TIMEOUT"
