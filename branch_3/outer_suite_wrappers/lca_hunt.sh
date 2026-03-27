#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH="$(cd "$SCRIPT_DIR/.." && pwd -P)"
ROOT="$(cd "$BRANCH/.." && pwd -P)"
TOOLING_ROOT="$ROOT/lca_tree_stress_v5/tooling"
export PYTHONDONTWRITEBYTECODE=1
SOLVER="$BRANCH/boj28350_resume/solve"
SOURCE="$BRANCH/boj28350_resume/boj28350_branch_3_solver.cpp"
OUTDIR="$(python3 "$BRANCH/artifact_paths.py" lca_hunt "${1:-}")"
SIZES="${2:-12000,24000,48000,99999}"
SEEDS="${3:-1,2,3}"
TIMEOUT="${4:-8.0}"
ARTIFACTS_ROOT="$BRANCH/artifacts/lca_tree_stress_v5"

usage() {
  cat >&2 <<'EOF'
usage: ./outer_suite_wrappers/lca_hunt.sh [label] [sizes_csv] [seeds_csv] [timeout_sec]
[lca_hunt] diagnostic-only helper for hardest-case search and reporting
[lca_hunt] formal acceptance uses ./outer_suite_wrappers/lca_strong_gate.sh and ./outer_suite_wrappers/lca_boj3s_gate.sh
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

source "$BRANCH/solver_release_env.sh"

case "$OUTDIR" in
  "$ARTIFACTS_ROOT"|"$ARTIFACTS_ROOT"/*)
    ;;
  *)
    echo "[lca_hunt] path escaped branch-local artifacts root: $OUTDIR" >&2
    exit 1
    ;;
esac

mkdir -p "$OUTDIR"
cd "$OUTDIR"

if [[ ! -x "$SOLVER" || "$SOURCE" -nt "$SOLVER" ]]; then
  "$BRANCH/build.sh"
fi

# Keep the upstream hardest-case search intact, but make the branch contract
# explicit: this wrapper is for diagnosis, not for formal acceptance.
echo "[lca_hunt] diagnostic-only run; required acceptance gates are ./outer_suite_wrappers/lca_strong_gate.sh and ./outer_suite_wrappers/lca_boj3s_gate.sh" >&2

exec "$TOOLING_ROOT/hunt.sh" "$SOLVER" "$OUTDIR" "$SIZES" "$SEEDS" "$TIMEOUT"
