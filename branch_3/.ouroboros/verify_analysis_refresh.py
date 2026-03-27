#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path


os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline-epoch", required=True, type=float)
    parser.add_argument("--analysis-log", required=True)
    parser.add_argument("--target", action="append", default=[])
    parser.add_argument(
        "--require-json-key",
        action="append",
        default=[],
        help="Require a truthy key in a JSON file using path:key syntax",
    )
    parser.add_argument(
        "--latest-failure-report",
        help="Failure report JSON whose attempt/session/timestamp must match the refreshed state",
    )
    parser.add_argument(
        "--latest-failure-breakdown",
        help="Failure breakdown JSON whose attempt/session/timestamp must match the refreshed state",
    )
    parser.add_argument(
        "--require-current-state",
        help="JSON analysis state file that must be marked current for the latest failure",
    )
    return parser.parse_args()


def latest_mtime(paths: list[Path]) -> float:
    mtimes = []
    for path in paths:
        try:
            mtimes.append(path.stat().st_mtime)
        except OSError:
            continue
    return max(mtimes) if mtimes else 0.0


def load_json_dict(path: Path) -> dict:
    payload = json.loads(path.read_text())
    if not isinstance(payload, dict):
        raise ValueError("payload is not a JSON object")
    return payload


def format_attempt_label(value: object) -> str | None:
    if value is None:
        return None
    if isinstance(value, str) and value.startswith("attempt_"):
        return value
    try:
        return f"attempt_{int(value):03d}"
    except (TypeError, ValueError):
        text = str(value).strip()
        return text or None


def normalize_failed_acs(values: object) -> list[str]:
    normalized: list[str] = []
    if not isinstance(values, list):
        return normalized
    for item in values:
        if isinstance(item, dict):
            item = item.get("ac_index") or item.get("ac")
        elif isinstance(item, (list, tuple)):
            item = item[0] if item else None
        if item is None:
            continue
        text = str(item).strip()
        if text and text not in normalized:
            normalized.append(text)
    return normalized


def build_failure_signature(report_payload: dict, breakdown_payload: dict) -> dict[str, object]:
    attempt_label = (
        format_attempt_label(breakdown_payload.get("attempt"))
        or format_attempt_label(report_payload.get("attempt"))
    )
    session_id = breakdown_payload.get("session_id") or report_payload.get("session_id")
    execution_id = breakdown_payload.get("execution_id") or report_payload.get("execution_id")
    timestamp = breakdown_payload.get("timestamp") or report_payload.get("timestamp")
    failed_acs = normalize_failed_acs(report_payload.get("failed_acs"))
    if not failed_acs:
        failed_acs = normalize_failed_acs(
            [item.get("ac_index") for item in breakdown_payload.get("failed_ac_breakdowns") or []]
        )
    signature = "|".join(
        [
            attempt_label or "unknown-attempt",
            session_id or "unknown-session",
            timestamp or "unknown-timestamp",
            ",".join(failed_acs) or "none",
        ]
    )
    return {
        "attempt_label": attempt_label,
        "session_id": session_id,
        "execution_id": execution_id,
        "timestamp": timestamp,
        "failed_acs": failed_acs,
        "failure_signature": signature,
    }


def verify_current_state(state_path: Path, report_path: Path, breakdown_path: Path) -> list[str]:
    report_payload = load_json_dict(report_path)
    breakdown_payload = load_json_dict(breakdown_path)
    state_payload = load_json_dict(state_path)
    expected = build_failure_signature(report_payload, breakdown_payload)

    report_attempt = format_attempt_label(report_payload.get("attempt"))
    breakdown_attempt = format_attempt_label(breakdown_payload.get("attempt"))
    if report_attempt and breakdown_attempt and report_attempt != breakdown_attempt:
        raise ValueError(
            f"latest failure attempt mismatch between `{report_path}` ({report_attempt}) and "
            f"`{breakdown_path}` ({breakdown_attempt})"
        )
    if report_payload.get("session_id") and breakdown_payload.get("session_id"):
        if report_payload["session_id"] != breakdown_payload["session_id"]:
            raise ValueError(
                f"latest failure session mismatch between `{report_path}` and `{breakdown_path}`"
            )
    if report_payload.get("timestamp") and breakdown_payload.get("timestamp"):
        if report_payload["timestamp"] != breakdown_payload["timestamp"]:
            raise ValueError(
                f"latest failure timestamp mismatch between `{report_path}` and `{breakdown_path}`"
            )

    current_payload = state_payload.get("current_failure")
    if current_payload is not None and not isinstance(current_payload, dict):
        raise ValueError(f"`current_failure` in `{state_path}` is not a JSON object")

    actual = {
        "current_for_latest_failure": state_payload.get("current_for_latest_failure"),
        "attempt_label": state_payload.get("current_failure_attempt")
        or (current_payload or {}).get("attempt_label")
        or state_payload.get("last_failed_attempt"),
        "session_id": state_payload.get("current_failure_session_id")
        or (current_payload or {}).get("session_id")
        or state_payload.get("last_failed_session_id"),
        "execution_id": state_payload.get("current_failure_execution_id")
        or (current_payload or {}).get("execution_id"),
        "timestamp": state_payload.get("current_failure_timestamp")
        or (current_payload or {}).get("timestamp"),
        "failed_acs": state_payload.get("current_failure_failed_acs")
        or (current_payload or {}).get("failed_acs")
        or [],
        "failure_signature": state_payload.get("current_failure_signature")
        or (current_payload or {}).get("failure_signature"),
    }

    if actual["current_for_latest_failure"] is not True:
        raise ValueError(f"`{state_path}` is not marked current_for_latest_failure=true")
    if normalize_failed_acs(actual["failed_acs"]) != expected["failed_acs"]:
        raise ValueError(
            f"`{state_path}` failed AC marker {normalize_failed_acs(actual['failed_acs'])} "
            f"does not match latest failure {expected['failed_acs']}"
        )
    for key in ("attempt_label", "session_id", "timestamp", "failure_signature"):
        if actual.get(key) != expected.get(key):
            raise ValueError(
                f"`{state_path}` {key} `{actual.get(key)}` does not match latest failure "
                f"`{expected.get(key)}`"
            )
    if expected.get("execution_id") and actual.get("execution_id") != expected.get("execution_id"):
        raise ValueError(
            f"`{state_path}` execution_id `{actual.get('execution_id')}` does not match latest failure "
            f"`{expected.get('execution_id')}`"
        )

    return [
        f"{state_path}:current_for_latest_failure",
        f"{state_path}:current_failure_attempt={expected['attempt_label']}",
        f"{state_path}:current_failure_session_id={expected['session_id']}",
        f"{state_path}:current_failure_signature={expected['failure_signature']}",
    ]


def main() -> int:
    args = parse_args()
    targets = [Path(item) for item in args.target]
    analysis_log = Path(args.analysis_log)

    refreshed_targets = []
    for path in targets:
        try:
            mtime = path.stat().st_mtime
        except OSError:
            continue
        if mtime >= args.baseline_epoch:
            refreshed_targets.append(path)

    analysis_log_mtime = latest_mtime([analysis_log])

    if not refreshed_targets:
        print("analysis refresh verification failed: no target file updated after baseline")
        return 1
    if analysis_log_mtime < args.baseline_epoch:
        print("analysis refresh verification failed: analysis log did not update after baseline")
        return 1

    verified_json_keys = []
    for spec in args.require_json_key:
        if ":" not in spec:
            print(f"analysis refresh verification failed: malformed require-json-key `{spec}`")
            return 1
        path_text, key = spec.rsplit(":", 1)
        path = Path(path_text)
        try:
            import json

            payload = json.loads(path.read_text())
        except Exception as exc:  # noqa: BLE001
            print(f"analysis refresh verification failed: could not read `{path}` ({exc})")
            return 1
        value = payload.get(key) if isinstance(payload, dict) else None
        if not value:
            print(f"analysis refresh verification failed: `{key}` missing or empty in `{path}`")
            return 1
        verified_json_keys.append(f"{path}:{key}")

    verified_current_state = []
    if args.require_current_state:
        if not args.latest_failure_report or not args.latest_failure_breakdown:
            print(
                "analysis refresh verification failed: require-current-state also needs "
                "--latest-failure-report and --latest-failure-breakdown"
            )
            return 1
        state_path = Path(args.require_current_state)
        report_path = Path(args.latest_failure_report)
        breakdown_path = Path(args.latest_failure_breakdown)
        try:
            verified_current_state = verify_current_state(state_path, report_path, breakdown_path)
        except Exception as exc:  # noqa: BLE001
            print(f"analysis refresh verification failed: {exc}")
            return 1

    print("analysis refresh verified")
    for path in refreshed_targets:
        print(path)
    for item in verified_json_keys:
        print(item)
    for item in verified_current_state:
        print(item)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
