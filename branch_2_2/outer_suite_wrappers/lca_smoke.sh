#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH="$(cd "$SCRIPT_DIR/.." && pwd -P)"
ROOT="$(cd "$BRANCH/.." && pwd -P)"
SOLVER="$BRANCH/round45_resume/solve"
OUTROOT_INPUT="${1:-artifacts/lca_tree_stress_v5/smoke}"

if [[ "$OUTROOT_INPUT" != /* ]]; then
  OUTROOT="$BRANCH/$OUTROOT_INPUT"
else
  OUTROOT="$OUTROOT_INPUT"
fi

if [[ ! -x "$SOLVER" ]]; then
  "$BRANCH/build.sh" --mode plain
fi

exec "$ROOT/lca_tree_stress_v5/smoke.sh" "$SOLVER" "$OUTROOT"
