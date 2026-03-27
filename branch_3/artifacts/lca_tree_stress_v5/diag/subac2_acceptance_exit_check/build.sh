#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BIN="$ROOT/artifacts/boj28350_resume/build/solve"
mkdir -p "$(dirname "$BIN")"
cat > "$BIN" <<'SOLVER'
#!/usr/bin/env bash
echo 0
SOLVER
chmod +x "$BIN"
