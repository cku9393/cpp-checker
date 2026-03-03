#!/usr/bin/env bash
set -euo pipefail

PREFIX="${1:-$PWD/.deps/ogdf-install}"
SRC_DIR="${2:-$PWD/.deps/OGDF}"
BUILD_DIR="${3:-$PWD/.deps/ogdf-build}"

mkdir -p "$(dirname "$PREFIX")"
mkdir -p "$(dirname "$SRC_DIR")"
mkdir -p "$(dirname "$BUILD_DIR")"

if [ ! -d "$SRC_DIR/.git" ]; then
  git clone --depth 1 https://github.com/ogdf/ogdf.git "$SRC_DIR"
fi

cmake -S "$SRC_DIR" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DOGDF_WARNING_ERRORS=OFF

cmake --build "$BUILD_DIR" -j
cmake --install "$BUILD_DIR"

echo "OGDF installed at: $PREFIX"
