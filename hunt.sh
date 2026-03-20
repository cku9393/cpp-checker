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
  echo "[hunt.sh] python interpreter not found" >&2
  exit 127
fi
SOLVER="${1:-./solve}"
OUTDIR="${2:-hunt_out}"
SIZES="${3:-12000,24000,48000,99999}"
SEEDS="${4:-1,2,3}"
TIMEOUT="${5:-8.0}"
"$PYTHON_BIN" hunt_hardest.py --solver "$SOLVER" --out "$OUTDIR" --sizes "$SIZES" --seeds "$SEEDS" --timeout "$TIMEOUT"
