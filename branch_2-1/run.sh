#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
WS="$ROOT/raw_engine_v1_package"
BUILD_DIR="${BRANCH21_BUILD_DIR:-$WS/build-debug}"
SOLVER="${BRANCH21_SOLVER:-$BUILD_DIR/tests/raw_engine_tests}"

if [[ ! -x "$SOLVER" ]]; then
  echo "[branch_2-1/run.sh] missing solver binary at $SOLVER" >&2
  echo "[branch_2-1/run.sh] run ./build.sh first" >&2
  exit 2
fi

cd "$WS"
exec "$SOLVER" "$@"
