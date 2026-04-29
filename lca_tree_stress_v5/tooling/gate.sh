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
  echo "[gate.sh] python interpreter not found" >&2
  exit 127
fi
SOLVER="${1:-./solve}"
PRESET="${2:-suite_presets/strong_gate.json}"
OUTDIR="${3:-gate_out}"
LIMIT_SCALE="${4:-1.0}"
"$PYTHON_BIN" certify_suite.py --solver "$SOLVER" --preset "$PRESET" --out "$OUTDIR" --limit-scale "$LIMIT_SCALE"
