#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(
  unset CDPATH
  cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
  pwd -P
)"
TARGET_WRAPPER="$SCRIPT_DIR/outer_suite_wrappers/lca_smoke_target.sh"

if [[ ! -x "$TARGET_WRAPPER" ]]; then
  echo "[lca_smoke_target] missing executable target wrapper: $TARGET_WRAPPER" >&2
  exit 1
fi

exec /bin/bash "$TARGET_WRAPPER" "$@"
