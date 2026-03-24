#!/usr/bin/env bash
set -euo pipefail

BRANCH="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$BRANCH/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
SOLVER="$BRANCH/boj28350_resume/solve"
SOURCE="$BRANCH/boj28350_resume/boj28350_branch_3_solver.cpp"
OUTDIR="$(python3 "$BRANCH/artifact_paths.py" lca_strong_gate "${1:-}")"
LIMIT_SCALE="${2:-1.0}"
PRESET="$ROOT/suite_presets/strong_gate.json"
STAGE_FILTER="${LCA_STAGE_FILTER:-}"
TMP_PARENT="$BRANCH/artifacts/lca_tree_stress_v5/.tmp"
WORKDIR=""

source "$BRANCH/solver_release_env.sh"

cleanup() {
  if [[ -n "${WORKDIR:-}" && -e "$WORKDIR" ]]; then
    rm -rf "$WORKDIR"
  fi
  rmdir "$TMP_PARENT" 2>/dev/null || true
}

if [[ -n "$STAGE_FILTER" ]]; then
  mkdir -p "$TMP_PARENT"
  WORKDIR="$(mktemp -d "$TMP_PARENT/lca_strong_gate.XXXXXX")"
  PRESET="$WORKDIR/preset.json"
  python3 - "$ROOT/suite_presets/strong_gate.json" "$STAGE_FILTER" "$PRESET" <<'PY'
import json
import pathlib
import sys

src = pathlib.Path(sys.argv[1])
stage_name = sys.argv[2]
dst = pathlib.Path(sys.argv[3])

with src.open(encoding="utf-8") as f:
    preset = json.load(f)

stages = [stage for stage in preset.get("stages", []) if stage.get("name") == stage_name]
if not stages:
    raise SystemExit(f"[lca_strong_gate] unknown stage filter: {stage_name}")

preset["stages"] = stages
with dst.open("w", encoding="utf-8") as f:
    json.dump(preset, f, indent=2)
    f.write("\n")
PY
  trap cleanup EXIT
fi

if [[ ! -x "$SOLVER" || "$SOURCE" -nt "$SOLVER" ]]; then
  "$BRANCH/build.sh"
fi

if [[ -n "$STAGE_FILTER" ]]; then
  "$ROOT/gate.sh" "$SOLVER" "$PRESET" "$OUTDIR" "$LIMIT_SCALE"
  exit $?
fi

exec "$ROOT/gate.sh" "$SOLVER" "$PRESET" "$OUTDIR" "$LIMIT_SCALE"
