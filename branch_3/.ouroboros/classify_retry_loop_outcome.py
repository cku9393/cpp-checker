#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True


ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]")
FAIL_AC_RE = re.compile(r"### AC (\d+): \[FAIL\] ")
BLOCKED_AC_RE = re.compile(r"### AC (\d+): \[BLOCKED\] ")

TERMINAL_ABORT_PHRASES = (
    "stale-state preflight failed; aborting retry loop before workflow start",
    "analysis seed preflight failed; aborting retry loop before analysis rounds",
    "failed to refresh mandatory analysis assets; aborting retry loop instead of starting a blind solver retry",
    "next probe runner failed; aborting retry loop instead of starting a solver retry without probe evidence",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Classify whether a retry-loop workflow log represents a retryable intermediate failure."
    )
    parser.add_argument("--workflow-log", required=True)
    parser.add_argument("--attempt-number", type=int)
    parser.add_argument("--field")
    return parser.parse_args()


def strip_ansi(text: str) -> str:
    return ANSI_ESCAPE_RE.sub("", text)


def _marker_for_attempt(prefix: str, attempt_number: int | None) -> re.Pattern[str]:
    if attempt_number is None:
        return re.compile(rf"attempt \d+ {re.escape(prefix)}")
    return re.compile(rf"attempt {attempt_number} {re.escape(prefix)}")


def classify(clean_text: str, attempt_number: int | None) -> dict[str, object]:
    lowered = clean_text.lower()
    success_marker_present = bool(_marker_for_attempt("succeeded", attempt_number).search(lowered))
    failure_marker_present = bool(
        _marker_for_attempt("failed with exit code", attempt_number).search(lowered)
    )
    guard_retryable_failure = "guard rejected a nominal pass; converting to retryable failure" in lowered
    parallel_summary_present = "parallel execution complete" in lowered
    failed_ac_indices = [int(match.group(1)) for match in FAIL_AC_RE.finditer(clean_text)]
    blocked_ac_indices = [int(match.group(1)) for match in BLOCKED_AC_RE.finditer(clean_text)]
    terminal_abort_detected = any(phrase in lowered for phrase in TERMINAL_ABORT_PHRASES)

    retryable_intermediate_failure = False
    reason = "no_retry_signal"
    if success_marker_present:
        reason = "success_marker"
    elif terminal_abort_detected:
        reason = "terminal_retry_abort"
    elif failure_marker_present:
        retryable_intermediate_failure = True
        reason = "explicit_retryable_failure_marker"
    elif guard_retryable_failure:
        retryable_intermediate_failure = True
        reason = "guard_retryable_failure"
    elif parallel_summary_present and failed_ac_indices:
        retryable_intermediate_failure = True
        reason = "failed_acceptance_summary"
    elif parallel_summary_present and blocked_ac_indices:
        retryable_intermediate_failure = True
        reason = "blocked_acceptance_summary"

    return {
        "success_marker_present": success_marker_present,
        "failure_marker_present": failure_marker_present,
        "guard_retryable_failure": guard_retryable_failure,
        "parallel_summary_present": parallel_summary_present,
        "failed_ac_indices": failed_ac_indices,
        "blocked_ac_indices": blocked_ac_indices,
        "terminal_abort_detected": terminal_abort_detected,
        "retryable_intermediate_failure": retryable_intermediate_failure,
        "reason": reason,
    }


def main() -> int:
    args = parse_args()
    workflow_log = Path(args.workflow_log)
    clean_text = strip_ansi(workflow_log.read_text(encoding="utf-8", errors="replace"))
    payload = classify(clean_text, args.attempt_number)

    if args.field:
        value = payload.get(args.field)
        if isinstance(value, bool):
            print("true" if value else "false")
        elif isinstance(value, (dict, list)):
            print(json.dumps(value, ensure_ascii=False, sort_keys=True))
        elif value is None:
            print("")
        else:
            print(value)
        return 0

    print(json.dumps(payload, indent=2, ensure_ascii=False, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
