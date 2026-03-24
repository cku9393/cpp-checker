#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HARNESS="${ROOT}/build/rewrite_r_harness"
DUMP_DIR="${1:-${ROOT}/dumps/release_gate}"

HARD_DIR="${DUMP_DIR}/hard_compare"
RANDOM_DIR="${DUMP_DIR}/random_sanity_s1_r100"
SMOKE_DIR="${DUMP_DIR}/direct_solver_smoke"
SUMMARY_JSON="${DUMP_DIR}/summary.json"
SUMMARY_MD="${DUMP_DIR}/RELEASE_GATE_RESULT_ko.md"

mkdir -p "${HARD_DIR}" "${RANDOM_DIR}" "${SMOKE_DIR}"

if [[ ! -x "${HARNESS}" ]]; then
  echo "release gate: missing harness binary at ${HARNESS}" >&2
  exit 2
fi

run_and_log() {
  local logfile="$1"
  shift
  "$@" > "${logfile}" 2>&1
}

run_and_log "${HARD_DIR}/run.log" \
  "${HARNESS}" \
  --backend ogdf \
  --mode solver-compare \
  --manifest "${ROOT}/regressions/rewrite_seq_cases.json" \
  --baseline oracle \
  --oracle-handoff normalize \
  --dump-dir "${HARD_DIR}"

run_and_log "${RANDOM_DIR}/run.log" \
  "${HARNESS}" \
  --backend ogdf \
  --mode solver-compare \
  --baseline oracle \
  --oracle-handoff normalize \
  --seed 1 \
  --rounds 100 \
  --dump-dir "${RANDOM_DIR}"

run_and_log "${SMOKE_DIR}/run.log" \
  "${HARNESS}" \
  --backend ogdf \
  --mode rewrite-seq \
  --seed 1 \
  --rounds 10 \
  --dump-dir "${SMOKE_DIR}"

python3 - "${ROOT}" "${DUMP_DIR}" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
dump_dir = pathlib.Path(sys.argv[2])

hard_path = dump_dir / "hard_compare" / "summary.json"
random_path = dump_dir / "random_sanity_s1_r100" / "summary.json"
smoke_log = dump_dir / "direct_solver_smoke" / "run.log"

def load_json(path: pathlib.Path):
    return json.loads(path.read_text())

hard = load_json(hard_path)
rand = load_json(random_path)

hard_ok = (
    hard.get("compareCases") == 6 and
    hard.get("compareFailed") == 0 and
    hard.get("oracleFailCount") == 0 and
    hard.get("rewriteSeqFailCount") == 0 and
    hard.get("oracleVsRewriteMismatchCount") == 0 and
    hard.get("explicitMismatchCount") == 0 and
    hard.get("summaryWriteMode") == "atomic" and
    hard.get("summaryValidated") is True
)

random_ok = (
    rand.get("compareCases") == 100 and
    rand.get("compareFailed") == 0 and
    rand.get("oracleFailCount") == 0 and
    rand.get("rewriteSeqFailCount") == 0 and
    rand.get("oracleVsRewriteMismatchCount") == 0 and
    rand.get("explicitMismatchCount") == 0 and
    rand.get("summaryWriteMode") == "atomic" and
    rand.get("summaryValidated") is True
)

smoke_ok = smoke_log.exists()
all_valid = (
    hard.get("summaryValidated") is True and
    rand.get("summaryValidated") is True and
    hard.get("summaryWriteMode") == "atomic" and
    rand.get("summaryWriteMode") == "atomic"
)

status = "green" if hard_ok and random_ok and smoke_ok and all_valid else "fail"
result = {
    "releaseGateStatus": status,
    "defaultSolverPath": "rewrite-seq",
    "legacyPathStatus": "diagnostic-only",
    "hardCompareStatus": "green" if hard_ok else "fail",
    "randomSanityStatus": "green" if random_ok else "fail",
    "directSolverSmokeStatus": "green" if smoke_ok else "fail",
    "oracleVsRewriteMismatchCount": (
        hard.get("oracleVsRewriteMismatchCount", 0) +
        rand.get("oracleVsRewriteMismatchCount", 0)
    ),
    "allSummariesValidated": all_valid,
    "summaryWriteMode": "atomic",
    "hardCompareSummaryPath": str(hard_path),
    "randomSanitySummaryPath": str(random_path),
    "directSolverSmokeLogPath": str(smoke_log)
}

summary_path = dump_dir / "summary.json"
summary_path.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n")
json.loads(summary_path.read_text())

report = f"""# RELEASE GATE RESULT

- release gate status: {status}
- hard compare status: {result['hardCompareStatus']}
- random sanity status: {result['randomSanityStatus']}
- direct solver smoke status: {result['directSolverSmokeStatus']}
- oracle-vs-rewrite mismatch count: {result['oracleVsRewriteMismatchCount']}
- all summaries validated: {str(result['allSummariesValidated']).lower()}
- legacy path status: diagnostic-only

## Validation

- hard compare: {hard.get('comparePassed', 0)} passed / {hard.get('compareFailed', 0)} failed
- random sanity s1_r100: {rand.get('comparePassed', 0)} passed / {rand.get('compareFailed', 0)} failed
- direct solver smoke: `--mode rewrite-seq --seed 1 --rounds 10`

## Artifact Paths

- hard compare summary: `{hard_path}`
- random sanity summary: `{random_path}`
- direct solver smoke log: `{smoke_log}`
"""
md_path = dump_dir / "RELEASE_GATE_RESULT_ko.md"
md_path.write_text(report)

if status != "green":
    sys.exit(1)
PY

echo "release gate summary: ${SUMMARY_JSON}"
