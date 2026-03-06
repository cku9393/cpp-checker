#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./build/rewrite_r_harness}"
SEED="${2:-1}"
ROUNDS_STATIC="${3:-1000}"
ROUNDS_DUMMY="${4:-1000}"

mkdir -p dumps/static dumps/dummy

echo "[1/4] static manual-only"
"$BIN" \
  --backend ogdf \
  --mode static \
  --seed "$SEED" \
  --rounds 1 \
  --manual-only \
  --dump-dir dumps/static \
  --dry-run 0

echo "[2/4] static random"
"$BIN" \
  --backend ogdf \
  --mode static \
  --seed "$SEED" \
  --rounds "$ROUNDS_STATIC" \
  --dump-dir dumps/static \
  --dry-run 0

echo "[3/4] dummy manual-only"
"$BIN" \
  --backend ogdf \
  --mode dummy \
  --seed "$SEED" \
  --rounds 1 \
  --manual-only \
  --dump-dir dumps/dummy \
  --dry-run 0

echo "[4/4] dummy random"
"$BIN" \
  --backend ogdf \
  --mode dummy \
  --seed "$SEED" \
  --rounds "$ROUNDS_DUMMY" \
  --dump-dir dumps/dummy \
  --dry-run 0
