#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
WS="$ROOT/raw_engine_v1_package"
BUILD_DIR="${BRANCH21_BUILD_DIR:-$WS/build-debug}"
CTEST_BIN="${CTEST_BIN:-ctest}"
LABEL="${BRANCH21_CTEST_LABEL:-core}"

exec "$CTEST_BIN" --test-dir "$BUILD_DIR" -L "$LABEL" --output-on-failure "$@"
