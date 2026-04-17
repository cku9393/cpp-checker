#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"
cd "$BRANCH_ROOT"
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
SMOKE_WRAPPER="$BRANCH_ROOT/lca_smoke.sh"
SMOKE_REPEATABILITY_WRAPPER="$BRANCH_ROOT/lca_smoke_repeatability.sh"
STRONG_WRAPPER="$BRANCH_ROOT/lca_strong_gate.sh"
BOJ3S_WRAPPER="$BRANCH_ROOT/lca_boj3s_gate.sh"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
SMOKE_STATUS_ROOT="$ARTIFACTS_ROOT/smoke_latest_status"
TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
STAGE_PARENT="$ARTIFACTS_ROOT/.repeatability_stage"
RUN_WORK_TEMPLATE="lca_acceptance_repeatability.XXXXXX"
LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
LOCKDIR="$LOCK_ROOT/lca_acceptance_repeatability"
LOCK_PID_FILE="$LOCKDIR/pid"
OUTROOT=""
OUTPARENT=""
BACKUP_ROOT=""
WORKDIR=""
ROOT_GUARD_DIR=""
ROOT_GUARD_MARKER=""
LOCK_HELD=0
RUN_COUNT="${LCA_ACCEPTANCE_REPEAT_COUNT:-2}"
COMPLETED_RUNS=0
FAILED_RUN=""
FAILED_STAGE=""
FAILURE_REASON=""
FAILURE_HINT=""
REQUIRED_SEQUENCE="lca_smoke -> lca_strong_gate -> lca_boj3s_gate"
SMOKE_PRECHECK="./lca_smoke_repeatability.sh"
SMOKE_REPEATABILITY_COUNT="${LCA_ACCEPTANCE_SMOKE_REPEAT_COUNT:-2}"
SMOKE_OUTROOT=""
SMOKE_REPEATABILITY_OUTROOT=""
STRONG_OUTROOT=""
BOJ3S_OUTROOT=""
BASELINE_RUN=""
RUN_TOKEN=""
SIGNATURE_FIELDS="smoke_status[public_status,result_family,normalized_exit_code,raw_exit_code,normalized_outcome,outcome_source,source_failure_kind,source_failure_origin,source_failure_retryable,triage_stage_scope,triage_stage],smoke_snapshot_manifest,gate_pass_signature[verdict,preset,reasons,stages[name,status,cases,timeouts,re_wa,limit_scale,scale_fail],gate_config[selected_preset_sha256,runtime_env[stage_filter,limit_scale,heartbeat_interval,stale_lock_seconds,profile_mode,local_skip_self_test,enable_state_load_materialization_opt,enable_prev_state_writeback_opt,enable_layout_signature_gate_opt,enable_layout_reuse_zero_elision_opt,strong_gate_release_profile,solver_env_scrub]]],fresh_runtime_artifacts"

export PYTHONDONTWRITEBYTECODE=1

fail() {
  echo "[lca_acceptance_repeatability] $*" >&2
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
usage: ./outer_suite_wrappers/lca_acceptance_repeatability.sh [repeat-count]
[lca_acceptance_repeatability] repeat-count defaults to $LCA_ACCEPTANCE_REPEAT_COUNT or 2
[lca_acceptance_repeatability] prechecks ./lca_smoke.sh via ./lca_smoke_repeatability.sh before rerunning ./lca_strong_gate.sh -> ./lca_boj3s_gate.sh per cycle
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

path_has_dataless_flag() {
  local path="$1"
  local flags=""
  if [[ -z "$path" || ! -e "$path" ]]; then
    return 1
  fi
  if ! flags="$(stat -f '%Sf' "$path" 2>/dev/null)"; then
    return 1
  fi
  [[ "$flags" == *dataless* ]]
}

require_materialized_file() {
  local path="$1"
  local label="$2"
  require_file "$path" "$label"
  if path_has_dataless_flag "$path"; then
    fail "dataless file ${label}: $path"
  fi
}

require_materialized_executable() {
  local path="$1"
  local label="$2"
  require_executable "$path" "$label"
  if path_has_dataless_flag "$path"; then
    fail "dataless executable ${label}: $path"
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
    fail "another lca_acceptance_repeatability.sh run is active (pid $holder)"
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
    for stale in "$STAGE_PARENT"/lca_acceptance_repeatability.*; do
      rm -rf "$stale"
    done
    shopt -u nullglob
  fi
}

prepare_root_guard() {
  ROOT_GUARD_DIR="$ARTIFACTS_ROOT/acceptance_repeatability.root_guard"
  ROOT_GUARD_MARKER="$ROOT_GUARD_DIR/root_guard.txt"
  ensure_under_artifacts "$ROOT_GUARD_DIR"
  ensure_under_artifacts "$ROOT_GUARD_MARKER"
  mkdir -p "$ROOT_GUARD_DIR"
  {
    echo "created_by=./lca_acceptance_repeatability.sh"
    echo "pid=$$"
    echo "artifacts_root=$ARTIFACTS_ROOT"
  } > "$ROOT_GUARD_MARKER"
}

assert_root_guard_intact() {
  local stage_human="$1"
  local run_dir="$2"

  if [[ ! -d "$run_dir" ]]; then
    FAILURE_REASON="repeatability run workspace disappeared during ${stage_human} execution"
    FAILURE_HINT="inspect shared artifact cleanup behavior and rerun with preserved wrapper logs for $run_dir"
    return 1
  fi
  if [[ ! -f "$ROOT_GUARD_MARKER" ]]; then
    FAILURE_REASON="shared artifact root was cleared during ${stage_human} execution"
    FAILURE_HINT="inspect ${stage_human} cleanup behavior; artifacts/lca_tree_stress_v5 must survive consecutive full-flow reruns"
    return 1
  fi
  return 0
}

publish_output() {
  local outleaf

  if [[ -z "$OUTROOT" || -z "$WORKDIR" || ! -d "$WORKDIR" ]]; then
    return 0
  fi

  outleaf="${OUTROOT##*/}"
  mkdir -p "$OUTPARENT"
  if [[ -e "$OUTROOT" ]]; then
    rm -rf "$BACKUP_ROOT"
    if ! move_path_retry "$OUTROOT" "$BACKUP_ROOT"; then
      echo "[lca_acceptance_repeatability] failed to rotate prior output into backup: $OUTROOT -> $BACKUP_ROOT" >&2
      return 1
    fi
  fi
  if ! move_path_retry "$WORKDIR" "$OUTPARENT/$outleaf"; then
    echo "[lca_acceptance_repeatability] failed to publish repeatability output: $WORKDIR -> $OUTPARENT/$outleaf" >&2
    return 1
  fi
  WORKDIR=""
  rm -rf "$BACKUP_ROOT"
  return 0
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
        echo "required_sequence=$REQUIRED_SEQUENCE"
        echo "failure_reason=${FAILURE_REASON:-interrupted before summary generation}"
        echo "failure_hint=${FAILURE_HINT:-inspect run logs under runs/}"
      } > "$WORKDIR/summary.txt"
    fi
    if ! publish_output; then
      rc=1
    fi
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

record_result_row() {
  local run_label="$1"
  local stage_name="$2"
  local verdict="$3"
  local signature_status="${4:-na}"
  printf '%s\t%s\t%s\t%s\n' "$run_label" "$stage_name" "$verdict" "$signature_status" >> "$WORKDIR/results.tsv"
}

read_summary_field() {
  local summary_path="$1"
  local key="$2"

  python3 - "$summary_path" "$key" <<'PY'
from __future__ import annotations

from pathlib import Path
import sys

summary_path = Path(sys.argv[1])
key = sys.argv[2]

if not summary_path.is_file():
    raise SystemExit(0)

for line in summary_path.read_text(encoding="utf-8").splitlines():
    if "=" not in line:
        continue
    current_key, value = line.split("=", 1)
    if current_key == key:
        print(value)
        break
PY
}

snapshot_smoke_repeatability_output() {
  local run_dir="$1"
  local snapshot_dir="$run_dir/smoke_repeatability"

  if [[ ! -d "$SMOKE_REPEATABILITY_OUTROOT" ]]; then
    return 0
  fi

  mkdir -p "$snapshot_dir"
  if [[ -f "$SMOKE_REPEATABILITY_OUTROOT/summary.txt" ]]; then
    cp "$SMOKE_REPEATABILITY_OUTROOT/summary.txt" "$snapshot_dir/summary.txt"
  fi
  if [[ -f "$SMOKE_REPEATABILITY_OUTROOT/failure_events.tsv" ]]; then
    cp "$SMOKE_REPEATABILITY_OUTROOT/failure_events.tsv" "$snapshot_dir/failure_events.tsv"
  fi
}

write_tree_freshness_probe() {
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

verify_tree_freshness() {
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
        reason = f"{bundle_label} disappeared after the wrapper returned exit code 0"
    elif artifact_issue == "non_directory_output_root":
        reason = f"{bundle_label} is not a directory after the wrapper returned exit code 0"
    elif artifact_issue.startswith("missing_required="):
        name = artifact_issue.split("=", 1)[1]
        reason = f"{bundle_label} was missing {name} after the wrapper returned exit code 0"
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

snapshot_smoke_output() {
  local run_dir="$1"
  local snapshot_root="$run_dir/smoke_snapshot"

  if [[ ! -d "$snapshot_root" ]]; then
    if [[ ! -d "$SMOKE_OUTROOT" ]]; then
      FAILURE_REASON="lca_smoke returned exit code 0 but did not leave an exported smoke snapshot"
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/smoke.stdout.txt and ${run_dir#$WORKDIR/}/smoke.stderr.txt"
      return 1
    fi
    cp -R "$SMOKE_OUTROOT" "$snapshot_root"
  fi

  python3 - "$snapshot_root" "$run_dir/smoke_manifest.tsv" "$run_dir/smoke_timings.tsv" <<'PY'
from __future__ import annotations

import hashlib
import json
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

snapshot_smoke_status() {
  local run_dir="$1"
  local snapshot_root="$run_dir/status_snapshot"
  local signature_path="$run_dir/status_signature.txt"

  if [[ ! -d "$SMOKE_STATUS_ROOT" ]]; then
    FAILURE_REASON="lca_smoke returned exit code 0 but did not publish smoke_latest_status/"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/smoke.stderr.txt"
    return 1
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
    raise SystemExit("smoke status summary is missing required keys: " + ",".join(missing))

lines = [f"{key}={entries[key]}" for key in required_keys]
for key in optional_keys:
    if key in entries:
        lines.append(f"{key}={entries[key]}")
signature_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
PY
}

assert_smoke_status_pass() {
  local signature_path="$1"
  local report_path="$2"

  python3 - "$signature_path" "$report_path" <<'PY'
from __future__ import annotations

from pathlib import Path
import sys

signature_path = Path(sys.argv[1])
report_path = Path(sys.argv[2])
expected = {
    "public_status": "PASS",
    "result_family": "none",
    "normalized_exit_code": "0",
    "raw_exit_code": "0",
    "normalized_outcome": "pass",
}
entries: dict[str, str] = {}
for line in signature_path.read_text(encoding="utf-8").splitlines():
    if "=" not in line:
        continue
    key, value = line.split("=", 1)
    entries[key] = value

bad = []
for key, expected_value in expected.items():
    actual = entries.get(key, "<missing>")
    if actual != expected_value:
        bad.append(f"{key}\texpected={expected_value}\tactual={actual}")

status = "PASS" if not bad else "FAIL"
lines = [f"status={status}", f"signature={signature_path}"]
lines.extend(bad)
report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
raise SystemExit(0 if not bad else 1)
PY
}

compare_smoke_status_signatures() {
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

compare_smoke_snapshot_manifests() {
  local baseline_run_dir="$1"
  local current_run_dir="$2"
  local report_path="$current_run_dir/smoke_manifest_diff.txt"
  local baseline_rel="${baseline_run_dir#$WORKDIR/}"
  local current_rel="${current_run_dir#$WORKDIR/}"

  python3 - "$baseline_run_dir/smoke_manifest.tsv" "$current_run_dir/smoke_manifest.tsv" "$report_path" "$baseline_rel" "$current_rel" <<'PY'
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

snapshot_gate_output() {
  local gate_label="$1"
  local gate_outroot="$2"
  local run_dir="$3"
  local snapshot_dir="$run_dir/$gate_label"
  local certify_json="$gate_outroot/certify.json"
  local certify_summary="$gate_outroot/certify_summary.md"
  local runtime_env="$gate_outroot/runtime_env.txt"
  local preflight_manifest="$gate_outroot/preflight_manifest.tsv"
  local selected_preset="$gate_outroot/selected_preset.json"
  local repeatability_manifest="$gate_outroot/repeatability_gate_manifest.txt"

  if [[ ! -d "$gate_outroot" ]]; then
    FAILURE_REASON="${gate_label} returned exit code 0 but did not publish its output root"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/${gate_label}.stderr.txt"
    return 1
  fi
  if [[ ! -f "$certify_json" ]]; then
    FAILURE_REASON="${gate_label} returned exit code 0 but did not publish certify.json"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/${gate_label}.stderr.txt"
    return 1
  fi
  mkdir -p "$snapshot_dir"
  cp "$certify_json" "$snapshot_dir/certify.json"
  if [[ -f "$certify_summary" ]]; then
    cp "$certify_summary" "$snapshot_dir/certify_summary.md"
  fi
  if [[ -f "$runtime_env" ]]; then
    cp "$runtime_env" "$snapshot_dir/runtime_env.txt"
  fi
  if [[ -f "$preflight_manifest" ]]; then
    cp "$preflight_manifest" "$snapshot_dir/preflight_manifest.tsv"
  fi
  if [[ -f "$selected_preset" ]]; then
    cp "$selected_preset" "$snapshot_dir/selected_preset.json"
  fi
  if [[ -f "$repeatability_manifest" ]]; then
    cp "$repeatability_manifest" "$snapshot_dir/repeatability_gate_manifest.txt"
  fi
  printf '%s\n' "$gate_outroot" > "$snapshot_dir/live_output_root.txt"
}

extract_pass_signature() {
  local snapshot_dir="$1"
  local signature_path="$2"
  local report_path="$3"
  local gate_human="$4"
  python3 - "$snapshot_dir" "$signature_path" "$report_path" "$gate_human" <<'PY'
from __future__ import annotations

import json
import hashlib
import pathlib
import sys

snapshot_dir = pathlib.Path(sys.argv[1])
signature_path = pathlib.Path(sys.argv[2])
report_path = pathlib.Path(sys.argv[3])
gate_human = sys.argv[4]

certify_path = snapshot_dir / "certify.json"
selected_preset_path = snapshot_dir / "selected_preset.json"
runtime_env_path = snapshot_dir / "runtime_env.txt"
preflight_manifest_path = snapshot_dir / "preflight_manifest.tsv"
repeatability_manifest_path = snapshot_dir / "repeatability_gate_manifest.txt"

missing = [
    str(path)
    for path in (
        certify_path,
        selected_preset_path,
        runtime_env_path,
        preflight_manifest_path,
        repeatability_manifest_path,
    )
    if not path.is_file()
]
if missing:
    report_path.write_text(
        "\n".join(
            [
                f"gate={gate_human}",
                f"snapshot_dir={snapshot_dir}",
                "status=missing_gate_config_artifacts",
                "missing=" + " | ".join(missing),
            ]
        )
        + "\n",
        encoding="utf-8",
    )
    raise SystemExit(1)


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_key_values(path: pathlib.Path) -> dict[str, str]:
    rows: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        rows[key] = value
    return rows


runtime_entries = load_key_values(runtime_env_path)
stable_runtime_keys = (
    "stage_filter",
    "limit_scale",
    "heartbeat_interval",
    "stale_lock_seconds",
    "profile_mode",
    "local_skip_self_test",
    "enable_state_load_materialization_opt",
    "enable_prev_state_writeback_opt",
    "enable_layout_signature_gate_opt",
    "enable_layout_reuse_zero_elision_opt",
    "strong_gate_release_profile",
    "solver_env_scrub",
)

payload = json.loads(certify_path.read_text(encoding="utf-8"))
signature = {
    "verdict": payload.get("verdict"),
    "preset": payload.get("preset"),
    "reasons": payload.get("reasons", []),
    "stages": [
        {
            "name": stage.get("name"),
            "status": stage.get("status"),
            "cases": stage.get("cases"),
            "timeouts": stage.get("timeouts"),
            "re_wa": stage.get("re_wa"),
            "limit_scale": stage.get("limit_scale"),
            "scale_fail": stage.get("scale_fail", []),
        }
        for stage in payload.get("stages", [])
    ],
    "gate_config": {
        "selected_preset_sha256": sha256(selected_preset_path),
        "runtime_env": {
            key: runtime_entries.get(key, "")
            for key in stable_runtime_keys
        },
    },
}
bad_stages = [stage["name"] for stage in signature["stages"] if stage.get("status") != "PASS"]
lines = [
    f"gate={gate_human}",
    f"certify_json={certify_path}",
    f"verdict={signature['verdict']}",
    f"preset={signature['preset']}",
    f"selected_preset_sha256={signature['gate_config']['selected_preset_sha256']}",
    "runtime_env_config=" + json.dumps(signature["gate_config"]["runtime_env"], sort_keys=True),
    f"bad_stages={','.join(bad_stages) if bad_stages else 'none'}",
]
report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
signature_path.write_text(json.dumps(signature, sort_keys=True, indent=2) + "\n", encoding="utf-8")
if signature["verdict"] != "PASS" or bad_stages:
    raise SystemExit(1)
PY
}

compare_gate_signatures() {
  local baseline_run_dir="$1"
  local current_run_dir="$2"
  local gate_label="$3"
  local gate_human="$4"
  local report_path="$current_run_dir/${gate_label}.signature_diff.txt"
  local baseline_rel="${baseline_run_dir#$WORKDIR/}"
  local current_rel="${current_run_dir#$WORKDIR/}"

  python3 - "$baseline_run_dir/$gate_label/pass_signature.json" "$current_run_dir/$gate_label/pass_signature.json" "$report_path" "$baseline_rel" "$current_rel" "$gate_human" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

baseline_path = pathlib.Path(sys.argv[1])
current_path = pathlib.Path(sys.argv[2])
report_path = pathlib.Path(sys.argv[3])
baseline_rel = sys.argv[4]
current_rel = sys.argv[5]
gate_human = sys.argv[6]

with baseline_path.open(encoding="utf-8") as f:
    baseline = json.load(f)
with current_path.open(encoding="utf-8") as f:
    current = json.load(f)

lines = [
    f"gate={gate_human}",
    f"baseline_run={baseline_rel}",
    f"current_run={current_rel}",
    "compared_signature_fields=verdict,preset,reasons,stages[name,status,cases,timeouts,re_wa,limit_scale,scale_fail],gate_config[selected_preset_sha256,runtime_env[stage_filter,limit_scale,heartbeat_interval,stale_lock_seconds,profile_mode,local_skip_self_test,enable_state_load_materialization_opt,enable_prev_state_writeback_opt,enable_layout_signature_gate_opt,enable_layout_reuse_zero_elision_opt,strong_gate_release_profile,solver_env_scrub]]",
]
if baseline != current:
    lines.append("status=DIFF")
    lines.append("baseline_signature=" + json.dumps(baseline, sort_keys=True, ensure_ascii=False))
    lines.append("current_signature=" + json.dumps(current, sort_keys=True, ensure_ascii=False))
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    raise SystemExit(1)

lines.append("status=MATCH")
report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
PY
}

run_smoke_once() {
  local run_dir="$1"
  local smoke_signature_status="baseline"
  local status_freshness_marker="$run_dir/smoke_status.freshness_start.marker"
  local status_freshness_report="$run_dir/smoke_status.freshness_report.txt"
  local smoke_repeatability_summary_path="$run_dir/smoke_repeatability/summary.txt"
  local rc=0

  mkdir -p "$run_dir"
  write_tree_freshness_probe "$SMOKE_STATUS_ROOT" "$status_freshness_marker" summary.txt latest_status_report.md
  set +e
  LCA_REPEATABILITY_RUN_TOKEN="$RUN_TOKEN" \
    LCA_REPEATABILITY_CYCLE="${run_dir##*/}" \
    LCA_REPEATABILITY_GATE_LABEL="lca_smoke_repeatability" \
    "$SMOKE_REPEATABILITY_WRAPPER" "$SMOKE_REPEATABILITY_COUNT" >"$run_dir/lca_smoke_repeatability.stdout.txt" 2>"$run_dir/lca_smoke_repeatability.stderr.txt"
  rc=$?
  set -e
  printf '%s\n' "$rc" > "$run_dir/lca_smoke_repeatability.exit_code.txt"
  snapshot_smoke_repeatability_output "$run_dir"
  if (( rc != 0 )); then
    if [[ -f "$smoke_repeatability_summary_path" ]]; then
      FAILURE_REASON="$(read_summary_field "$smoke_repeatability_summary_path" "failure_reason")"
      FAILURE_HINT="$(read_summary_field "$smoke_repeatability_summary_path" "failure_hint")"
    fi
    if [[ -z "$FAILURE_REASON" ]]; then
      FAILURE_REASON="lca_smoke_repeatability failed before the smoke iteration gate on ${run_dir##*/}"
    fi
    if [[ -z "$FAILURE_HINT" ]]; then
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/lca_smoke_repeatability.stderr.txt and ${run_dir#$WORKDIR/}/smoke_repeatability/summary.txt"
    fi
    return "$rc"
  fi
  if [[ "$(read_summary_field "$smoke_repeatability_summary_path" "status")" != "PASS" ]]; then
    FAILURE_REASON="lca_smoke_repeatability returned exit code 0 but did not publish a PASS summary"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/smoke_repeatability/summary.txt"
    return 98
  fi
  if [[ "$(read_summary_field "$smoke_repeatability_summary_path" "supports_solver_iteration")" != "1" ]]; then
    FAILURE_REASON="lca_smoke_repeatability returned exit code 0 but did not mark smoke as safe for solver iteration"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/smoke_repeatability/summary.txt"
    return 98
  fi
  set +e
  LCA_SMOKE_EXPORT_SNAPSHOT_ROOT="$run_dir/smoke_snapshot" \
    LCA_REPEATABILITY_RUN_TOKEN="$RUN_TOKEN" \
    LCA_REPEATABILITY_CYCLE="${run_dir##*/}" \
    LCA_REPEATABILITY_GATE_LABEL="lca_smoke" \
    "$SMOKE_WRAPPER" >"$run_dir/smoke.stdout.txt" 2>"$run_dir/smoke.stderr.txt"
  rc=$?
  set -e
  printf '%s\n' "$rc" > "$run_dir/smoke.exit_code.txt"
  if (( rc != 0 )); then
    return "$rc"
  fi
  if ! assert_root_guard_intact "lca_smoke" "$run_dir"; then
    return 95
  fi
  if ! verify_tree_freshness "lca_smoke_status" "$status_freshness_marker" "$SMOKE_STATUS_ROOT" "$status_freshness_report" summary.txt latest_status_report.md; then
    FAILURE_REASON="lca_smoke returned exit code 0 but reused prior smoke status artifacts instead of regenerating current-run pass evidence"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/smoke_status.freshness_report.txt"
    return 94
  fi
  if ! snapshot_smoke_status "$run_dir"; then
    return 93
  fi
  if ! assert_smoke_status_pass "$run_dir/status_signature.txt" "$run_dir/status_pass_report.txt"; then
    FAILURE_REASON="lca_smoke returned exit code 0 but smoke_latest_status did not publish a PASS signature"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/status_signature.txt and ${run_dir#$WORKDIR/}/status_pass_report.txt"
    return 92
  fi
  if ! snapshot_smoke_output "$run_dir"; then
    return 91
  fi
  if [[ -n "$BASELINE_RUN" && "$run_dir" != "$BASELINE_RUN" ]]; then
    if ! compare_smoke_status_signatures "$BASELINE_RUN" "$run_dir"; then
      FAILURE_REASON="smoke status divergence between ${BASELINE_RUN##*/} and ${run_dir##*/}"
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/status_signature_diff.txt and the two status_snapshot trees"
      return 90
    fi
    if ! compare_smoke_snapshot_manifests "$BASELINE_RUN" "$run_dir"; then
      FAILURE_REASON="smoke divergence between ${BASELINE_RUN##*/} and ${run_dir##*/}"
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/smoke_manifest_diff.txt and the two smoke_snapshot trees"
      return 89
    fi
    smoke_signature_status="matched_baseline"
  fi

  record_result_row "${run_dir##*/}" "lca_smoke" "PASS" "$smoke_signature_status"
  return 0
}

run_gate_once() {
  local gate_label="$1"
  local gate_human="$2"
  local wrapper_path="$3"
  local gate_outroot="$4"
  local run_dir="$5"
  local freshness_marker="$run_dir/${gate_label}.freshness_start.marker"
  local freshness_report="$run_dir/${gate_label}.freshness_report.txt"
  local signature_status="baseline"
  local rc=0

  write_tree_freshness_probe "$gate_outroot" "$freshness_marker" certify.json certify_summary.md runtime_env.txt preflight_manifest.tsv selected_preset.json repeatability_gate_manifest.txt
  set +e
  LCA_REPEATABILITY_RUN_TOKEN="$RUN_TOKEN" \
    LCA_REPEATABILITY_CYCLE="${run_dir##*/}" \
    LCA_REPEATABILITY_GATE_LABEL="$gate_human" \
    "$wrapper_path" >"$run_dir/${gate_label}.stdout.txt" 2>"$run_dir/${gate_label}.stderr.txt"
  rc=$?
  set -e
  printf '%s\n' "$rc" > "$run_dir/${gate_label}.exit_code.txt"
  if (( rc != 0 )); then
    return "$rc"
  fi
  if ! assert_root_guard_intact "$gate_human" "$run_dir"; then
    return 97
  fi
  if ! verify_tree_freshness "$gate_human" "$freshness_marker" "$gate_outroot" "$freshness_report" certify.json certify_summary.md runtime_env.txt preflight_manifest.tsv selected_preset.json repeatability_gate_manifest.txt; then
    FAILURE_REASON="${gate_human} returned exit code 0 but reused prior gate artifacts instead of regenerating current-run pass evidence"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/${gate_label}.freshness_report.txt before trusting ${gate_outroot}"
    return 96
  fi
  if ! snapshot_gate_output "$gate_label" "$gate_outroot" "$run_dir"; then
    return 95
  fi
  if ! extract_pass_signature \
    "$run_dir/$gate_label" \
    "$run_dir/$gate_label/pass_signature.json" \
    "$run_dir/$gate_label/pass_signature_report.txt" \
    "$gate_human"; then
    FAILURE_REASON="${gate_human} returned exit code 0 but did not produce a stable PASS signature"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/${gate_label}/pass_signature_report.txt and ${run_dir#$WORKDIR/}/${gate_label}/certify.json"
    return 94
  fi
  if [[ -n "$BASELINE_RUN" && "$run_dir" != "$BASELINE_RUN" ]]; then
    if ! compare_gate_signatures "$BASELINE_RUN" "$run_dir" "$gate_label" "$gate_human"; then
      FAILURE_REASON="${gate_human} PASS signature diverged from ${BASELINE_RUN##*/} on ${run_dir##*/}"
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/${gate_label}.signature_diff.txt"
      return 93
    fi
    signature_status="matched_baseline"
  fi

  record_result_row "${run_dir##*/}" "$gate_human" "PASS" "$signature_status"
  return 0
}

write_summary() {
  local status="$1"

  {
    echo "status=$status"
    echo "requested_runs=$RUN_COUNT"
    echo "completed_runs=$COMPLETED_RUNS"
    echo "required_sequence=$REQUIRED_SEQUENCE"
    echo "smoke_precheck=$SMOKE_PRECHECK $SMOKE_REPEATABILITY_COUNT"
    echo "reproducibility_scope=consecutive_same_worktree_full_flow_cycles"
    echo "artifacts_root=$ARTIFACTS_ROOT"
    echo "artifact_root_guard=$ROOT_GUARD_MARKER"
    echo "smoke_output=$SMOKE_OUTROOT"
    echo "smoke_repeatability_output=$SMOKE_REPEATABILITY_OUTROOT"
    echo "strong_gate_output=$STRONG_OUTROOT"
    echo "boj3s_gate_output=$BOJ3S_OUTROOT"
    if [[ -n "$FAILED_RUN" ]]; then
      echo "failed_run=$FAILED_RUN"
    fi
    if [[ -n "$FAILED_STAGE" ]]; then
      echo "failed_stage=$FAILED_STAGE"
    fi
    if [[ -n "$FAILURE_REASON" ]]; then
      echo "failure_reason=$FAILURE_REASON"
    fi
    if [[ -n "$FAILURE_HINT" ]]; then
      echo "failure_hint=$FAILURE_HINT"
    fi
    if [[ -n "$BASELINE_RUN" ]]; then
      echo "baseline_run=${BASELINE_RUN#$WORKDIR/}"
    fi
    echo "compared_signature_fields=$SIGNATURE_FIELDS"
    echo "results_tsv=results.tsv"
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
require_materialized_file "$ARTIFACT_RESOLVER" "artifact resolver"
require_materialized_file "$RELEASE_ENV" "release env wrapper"
require_materialized_executable "$SMOKE_REPEATABILITY_WRAPPER" "smoke repeatability wrapper"
require_materialized_executable "$SMOKE_WRAPPER" "smoke wrapper"
require_materialized_executable "$STRONG_WRAPPER" "strong gate wrapper"
require_materialized_executable "$BOJ3S_WRAPPER" "BOJ 3s gate wrapper"
validate_run_count

source "$RELEASE_ENV"

ensure_under_artifacts "$ARTIFACTS_ROOT"
ensure_under_artifacts "$TMP_PARENT"
ensure_under_artifacts "$STAGE_PARENT"
ensure_under_artifacts "$LOCK_ROOT"
mkdir -p "$ARTIFACTS_ROOT"
acquire_lock
SMOKE_OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_smoke)"
SMOKE_REPEATABILITY_OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_smoke_repeatability)"
STRONG_OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_strong_gate)"
BOJ3S_OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_boj3s_gate)"
OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_acceptance_repeatability)"
OUTPARENT="$(dirname "$OUTROOT")"
BACKUP_ROOT="${OUTROOT}.previous"
RUN_TOKEN="lca_acceptance_repeatability.$$.$(date +%s)"
ensure_under_artifacts "$SMOKE_OUTROOT"
ensure_under_artifacts "$SMOKE_REPEATABILITY_OUTROOT"
ensure_under_artifacts "$STRONG_OUTROOT"
ensure_under_artifacts "$BOJ3S_OUTROOT"
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
prepare_root_guard
WORKDIR="$(mktemp -d "$STAGE_PARENT/$RUN_WORK_TEMPLATE")"
ensure_under_artifacts "$WORKDIR"
mkdir -p "$WORKDIR/runs"
printf 'run\tstage\tverdict\tsignature_status\n' > "$WORKDIR/results.tsv"

for (( run_index = 1; run_index <= RUN_COUNT; ++run_index )); do
  run_label="$(printf 'run%02d' "$run_index")"
  run_dir="$WORKDIR/runs/$run_label"
  mkdir -p "$run_dir"

  if ! run_smoke_once "$run_dir"; then
    FAILED_RUN="$run_label"
    FAILED_STAGE="lca_smoke"
    if [[ -z "$FAILURE_REASON" ]]; then
      FAILURE_REASON="lca_smoke failed on $run_label with exit code $(<"$run_dir/smoke.exit_code.txt")"
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/smoke.stderr.txt and ${run_dir#$WORKDIR/}/smoke.stdout.txt"
    fi
    write_summary FAIL
    fail "$FAILURE_REASON; $FAILURE_HINT"
  fi

  if ! run_gate_once "strong_gate" "lca_strong_gate" "$STRONG_WRAPPER" "$STRONG_OUTROOT" "$run_dir"; then
    FAILED_RUN="$run_label"
    FAILED_STAGE="lca_strong_gate"
    if [[ -z "$FAILURE_REASON" ]]; then
      FAILURE_REASON="lca_strong_gate failed on $run_label with exit code $(<"$run_dir/strong_gate.exit_code.txt")"
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/strong_gate.stderr.txt and ${run_dir#$WORKDIR/}/strong_gate.stdout.txt"
    fi
    write_summary FAIL
    fail "$FAILURE_REASON; $FAILURE_HINT"
  fi

  if ! run_gate_once "boj3s_gate" "lca_boj3s_gate" "$BOJ3S_WRAPPER" "$BOJ3S_OUTROOT" "$run_dir"; then
    FAILED_RUN="$run_label"
    FAILED_STAGE="lca_boj3s_gate"
    if [[ -z "$FAILURE_REASON" ]]; then
      FAILURE_REASON="lca_boj3s_gate failed on $run_label with exit code $(<"$run_dir/boj3s_gate.exit_code.txt")"
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/boj3s_gate.stderr.txt and ${run_dir#$WORKDIR/}/boj3s_gate.stdout.txt"
    fi
    write_summary FAIL
    fail "$FAILURE_REASON; $FAILURE_HINT"
  fi

  if [[ -z "$BASELINE_RUN" ]]; then
    BASELINE_RUN="$run_dir"
  fi
  COMPLETED_RUNS="$run_index"
done

FAILURE_REASON=""
FAILURE_HINT=""
write_summary PASS
publish_output || fail "failed to publish repeatability output under $OUTROOT"
echo "[lca_acceptance_repeatability] PASS ($RUN_COUNT matching full-flow cycles)" >&2
