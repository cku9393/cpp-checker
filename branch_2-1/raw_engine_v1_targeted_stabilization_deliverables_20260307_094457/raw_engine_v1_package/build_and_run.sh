#!/usr/bin/env bash
set -euo pipefail
CMAKE_BIN=${CMAKE_BIN:-cmake}
CTEST_BIN=${CTEST_BIN:-ctest}

"$CMAKE_BIN" -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
"$CMAKE_BIN" --build build-debug -j
"$CTEST_BIN" --test-dir build-debug --output-on-failure
