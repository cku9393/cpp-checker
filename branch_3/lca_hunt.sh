#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

usage() {
  cat >&2 <<'EOF'
usage: ./lca_hunt.sh [label] [sizes_csv] [seeds_csv] [timeout_sec]
[lca_hunt] optional diagnostic helper for hardest-case search/reporting
[lca_hunt] not part of formal acceptance; required gates are ./lca_strong_gate.sh and ./lca_boj3s_gate.sh
[lca_hunt] forwards to ./outer_suite_wrappers/lca_hunt.sh
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

exec "$SCRIPT_DIR/outer_suite_wrappers/lca_hunt.sh" "$@"
