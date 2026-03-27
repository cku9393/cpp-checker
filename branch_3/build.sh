#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
cd "$SCRIPT_DIR"
export PYTHONDONTWRITEBYTECODE=1
source ./solver_release_env.sh
python3 boj28350_resume.py build "$@"
