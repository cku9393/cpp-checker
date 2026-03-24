#!/usr/bin/env bash
set -euo pipefail

BRANCH="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$BRANCH/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
SOLVER="$BRANCH/boj28350_resume/solve"
SOURCE="$BRANCH/boj28350_resume/boj28350_branch_3_solver.cpp"
OUTDIR="$(python3 "$BRANCH/artifact_paths.py" lca_hunt "${1:-}")"
SIZES="${2:-12000,24000,48000,99999}"
SEEDS="${3:-1,2,3}"
TIMEOUT="${4:-8.0}"

source "$BRANCH/solver_release_env.sh"

if [[ ! -x "$SOLVER" || "$SOURCE" -nt "$SOLVER" ]]; then
  "$BRANCH/build.sh"
fi

# Keep the upstream hardest-case search intact, but make the branch contract
# explicit: this wrapper is for diagnosis, not for formal acceptance.
echo "[lca_hunt] diagnostic-only run; required acceptance gates are ./lca_strong_gate.sh and ./lca_boj3s_gate.sh" >&2

exec "$ROOT/hunt.sh" "$SOLVER" "$OUTDIR" "$SIZES" "$SEEDS" "$TIMEOUT"
