#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
WS="$ROOT/ogdf_local_harness_bundle_v2"
CMAKE_BIN="${CMAKE_BIN:-cmake}"
USE_OGDF="${BRANCH1_USE_OGDF:-ON}"

cmd=(
  "$CMAKE_BIN"
  -S "$WS"
  -B "$WS/build"
  -DHARNESS_PROJECT_USE_FREE_FUNCTION_HOOKS=ON
  -DUSE_OGDF="$USE_OGDF"
)

if [[ -n "${OGDF_ROOT:-}" ]]; then
  cmd+=(-DOGDF_ROOT="$OGDF_ROOT")
fi

cmd+=("$@")
"${cmd[@]}"
exec "$CMAKE_BIN" --build "$WS/build" -j
