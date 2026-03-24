#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

MODE="${1:?mode}"
N="${2:?n}"
SEED="${3:-1}"
SHUFFLE_LABELS="${4:-0}"
SHUFFLE_QUERIES="${5:-0}"
SOLVER="${6:-./solve}"
OUTDIR="${7:-_case}"

mkdir -p "$OUTDIR"
GEN_ARGS=(--mode "$MODE" --n "$N" --seed "$SEED" --meta "$OUTDIR/meta.json" --parent-out "$OUTDIR/hidden_parent.txt")
if [[ "$SHUFFLE_LABELS" == "1" ]]; then
  GEN_ARGS+=(--shuffle-labels)
fi
if [[ "$SHUFFLE_QUERIES" == "1" ]]; then
  GEN_ARGS+=(--shuffle-queries)
fi

python3 gen_case.py "${GEN_ARGS[@]}" > "$OUTDIR/in.txt"
/usr/bin/time -f "%e %M" -o "$OUTDIR/time.txt" "$SOLVER" < "$OUTDIR/in.txt" > "$OUTDIR/out.txt"
python3 validator.py "$OUTDIR/in.txt" "$OUTDIR/out.txt"
read -r ELAPSED MEM < "$OUTDIR/time.txt"
echo "[run_case] mode=$MODE n=$N seed=$SEED time=${ELAPSED}s mem=${MEM}KB"
echo "[run_case] artifacts: $OUTDIR/in.txt $OUTDIR/out.txt $OUTDIR/meta.json"
