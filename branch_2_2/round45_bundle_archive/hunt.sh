#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
SOLVER="${1:-./solve}"
OUTDIR="${2:-hunt_out}"
SIZES="${3:-12000,24000,48000,99999}"
SEEDS="${4:-1,2,3}"
TIMEOUT="${5:-8.0}"
python3 hunt_hardest.py --solver "$SOLVER" --out "$OUTDIR" --sizes "$SIZES" --seeds "$SEEDS" --timeout "$TIMEOUT"
