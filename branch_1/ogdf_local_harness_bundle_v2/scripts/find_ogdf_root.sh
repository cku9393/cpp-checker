#!/usr/bin/env bash
set -euo pipefail

candidates=("$@" /opt/homebrew /usr/local /usr "$HOME/.local" "$PWD/.deps/ogdf-install" "$PWD/.deps/ogdf-build")

for base in "${candidates[@]}"; do
  [ -e "$base" ] || continue

  # install prefix candidates: include/ogdf + lib/libOGDF*
  if [ -d "$base/include/ogdf" ]; then
    if ls "$base"/lib/libOGDF* >/dev/null 2>&1 || ls "$base"/lib64/libOGDF* >/dev/null 2>&1; then
      echo "$base"
    fi
  fi

  # build-tree candidates: build/include + root has libOGDF*
  if [ -d "$base/include/ogdf" ] && { ls "$base"/libOGDF* >/dev/null 2>&1 || ls "$base"/libCOIN* >/dev/null 2>&1; }; then
    echo "$base"
  fi

done | awk '!seen[$0]++'
