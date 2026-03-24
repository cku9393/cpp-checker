#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
WS="$ROOT/ogdf_local_harness_bundle_v2"
DUMP_DIR="${BRANCH1_SMOKE_OUT:-$WS/dumps/branch_smoke}"

if [[ $# -eq 0 ]]; then
  set -- \
    --backend "${BRANCH1_SMOKE_BACKEND:-ogdf}" \
    --mode rewrite-seq \
    --seed 1 \
    --rounds 10 \
    --dump-dir "$DUMP_DIR"
fi

exec "$ROOT/run.sh" "$@"
