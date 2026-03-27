#!/bin/zsh
set -euo pipefail

cd "/Users/free_1/Library/Mobile Documents/iCloud~md~obsidian/Documents/cpp-checker/branch_3"

log_file="${1:?expected launch log file path}"
mkdir -p "$(dirname "$log_file")"

exec >> "$log_file" 2>&1

caffeinate -ims zsh ".ouroboros/run_until_pass_progress40.sh"
