#!/usr/bin/env bash
set -euo pipefail

BRANCH="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$BRANCH/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
SOLVER="$BRANCH/boj28350_resume/solve"
SOURCE="$BRANCH/boj28350_resume/boj28350_branch_3_solver.cpp"
OUTROOT="$(python3 "$BRANCH/artifact_paths.py" lca_smoke "${1:-}")"
OUTPARENT="$(dirname "$OUTROOT")"
TMP_PARENT="$BRANCH/artifacts/lca_tree_stress_v5/.tmp"
BACKUP_ROOT="${OUTROOT}.previous"

mkdir -p "$OUTPARENT" "$TMP_PARENT"

WORKDIR="$(mktemp -d "$TMP_PARENT/lca_smoke.XXXXXX")"

restore_previous_output() {
  if [[ -e "$BACKUP_ROOT" && ! -e "$OUTROOT" ]]; then
    mv "$BACKUP_ROOT" "$OUTROOT"
  fi
}

cleanup() {
  local rc=$?
  if [[ -n "${WORKDIR:-}" && -e "$WORKDIR" ]]; then
    rm -rf "$WORKDIR"
  fi
  if (( rc != 0 )); then
    restore_previous_output
  fi
  if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
    rm -rf "$BACKUP_ROOT"
  fi
  rmdir "$TMP_PARENT" 2>/dev/null || true
  exit "$rc"
}

trap cleanup EXIT

restore_previous_output
if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
  rm -rf "$BACKUP_ROOT"
fi

if [[ ! -x "$SOLVER" || "$SOURCE" -nt "$SOLVER" ]]; then
  "$BRANCH/build.sh"
fi

source "$BRANCH/solver_release_env.sh"

"$ROOT/lca_tree_stress_v5/smoke.sh" "$SOLVER" "$WORKDIR"
if [[ -e "$OUTROOT" ]]; then
  rm -rf "$BACKUP_ROOT"
  mv "$OUTROOT" "$BACKUP_ROOT"
fi
mv "$WORKDIR" "$OUTROOT"
WORKDIR=""
rm -rf "$BACKUP_ROOT"
