#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
SOLVER="${1:-./solve}"
PRESET="${2:-suite_presets/strong_gate.json}"
OUTDIR="${3:-gate_out}"
LIMIT_SCALE="${4:-1.0}"
python3 certify_suite.py --solver "$SOLVER" --preset "$PRESET" --out "$OUTDIR" --limit-scale "$LIMIT_SCALE"
