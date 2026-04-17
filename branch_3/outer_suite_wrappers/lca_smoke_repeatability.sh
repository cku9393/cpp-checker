#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"
cd "$BRANCH_ROOT"
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
SMOKE_WRAPPER="$BRANCH_ROOT/lca_smoke.sh"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
SMOKE_FAILURE_ROOT="$ARTIFACTS_ROOT/smoke_latest_failure"
SMOKE_STATUS_ROOT="$ARTIFACTS_ROOT/smoke_latest_status"
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
CURRENT_OUTPUT_SNAPSHOT_SOURCE=""
SEQUENCE_FAILED=0
SEQUENCE_OUTCOME_CONSISTENCY="matching"
SEQUENCE_FIRST_FAILURE_REASON=""
SEQUENCE_FIRST_FAILURE_HINT=""
SEQUENCE_FIRST_FAILURE_RUN_LABEL=""
SEQUENCE_FIRST_FAILURE_KIND=""
FAILURE_EVENTS_PATH=""
FAILURE_EVENT_COUNT=0

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
        echo "normalized_files=run_case.stdout.txt,solver_env_snapshot.json"
        echo "normalized_run_case_stdout_lines=[run_case] mode=... time=... mem=... ; [run_case] artifacts: ..."
        echo "normalized_solver_env_snapshot_fields=solver.mtime_ns,solver.sha256,solver.path,tracked_env.DENSE_PROFILE_OUTDIR"
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

write_smoke_bundle_freshness_probe() {
  local bundle_root="$1"
  local freshness_marker="$2"
  shift 2

  python3 - "$bundle_root" "$freshness_marker" "$@" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

bundle_root = pathlib.Path(sys.argv[1])
marker_path = pathlib.Path(sys.argv[2])
artifact_names = sys.argv[3:]


def describe(path: pathlib.Path) -> dict[str, int | str] | None:
    if not path.exists():
        return None
    stat = path.stat()
    return {
        "path": str(path),
        "device": stat.st_dev,
        "inode": stat.st_ino,
        "size": stat.st_size,
    }


probe = {name: describe(bundle_root / name) for name in artifact_names}
marker_path.write_text(json.dumps(probe, sort_keys=True, indent=2) + "\n", encoding="utf-8")
PY
}

verify_smoke_bundle_freshness() {
  local bundle_label="$1"
  local freshness_marker="$2"
  local bundle_root="$3"
  local report_path="$4"
  shift 4

  python3 - "$bundle_label" "$freshness_marker" "$bundle_root" "$report_path" "$@" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

bundle_label = sys.argv[1]
marker_path = pathlib.Path(sys.argv[2])
bundle_root = pathlib.Path(sys.argv[3])
report_path = pathlib.Path(sys.argv[4])
artifact_names = sys.argv[5:]


def describe(path: pathlib.Path) -> dict[str, int | str] | None:
    if not path.exists():
        return None
    stat = path.stat()
    return {
        "path": str(path),
        "device": stat.st_dev,
        "inode": stat.st_ino,
        "size": stat.st_size,
    }


def same_identity(left: dict[str, int | str] | None, right: dict[str, int | str] | None) -> bool:
    if left is None or right is None:
        return False
    return (
        left.get("device") == right.get("device")
        and left.get("inode") == right.get("inode")
        and left.get("size") == right.get("size")
    )


before = None
status = "fresh"
artifact_issue = "none"
reason = "ok"
after_artifacts: dict[str, dict[str, int | str] | None] = {}
issues: list[str] = []
required_artifacts = tuple((name, bundle_root / name) for name in artifact_names)

if not marker_path.exists() or not marker_path.is_file():
    issues.append("missing_freshness_probe")
else:
    before = json.loads(marker_path.read_text(encoding="utf-8"))

if not issues and not bundle_root.exists():
    issues.append("missing_output_root")
elif not issues and not bundle_root.is_dir():
    issues.append("non_directory_output_root")
elif not issues:
    for name, path in required_artifacts:
        after_artifacts[name] = describe(path)
        if after_artifacts[name] is None:
            issues.append(f"missing_required={name}")
    for name, _path in required_artifacts:
        if after_artifacts.get(name) is None:
            continue
        if same_identity((before or {}).get(name), after_artifacts.get(name)):
            issues.append(f"stale_required={name}")

if issues:
    status = "stale_or_missing_current_run_artifacts"
    artifact_issue = issues[0]
    if artifact_issue == "missing_freshness_probe":
        reason = "repeatability wrapper did not preserve the pre-run freshness probe"
    elif artifact_issue == "missing_output_root":
        reason = f"{bundle_label} disappeared after lca_smoke.sh returned"
    elif artifact_issue == "non_directory_output_root":
        reason = f"{bundle_label} is not a directory after lca_smoke.sh returned"
    elif artifact_issue.startswith("missing_required="):
        name = artifact_issue.split("=", 1)[1]
        reason = f"{bundle_label} was missing {name} after lca_smoke.sh returned"
    elif artifact_issue.startswith("stale_required="):
        name = artifact_issue.split("=", 1)[1]
        reason = f"{bundle_label} reused {name} instead of regenerating current-run evidence"
    else:
        reason = f"{bundle_label} freshness check found missing or stale artifacts"

lines = [
    f"bundle={bundle_label}",
    f"status={status}",
    f"artifact_issue={artifact_issue}",
    f"issue_count={len(issues)}",
    f"reason={reason}",
    f"freshness_probe={marker_path}",
    f"live_output_root={bundle_root}",
]
for issue in issues:
    lines.append("issue=" + issue)
for name, _path in required_artifacts:
    lines.append("before_" + name + "=" + json.dumps((before or {}).get(name), sort_keys=True))
    lines.append("after_" + name + "=" + json.dumps(after_artifacts.get(name), sort_keys=True))
report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
raise SystemExit(0 if status == "fresh" else 1)
PY
}

prepare_smoke_run_freshness_probes() {
  local run_dir="$1"

  write_smoke_bundle_freshness_probe \
    "$SMOKE_STATUS_ROOT" \
    "$run_dir/status_bundle_before.json" \
    summary.txt \
    latest_status_report.md
  write_smoke_bundle_freshness_probe \
    "$SMOKE_FAILURE_ROOT" \
    "$run_dir/failure_bundle_before.json" \
    failure_summary.txt \
    latest_failure_report.md
  write_smoke_bundle_freshness_probe \
    "$SMOKE_OUTROOT" \
    "$run_dir/output_bundle_before.json" \
    suite_config.txt \
    suite_plan.tsv \
    environment_validation.txt
}

verify_current_run_freshness() {
  local run_dir="$1"
  local run_exit_code="$2"
  local result_family="$3"
  local run_rel="${run_dir#$WORKDIR/}"

  if ! verify_smoke_bundle_freshness \
    "smoke_latest_status" \
    "$run_dir/status_bundle_before.json" \
    "$SMOKE_STATUS_ROOT" \
    "$run_dir/status_bundle_freshness.txt" \
    summary.txt \
    latest_status_report.md; then
    FAILURE_REASON="smoke status bundle was not regenerated for $run_rel"
    FAILURE_HINT="inspect $run_rel/status_bundle_freshness.txt and the live $SMOKE_STATUS_ROOT bundle"
    return 1
  fi

  if [[ "$run_exit_code" == "0" ]]; then
    if [[ "$CURRENT_OUTPUT_SNAPSHOT_SOURCE" != "published_smoke_output" ]]; then
      return 0
    fi
    if ! verify_smoke_bundle_freshness \
      "published smoke output" \
      "$run_dir/output_bundle_before.json" \
      "$SMOKE_OUTROOT" \
      "$run_dir/output_bundle_freshness.txt" \
      suite_config.txt \
      suite_plan.tsv \
      environment_validation.txt; then
      FAILURE_REASON="smoke pass reused stale published output for $run_rel"
      FAILURE_HINT="inspect $run_rel/output_bundle_freshness.txt and the live $SMOKE_OUTROOT bundle"
      return 1
    fi
    return 0
  fi

  if [[ "$result_family" != "solver" ]]; then
    return 0
  fi

  if ! verify_smoke_bundle_freshness \
    "smoke_latest_failure" \
    "$run_dir/failure_bundle_before.json" \
    "$SMOKE_FAILURE_ROOT" \
    "$run_dir/failure_bundle_freshness.txt" \
    failure_summary.txt \
    latest_failure_report.md; then
    FAILURE_REASON="smoke solver-failure bundle was not regenerated for $run_rel"
    FAILURE_HINT="inspect $run_rel/failure_bundle_freshness.txt and the live $SMOKE_FAILURE_ROOT bundle"
    return 1
  fi
}

run_smoke_once() {
  local run_dir="$1"
  local rc=0

  mkdir -p "$run_dir"
  prepare_smoke_run_freshness_probes "$run_dir"
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

  CURRENT_OUTPUT_SNAPSHOT_SOURCE="external_snapshot"
  if [[ ! -d "$snapshot_root" ]]; then
    if [[ ! -d "$SMOKE_OUTROOT" ]]; then
      fail "expected wrapper-exported smoke snapshot or published smoke output after successful run: $snapshot_root"
    fi
    CURRENT_OUTPUT_SNAPSHOT_SOURCE="published_smoke_output"
    cp -R "$SMOKE_OUTROOT" "$snapshot_root"
  fi
  printf '%s\n' "$CURRENT_OUTPUT_SNAPSHOT_SOURCE" > "$run_dir/output_snapshot_source.txt"
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

    if file_path.name == "solver_env_snapshot.json":
        payload = json.loads(raw.decode("utf-8"))
        solver = payload.get("solver")
        if isinstance(solver, dict):
            solver.pop("mtime_ns", None)
            solver.pop("sha256", None)
            if "path" in solver:
                solver["path"] = "<normalized>"
        tracked_env = payload.get("tracked_env")
        if isinstance(tracked_env, dict) and "DENSE_PROFILE_OUTDIR" in tracked_env:
            tracked_env["DENSE_PROFILE_OUTDIR"] = "<normalized>"
        return (json.dumps(payload, sort_keys=True, ensure_ascii=False, indent=2) + "\n").encode("utf-8")

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

snapshot_smoke_status() {
  local run_dir="$1"
  local snapshot_root="$run_dir/status_snapshot"
  local signature_path="$run_dir/status_signature.txt"

  if [[ ! -d "$SMOKE_STATUS_ROOT" ]]; then
    fail "expected published smoke status root after run: $SMOKE_STATUS_ROOT"
  fi

  mkdir -p "$snapshot_root"
  cp -R "$SMOKE_STATUS_ROOT"/. "$snapshot_root"

  python3 - "$snapshot_root/summary.txt" "$signature_path" <<'PY'
from __future__ import annotations

from pathlib import Path
import sys

summary_path = Path(sys.argv[1])
signature_path = Path(sys.argv[2])
required_keys = [
    "public_status",
    "result_family",
    "normalized_exit_code",
    "raw_exit_code",
    "normalized_outcome",
]
optional_keys = [
    "outcome_source",
    "source_failure_kind",
    "source_failure_origin",
    "source_failure_retryable",
    "triage_stage_scope",
    "triage_stage",
]

if not summary_path.is_file():
    raise SystemExit(f"missing smoke status summary: {summary_path}")

entries: dict[str, str] = {}
for line in summary_path.read_text(encoding="utf-8").splitlines():
    if "=" not in line:
        continue
    key, value = line.split("=", 1)
    entries[key] = value

missing = [key for key in required_keys if key not in entries]
if missing:
    raise SystemExit(
        "smoke status summary is missing required keys: " + ",".join(missing)
    )

lines = [f"{key}={entries[key]}" for key in required_keys]
for key in optional_keys:
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
    "normalized_files=run_case.stdout.txt,solver_env_snapshot.json",
    "normalized_run_case_stdout_lines=[run_case] mode=... time=... mem=... ; [run_case] artifacts: ...",
    "normalized_solver_env_snapshot_fields=solver.mtime_ns,solver.sha256,solver.path,tracked_env.DENSE_PROFILE_OUTDIR",
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

compare_status_signatures() {
  local baseline_run_dir="$1"
  local current_run_dir="$2"
  local report_path="$current_run_dir/status_signature_diff.txt"
  local baseline_rel="${baseline_run_dir#$WORKDIR/}"
  local current_rel="${current_run_dir#$WORKDIR/}"

  python3 - "$baseline_run_dir/status_signature.txt" "$current_run_dir/status_signature.txt" "$report_path" "$baseline_rel" "$current_rel" <<'PY'
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
    f"baseline_status_signature={baseline_rel}/status_signature.txt",
    f"current_status_signature={current_rel}/status_signature.txt",
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

read_signature_field() {
  local signature_path="$1"
  local key="$2"

  python3 - "$signature_path" "$key" <<'PY'
from __future__ import annotations

from pathlib import Path
import sys

signature_path = Path(sys.argv[1])
key = sys.argv[2]

for line in signature_path.read_text(encoding="utf-8").splitlines():
    if "=" not in line:
        continue
    current_key, value = line.split("=", 1)
    if current_key == key:
        print(value)
        break
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

sanitize_failure_event_field() {
  local value="$1"
  value="${value//$'\t'/ }"
  value="${value//$'\n'/ }"
  printf '%s' "$value"
}

prepare_failure_events_log() {
  FAILURE_EVENTS_PATH="$WORKDIR/failure_events.tsv"
  printf 'run\tkind\treason\thint\n' > "$FAILURE_EVENTS_PATH"
}

record_sequence_failure() {
  local run_label="$1"
  local failure_kind="$2"
  local failure_reason="$3"
  local failure_hint="$4"

  FAILURE_EVENT_COUNT=$(( FAILURE_EVENT_COUNT + 1 ))
  if [[ -n "$FAILURE_EVENTS_PATH" ]]; then
    printf \
      '%s\t%s\t%s\t%s\n' \
      "$run_label" \
      "$(sanitize_failure_event_field "$failure_kind")" \
      "$(sanitize_failure_event_field "$failure_reason")" \
      "$(sanitize_failure_event_field "$failure_hint")" \
      >> "$FAILURE_EVENTS_PATH"
  fi

  if (( SEQUENCE_FAILED == 0 )); then
    SEQUENCE_FAILED=1
    SEQUENCE_FIRST_FAILURE_REASON="$failure_reason"
    SEQUENCE_FIRST_FAILURE_HINT="$failure_hint"
    SEQUENCE_FIRST_FAILURE_RUN_LABEL="$run_label"
    SEQUENCE_FIRST_FAILURE_KIND="$failure_kind"
  fi
}

write_summary() {
  local status="$1"
  local baseline_label="$2"
  local baseline_run_dir="$3"
  local latest_run_dir="$4"
  local baseline_outcome="$5"
  local latest_outcome="$6"
  local outcome_consistency="$7"
  local baseline_result_family="$8"
  local latest_result_family="$9"
  local supports_solver_iteration="0"
  local solver_iteration_basis="unstable_or_non_solver_signal"

  if [[ "$status" == "PASS" ]]; then
    supports_solver_iteration="1"
    solver_iteration_basis="stable_green_smoke"
  elif [[ "$status" == "CONSISTENT_FAIL" && "$baseline_result_family" == "solver" ]]; then
    supports_solver_iteration="1"
    solver_iteration_basis="stable_solver_failure_signal"
  elif [[ "$outcome_consistency" != "matching" ]]; then
    solver_iteration_basis="diverged_back_to_back_runs"
  elif [[ "$baseline_result_family" != "solver" ]]; then
    solver_iteration_basis="stable_non_solver_failure"
  fi

  {
    echo "status=$status"
    echo "requested_runs=$RUN_COUNT"
    echo "completed_runs=$COMPLETED_RUNS"
    echo "check_target=./lca_smoke.sh"
    echo "reproducibility_scope=consecutive_same_worktree_runs"
    echo "supports_solver_iteration=$supports_solver_iteration"
    echo "solver_iteration_basis=$solver_iteration_basis"
    echo "baseline_run=$baseline_label"
    echo "latest_run=${latest_run_dir#$WORKDIR/}"
    echo "baseline_outcome=$baseline_outcome"
    echo "latest_outcome=$latest_outcome"
    echo "outcome_consistency=$outcome_consistency"
    echo "baseline_result_family=$baseline_result_family"
    echo "latest_result_family=$latest_result_family"
    echo "baseline_status_snapshot=${baseline_run_dir#$WORKDIR/}/status_snapshot"
    echo "latest_status_snapshot=${latest_run_dir#$WORKDIR/}/status_snapshot"
    echo "baseline_status_signature=${baseline_run_dir#$WORKDIR/}/status_signature.txt"
    echo "latest_status_signature=${latest_run_dir#$WORKDIR/}/status_signature.txt"
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
    if (( FAILURE_EVENT_COUNT > 0 )); then
      echo "failure_count=$FAILURE_EVENT_COUNT"
      echo "failure_events=failure_events.tsv"
    fi
    if [[ -n "$SEQUENCE_FIRST_FAILURE_RUN_LABEL" ]]; then
      echo "first_failed_run=$SEQUENCE_FIRST_FAILURE_RUN_LABEL"
    fi
    if [[ -n "$SEQUENCE_FIRST_FAILURE_KIND" ]]; then
      echo "first_failure_kind=$SEQUENCE_FIRST_FAILURE_KIND"
    fi
    echo "ignored_files=time.txt"
    echo "normalized_files=run_case.stdout.txt,solver_env_snapshot.json"
    echo "normalized_run_case_stdout_lines=[run_case] mode=... time=... mem=... ; [run_case] artifacts: ..."
    echo "normalized_solver_env_snapshot_fields=solver.mtime_ns,solver.sha256,solver.path,tracked_env.DENSE_PROFILE_OUTDIR"
    echo "live_smoke_output=$SMOKE_OUTROOT"
    echo "live_smoke_failure_root=$SMOKE_FAILURE_ROOT"
  } > "$WORKDIR/summary.txt"
}

write_freshness_failure_summary() {
  local run_label="$1"
  local run_dir="$2"
  local latest_outcome="$3"
  local latest_result_family="$4"
  local summary_baseline_label="$run_label"
  local summary_baseline_run_dir="$run_dir"
  local summary_baseline_outcome="$latest_outcome"
  local summary_baseline_result_family="$latest_result_family"
  local outcome_consistency="matching"

  if [[ -n "${baseline_run_dir:-}" ]]; then
    summary_baseline_label="$baseline_label"
    summary_baseline_run_dir="$baseline_run_dir"
    summary_baseline_outcome="$baseline_outcome"
    summary_baseline_result_family="$baseline_result_family"
    if [[ "$summary_baseline_outcome" != "$latest_outcome" ]]; then
      outcome_consistency="diverged"
    fi
  fi

  write_summary \
    FAIL \
    "$summary_baseline_label" \
    "$summary_baseline_run_dir" \
    "$run_dir" \
    "$summary_baseline_outcome" \
    "$latest_outcome" \
    "$outcome_consistency" \
    "$summary_baseline_result_family" \
    "$latest_result_family"
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
require_executable "$SMOKE_WRAPPER" "branch-local smoke entrypoint"
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
ensure_under_artifacts "$SMOKE_STATUS_ROOT"
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
prepare_failure_events_log

baseline_run_dir=""
baseline_label=""
latest_run_dir=""
baseline_outcome=""
latest_outcome=""
baseline_result_family=""
latest_result_family=""
run_exit_code=""

for (( run_index = 1; run_index <= RUN_COUNT; ++run_index )); do
  run_label="$(printf 'run%02d' "$run_index")"
  run_dir="$WORKDIR/runs/$run_label"
  latest_run_dir="$run_dir"
  CURRENT_OUTPUT_SNAPSHOT_SOURCE=""

  if run_smoke_once "$run_dir"; then
    run_exit_code="0"
  else
    run_exit_code="$(
      if [[ -f "$run_dir/exit_code.txt" ]]; then
        printf '%s' "$(<"$run_dir/exit_code.txt")"
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
  snapshot_smoke_status "$run_dir"
  latest_result_family="$(read_signature_field "$run_dir/status_signature.txt" "result_family")"
  COMPLETED_RUNS="$run_index"

  if [[ -z "$baseline_run_dir" ]]; then
    baseline_run_dir="$run_dir"
    baseline_label="$run_label"
    baseline_outcome="$latest_outcome"
    baseline_result_family="$latest_result_family"
  fi

  if ! verify_current_run_freshness "$run_dir" "$run_exit_code" "$latest_result_family"; then
    record_sequence_failure "$run_label" "freshness" "$FAILURE_REASON" "$FAILURE_HINT"
    continue
  fi

  if [[ "$run_dir" == "$baseline_run_dir" ]]; then
    continue
  fi

  if [[ "$baseline_outcome" != "$latest_outcome" ]]; then
    FAILURE_REASON="smoke outcome divergence between $baseline_label ($baseline_outcome) and $run_label ($latest_outcome)"
    FAILURE_HINT="inspect ${baseline_run_dir#$WORKDIR/}/outcome.txt, ${run_dir#$WORKDIR/}/outcome.txt, and the run-local logs"
    SEQUENCE_OUTCOME_CONSISTENCY="diverged"
    record_sequence_failure "$run_label" "outcome_divergence" "$FAILURE_REASON" "$FAILURE_HINT"
    continue
  fi

  if ! compare_status_signatures "$baseline_run_dir" "$run_dir"; then
    FAILURE_REASON="smoke status divergence between $baseline_label and $run_label"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/status_signature_diff.txt and the two status_snapshot trees"
    record_sequence_failure "$run_label" "status_signature_divergence" "$FAILURE_REASON" "$FAILURE_HINT"
    continue
  fi

  if [[ "$run_exit_code" == "0" ]]; then
    if compare_snapshot_manifests "$baseline_run_dir" "$run_dir"; then
      continue
    fi
    FAILURE_REASON="smoke divergence between $baseline_label and $run_label"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/manifest_diff.txt and the two smoke_snapshot trees"
    record_sequence_failure "$run_label" "snapshot_manifest_divergence" "$FAILURE_REASON" "$FAILURE_HINT"
    continue
  fi

  if ! compare_failure_signatures "$baseline_run_dir" "$run_dir"; then
    FAILURE_REASON="smoke failure signature divergence between $baseline_label and $run_label"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/failure_signature_diff.txt and the two failure_snapshot trees"
    record_sequence_failure "$run_label" "failure_signature_divergence" "$FAILURE_REASON" "$FAILURE_HINT"
    continue
  fi
done

if (( SEQUENCE_FAILED != 0 )); then
  FAILURE_REASON="$SEQUENCE_FIRST_FAILURE_REASON"
  FAILURE_HINT="$SEQUENCE_FIRST_FAILURE_HINT"
  write_summary \
    FAIL \
    "$baseline_label" \
    "$baseline_run_dir" \
    "$latest_run_dir" \
    "$baseline_outcome" \
    "$latest_outcome" \
    "$SEQUENCE_OUTCOME_CONSISTENCY" \
    "$baseline_result_family" \
    "$latest_result_family"
  fail "$FAILURE_REASON; $FAILURE_HINT"
fi

if [[ "$baseline_outcome" != PASS:* ]]; then
  if [[ "$baseline_result_family" != "solver" ]]; then
    FAILURE_REASON="smoke repeated a non-solver failure across $RUN_COUNT runs ($baseline_outcome)"
    FAILURE_HINT="inspect ${baseline_run_dir#$WORKDIR/}/status_signature.txt and ${baseline_run_dir#$WORKDIR/}/status_snapshot/summary.txt before continuing solver iteration"
    write_summary FAIL "$baseline_label" "$baseline_run_dir" "$latest_run_dir" "$baseline_outcome" "$latest_outcome" "matching" "$baseline_result_family" "$latest_result_family"
    fail "$FAILURE_REASON; $FAILURE_HINT"
  fi
  FAILURE_REASON="smoke failed consistently across $RUN_COUNT runs ($baseline_outcome)"
  FAILURE_HINT="inspect ${baseline_run_dir#$WORKDIR/}/failure_signature.txt and ${baseline_run_dir#$WORKDIR/}/failure_snapshot for the stable failure bundle"
  write_summary CONSISTENT_FAIL "$baseline_label" "$baseline_run_dir" "$latest_run_dir" "$baseline_outcome" "$latest_outcome" "matching" "$baseline_result_family" "$latest_result_family"
  fail "$FAILURE_REASON; $FAILURE_HINT"
fi

FAILURE_REASON=""
FAILURE_HINT=""
write_summary PASS "$baseline_label" "$baseline_run_dir" "$latest_run_dir" "$baseline_outcome" "$latest_outcome" "matching" "$baseline_result_family" "$latest_result_family"
publish_output
release_lock || fail "failed to release repeatability lock after successful publish"
rmdir "$STAGE_PARENT" 2>/dev/null || true
rmdir "$TMP_PARENT" 2>/dev/null || true
echo "[lca_smoke_repeatability] PASS ($RUN_COUNT matching smoke runs)" >&2
