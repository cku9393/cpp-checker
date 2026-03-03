#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

SOLVER="${1:-./solve}"
OUT="${2:-bench_out}"
SIZES="${3:-9999,19999,39999,79999,99999}"
SEEDS="${4:-1}"
TIMEOUT="${5:-}"
SHUF_L="${6:-1}"
SHUF_Q="${7:-1}"
ARG8="${8:-}"
ARG9="${9:-}"

MODES=""
KEEP="0"
if [[ -n "$ARG9" ]]; then
  MODES="$ARG8"
  KEEP="$ARG9"
elif [[ "$ARG8" == "0" || "$ARG8" == "1" ]]; then
  KEEP="$ARG8"
else
  MODES="$ARG8"
fi

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
if [[ "$KEEP" == "1" ]]; then
  ARGS+=(--keep)
fi

python3 bench_report.py "${ARGS[@]}"
echo
echo "[bench] summary table: $OUT/bench_summary.md"
