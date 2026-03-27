#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
BRANCH_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"
cd "$BRANCH_ROOT"
ARTIFACT_RESOLVER="$BRANCH_ROOT/artifact_paths.py"
RELEASE_ENV="$BRANCH_ROOT/solver_release_env.sh"
STRONG_WRAPPER="$BRANCH_ROOT/lca_strong_gate.sh"
BOJ3S_WRAPPER="$BRANCH_ROOT/lca_boj3s_gate.sh"
ARTIFACTS_ROOT="$BRANCH_ROOT/artifacts/lca_tree_stress_v5"
TMP_PARENT="$ARTIFACTS_ROOT/.tmp"
STAGE_PARENT="$ARTIFACTS_ROOT/.repeatability_stage"
RUN_WORK_TEMPLATE="lca_required_repeatability.XXXXXX"
LOCK_ROOT="$ARTIFACTS_ROOT/.locks"
LOCKDIR="$LOCK_ROOT/lca_required_repeatability"
LOCK_PID_FILE="$LOCKDIR/pid"
OUTROOT=""
OUTPARENT=""
BACKUP_ROOT=""
WORKDIR=""
LOCK_HELD=0
RUN_COUNT="${LCA_REQUIRED_REPEAT_COUNT:-2}"
COMPLETED_RUNS=0
FAILED_RUN=""
FAILED_GATE=""
FAILURE_REASON=""
FAILURE_HINT=""
REQUIRED_SEQUENCE="lca_strong_gate -> lca_boj3s_gate"
STRONG_OUTROOT=""
BOJ3S_OUTROOT=""
BASELINE_RUN=""
SIGNATURE_FIELDS="verdict,preset,reasons,stages[name,status,cases,timeouts,re_wa,limit_scale,scale_fail]"

export PYTHONDONTWRITEBYTECODE=1

fail() {
  echo "[lca_required_repeatability] $*" >&2
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
usage: ./outer_suite_wrappers/lca_required_repeatability.sh [repeat-count]
[lca_required_repeatability] repeat-count defaults to $LCA_REQUIRED_REPEAT_COUNT or 2
[lca_required_repeatability] runs ./lca_strong_gate.sh then ./lca_boj3s_gate.sh per cycle
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
    fail "another lca_required_repeatability.sh run is active (pid $holder)"
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
    for stale in "$STAGE_PARENT"/lca_required_repeatability.*; do
      rm -rf "$stale"
    done
    shopt -u nullglob
  fi
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
      echo "[lca_required_repeatability] failed to rotate prior repeatability output into backup: $OUTROOT -> $BACKUP_ROOT" >&2
      return 1
    fi
  fi
  if ! move_path_retry "$WORKDIR" "$OUTPARENT/$outleaf"; then
    echo "[lca_required_repeatability] failed to publish repeatability output: $WORKDIR -> $OUTPARENT/$outleaf" >&2
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
  local gate_name="$2"
  local verdict="$3"
  local signature_status="${4:-na}"
  printf '%s\t%s\t%s\t%s\n' "$run_label" "$gate_name" "$verdict" "$signature_status" >> "$WORKDIR/results.tsv"
}

snapshot_gate_output() {
  local gate_label="$1"
  local gate_outroot="$2"
  local run_dir="$3"
  local snapshot_dir="$run_dir/$gate_label"
  local certify_json="$gate_outroot/certify.json"
  local certify_summary="$gate_outroot/certify_summary.md"

  if [[ ! -d "$gate_outroot" ]]; then
    fail "expected published gate output after successful run: $gate_outroot"
  fi
  if [[ ! -f "$certify_json" ]]; then
    fail "missing gate verdict artifact after successful run: $certify_json"
  fi
  mkdir -p "$snapshot_dir"
  cp "$certify_json" "$snapshot_dir/certify.json"
  if [[ -f "$certify_summary" ]]; then
    cp "$certify_summary" "$snapshot_dir/certify_summary.md"
  fi
  printf '%s\n' "$gate_outroot" > "$snapshot_dir/live_output_root.txt"
}

extract_pass_signature() {
  local certify_json="$1"
  local signature_path="$2"
  local report_path="$3"
  local gate_human="$4"
  python3 - "$certify_json" "$signature_path" "$report_path" "$gate_human" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

certify_path = pathlib.Path(sys.argv[1])
signature_path = pathlib.Path(sys.argv[2])
report_path = pathlib.Path(sys.argv[3])
gate_human = sys.argv[4]

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
}
bad_stages = [stage["name"] for stage in signature["stages"] if stage.get("status") != "PASS"]
lines = [
    f"gate={gate_human}",
    f"certify_json={certify_path}",
    f"verdict={signature['verdict']}",
    f"preset={signature['preset']}",
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
    "compared_signature_fields=verdict,preset,reasons,stages[name,status,cases,timeouts,re_wa,limit_scale,scale_fail]",
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

run_gate_once() {
  local gate_label="$1"
  local gate_human="$2"
  local wrapper_path="$3"
  local gate_outroot="$4"
  local run_dir="$5"
  local stdout_path="$run_dir/${gate_label}.stdout.txt"
  local stderr_path="$run_dir/${gate_label}.stderr.txt"
  local exit_code_path="$run_dir/${gate_label}.exit_code.txt"
  local verdict_path="$run_dir/${gate_label}.verdict.txt"
  local signature_status="baseline"
  local rc=0

  set +e
  "$wrapper_path" >"$stdout_path" 2>"$stderr_path"
  rc=$?
  set -e
  printf '%s\n' "$rc" > "$exit_code_path"
  if (( rc != 0 )); then
    return "$rc"
  fi

  snapshot_gate_output "$gate_label" "$gate_outroot" "$run_dir"
  if ! extract_pass_signature \
    "$run_dir/$gate_label/certify.json" \
    "$run_dir/$gate_label/pass_signature.json" \
    "$run_dir/$gate_label/pass_signature_report.txt" \
    "$gate_human"; then
    FAILURE_REASON="${gate_human} returned exit code 0 but did not produce a stable PASS signature"
    FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/${gate_label}/pass_signature_report.txt and ${run_dir#$WORKDIR/}/${gate_label}/certify.json"
    return 99
  fi

  printf 'PASS\n' > "$verdict_path"
  if [[ -n "$BASELINE_RUN" && "$run_dir" != "$BASELINE_RUN" ]]; then
    if ! compare_gate_signatures "$BASELINE_RUN" "$run_dir" "$gate_label" "$gate_human"; then
      FAILURE_REASON="${gate_human} PASS signature diverged from ${BASELINE_RUN##*/} on ${run_dir##*/}"
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/${gate_label}.signature_diff.txt"
      return 98
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
    echo "strong_gate_output=$STRONG_OUTROOT"
    echo "boj3s_gate_output=$BOJ3S_OUTROOT"
    if [[ -n "$FAILED_RUN" ]]; then
      echo "failed_run=$FAILED_RUN"
    fi
    if [[ -n "$FAILED_GATE" ]]; then
      echo "failed_gate=$FAILED_GATE"
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
require_file "$ARTIFACT_RESOLVER" "artifact resolver"
require_file "$RELEASE_ENV" "release env wrapper"
require_executable "$STRONG_WRAPPER" "strong gate wrapper"
require_executable "$BOJ3S_WRAPPER" "BOJ 3s gate wrapper"
validate_run_count

source "$RELEASE_ENV"

ensure_under_artifacts "$ARTIFACTS_ROOT"
ensure_under_artifacts "$TMP_PARENT"
ensure_under_artifacts "$STAGE_PARENT"
ensure_under_artifacts "$LOCK_ROOT"
mkdir -p "$ARTIFACTS_ROOT"
acquire_lock
STRONG_OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_strong_gate)"
BOJ3S_OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_boj3s_gate)"
OUTROOT="$(python3 "$ARTIFACT_RESOLVER" lca_required_repeatability)"
OUTPARENT="$(dirname "$OUTROOT")"
BACKUP_ROOT="${OUTROOT}.previous"
ensure_under_artifacts "$STRONG_OUTROOT"
ensure_under_artifacts "$BOJ3S_OUTROOT"
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
printf 'run\tgate\tverdict\tsignature_status\n' > "$WORKDIR/results.tsv"

for (( run_index = 1; run_index <= RUN_COUNT; ++run_index )); do
  run_label="$(printf 'run%02d' "$run_index")"
  run_dir="$WORKDIR/runs/$run_label"
  mkdir -p "$run_dir"

  if ! run_gate_once "strong_gate" "lca_strong_gate" "$STRONG_WRAPPER" "$STRONG_OUTROOT" "$run_dir"; then
    FAILED_RUN="$run_label"
    FAILED_GATE="lca_strong_gate"
    if [[ -z "$FAILURE_REASON" ]]; then
      FAILURE_REASON="required gate lca_strong_gate failed on $run_label with exit code $(<"$run_dir/strong_gate.exit_code.txt")"
      FAILURE_HINT="inspect ${run_dir#$WORKDIR/}/strong_gate.stderr.txt and ${run_dir#$WORKDIR/}/strong_gate.stdout.txt"
    fi
    write_summary FAIL
    fail "$FAILURE_REASON; $FAILURE_HINT"
  fi

  if ! run_gate_once "boj3s_gate" "lca_boj3s_gate" "$BOJ3S_WRAPPER" "$BOJ3S_OUTROOT" "$run_dir"; then
    FAILED_RUN="$run_label"
    FAILED_GATE="lca_boj3s_gate"
    if [[ -z "$FAILURE_REASON" ]]; then
      FAILURE_REASON="required gate lca_boj3s_gate failed on $run_label with exit code $(<"$run_dir/boj3s_gate.exit_code.txt")"
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
echo "[lca_required_repeatability] PASS ($RUN_COUNT matching required gate cycles)" >&2
