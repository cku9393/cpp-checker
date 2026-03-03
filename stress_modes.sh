#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

SOLVER="${1:-./solve}"
SEED="${2:-1}"
SHUFFLE_LABELS="${3:-1}"
SHUFFLE_QUERIES="${4:-1}"

MODES=(
  comb_core
  comb_plus_unary
  comb_dense
  chain_unary
  star_pairs
  balanced_sibling
  balanced_dense
  broom_mixed
  caterpillar_mixed
  random_recursive_mixed
)

SIZES=(9999 19999 39999 79999 99999)

mkdir -p _runs
for mode in "${MODES[@]}"; do
  echo "===== $mode ====="
  for n in "${SIZES[@]}"; do
    outdir="_runs/${mode}_${n}_${SEED}"
    ./run_case.sh "$mode" "$n" "$SEED" "$SHUFFLE_LABELS" "$SHUFFLE_QUERIES" "$SOLVER" "$outdir"
  done
  echo
 done
