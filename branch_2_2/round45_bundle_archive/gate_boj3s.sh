#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
SOLVER="${1:-./solve}"
OUTDIR="${2:-boj3s_out}"
LIMIT_SCALE="${3:-1.0}"
python3 certify_suite.py --solver "$SOLVER" --preset suite_presets/boj_3s_hard_gate.json --out "$OUTDIR" --limit-scale "$LIMIT_SCALE"
