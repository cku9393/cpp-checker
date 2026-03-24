#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec "$ROOT/build.sh" \
  --source lca_tree_stress_v5/lca_tree_stress_v5_solver.cpp \
  --out "$ROOT/lca_tree_stress_v5/solve" \
  --define DENSE_DECOMPOSESERIES_ROUND38_PROFILE=1 \
  "$@"
