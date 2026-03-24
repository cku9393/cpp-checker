#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
SOLVER="${BRANCH3_SOLVER:-$ROOT/boj28350_resume/solve}"

if [[ ! -x "$SOLVER" ]]; then
  echo "[branch_3/run.sh] missing solver binary at $SOLVER" >&2
  echo "[branch_3/run.sh] run ./build.sh first" >&2
  exit 2
fi

cd "$ROOT/boj28350_resume"
exec "$SOLVER" "$@"
