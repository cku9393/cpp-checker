#!/usr/bin/env bash
set -euo pipefail
MODE="$1"
N="$2"
SEED="$3"
SHUF_L="$4"
SHUF_Q="$5"
SOLVER="$6"
OUTDIR="$7"
mkdir -p "$OUTDIR"
python3 gen_case.py --mode "$MODE" --n "$N" --seed "$SEED" --shuffle-labels --shuffle-queries > "$OUTDIR/in.txt"
export DENSE_SHADOW_CASE_MODE="$MODE"
export DENSE_SHADOW_CASE_N="$N"
export DENSE_SHADOW_CASE_SEED="$SEED"
export DENSE_PROFILE_OUTDIR="$OUTDIR"
export DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
/usr/bin/time -f "%e %M" -o "$OUTDIR/time.txt" timeout 45 "$SOLVER" < "$OUTDIR/in.txt" > "$OUTDIR/out.txt" || true
rc=$?
echo "$rc" > "$OUTDIR/rc.txt"
if [[ "$rc" -eq 0 ]]; then
  python3 validator.py "$OUTDIR/in.txt" "$OUTDIR/out.txt" > "$OUTDIR/validator.txt" || true
fi
