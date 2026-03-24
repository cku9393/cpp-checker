#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
WS="$ROOT/raw_engine_v1_package"
BUILD_DIR="${BRANCH21_BUILD_DIR:-$WS/build-debug}"
BUILD_TYPE="${BRANCH21_BUILD_TYPE:-Debug}"
CMAKE_BIN="${CMAKE_BIN:-cmake}"

"$CMAKE_BIN" -S "$WS" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "$@"
exec "$CMAKE_BIN" --build "$BUILD_DIR" -j
