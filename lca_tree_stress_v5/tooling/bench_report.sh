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
  echo "[bench_report.sh] python interpreter not found" >&2
  exit 127
fi

SOLVER="${1:-./solve}"
OUT="${2:-bench_out}"
SIZES="${3:-9999,19999,39999,79999,99999}"
SEEDS="${4:-1}"
TIMEOUT="${5:-}"
SHUF_L="${6:-1}"
SHUF_Q="${7:-1}"
MODES="${8:-}"

ARGS=(--solver "$SOLVER" --out "$OUT" --sizes "$SIZES" --seeds "$SEEDS")
if [[ -n "$MODES" ]]; then
  ARGS+=(--modes "$MODES")
fi
if [[ "$SHUF_L" == "1" ]]; then
  ARGS+=(--shuffle-labels)
fi
if [[ "$SHUF_Q" == "1" ]]; then
  ARGS+=(--shuffle-queries)
fi
if [[ -n "$TIMEOUT" ]]; then
  ARGS+=(--timeout "$TIMEOUT")
fi

"$PYTHON_BIN" bench_report.py "${ARGS[@]}"
echo
echo "[bench] summary table: $OUT/bench_summary.md"
