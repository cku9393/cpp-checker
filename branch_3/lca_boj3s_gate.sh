#!/usr/bin/env bash
set -euo pipefail

BRANCH="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$BRANCH/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
SOLVER="$BRANCH/boj28350_resume/solve"
SOURCE="$BRANCH/boj28350_resume/boj28350_branch_3_solver.cpp"
OUTDIR="$(python3 "$BRANCH/artifact_paths.py" lca_boj3s_gate "${1:-}")"
LIMIT_SCALE="${2:-1.0}"

source "$BRANCH/solver_release_env.sh"

if [[ ! -x "$SOLVER" || "$SOURCE" -nt "$SOLVER" ]]; then
  "$BRANCH/build.sh"
fi

exec "$ROOT/gate_boj3s.sh" "$SOLVER" "$OUTDIR" "$LIMIT_SCALE"
