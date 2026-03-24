#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
WS="$ROOT/ogdf_local_harness_bundle_v2"
HARNESS="${BRANCH1_SOLVER:-$WS/build/rewrite_r_harness}"

if [[ ! -x "$HARNESS" ]]; then
  echo "[branch_1/run.sh] missing solver binary at $HARNESS" >&2
  echo "[branch_1/run.sh] run ./build.sh first" >&2
  exit 2
fi

cd "$WS"
exec "$HARNESS" "$@"
