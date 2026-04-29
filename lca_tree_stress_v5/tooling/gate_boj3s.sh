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
  echo "[gate_boj3s.sh] python interpreter not found" >&2
  exit 127
fi
SOLVER="${1:-./solve}"
OUTDIR="${2:-boj3s_out}"
LIMIT_SCALE="${3:-1.0}"
"$PYTHON_BIN" certify_suite.py --solver "$SOLVER" --preset suite_presets/boj_3s_hard_gate.json --out "$OUTDIR" --limit-scale "$LIMIT_SCALE"
