#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"
cd "$BRANCH_ROOT"
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
SMOKE_WRAPPER="$SCRIPT_DIR/lca_smoke.sh"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
SMOKE_FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
STAGE_PARENT="$ARTIFACTS_ROOT/.repeatability_stage"
RUN_WORK_TEMPLATE="lca_smoke_repeatability.XXXXXX"
LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
LOCKDIR="$LOCK_ROOT/lca_smoke_repeatability"
LOCK_PID_FILE="$LOCKDIR/pid"
OUTROOT=""
OUTPARENT=""
BACKUP_ROOT=""
WORKDIR=""
LOCK_HELD=0
RUN_COUNT="${LCA_SMOKE_REPEAT_COUNT:-3}"
COMPLETED_RUNS=0
FAILURE_REASON=""
FAILURE_HINT=""

export PYTHONDONTWRITEBYTECODE=1

fail() {
  echo "[lca_smoke_repeatability] $*" >&2
  exit 1
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

usage() {
  cat >&2 <<'EOF'
usage: ./outer_suite_wrappers/lca_smoke_repeatability.sh [repeat-count]
[lca_smoke_repeatability] repeat-count defaults to $LCA_SMOKE_REPEAT_COUNT or 3
[lca_smoke_repeatability] output is fixed to branch-local artifacts
EOF
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
  if [[ ! -x "$path" ]]; then
    fail "missing executable ${label}: $path"
  fi
}

move_path_retry() {
  local source="$1"
  local target="$2"
  local attempt
  local target_parent=""

  for attempt in 1 2 3 4 5; do
    if [[ ! -e "$source" ]]; then
      if [[ -e "$target" ]]; then
        return 0
      fi
      sleep 0.1
      continue
    fi

    target_parent="$(dirname "$target")"
    mkdir -p "$target_parent" 2>/dev/null || true
    if mv "$source" "$target" 2>/dev/null; then
      return 0
    fi
    if [[ ! -e "$source" && -e "$target" ]]; then
      return 0
    fi
    sleep 0.1
  done
  return 1
}

release_lock() {
  if (( LOCK_HELD )) && [[ -d "$LOCKDIR" ]]; then
    rm -rf "$LOCKDIR"
  fi
  LOCK_HELD=0
  rmdir "$LOCK_ROOT" 2>/dev/null || true
}

acquire_lock() {
  local holder=""
  mkdir -p "$LOCK_ROOT"
  if mkdir "$LOCKDIR" 2>/dev/null; then
    printf '%s\n' "$$" > "$LOCK_PID_FILE"
    LOCK_HELD=1
    return
  fi

  if [[ -f "$LOCK_PID_FILE" ]]; then
    read -r holder < "$LOCK_PID_FILE" || holder=""
  fi
  if [[ -n "$holder" ]] && kill -0 "$holder" 2>/dev/null; then
    fail "another lca_smoke_repeatability.sh run is active (pid $holder)"
  fi

  rm -rf "$LOCKDIR"
  if ! mkdir "$LOCKDIR" 2>/dev/null; then
    fail "failed to acquire repeatability lock: $LOCKDIR"
  fi
  printf '%s\n' "$$" > "$LOCK_PID_FILE"
  LOCK_HELD=1
}

clear_stale_state() {
  local stale
  if [[ -d "$STAGE_PARENT" ]]; then
    shopt -s nullglob
    for stale in "$STAGE_PARENT"/lca_smoke_repeatability.*; do
      rm -rf "$stale"
    done
    shopt -u nullglob
  fi
}

publish_output() {
  local outleaf

  if [[ -z "$OUTROOT" || -z "$WORKDIR" || ! -d "$WORKDIR" ]]; then
    return
  fi

  outleaf="${OUTROOT##*/}"
  mkdir -p "$OUTPARENT"
  if [[ -e "$OUTROOT" ]]; then
    rm -rf "$BACKUP_ROOT"
    move_path_retry "$OUTROOT" "$BACKUP_ROOT" || fail "failed to rotate prior repeatability output into backup: $OUTROOT -> $BACKUP_ROOT"
  fi
  move_path_retry "$WORKDIR" "$OUTPARENT/$outleaf" || fail "failed to publish repeatability output: $WORKDIR -> $OUTPARENT/$outleaf"
  WORKDIR=""
  rm -rf "$BACKUP_ROOT"
}

cleanup() {
  local rc="${1:-$?}"
  trap - EXIT
  set +e
  if [[ -n "${WORKDIR:-}" && -d "$WORKDIR" ]]; then
    if [[ ! -f "$WORKDIR/summary.txt" ]]; then
      {
        echo "status=FAIL"
        echo "requested_runs=$RUN_COUNT"
        echo "completed_runs=$COMPLETED_RUNS"
        echo "failure_reason=${FAILURE_REASON:-interrupted before summary generation}"
        echo "failure_hint=${FAILURE_HINT:-inspect run logs under runs/}"
        echo "ignored_files=time.txt"
        echo "normalized_files=run_case.stdout.txt"
        echo "normalized_run_case_stdout_lines=[run_case] mode=... time=... mem=... ; [run_case] artifacts: ..."
      } > "$WORKDIR/summary.txt"
    fi
    publish_output
  fi
  release_lock
  rmdir "$STAGE_PARENT" 2>/dev/null || true
  rmdir "$TMP_PARENT" 2>/dev/null || true
  exit "$rc"
}

validate_run_count() {
  case "$RUN_COUNT" in
    ''|*[!0-9]*)
      fail "repeat-count must be an integer >= 2 (got: $RUN_COUNT)"
      ;;
  esac
  if (( RUN_COUNT < 2 )); then
    fail "repeat-count must be at least 2 (got: $RUN_COUNT)"
  fi
}

run_smoke_once() {
  local run_dir="$1"
  local rc=0

  mkdir -p "$run_dir"
  set +e
  LCA_SMOKE_EXPORT_SNAPSHOT_ROOT="$run_dir/smoke_snapshot" \
    "$SMOKE_WRAPPER" >"$run_dir/lca_smoke.stdout.txt" 2>"$run_dir/lca_smoke.stderr.txt"
  rc=$?
  set -e
  if [[ ! -d "$run_dir" ]]; then
    FAILURE_REASON="repeatability run workspace disappeared during smoke execution"
    FAILURE_HINT="inspect shared smoke cleanup behavior and rerun with preserved wrapper logs for $run_dir"
    fail "$FAILURE_REASON: $run_dir; $FAILURE_HINT"
  fi
  printf '%s\n' "$rc" > "$run_dir/exit_code.txt"
  return "$rc"
}

snapshot_smoke_output() {
  local run_dir="$1"
  local snapshot_root="$run_dir/smoke_snapshot"

  if [[ ! -d "$snapshot_root" ]]; then
    if [[ ! -d "$SMOKE_OUTROOT" ]]; then
      fail "expected wrapper-exported smoke snapshot or published smoke output after successful run: $snapshot_root"
    fi
    cp -R "$SMOKE_OUTROOT" "$snapshot_root"
  fi
  python3 - "$snapshot_root" "$run_dir/manifest.tsv" "$run_dir/timings.tsv" <<'PY'
from __future__ import annotations

import json
import hashlib
import pathlib
import re
import sys

snapshot = pathlib.Path(sys.argv[1])
manifest_path = pathlib.Path(sys.argv[2])
timings_path = pathlib.Path(sys.argv[3])
manifest_rows = []
timing_rows = []
run_case_metric_re = re.compile(r"^\[run_case\] mode=.* time=.* mem=.*$")
run_case_artifact_re = re.compile(r"^\[run_case\] artifacts: .*$")


def canonical_bytes(file_path: pathlib.Path) -> bytes:
    raw = file_path.read_bytes()
    if file_path.name == "run_case_result.json":
        payload = json.loads(raw.decode("utf-8"))
        payload.pop("sec", None)
        payload.pop("rss_kb", None)
        return (json.dumps(payload, sort_keys=True, ensure_ascii=False, indent=2) + "\n").encode("utf-8")

    if file_path.name == "environment_validation.txt":
        stable_lines = []
        for line in raw.decode("utf-8").splitlines():
            if line.startswith("external_snapshot_root="):
                stable_lines.append("external_snapshot_root=<normalized>")
                continue
            stable_lines.append(line)
        return ("\n".join(stable_lines) + "\n").encode("utf-8")

    if file_path.name != "run_case.stdout.txt":
        return raw

    text = raw.decode("utf-8")
    stable_lines = []
    for line in text.splitlines():
        if run_case_metric_re.match(line):
            continue
        if run_case_artifact_re.match(line):
            continue
        stable_lines.append(line)

    normalized = "\n".join(stable_lines)
    if text.endswith("\n"):
        normalized += "\n"
    return normalized.encode("utf-8")

for file_path in sorted(snapshot.rglob("*")):
    if not file_path.is_file():
        continue
    rel = file_path.relative_to(snapshot).as_posix()
    if file_path.name == "time.txt":
        timing_rows.append(rel + "\t" + file_path.read_text(encoding="utf-8").strip())
        continue
    digest = hashlib.sha256(canonical_bytes(file_path)).hexdigest()
    manifest_rows.append(rel + "\t" + digest)

manifest_path.write_text("\n".join(manifest_rows) + ("\n" if manifest_rows else ""), encoding="utf-8")
timings_path.write_text("\n".join(timing_rows) + ("\n" if timing_rows else ""), encoding="utf-8")
PY
}

snapshot_smoke_failure() {
  local run_dir="$1"
  local snapshot_root="$run_dir/failure_snapshot"
  local signature_path="$run_dir/failure_signature.txt"

  mkdir -p "$snapshot_root"
  if [[ -d "$SMOKE_FAILURE_ROOT" ]]; then
    cp -R "$SMOKE_FAILURE_ROOT"/. "$snapshot_root"
  fi

  python3 - "$run_dir/exit_code.txt" "$snapshot_root" "$signature_path" <<'PY'
from __future__ import annotations

from pathlib import Path
import sys

exit_code_path = Path(sys.argv[1])
snapshot_root = Path(sys.argv[2])
signature_path = Path(sys.argv[3])
summary_path = snapshot_root / "failure_summary.txt"
report_path = snapshot_root / "latest_failure_report.md"
keys = [
    "exit_code",
    "helper_exit_code",
    "failure_kind",
    "failure_origin",
    "failure_retryable",
    "failure_summary",
    "solver_exit_code",
    "solver_signal",
    "failed_stage",
    "failed_case_index",
    "failed_mode",
    "failed_n",
    "failed_seed",
    "failed_shuffle_labels",
    "failed_shuffle_queries",
    "failed_timeout_s",
    "failed_case_tag",
    "manifest_row",
]

entries: dict[str, str] = {}
for line in summary_path.read_text(encoding="utf-8").splitlines() if summary_path.exists() else []:
    if "=" not in line:
        continue
    key, value = line.split("=", 1)
    entries[key] = value

lines = [
    f"run_exit_code={exit_code_path.read_text(encoding='utf-8').strip()}",
    f"failure_root_state={'present' if any(snapshot_root.iterdir()) else 'missing'}",
    f"failure_summary_state={'present' if summary_path.exists() else 'missing'}",
    f"failure_report_state={'present' if report_path.exists() else 'missing'}",
]
for key in keys:
    if key in entries:
        lines.append(f"{key}={entries[key]}")

signature_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
PY
}

compare_snapshot_manifests() {
  local baseline_run_dir="$1"
  local current_run_dir="$2"
  local report_path="$current_run_dir/manifest_diff.txt"
  local baseline_rel="${baseline_run_dir#$WORKDIR/}"
  local current_rel="${current_run_dir#$WORKDIR/}"

  python3 - "$baseline_run_dir/manifest.tsv" "$current_run_dir/manifest.tsv" "$report_path" "$baseline_rel" "$current_rel" <<'PY'
from __future__ import annotations

import pathlib
import sys

baseline_manifest = pathlib.Path(sys.argv[1])
current_manifest = pathlib.Path(sys.argv[2])
report_path = pathlib.Path(sys.argv[3])
baseline_rel = sys.argv[4]
current_rel = sys.argv[5]


def load_manifest(path: pathlib.Path) -> dict[str, str]:
    rows: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        rel, digest = line.split("\t", 1)
        rows[rel] = digest
    return rows


baseline = load_manifest(baseline_manifest)
current = load_manifest(current_manifest)
missing = sorted(set(baseline) - set(current))
extra = sorted(set(current) - set(baseline))
changed = sorted(rel for rel in baseline.keys() & current.keys() if baseline[rel] != current[rel])

lines = [
    f"baseline_run={baseline_rel}",
    f"current_run={current_rel}",
    f"baseline_snapshot={baseline_rel}/smoke_snapshot",
    f"current_snapshot={current_rel}/smoke_snapshot",
    "ignored_files=time.txt",
    "normalized_files=run_case.stdout.txt",
    "normalized_run_case_stdout_lines=[run_case] mode=... time=... mem=... ; [run_case] artifacts: ...",
]
if missing:
    lines.append("missing_files:")
    lines.extend(missing[:20])
if extra:
    lines.append("extra_files:")
    lines.extend(extra[:20])
if changed:
    lines.append("changed_files:")
    for rel in changed[:20]:
      lines.append(f"{rel}\tbaseline={baseline[rel]}\tcurrent={current[rel]}")

report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
if missing or extra or changed:
    sys.exit(1)
PY
}

compare_failure_signatures() {
  local baseline_run_dir="$1"
  local current_run_dir="$2"
  local report_path="$current_run_dir/failure_signature_diff.txt"
  local baseline_rel="${baseline_run_dir#$WORKDIR/}"
  local current_rel="${current_run_dir#$WORKDIR/}"

  python3 - "$baseline_run_dir/failure_signature.txt" "$current_run_dir/failure_signature.txt" "$report_path" "$baseline_rel" "$current_rel" <<'PY'
from __future__ import annotations

import pathlib
import sys

baseline_path = pathlib.Path(sys.argv[1])
current_path = pathlib.Path(sys.argv[2])
report_path = pathlib.Path(sys.argv[3])
baseline_rel = sys.argv[4]
current_rel = sys.argv[5]


def load_signature(path: pathlib.Path) -> dict[str, str]:
    rows: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        rows[key] = value
    return rows


baseline = load_signature(baseline_path)
current = load_signature(current_path)
keys = sorted(set(baseline) | set(current))
changed = [key for key in keys if baseline.get(key) != current.get(key)]

lines = [
    f"baseline_run={baseline_rel}",
    f"current_run={current_rel}",
    f"baseline_signature={baseline_rel}/failure_signature.txt",
    f"current_signature={current_rel}/failure_signature.txt",
]
for key in changed:
    lines.append(
        f"{key}\tbaseline={baseline.get(key, '<missing>')}\tcurrent={current.get(key, '<missing>')}"
    )

report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
if changed:
    sys.exit(1)
PY
}

format_outcome_label() {
  local rc="$1"
  if [[ "$rc" == "0" ]]; then
    printf 'PASS:0\n'
  else
    printf 'FAIL:%s\n' "$rc"
  fi
}

write_summary() {
  local status="$1"
  local baseline_label="$2"
  local baseline_run_dir="$3"
  local latest_run_dir="$4"
  local baseline_outcome="$5"
  local latest_outcome="$6"

  {
    echo "status=$status"
    echo "requested_runs=$RUN_COUNT"
    echo "completed_runs=$COMPLETED_RUNS"
    echo "baseline_run=$baseline_label"
    echo "latest_run=${latest_run_dir#$WORKDIR/}"
    echo "baseline_outcome=$baseline_outcome"
    echo "latest_outcome=$latest_outcome"
    if [[ "$status" == "FAIL" ]]; then
      echo "outcome_consistency=diverged"
    else
      echo "outcome_consistency=matching"
    fi
    if [[ "$baseline_outcome" == PASS:* ]]; then
      echo "baseline_snapshot=${baseline_run_dir#$WORKDIR/}/smoke_snapshot"
    else
      echo "baseline_failure_snapshot=${baseline_run_dir#$WORKDIR/}/failure_snapshot"
      echo "baseline_failure_signature=${baseline_run_dir#$WORKDIR/}/failure_signature.txt"
    fi
    if [[ "$latest_outcome" == PASS:* ]]; then
      echo "latest_snapshot=${latest_run_dir#$WORKDIR/}/smoke_snapshot"
    else
      echo "latest_failure_snapshot=${latest_run_dir#$WORKDIR/}/failure_snapshot"
      echo "latest_failure_signature=${latest_run_dir#$WORKDIR/}/failure_signature.txt"
    fi
    if [[ -n "$FAILURE_REASON" ]]; then
      echo "failure_reason=$FAILURE_REASON"
    fi
    if [[ -n "$FAILURE_HINT" ]]; then
      echo "failure_hint=$FAILURE_HINT"
    fi
    echo "ignored_files=time.txt"
    echo "normalized_files=run_case.stdout.txt"
    echo "normalized_run_case_stdout_lines=[run_case] mode=... time=... mem=... ; [run_case] artifacts: ..."
    echo "live_smoke_output=$SMOKE_OUTROOT"
    echo "live_smoke_failure_root=$SMOKE_FAILURE_ROOT"
  } > "$WORKDIR/summary.txt"
}

if (( $# > 1 )); then
  usage
fi
if (( $# == 1 )); then
  RUN_COUNT="$1"
fi

trap 'cleanup "$?"' EXIT

require_command python3
require_command mktemp
require_command cp
require_command dirname
require_file "$ARTIFACT_RESOLVER" "artifact resolver"
require_file "$RELEASE_ENV" "release env wrapper"
require_executable "$SMOKE_WRAPPER" "smoke wrapper"
validate_run_count

source "$RELEASE_ENV"

ensure_under_artifacts "$ARTIFACTS_ROOT"
ensure_under_artifacts "$TMP_PARENT"
ensure_under_artifacts "$STAGE_PARENT"
ensure_under_artifacts "$LOCK_ROOT"
mkdir -p "$ARTIFACTS_ROOT"
acquire_lock
SMOKE_OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_smoke)"
OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_smoke_repeatability)"
OUTPARENT="$(dirname "$OUTROOT")"
BACKUP_ROOT="${OUTROOT}.previous"
ensure_under_artifacts "$SMOKE_OUTROOT"
ensure_under_artifacts "$OUTROOT"
ensure_under_artifacts "$OUTPARENT"
ensure_under_artifacts "$BACKUP_ROOT"
if [[ -e "$OUTROOT" && ! -d "$OUTROOT" ]]; then
  fail "output path exists but is not a directory: $OUTROOT"
fi
if [[ -e "$BACKUP_ROOT" && ! -d "$BACKUP_ROOT" ]]; then
  fail "backup path exists but is not a directory: $BACKUP_ROOT"
fi

mkdir -p "$TMP_PARENT"
mkdir -p "$STAGE_PARENT"
clear_stale_state
WORKDIR="$(mktemp -d "$STAGE_PARENT/$RUN_WORK_TEMPLATE")"
ensure_under_artifacts "$WORKDIR"
mkdir -p "$WORKDIR/runs"

baseline_run_dir=""
baseline_label=""
latest_run_dir=""
baseline_outcome=""
latest_outcome=""
run_exit_code=""

for (( run_index = 1; run_index <= RUN_COUNT; ++run_index )); do
  run_label="$(printf 'run%02d' "$run_index")"
  run_dir="$WORKDIR/runs/$run_label"
  latest_run_dir="$run_dir"

  if run_smoke_once "$run_dir"; then
    run_exit_code="0"
  else
    run_exit_code="$(
      if [[ -f "$run_dir/exit_code.txt" ]]; then
        <"$run_dir/exit_code.txt"
      else
        printf 'missing'
      fi
    )"
  fi

  latest_outcome="$(format_outcome_label "$run_exit_code")"
  printf '%s\n' "$latest_outcome" > "$run_dir/outcome.txt"

  if [[ "$run_exit_code" == "0" ]]; then
    snapshot_smoke_output "$run_dir"
  else
    snapshot_smoke_failure "$run_dir"
  fi
  COMPLETED_RUNS="$run_index"

  if [[ -z "$baseline_run_dir" ]]; then
    baseline_run_dir="$run_dir"
    baseline_label="$run_label"
    baseline_outcome="$latest_outcome"
    continue
  fi

  if [[ "$baseline_outcome" != "$latest_outcome" ]]; then
    FAILURE_REASON="smoke outcome divergence between $baseline_label ($baseline_outcome) and $run_label ($latest_outcome)"
    FAILURE_HINT="inspect ${baseline_run_dir#$WORKDIR/}/outcome.txt, ${run_dir#$WORKDIR/}/outcome.txt, and the run-local logs"
    write_summary FAIL "$baseline_label" "$baseline_run_dir" "$run_dir" "$baseline_outcome" "$latest_outcome"
    fail "$FAILURE_REASON; $FAILURE_HINT"
  fi

  if [[ "$run_exit_code" == "0" ]]; then
    if compare_snapshot_manifests "$baseline_run_dir" "$run_dir"; then
      continue
    fi
    FAILURE_REASON="smoke divergence between $baseline_label and $run_label"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/manifest_diff.txt and the two smoke_snapshot trees"
    write_summary FAIL "$baseline_label" "$baseline_run_dir" "$run_dir" "$baseline_outcome" "$latest_outcome"
    fail "$FAILURE_REASON; $FAILURE_HINT"
  fi

  if ! compare_failure_signatures "$baseline_run_dir" "$run_dir"; then
    FAILURE_REASON="smoke failure signature divergence between $baseline_label and $run_label"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/failure_signature_diff.txt and the two failure_snapshot trees"
    write_summary FAIL "$baseline_label" "$baseline_run_dir" "$run_dir" "$baseline_outcome" "$latest_outcome"
    fail "$FAILURE_REASON; $FAILURE_HINT"
  fi
done

if [[ "$baseline_outcome" != PASS:* ]]; then
  FAILURE_REASON="smoke failed consistently across $RUN_COUNT runs ($baseline_outcome)"
  FAILURE_HINT="inspect ${baseline_run_dir#$WORKDIR/}/failure_signature.txt and ${baseline_run_dir#$WORKDIR/}/failure_snapshot for the stable failure bundle"
  write_summary CONSISTENT_FAIL "$baseline_label" "$baseline_run_dir" "$latest_run_dir" "$baseline_outcome" "$latest_outcome"
  fail "$FAILURE_REASON; $FAILURE_HINT"
fi

FAILURE_REASON=""
FAILURE_HINT=""
write_summary PASS "$baseline_label" "$baseline_run_dir" "$latest_run_dir" "$baseline_outcome" "$latest_outcome"
publish_output
release_lock || fail "failed to release repeatability lock after successful publish"
rmdir "$STAGE_PARENT" 2>/dev/null || true
rmdir "$TMP_PARENT" 2>/dev/null || true
echo "[lca_smoke_repeatability] PASS ($RUN_COUNT matching smoke runs)" >&2
