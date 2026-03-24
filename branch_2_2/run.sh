#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
MODE="${BRANCH22_SOLVER_MODE:-profile}"

case "$MODE" in
  profile)
    SOLVER="$ROOT/round45_resume/solve_prof"
    ;;
  plain)
    SOLVER="$ROOT/round45_resume/solve"
    ;;
  *)
    echo "[branch_2_2/run.sh] unsupported BRANCH22_SOLVER_MODE: $MODE" >&2
    exit 2
    ;;
esac

if [[ ! -x "$SOLVER" ]]; then
  echo "[branch_2_2/run.sh] missing solver binary at $SOLVER" >&2
  echo "[branch_2_2/run.sh] run ./build.sh first" >&2
  exit 2
fi

cd "$ROOT/round45_resume"
exec "$SOLVER" "$@"
