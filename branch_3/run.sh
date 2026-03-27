#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
SOLVER="${BRANCH3_SOLVER:-$ROOT/boj28350_resume/solve}"
RELEASE_ENV="$ROOT/solver_release_env.sh"
ARTIFACT_RESOLVER="$ROOT/artifact_paths.py"

if [[ ! -x "$SOLVER" ]]; then
  echo "[branch_3/run.sh] missing solver binary at $SOLVER" >&2
  echo "[branch_3/run.sh] run ./build.sh first" >&2
  exit 2
fi

if [[ ! -f "$ARTIFACT_RESOLVER" ]]; then
  echo "[branch_3/run.sh] missing artifact resolver at $ARTIFACT_RESOLVER" >&2
  exit 2
fi

if [[ -f "$RELEASE_ENV" ]]; then
  source "$RELEASE_ENV"
fi

DENSE_PROFILE_OUTDIR="$(python3 "$ARTIFACT_RESOLVER" boj28350_direct_solver_aux "${DENSE_PROFILE_OUTDIR:-}")"
export DENSE_PROFILE_OUTDIR
mkdir -p "$DENSE_PROFILE_OUTDIR"
cd "$DENSE_PROFILE_OUTDIR"
exec "$SOLVER" "$@"
