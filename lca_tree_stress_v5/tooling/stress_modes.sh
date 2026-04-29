#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
if [[ -n "${PYTHON:-}" ]]; then
  PYTHON_BIN="$PYTHON"
elif command -v python3 >/dev/null 2>&1; then
  PYTHON_BIN=python3
elif command -v python >/dev/null 2>&1; then
  PYTHON_BIN=python
else
  echo "[stress_modes.sh] python interpreter not found" >&2
  exit 127
fi
exec "$PYTHON_BIN" stress_modes.py "$@"
