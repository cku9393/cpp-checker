#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${1:-$PWD}"
PREFIX="${2:-$ROOT_DIR/.deps/ogdf-install}"
SRC_DIR="${3:-$ROOT_DIR/.deps/OGDF}"
BUILD_DIR="${4:-$ROOT_DIR/.deps/ogdf-build}"
HARNESS_BUILD_DIR="${5:-$ROOT_DIR/build}"

"$ROOT_DIR/scripts/install_ogdf_local.sh" "$PREFIX" "$SRC_DIR" "$BUILD_DIR"
OGDF_ROOT="$PREFIX"

cmake -S "$ROOT_DIR" -B "$HARNESS_BUILD_DIR" \
  -DUSE_OGDF=ON \
  -DOGDF_ROOT="$OGDF_ROOT"
cmake --build "$HARNESS_BUILD_DIR" -j

echo "Configured harness with OGDF_ROOT=$OGDF_ROOT"
