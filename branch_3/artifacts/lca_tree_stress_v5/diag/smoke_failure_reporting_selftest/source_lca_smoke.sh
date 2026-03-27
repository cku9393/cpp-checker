#!/usr/bin/env bash
set -euo pipefail

LCA_SMOKE_CLEAN_ENV_FLAG="LCA_SMOKE_CLEAN_ENV_READY"
LCA_SMOKE_CLEAN_PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"

if [[ "${!LCA_SMOKE_CLEAN_ENV_FLAG:-0}" != "1" ]]; then
  exec /usr/bin/env -i \
    HOME="${HOME:-}" \
    PATH="$LCA_SMOKE_CLEAN_PATH" \
    TERM="${TERM:-dumb}" \
    "$LCA_SMOKE_CLEAN_ENV_FLAG=1" \
    /usr/bin/env bash "$0" "$@"
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"
cd "$BRANCH_ROOT"
SUITE_ROOT="$(cd "$BRANCH_ROOT/.." && pwd -P)"
export PYTHONDONTWRITEBYTECODE=1
SOLVER="$BRANCH_ROOT/boj28350_resume/solve"
SOURCE="$BRANCH_ROOT/boj28350_resume/boj28350_branch_3_solver.cpp"
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
BUILD_WRAPPER="$BRANCH_ROOT/build.sh"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
RUN_CASE_HELPER="$BRANCH_ROOT/branch_run_case.py"
SMOKE_CASES="$BRANCH_ROOT/boj28350_resume/smoke_cases.tsv"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
RUN_STAGE_ROOT="$ARTIFACTS_ROOT/staging/lca_smoke_work"
FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
LEGACY_TMP_GLOB="lca_smoke.*"
LEGACY_OUT_GLOB=".lca_smoke_in_progress.*"
RUN_WORK_GLOB="run.*"
RUN_WORK_TEMPLATE="run.XXXXXX"
LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
LOCKDIR="$LOCK_ROOT/lca_smoke"
LOCK_PID_FILE="$LOCKDIR/pid"
OUTROOT=""
OUTPARENT=""
BACKUP_ROOT=""
WORKDIR=""
LOCK_HELD=0
CURRENT_CASE_STAGE=""
CURRENT_CASE_MODE=""
CURRENT_CASE_N=""
CURRENT_CASE_SEED=""
CURRENT_CASE_SHUFFLE_LABELS=""
CURRENT_CASE_SHUFFLE_QUERIES=""
CURRENT_CASE_TIMEOUT=""
CURRENT_CASE_TAG=""
CURRENT_CASE_MANIFEST_ROW=""
CURRENT_CASE_STDOUT=""
CURRENT_CASE_STDERR=""
CURRENT_CASE_EXEC_COMMAND=""
CURRENT_CASE_REPRO_COMMAND=""
CURRENT_CASE_PRESERVED_INPUT_COMMAND=""
CURRENT_CASE_REPRO_DIR=""
CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR=""
CURRENT_CASE_SOLVER_SNAPSHOT=""
CURRENT_FAILURE_RC=0

sanitize_shell_state() {
  unset CDPATH BASH_ENV ENV GLOBIGNORE
  unalias -a 2>/dev/null || true
  set +f
  shopt -u dotglob extglob failglob nocaseglob nullglob
}

fail() {
  echo "[lca_smoke] $*" >&2
  exit 1
}

usage() {
  echo "usage: ./outer_suite_wrappers/lca_smoke.sh" >&2
  echo "[lca_smoke] smoke output is fixed to branch-local artifacts" >&2
  exit 2
}

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    fail "missing required tool: $1"
  fi
}

require_file() {
  local path="$1"
  local label="$2"
  if [[ ! -f "$path" ]]; then
    fail "missing ${label}: $path"
  fi
}

require_executable() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    fail "missing executable ${label}: $path"
  fi
  if [[ ! -f "$path" ]]; then
    fail "executable ${label} is not a regular file: $path"
  fi
  if [[ ! -x "$path" ]]; then
    fail "missing executable ${label}: $path"
  fi
}

remove_path_retry() {
  local target="$1"
  local attempt
  for attempt in 1 2 3 4 5; do
    if [[ ! -e "$target" ]]; then
      return 0
    fi
    if [[ -d "$target" && ! -L "$target" ]]; then
      rm -rf "$target" 2>/dev/null || true
      python3 - "$target" <<'PY' >/dev/null 2>&1 || true
import os
import shutil
import sys

path = sys.argv[1]
try:
    if os.path.islink(path) or os.path.isfile(path):
        os.unlink(path)
    elif os.path.isdir(path):
        shutil.rmtree(path, ignore_errors=True)
except FileNotFoundError:
    pass
PY
    else
      rm -f "$target" 2>/dev/null || true
    fi
    if [[ ! -e "$target" ]]; then
      return 0
    fi
    sleep 0.1
  done
  return 1
}

ensure_under_artifacts() {
  local path="$1"
  case "$path" in
    "$ARTIFACTS_ROOT"|"$ARTIFACTS_ROOT"/*)
      ;;
    *)
      fail "path escaped branch-local artifacts root: $path"
      ;;
  esac
}

enter_function_errexit() {
  local __state_var="$1"
  if [[ $- == *e* ]]; then
    printf -v "$__state_var" '1'
    return
  fi
  printf -v "$__state_var" '0'
  set -e
}

restore_function_errexit() {
  local prior_state="${1:-1}"
  if [[ "$prior_state" == "0" ]]; then
    set +e
  fi
}

resolve_output_roots() {
  OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_smoke)"
  if [[ -z "$OUTROOT" ]]; then
    fail "artifact resolver returned an empty smoke output path"
  fi
  OUTPARENT="$(dirname "$OUTROOT")"
  BACKUP_ROOT="${OUTROOT}.previous"

  ensure_under_artifacts "$OUTROOT"
  ensure_under_artifacts "$OUTPARENT"
  ensure_under_artifacts "$BACKUP_ROOT"
  ensure_under_artifacts "$FAILURE_ROOT"
}

configure_runtime_environment() {
  umask 022
  export LC_ALL=C
  export LANG=C
  export PATH="$LCA_SMOKE_CLEAN_PATH"
  export PYTHONIOENCODING=UTF-8
  export PYTHONUTF8=1
  export PYTHONNOUSERSITE=1
  export PYTHONHASHSEED=0
  export TZ=UTC
  export BRANCH_ROOT
  export SUITE_ROOT
  export BRANCH_ARTIFACT_TMP_ROOT="$TMP_PARENT"
  export TMPDIR="$TMP_PARENT"
  export TMP="$TMP_PARENT"
  export TEMP="$TMP_PARENT"
  export LCA_SMOKE_ARTIFACT_ROOT="$ARTIFACTS_ROOT"
  export LCA_SMOKE_OUTROOT="$OUTROOT"
  export LCA_SMOKE_STAGE_ROOT="$RUN_STAGE_ROOT"

  ensure_under_artifacts "$BRANCH_ARTIFACT_TMP_ROOT"
  ensure_under_artifacts "$LCA_SMOKE_ARTIFACT_ROOT"
  ensure_under_artifacts "$LCA_SMOKE_OUTROOT"
  ensure_under_artifacts "$LCA_SMOKE_STAGE_ROOT"
}

assert_runtime_environment() {
  local env_name=""
  if [[ "$PWD" != "$BRANCH_ROOT" ]]; then
    fail "runtime cwd drifted outside branch root: $PWD"
  fi
  for env_name in BRANCH_ARTIFACT_TMP_ROOT TMPDIR TMP TEMP; do
    if [[ -z "${!env_name:-}" ]]; then
      fail "required runtime variable is unset: $env_name"
    fi
    if [[ "${!env_name}" != "$TMP_PARENT" ]]; then
      fail "runtime variable $env_name must resolve to $TMP_PARENT (got: ${!env_name})"
    fi
    ensure_under_artifacts "${!env_name}"
  done
  if [[ "${LCA_SMOKE_OUTROOT:-}" != "$OUTROOT" ]]; then
    fail "LCA_SMOKE_OUTROOT drifted from resolved output path"
  fi
  if [[ "${LCA_SMOKE_STAGE_ROOT:-}" != "$RUN_STAGE_ROOT" ]]; then
    fail "LCA_SMOKE_STAGE_ROOT drifted from staging root"
  fi
}

restore_previous_output() {
  if [[ -n "$BACKUP_ROOT" && -e "$BACKUP_ROOT" && ! -e "$OUTROOT" ]]; then
    mv "$BACKUP_ROOT" "$OUTROOT"
  fi
}

clear_stale_state() {
  local stale
  restore_previous_output
  if [[ -d "$TMP_PARENT" ]]; then
    shopt -s nullglob
    for stale in "$TMP_PARENT"/$LEGACY_TMP_GLOB; do
      remove_path_retry "$stale" || fail "failed to clear stale temp path: $stale"
    done
    shopt -u nullglob
  fi
  if [[ -d "$OUTPARENT" ]]; then
    shopt -s nullglob
    for stale in "$OUTPARENT"/$LEGACY_OUT_GLOB; do
      remove_path_retry "$stale" || fail "failed to clear stale legacy output path: $stale"
    done
    shopt -u nullglob
  fi
  if [[ -d "$RUN_STAGE_ROOT" ]]; then
    remove_path_retry "$RUN_STAGE_ROOT" || fail "failed to clear stale run staging root: $RUN_STAGE_ROOT"
    mkdir -p "$RUN_STAGE_ROOT"
  fi
  if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
    remove_path_retry "$BACKUP_ROOT" || fail "failed to clear stale backup path: $BACKUP_ROOT"
  fi
}

clear_stale_failure_state() {
  if [[ -e "$FAILURE_ROOT" ]]; then
    remove_path_retry "$FAILURE_ROOT" || fail "failed to clear stale failure root: $FAILURE_ROOT"
  fi
}

release_lock() {
  if (( LOCK_HELD )) && [[ -d "$LOCKDIR" ]]; then
    remove_path_retry "$LOCKDIR" || fail "failed to release smoke lock: $LOCKDIR"
  fi
  LOCK_HELD=0
  rmdir "$LOCK_ROOT" 2>/dev/null || true
}

acquire_lock() {
  local holder=""
  mkdir -p "$LOCK_ROOT"
  while true; do
    if mkdir "$LOCKDIR" 2>/dev/null; then
      printf '%s\n' "$$" > "$LOCK_PID_FILE"
      LOCK_HELD=1
      return
    fi

    if [[ ! -f "$LOCK_PID_FILE" ]]; then
      sleep 0.05
      if [[ ! -f "$LOCK_PID_FILE" ]]; then
        remove_path_retry "$LOCKDIR" || fail "failed to clear stale smoke lock: $LOCKDIR"
      fi
      continue
    fi

    read -r holder < "$LOCK_PID_FILE" || holder=""
    if [[ -z "$holder" ]]; then
      sleep 0.05
      if [[ -f "$LOCK_PID_FILE" ]]; then
        continue
      fi
      remove_path_retry "$LOCKDIR" || fail "failed to clear empty smoke lock: $LOCKDIR"
      continue
    fi

    if kill -0 "$holder" 2>/dev/null; then
      fail "another lca_smoke.sh run is active (pid $holder)"
    fi

    remove_path_retry "$LOCKDIR" || fail "failed to clear stale smoke lock: $LOCKDIR"
  done
}

cleanup() {
  local rc="${1:-$?}"
  trap - EXIT
  set +e
  if [[ -n "${WORKDIR:-}" && -e "$WORKDIR" ]]; then
    remove_path_retry "$WORKDIR" || true
  fi
  if (( rc != 0 )); then
    restore_previous_output
  fi
  if [[ -e "$BACKUP_ROOT" && -e "$OUTROOT" ]]; then
    remove_path_retry "$BACKUP_ROOT" || true
  fi
  release_lock
  remove_path_retry "$RUN_STAGE_ROOT" 2>/dev/null || true
  rmdir "$(dirname "$RUN_STAGE_ROOT")" 2>/dev/null || true
  rmdir "$TMP_PARENT" 2>/dev/null || true
  exit "$rc"
}

quote_command() {
  local quoted=""
  local word=""
  for word in "$@"; do
    printf -v quoted '%s%q ' "$quoted" "$word"
  done
  printf '%s\n' "${quoted% }"
}

quote_word() {
  local quoted=""
  printf -v quoted '%q' "$1"
  printf '%s\n' "$quoted"
}

write_failure_runtime_env() {
  local runtime_env_txt="$FAILURE_ROOT/runtime_env.txt"
  local runtime_env_exports="$FAILURE_ROOT/runtime_env_exports.sh"
  python3 - "$runtime_env_txt" "$runtime_env_exports" <<'PY'
from __future__ import annotations

import os
import shlex
import sys
from pathlib import Path

runtime_env_txt = Path(sys.argv[1])
runtime_env_exports = Path(sys.argv[2])

base_names = {
    "HOME",
    "PATH",
    "TERM",
    "LC_ALL",
    "LANG",
    "TZ",
    "LOCAL_SKIP_SELF_TEST",
    "BRANCH_ROOT",
    "SUITE_ROOT",
    "BRANCH_ARTIFACT_TMP_ROOT",
    "TMPDIR",
    "TMP",
    "TEMP",
    "LCA_SMOKE_ARTIFACT_ROOT",
    "LCA_SMOKE_OUTROOT",
    "LCA_SMOKE_STAGE_ROOT",
}
prefixes = ("ENABLE_", "PROFILE_", "PYTHON")

selected: dict[str, str] = {}
for key in sorted(os.environ):
    if key in base_names or key.startswith(prefixes):
        selected[key] = os.environ[key]

runtime_env_txt.write_text(
    "".join(f"{key}={value}\n" for key, value in selected.items()),
    encoding="utf-8",
)

export_lines = ["#!/usr/bin/env bash", "set -euo pipefail", ""]
for key, value in selected.items():
    export_lines.append(f"export {key}={shlex.quote(value)}")
runtime_env_exports.write_text("\n".join(export_lines) + "\n", encoding="utf-8")
runtime_env_exports.chmod(0o755)
PY
}

write_failure_artifact_manifest() {
  local failure_case_dir="$1"
  local manifest_path="$FAILURE_ROOT/artifact_manifest.tsv"
  python3 - "$failure_case_dir" "$manifest_path" "$FAILURE_ROOT" "$CURRENT_CASE_SOLVER_SNAPSHOT" <<'PY'
from __future__ import annotations

import hashlib
import sys
from pathlib import Path

failure_case_dir = Path(sys.argv[1])
manifest_path = Path(sys.argv[2])
failure_root = Path(sys.argv[3])
solver_snapshot = Path(sys.argv[4])

entries = [
    ("failed_case_row", failure_root / "failed_case_row.tsv"),
    ("smoke_manifest_snapshot", failure_root / "smoke_cases_manifest.tsv"),
    ("runtime_env", failure_root / "runtime_env.txt"),
    ("runtime_env_exports", failure_root / "runtime_env_exports.sh"),
    ("commands", failure_root / "commands.txt"),
    ("seed_repro_script", failure_root / "repro_from_seed.sh"),
    ("preserved_input_replay_script", failure_root / "replay_preserved_input.sh"),
    ("solver_snapshot", solver_snapshot),
    ("case_input", failure_case_dir / "in.txt"),
    ("case_meta", failure_case_dir / "meta.json"),
    ("case_hidden_parent", failure_case_dir / "hidden_parent.txt"),
    ("case_output", failure_case_dir / "out.txt"),
    ("case_time", failure_case_dir / "time.txt"),
    ("case_solver_stderr", failure_case_dir / "solver_stderr.txt"),
    ("case_helper_stdout", failure_case_dir / "run_case.stdout.txt"),
    ("case_helper_stderr", failure_case_dir / "run_case.stderr.txt"),
]

rows = ["label\tpath\texists\tbytes\tsha256"]
for label, path in entries:
    exists = path.exists()
    size = path.stat().st_size if exists and path.is_file() else -1
    digest = "-"
    if exists and path.is_file():
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
    rows.append(
        "\t".join(
            [
                label,
                str(path),
                "1" if exists else "0",
                str(size),
                digest,
            ]
        )
    )

manifest_path.write_text("\n".join(rows) + "\n", encoding="utf-8")
PY
}

write_failure_debug_bundle() {
  local failure_case_dir="$1"
  local commands_txt="$FAILURE_ROOT/commands.txt"
  local seed_repro_script="$FAILURE_ROOT/repro_from_seed.sh"
  local preserved_input_script="$FAILURE_ROOT/replay_preserved_input.sh"
  local env_exports_path="$FAILURE_ROOT/runtime_env_exports.sh"

  printf '%s\n' "$CURRENT_CASE_MANIFEST_ROW" > "$FAILURE_ROOT/failed_case_row.tsv"
  cp "$SMOKE_CASES" "$FAILURE_ROOT/smoke_cases_manifest.tsv"
  write_failure_runtime_env

  {
    echo "executed_command=$CURRENT_CASE_EXEC_COMMAND"
    echo "seed_repro_command=$CURRENT_CASE_REPRO_COMMAND"
    echo "preserved_input_command=$CURRENT_CASE_PRESERVED_INPUT_COMMAND"
    echo "seed_repro_script=$seed_repro_script"
    echo "preserved_input_replay_script=$preserved_input_script"
    echo "runtime_env_exports=$env_exports_path"
    echo "solver_snapshot=$CURRENT_CASE_SOLVER_SNAPSHOT"
    echo "seed_repro_dir=$CURRENT_CASE_REPRO_DIR"
    echo "preserved_input_replay_dir=$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR"
  } > "$commands_txt"

  cat > "$seed_repro_script" <<EOF
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="\$(cd -- "\$(dirname -- "\${BASH_SOURCE[0]}")" && pwd -P)"
source "\$SCRIPT_DIR/runtime_env_exports.sh"
exec $CURRENT_CASE_REPRO_COMMAND
EOF
  chmod +x "$seed_repro_script"

  cat > "$preserved_input_script" <<EOF
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="\$(cd -- "\$(dirname -- "\${BASH_SOURCE[0]}")" && pwd -P)"
source "\$SCRIPT_DIR/runtime_env_exports.sh"
if [[ -n "\${BRANCH_ARTIFACT_TMP_ROOT:-}" ]]; then
  mkdir -p "\$BRANCH_ARTIFACT_TMP_ROOT"
fi
REPLAY_DIR="\${1:-$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR}"
rm -rf "\$REPLAY_DIR"
mkdir -p "\$REPLAY_DIR"
env \
  DENSE_SHADOW_CASE_MODE=$CURRENT_CASE_MODE \
  DENSE_SHADOW_CASE_N=$CURRENT_CASE_N \
  DENSE_SHADOW_CASE_SEED=$CURRENT_CASE_SEED \
  DENSE_PROFILE_OUTDIR="\$REPLAY_DIR" \
  DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1 \
  "$CURRENT_CASE_SOLVER_SNAPSHOT" \
  < "$failure_case_dir/in.txt" \
  > "\$REPLAY_DIR/out.txt" \
  2> "\$REPLAY_DIR/solver_stderr.txt"
echo "[lca_smoke] preserved-input replay artifacts: \$REPLAY_DIR"
EOF
  chmod +x "$preserved_input_script"

  write_failure_artifact_manifest "$failure_case_dir"
}

write_failure_summary() {
  local failure_case_dir="$1"
  local failure_summary="$FAILURE_ROOT/failure_summary.txt"
  local failure_report="$FAILURE_ROOT/latest_failure_report.md"
  local artifact_manifest="$FAILURE_ROOT/artifact_manifest.tsv"
  local env_snapshot="$FAILURE_ROOT/runtime_env.txt"
  local env_exports="$FAILURE_ROOT/runtime_env_exports.sh"
  local seed_repro_script="$FAILURE_ROOT/repro_from_seed.sh"
  local preserved_input_script="$FAILURE_ROOT/replay_preserved_input.sh"
  local manifest_snapshot="$FAILURE_ROOT/smoke_cases_manifest.tsv"
  local failed_case_row="$FAILURE_ROOT/failed_case_row.tsv"
  mkdir -p "$FAILURE_ROOT"
  write_failure_debug_bundle "$failure_case_dir"
  {
    echo "script=./outer_suite_wrappers/lca_smoke.sh"
    echo "exit_code=$CURRENT_FAILURE_RC"
    echo "failed_stage=$CURRENT_CASE_STAGE"
    echo "failed_mode=$CURRENT_CASE_MODE"
    echo "failed_n=$CURRENT_CASE_N"
    echo "failed_seed=$CURRENT_CASE_SEED"
    echo "failed_shuffle_labels=$CURRENT_CASE_SHUFFLE_LABELS"
    echo "failed_shuffle_queries=$CURRENT_CASE_SHUFFLE_QUERIES"
    echo "failed_timeout_s=$CURRENT_CASE_TIMEOUT"
    echo "failed_case_tag=$CURRENT_CASE_TAG"
    echo "manifest_row=$CURRENT_CASE_MANIFEST_ROW"
    echo "executed_command=$CURRENT_CASE_EXEC_COMMAND"
    echo "repro_command=$CURRENT_CASE_REPRO_COMMAND"
    echo "preserved_input_command=$CURRENT_CASE_PRESERVED_INPUT_COMMAND"
    echo "failure_root=$FAILURE_ROOT"
    echo "failure_case_dir=$failure_case_dir"
    echo "helper_stdout=$CURRENT_CASE_STDOUT"
    echo "helper_stderr=$CURRENT_CASE_STDERR"
    echo "solver_snapshot=$CURRENT_CASE_SOLVER_SNAPSHOT"
    echo "seed_repro_dir=$CURRENT_CASE_REPRO_DIR"
    echo "preserved_input_replay_dir=$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR"
    echo "failed_case_row_path=$failed_case_row"
    echo "manifest_snapshot_path=$manifest_snapshot"
    echo "runtime_env_path=$env_snapshot"
    echo "runtime_env_exports_path=$env_exports"
    echo "artifact_manifest_path=$artifact_manifest"
    echo "seed_repro_script=$seed_repro_script"
    echo "preserved_input_replay_script=$preserved_input_script"
    echo "input_path=$failure_case_dir/in.txt"
    echo "meta_path=$failure_case_dir/meta.json"
    echo "hidden_parent_path=$failure_case_dir/hidden_parent.txt"
    echo "output_path=$failure_case_dir/out.txt"
    echo "time_path=$failure_case_dir/time.txt"
    echo "solver_stderr_path=$failure_case_dir/solver_stderr.txt"
    echo "smoke_output_root=$OUTROOT"
    echo "smoke_manifest=$SMOKE_CASES"
  } > "$failure_summary"

  {
    echo "# lca_smoke Failure Report"
    echo
    echo "- Exit code: \`$CURRENT_FAILURE_RC\`"
    echo "- Case tag: \`$CURRENT_CASE_TAG\`"
    echo "- Stage: \`$CURRENT_CASE_STAGE\`"
    echo "- Mode: \`$CURRENT_CASE_MODE\`"
    echo "- n: \`$CURRENT_CASE_N\`"
    echo "- Seed: \`$CURRENT_CASE_SEED\`"
    echo "- Shuffle labels: \`$CURRENT_CASE_SHUFFLE_LABELS\`"
    echo "- Shuffle queries: \`$CURRENT_CASE_SHUFFLE_QUERIES\`"
    echo "- Timeout (s): \`$CURRENT_CASE_TIMEOUT\`"
    echo "- Manifest row: \`$CURRENT_CASE_MANIFEST_ROW\`"
    echo "- Failure root: \`$FAILURE_ROOT\`"
    echo "- Failure case dir: \`$failure_case_dir\`"
    echo "- Smoke output root: \`$OUTROOT\`"
    echo "- Smoke manifest: \`$SMOKE_CASES\`"
    echo "- Helper stdout: \`$CURRENT_CASE_STDOUT\`"
    echo "- Helper stderr: \`$CURRENT_CASE_STDERR\`"
    echo "- Frozen solver snapshot: \`$CURRENT_CASE_SOLVER_SNAPSHOT\`"
    echo "- Failed row snapshot: \`$failed_case_row\`"
    echo "- Smoke manifest snapshot: \`$manifest_snapshot\`"
    echo "- Runtime env snapshot: \`$env_snapshot\`"
    echo "- Runtime env exports: \`$env_exports\`"
    echo "- Artifact manifest: \`$artifact_manifest\`"
    echo "- Seed repro script: \`$seed_repro_script\`"
    echo "- Preserved-input replay script: \`$preserved_input_script\`"
    echo
    echo "## Commands"
    echo
    echo "Executed command:"
    echo
    echo "\`\`\`bash"
    echo "$CURRENT_CASE_EXEC_COMMAND"
    echo "\`\`\`"
    echo
    echo "Preferred seed repro invocation:"
    echo
    echo "\`\`\`bash"
    echo "bash $(quote_word "$seed_repro_script")"
    echo "\`\`\`"
    echo
    echo "Raw seed repro command body recorded for low-level debugging:"
    echo
    echo "\`\`\`bash"
    echo "$CURRENT_CASE_REPRO_COMMAND"
    echo "\`\`\`"
    echo
    echo "Preferred preserved-input replay invocation:"
    echo
    echo "\`\`\`bash"
    echo "bash $(quote_word "$preserved_input_script")"
    echo "\`\`\`"
    echo
    echo "Raw preserved-input replay command body recorded for low-level debugging:"
    echo
    echo "\`\`\`bash"
    echo "$CURRENT_CASE_PRESERVED_INPUT_COMMAND"
    echo "\`\`\`"
    echo
    echo "## Artifact Paths"
    echo
    echo "- input: \`$failure_case_dir/in.txt\`"
    echo "- meta: \`$failure_case_dir/meta.json\`"
    echo "- hidden parent: \`$failure_case_dir/hidden_parent.txt\`"
    echo "- solver output: \`$failure_case_dir/out.txt\`"
    echo "- timing: \`$failure_case_dir/time.txt\`"
    echo "- solver stderr: \`$failure_case_dir/solver_stderr.txt\`"
    echo "- helper stdout: \`$CURRENT_CASE_STDOUT\`"
    echo "- helper stderr: \`$CURRENT_CASE_STDERR\`"
    echo "- artifact manifest: \`$artifact_manifest\`"
    echo
    echo "## Preserved Debug Bundle"
    echo
    echo "- \`solver_snapshot\` freezes the exact failing binary."
    echo "- \`runtime_env_exports.sh\` restores the branch-local release env that the failure used."
    echo "- \`repro_from_seed.sh\` regenerates the same seed into \`$CURRENT_CASE_REPRO_DIR\` without overwriting the preserved failure tree."
    echo "- \`replay_preserved_input.sh\` reruns the frozen solver directly on the preserved \`in.txt\` without regenerating the case."
    echo "- \`artifact_manifest.tsv\` records existence, size, and SHA-256 for every preserved debug artifact."
    if [[ -s "$failure_case_dir/time.txt" ]]; then
      echo
      echo "## Timing Artifact"
      echo
      echo "\`\`\`text"
      cat "$failure_case_dir/time.txt"
      echo "\`\`\`"
    fi
    if [[ -s "$failure_case_dir/solver_stderr.txt" ]]; then
      echo
      echo "## Solver stderr tail"
      echo
      echo "\`\`\`text"
      tail -n 20 "$failure_case_dir/solver_stderr.txt"
      echo "\`\`\`"
    fi
    if [[ -s "$CURRENT_CASE_STDERR" ]]; then
      echo
      echo "## Helper stderr tail"
      echo
      echo "\`\`\`text"
      tail -n 20 "$CURRENT_CASE_STDERR"
      echo "\`\`\`"
    fi
    if [[ -s "$CURRENT_CASE_STDOUT" ]]; then
      echo
      echo "## Helper stdout tail"
      echo
      echo "\`\`\`text"
      tail -n 20 "$CURRENT_CASE_STDOUT"
      echo "\`\`\`"
    fi
  } > "$failure_report"
}

preserve_failure_artifacts() {
  local -a repro_cmd
  local -a replay_cmd
  local solver_for_repro="$SOLVER"
  local failure_case_dir=""
  local input_q=""
  local out_q=""
  local stderr_q=""
  local replay_prefix=""
  if [[ -z "${WORKDIR:-}" || ! -d "$WORKDIR" ]]; then
    return
  fi
  remove_path_retry "$FAILURE_ROOT" || fail "failed to clear previous failure root: $FAILURE_ROOT"
  mkdir -p "$(dirname "$FAILURE_ROOT")"
  mv "$WORKDIR" "$FAILURE_ROOT"
  WORKDIR=""
  failure_case_dir="$FAILURE_ROOT/$CURRENT_CASE_TAG"
  CURRENT_CASE_STDOUT="$FAILURE_ROOT/$CURRENT_CASE_TAG/run_case.stdout.txt"
  CURRENT_CASE_STDERR="$FAILURE_ROOT/$CURRENT_CASE_TAG/run_case.stderr.txt"
  CURRENT_CASE_SOLVER_SNAPSHOT="$FAILURE_ROOT/solver_snapshot"
  if [[ -x "$SOLVER" ]]; then
    cp "$SOLVER" "$CURRENT_CASE_SOLVER_SNAPSHOT"
    chmod +x "$CURRENT_CASE_SOLVER_SNAPSHOT"
    solver_for_repro="$CURRENT_CASE_SOLVER_SNAPSHOT"
  else
    CURRENT_CASE_SOLVER_SNAPSHOT="$SOLVER"
  fi
  CURRENT_CASE_REPRO_DIR="$FAILURE_ROOT/repro_from_seed/$CURRENT_CASE_TAG"
  CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR="$FAILURE_ROOT/replay_from_input/$CURRENT_CASE_TAG"
  repro_cmd=(
    python3
    "$RUN_CASE_HELPER"
    "$CURRENT_CASE_MODE"
    "$CURRENT_CASE_N"
    "$CURRENT_CASE_SEED"
    "$CURRENT_CASE_SHUFFLE_LABELS"
    "$CURRENT_CASE_SHUFFLE_QUERIES"
    "$solver_for_repro"
    "$CURRENT_CASE_REPRO_DIR"
    --timeout
    "$CURRENT_CASE_TIMEOUT"
    --env
    "DENSE_SHADOW_CASE_MODE=$CURRENT_CASE_MODE"
    --env
    "DENSE_SHADOW_CASE_N=$CURRENT_CASE_N"
    --env
    "DENSE_SHADOW_CASE_SEED=$CURRENT_CASE_SEED"
    --env
    "DENSE_PROFILE_OUTDIR=$CURRENT_CASE_REPRO_DIR"
    --env
    DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
  )
  CURRENT_CASE_REPRO_COMMAND="$(quote_command "${repro_cmd[@]}")"
  replay_cmd=(
    env
    "DENSE_SHADOW_CASE_MODE=$CURRENT_CASE_MODE"
    "DENSE_SHADOW_CASE_N=$CURRENT_CASE_N"
    "DENSE_SHADOW_CASE_SEED=$CURRENT_CASE_SEED"
    "DENSE_PROFILE_OUTDIR=$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR"
    DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
    "$solver_for_repro"
  )
  replay_prefix="$(quote_command "${replay_cmd[@]}")"
  input_q="$(quote_word "$failure_case_dir/in.txt")"
  out_q="$(quote_word "$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR/out.txt")"
  stderr_q="$(quote_word "$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR/solver_stderr.txt")"
  CURRENT_CASE_PRESERVED_INPUT_COMMAND="mkdir -p $(quote_word "$CURRENT_CASE_PRESERVED_INPUT_REPLAY_DIR") && ${replay_prefix} < ${input_q} > ${out_q} 2> ${stderr_q}"
}

report_failure_context() {
  local failure_case_dir="$FAILURE_ROOT/$CURRENT_CASE_TAG"
  echo "[lca_smoke] smoke case failed with exit code $CURRENT_FAILURE_RC" >&2
  echo "[lca_smoke] failed case: tag=$CURRENT_CASE_TAG stage=$CURRENT_CASE_STAGE mode=$CURRENT_CASE_MODE n=$CURRENT_CASE_N seed=$CURRENT_CASE_SEED shuffle_labels=$CURRENT_CASE_SHUFFLE_LABELS shuffle_queries=$CURRENT_CASE_SHUFFLE_QUERIES timeout_s=$CURRENT_CASE_TIMEOUT" >&2
  echo "[lca_smoke] manifest row: $CURRENT_CASE_MANIFEST_ROW" >&2
  echo "[lca_smoke] executed command: $CURRENT_CASE_EXEC_COMMAND" >&2
  echo "[lca_smoke] repro command: $CURRENT_CASE_REPRO_COMMAND" >&2
  echo "[lca_smoke] preserved-input replay command: $CURRENT_CASE_PRESERVED_INPUT_COMMAND" >&2
  echo "[lca_smoke] preserved failure root: $FAILURE_ROOT" >&2
  echo "[lca_smoke] preserved case dir: $failure_case_dir" >&2
  echo "[lca_smoke] helper stdout: $CURRENT_CASE_STDOUT" >&2
  echo "[lca_smoke] helper stderr: $CURRENT_CASE_STDERR" >&2
  echo "[lca_smoke] solver snapshot: $CURRENT_CASE_SOLVER_SNAPSHOT" >&2
  echo "[lca_smoke] artifact manifest: $FAILURE_ROOT/artifact_manifest.tsv" >&2
  echo "[lca_smoke] seed repro script: $FAILURE_ROOT/repro_from_seed.sh" >&2
  echo "[lca_smoke] preserved-input replay script: $FAILURE_ROOT/replay_preserved_input.sh" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/in.txt" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/meta.json" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/hidden_parent.txt" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/out.txt" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/time.txt" >&2
  echo "[lca_smoke] artifact: $failure_case_dir/solver_stderr.txt" >&2
  echo "[lca_smoke] stable smoke output root: $OUTROOT" >&2
  echo "[lca_smoke] smoke manifest: $SMOKE_CASES" >&2
  echo "[lca_smoke] failure summary: $FAILURE_ROOT/failure_summary.txt" >&2
  echo "[lca_smoke] failure report: $FAILURE_ROOT/latest_failure_report.md" >&2
  if [[ -s "$CURRENT_CASE_STDERR" ]]; then
    echo "[lca_smoke] helper stderr tail:" >&2
    tail -n 20 "$CURRENT_CASE_STDERR" >&2
  elif [[ -s "$CURRENT_CASE_STDOUT" ]]; then
    echo "[lca_smoke] helper stdout tail:" >&2
    tail -n 20 "$CURRENT_CASE_STDOUT" >&2
  fi
}

handle_signal() {
  local rc="$1"
  trap - HUP INT TERM
  exit "$rc"
}

build_solver_if_needed() {
  local build_rc=0
  local prior_errexit=1

  enter_function_errexit prior_errexit

  if [[ ! -x "$SOLVER" || "$SOURCE" -nt "$SOLVER" ]]; then
    if "$BUILD_WRAPPER"; then
      :
    else
      build_rc=$?
      echo "[lca_smoke] build wrapper failed with exit code $build_rc" >&2
      restore_function_errexit "$prior_errexit"
      return "$build_rc"
    fi
  fi
  require_executable "$SOLVER" "solver binary"
  restore_function_errexit "$prior_errexit"
}

load_release_environment() {
  source "$RELEASE_ENV"
  assert_runtime_environment
}

run_smoke_case() {
  local stage="$1"
  local mode="$2"
  local n="$3"
  local seed="$4"
  local shuffle_labels="$5"
  local shuffle_queries="$6"
  local case_tag="$7"
  local timeout_s="$8"
  local case_dir="$WORKDIR/$case_tag"
  local rc=0
  local prior_errexit=1
  local -a cmd

  enter_function_errexit prior_errexit
  mkdir -p "$case_dir"
  CURRENT_CASE_STAGE="$stage"
  CURRENT_CASE_MODE="$mode"
  CURRENT_CASE_N="$n"
  CURRENT_CASE_SEED="$seed"
  CURRENT_CASE_SHUFFLE_LABELS="$shuffle_labels"
  CURRENT_CASE_SHUFFLE_QUERIES="$shuffle_queries"
  CURRENT_CASE_TIMEOUT="$timeout_s"
  CURRENT_CASE_TAG="$case_tag"
  CURRENT_CASE_MANIFEST_ROW="$(printf '%s\t%s\t%s\t%s\t%s\t%s\t%s' "$stage" "$mode" "$n" "$seed" "$shuffle_labels" "$shuffle_queries" "$timeout_s")"
  CURRENT_CASE_STDOUT="$case_dir/run_case.stdout.txt"
  CURRENT_CASE_STDERR="$case_dir/run_case.stderr.txt"
  CURRENT_CASE_REPRO_COMMAND=""
  cmd=(
    python3
    "$RUN_CASE_HELPER"
    "$mode"
    "$n"
    "$seed"
    "$shuffle_labels"
    "$shuffle_queries"
    "$SOLVER"
    "$case_dir"
    --timeout
    "$timeout_s"
    --env
    "DENSE_SHADOW_CASE_MODE=$mode"
    --env
    "DENSE_SHADOW_CASE_N=$n"
    --env
    "DENSE_SHADOW_CASE_SEED=$seed"
    --env
    "DENSE_PROFILE_OUTDIR=$case_dir"
    --env
    DENSE_DECOMPOSESERIES_ROUND45_SHADOWCHECK=1
  )
  CURRENT_CASE_EXEC_COMMAND="$(quote_command "${cmd[@]}")"

  if "${cmd[@]}" >"$CURRENT_CASE_STDOUT" 2>"$CURRENT_CASE_STDERR"; then
    restore_function_errexit "$prior_errexit"
    return 0
  fi

  rc=$?
  CURRENT_FAILURE_RC="$rc"
  preserve_failure_artifacts
  write_failure_summary "$FAILURE_ROOT/$case_tag"
  report_failure_context
  restore_function_errexit "$prior_errexit"
  return "$rc"
}

run_smoke_suite() {
  local stage=""
  local mode=""
  local n=""
  local seed=""
  local shuffle_labels=""
  local shuffle_queries=""
  local timeout_s=""
  local rc=0
  local prior_errexit=1

  enter_function_errexit prior_errexit
  while IFS=$'\t' read -r stage mode n seed shuffle_labels shuffle_queries timeout_s; do
    if [[ -z "$stage" || "$stage" == "stage" ]]; then
      continue
    fi
    set +e
    run_smoke_case "$stage" "$mode" "$n" "$seed" "$shuffle_labels" "$shuffle_queries" "${stage}_${mode}_${n}_s${seed}" "$timeout_s"
    rc=$?
    set -e
    if (( rc == 0 )); then
      continue
    fi
    restore_function_errexit "$prior_errexit"
    return "$rc"
  done < "$SMOKE_CASES"
  restore_function_errexit "$prior_errexit"
}

publish_output() {
  local outleaf="${OUTROOT##*/}"

  if [[ ! -d "$WORKDIR" ]]; then
    fail "staging output directory disappeared before publish: $WORKDIR"
  fi

  mkdir -p "$OUTPARENT"
  if [[ -e "$OUTROOT" ]]; then
    remove_path_retry "$BACKUP_ROOT" || fail "failed to clear backup path before publish: $BACKUP_ROOT"
    mv "$OUTROOT" "$BACKUP_ROOT"
  fi
  mv "$WORKDIR" "$OUTPARENT/$outleaf"
  WORKDIR=""
  remove_path_retry "$BACKUP_ROOT" || fail "failed to clear backup path after publish: $BACKUP_ROOT"
}

trap 'cleanup "$?"' EXIT
trap 'handle_signal 129' HUP
trap 'handle_signal 130' INT
trap 'handle_signal 143' TERM

run_main() {
  local smoke_rc=0

  sanitize_shell_state
  mkdir -p "$ARTIFACTS_ROOT"
  resolve_output_roots
  configure_runtime_environment
  acquire_lock

  if [[ -e "$OUTROOT" && ! -d "$OUTROOT" ]]; then
    fail "output path exists but is not a directory: $OUTROOT"
  fi
  if [[ -e "$BACKUP_ROOT" && ! -d "$BACKUP_ROOT" ]]; then
    fail "backup path exists but is not a directory: $BACKUP_ROOT"
  fi

  mkdir -p "$OUTPARENT" "$TMP_PARENT" "$RUN_STAGE_ROOT"
  assert_runtime_environment
  clear_stale_state
  WORKDIR="$(mktemp -d "$RUN_STAGE_ROOT/$RUN_WORK_TEMPLATE")"

  if [[ -z "$WORKDIR" ]]; then
    fail "mktemp returned an empty smoke staging directory"
  fi
  ensure_under_artifacts "$WORKDIR"

  set +e
  build_solver_if_needed
  smoke_rc=$?
  set -e
  if (( smoke_rc != 0 )); then
    return "$smoke_rc"
  fi
  load_release_environment

  set +e
  run_smoke_suite
  smoke_rc=$?
  set -e
  if (( smoke_rc != 0 )); then
    return "$smoke_rc"
  fi
  publish_output
  clear_stale_failure_state
}

main() {
  if (( $# != 0 )); then
    usage
  fi

  require_command python3
  require_command mktemp
  require_command dirname
  require_file "$SOURCE" "solver source"
  require_file "$ARTIFACT_RESOLVER" "artifact resolver"
  require_file "$RELEASE_ENV" "release env wrapper"
  require_file "$RUN_CASE_HELPER" "branch-local case helper"
  require_file "$SMOKE_CASES" "smoke case manifest"
  require_executable "$BUILD_WRAPPER" "build wrapper"

  run_main
}

