#!/usr/bin/env bash
set -euo pipefail

WORKSPACE="$(cd "$(dirname "$0")" && pwd)"
TOOLING="$WORKSPACE/tooling"
exec "$TOOLING/build.sh" \
  --source "$WORKSPACE/lca_tree_stress_v5_solver.cpp" \
  --out "$WORKSPACE/solve" \
  --define DENSE_DECOMPOSESERIES_ROUND38_PROFILE=1 \
  "$@"
