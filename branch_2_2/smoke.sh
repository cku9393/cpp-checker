#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
exec python3 round45_resume.py smoke "$@"
